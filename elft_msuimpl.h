/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#ifndef ELFT_MSUIMPL_H_
#define ELFT_MSUIMPL_H_

#include <elft_msuimpl_enrolldb.h>

#include <elft.h>

namespace ELFT
{
	namespace MSUImplementationConstants
	{
		uint16_t versionNumber{0x0001};
		uint16_t productOwner{0x000F};
		std::string libraryIdentifier{"MSU-LatentAFIS"};
	}

	class MSUExtractionImplementation : public ExtractionInterface
	{
	public:
		SubmissionIdentification
		getIdentification()
		    const
		    override;

		CreateTemplateResult
		createTemplate(
		    const TemplateType templateType,
		    const std::string &identifier,
		    const std::vector<std::tuple<
       		        std::optional<Image>, std::optional<EFS>>> &samples)
		    const
		    override;

		std::optional<std::tuple<ReturnStatus,
		    std::vector<TemplateData>>>
		extractTemplateData(
		    const TemplateType templateType,
		    const CreateTemplateResult &templateResult)
		    const
		    override;

		ReturnStatus
		createReferenceDatabase(
		    const TemplateArchive &referenceTemplates,
		    const std::filesystem::path &databaseDirectory,
		    const uint64_t maxSize)
		    const
		    override;

		MSUExtractionImplementation(
		    const std::filesystem::path &configurationDirectory = {});

	private:
		const std::filesystem::path configurationDirectory{};
	};

	class MSUSearchImplementation : public SearchInterface
	{
	public:
		std::optional<ProductIdentifier>
		getIdentification()
		    const
		    override;

		ReturnStatus
		load(
		    const uint64_t maxSize)
		    override;

		SearchResult
		search(
		    const std::vector<std::byte> &probeTemplate,
		    const uint16_t maxCandidates)
		    const
		    override;

		std::optional<CorrespondenceResult>
		extractCorrespondence(
		    const std::vector<std::byte> &probeTemplate,
		    const SearchResult &searchResult)
		    const
		    override;

		MSUSearchImplementation(
		    const std::filesystem::path &configurationDirectory,
		    const std::filesystem::path &databaseDirectory);

	private:
		const std::filesystem::path configurationDirectory{};
		const std::filesystem::path databaseDirectory{};

		/* MSU parameters */
		const PQ::Matcher algorithm;
		MSUEnrollDB enrollDB;

		/**
		 * @brief
		 * Form path to codebook file.
		 *
		 * @param configurationDirectory
		 * Path to ELFT configuration directory.
		 * @param configName
		 * Name of the configuration file.
		 *
		 * @return
		 * Path to codebook.
		 */
		static std::filesystem::path
		getCodebookPath(
		    const std::filesystem::path &configurationDirectory,
		    const std::string &codebookFilename);
	};
}

#endif /* ELFT_MSUIMPL_H_ */
