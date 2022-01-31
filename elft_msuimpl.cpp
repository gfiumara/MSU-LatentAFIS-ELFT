/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#include <elft_msuimpl.h>

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
    databaseDirectory{databaseDirectory}
{
	/* Do NOT load templates into RAM here */
}

ELFT::ReturnStatus
ELFT::MSUSearchImplementation::load(
    const uint64_t maxSize)
{
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
	ELFT::SearchResult result{};
	result.candidateList.reserve(maxCandidates);
	result.decision = false;

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
