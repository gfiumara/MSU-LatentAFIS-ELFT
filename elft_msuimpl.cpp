/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#include <algorithm>
#include <filesystem>

#include <matcher.h>

#include <elft_msuimpl.h>

#ifdef NDEBUG
#include <chrono>
#include <fstream>

#include <unistd.h>

static std::string currentTime()
{
	const auto time = std::chrono::system_clock::to_time_t(
	    std::chrono::system_clock::now());
	std::string s = std::ctime(&time);
	s.pop_back();
	return (s);
}

#endif /* NDEBUG */

ELFT::MSUExtractionImplementation::MSUExtractionImplementation(
    const std::filesystem::path &configurationDirectory) :
    ELFT::ExtractionInterface(),
    configurationDirectory{configurationDirectory}
{

}

ELFT::ExtractionInterface::SubmissionIdentification
ELFT::MSUExtractionImplementation::getIdentification()
    const
{
	ExtractionInterface::SubmissionIdentification si{};
	si.versionNumber = MSUImplementationConstants::versionNumber;
	si.libraryIdentifier = MSUImplementationConstants::libraryIdentifier;

	return (si);
}

ELFT::CreateTemplateResult
ELFT::MSUExtractionImplementation::createTemplate(
    const ELFT::TemplateType templateType,
    const std::string &identifier,
    const std::vector<std::tuple<
        std::optional<ELFT::Image>, std::optional<ELFT::EFS>>> &samples)
    const
{
	return {};
}

std::optional<std::tuple<ELFT::ReturnStatus, std::vector<ELFT::TemplateData>>>
ELFT::MSUExtractionImplementation::extractTemplateData(
    const ELFT::TemplateType templateType,
    const ELFT::CreateTemplateResult &templateResult)
    const
{
	return {};
}

ELFT::ReturnStatus
ELFT::MSUExtractionImplementation::createReferenceDatabase(
    const TemplateArchive &referenceTemplates,
    const std::filesystem::path &databaseDirectory,
    const uint64_t maxSize)
    const
{
	try {
		std::filesystem::copy_file(referenceTemplates.archive,
		    databaseDirectory / "archive");
	} catch (const std::filesystem::filesystem_error &e) {
		return {ReturnStatus::Result::Failure, "Error when copying " +
		    referenceTemplates.archive.string() + ": " + e.what()};
	}

	try {
		std::filesystem::copy_file(referenceTemplates.manifest,
		    databaseDirectory / "manifest");
	} catch (const std::filesystem::filesystem_error &e) {
		return {ReturnStatus::Result::Failure, "Error when copying " +
		    referenceTemplates.manifest.string() + ": " + e.what()};
	}

	return {};
}

std::shared_ptr<ELFT::ExtractionInterface>
ELFT::ExtractionInterface::getImplementation(
    const std::filesystem::path &configurationDirectory)
{
	return (std::make_shared<MSUExtractionImplementation>(
	    configurationDirectory));
}

/******************************************************************************/

ELFT::MSUSearchImplementation::MSUSearchImplementation(
    const std::filesystem::path &configurationDirectory,
    const std::filesystem::path &databaseDirectory) :
    ELFT::SearchInterface(),
    configurationDirectory{configurationDirectory},
    databaseDirectory{databaseDirectory},
    algorithm{getCodebookPath(configurationDirectory,
        "codebook_EmbeddingSize_96_stride_16_subdim_6.dat")},
    enrollDB{databaseDirectory}
{
	/* Do NOT load templates into RAM here */
}

std::filesystem::path
ELFT::MSUSearchImplementation::getCodebookPath(
    const std::filesystem::path &configurationDirectory,
    const std::string &codebookFilename)
{
	const std::filesystem::path codebookFile{
	    configurationDirectory / codebookFilename};
	if (!std::filesystem::exists(codebookFile) ||
	    !std::filesystem::is_regular_file(codebookFile))
		throw std::runtime_error{"Codebook file (" + codebookFilename +
		    " at " + codebookFile.string() + ") does not exist."};

	return (codebookFile);
}

ELFT::ReturnStatus
ELFT::MSUSearchImplementation::load(
    const uint64_t maxSize)
{
	try {
		this->enrollDB.load(maxSize, this->algorithm);
	} catch (const std::exception &e) {
		return {ReturnStatus::Result::Failure, e.what()};
	}

	return {};
}

std::optional<ELFT::ProductIdentifier>
ELFT::MSUSearchImplementation::getIdentification()
    const
{
	ProductIdentifier id{};
	id.marketing = "NIST wrapper of MSU-LatentAFIS";

	return (id);
}

ELFT::SearchResult
ELFT::MSUSearchImplementation::search(
    const std::vector<std::byte> &probeTemplate,
    const uint16_t maxCandidates)
    const
{
	if (probeTemplate.empty()) {
		ELFT::SearchResult result{};
		result.decision = false;
		result.status = {ReturnStatus::Result::Failure,
		    "Template is empty"};
		return (result);
	}

	/*
	 * Re-implementation of relevant parts of Matcher::One2List_matching
	 */

	/* Convert to vector<uint8_t> (MSU is C++11. Likely fine, but safety) */
	std::vector<uint8_t> latent_uint8{};
	latent_uint8.reserve(probeTemplate.size());
	std::transform(probeTemplate.cbegin(), probeTemplate.cend(),
	    std::back_inserter(latent_uint8),
	    [](const std::byte b) -> uint8_t {
		return (static_cast<uint8_t>(b));
	});

	/* Parse template */
	LatentFPTemplate latent{};
	try {
		this->algorithm.load_FP_template(latent_uint8, latent);
	} catch (const std::exception &e) {
		ELFT::SearchResult result{};
		result.decision = false;
		result.status = {ReturnStatus::Result::Failure, e.what()};
		return (result);
	}
	if ((latent.m_nrof_minu_templates <= 0) &&
	    (latent.m_nrof_texture_templates <= 0)) {
		ELFT::SearchResult result{};
		result.decision = false;
		result.status = {ReturnStatus::Result::Failure,
		    "Template has no minutiae"};
		return (result);
	}

#ifdef NDEBUG
	std::ofstream log{"/tmp/elft-" + std::to_string(getpid())};
	uint64_t i{0};
#endif

	/* Compare every single template */
	std::vector<std::pair<std::string, float>> scores{};
	scores.reserve(maxCandidates);
	for (const auto &[key, entry] : this->enrollDB) {
#ifdef NDEBUG
		if ((++i % 1000) == 0) {
			log << "Time: " << currentTime() <<
			    ", Entry: " << i << ", Exemplar: " << key <<
			    std::endl;
		}
#endif
		RolledFPTemplate exemplar;
		try {
			exemplar = this->enrollDB.read(key, this->algorithm,
			    false);
		} catch (const std::exception &) {
			/* Can't load template, skip */
			continue;
		}

		std::vector<float> score{};
		const auto result = this->algorithm.
		    One2One_matching_selected_templates(latent, exemplar,
		    score);
		if (result == 1) {
			/* Latent template is empty */
			continue;
		} else if (result == 2) {
			/* Exemplar template is empty */
			continue;
		}

		/*
		 * Instead of storing hundreds of thousands of scores, we are
		 * only storing the top `maxCandidates` scores.
		 */
		const float finalScore{score[0] + score[1] + score[2] +
		    score[28]*0.3f};
		if (scores.size() < maxCandidates) {
			scores.emplace_back(key, finalScore);
		} else {
			std::sort(scores.begin(), scores.end(), EntrySorter());
			if (std::get<float>(scores.back()) < finalScore) {
				scores.pop_back();
				scores.emplace_back(key, finalScore);
			}
		}
	}

#ifdef NDEBUG
	log << "Time: " << currentTime() << ", Finished" << std::endl;
#endif

	ELFT::SearchResult result{};
	if (scores.empty()) {
		result.status = {ReturnStatus::Result::Failure,
		    "No scores generated"};
		result.decision = false;
		return (result);
	}

	/* Return top scores */
	result.candidateList.reserve(scores.size());
	for (const auto &entry : scores) {
		result.candidateList.emplace_back(
		    std::get<std::string>(entry),
		    ELFT::FrictionRidgeGeneralizedPosition::UnknownFinger,// XXX
		    std::get<float>(entry));
	}

	/* TODO: Find threshold */
	static const float decisionThreshold{0};
	result.decision = (std::get<float>(scores.front()) >=
	    decisionThreshold);

#ifdef NDEBUG
	log << "Time: " << currentTime() << ", Returning" << std::endl;
#endif

	return (result);
}

std::optional<ELFT::CorrespondenceResult>
ELFT::MSUSearchImplementation::extractCorrespondence(
    const std::vector<std::byte> &probeTemplate,
    const SearchResult &searchResult)
    const
{
	return {};
}

std::shared_ptr<ELFT::SearchInterface>
ELFT::SearchInterface::getImplementation(
    const std::filesystem::path &configurationDirectory,
    const std::filesystem::path &databaseDirectory)
{
	return (std::make_shared<MSUSearchImplementation>(
	    configurationDirectory, databaseDirectory));
}

bool
ELFT::MSUSearchImplementation::EntrySorter::operator()(
    const std::pair<std::string, float> &a,
    const std::pair<std::string, float> &b)
    const
{
	return (std::get<float>(a) > std::get<float>(b));
}
