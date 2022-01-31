/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#include <unordered_map>

#include <matcher.h>

#ifndef ELFT_MSUIMPL_ENROLLDB_H_
#define ELFT_MSUIMPL_ENROLLDB_H_

namespace ELFT
{
	/** Representation of manifest file provided from ELFT API. */
	struct MSUEnrollDBEntry
	{
		/** Offset in archive */
		std::streamoff offset{};
		/** Length of entry */
		std::streamsize length{};
		/** Whether this entry is in memDB */
		bool inMem{false};
	};

	/** Database that supports partial loading in memory. */
	class MSUEnrollDB
	{
	public:
		MSUEnrollDB(
		    const std::filesystem::path &databaseDirectory);

		/**
		 * @brief
		 * Populate database from disk.
		 *
		 * @param maxBytes
		 * Approximate upper bound on number of bytes to use. Templates
		 * in excess will not be loaded, meaning that calls to read()
		 * will hit the disk.
		 * @param algorithm
		 * MSU algorithm used to create templates.
		 */
		void
		load(
		    const uint64_t maxBytes,
		    const PQ::Matcher &algorithm);

		/**
		 * @brief
		 * Obtain an exemplar template.
		 *
		 * @param key
		 * Name of the exemplar template.
		 * @param algorithm
		 * MSU algorithm used to create templates.
		 * @param forceFromDisk
		 * Whether to force reading this template from disk, even if it
		 * is loaded into RAM.
		 *
		 * @return
		 * MSU exemplar template.
		 *
		 * @throw std::runtime_exception
		 * `key` does not exist.
		 */
		RolledFPTemplate
		read(
		    const std::string &key,
		    const PQ::Matcher &algorithm,
		    const bool forceFromDisk)
		    const;

		/**
		 * @brief
		 * Obtain number of enrollment set templates loaded.
		 *
		 * @param inMem
		 * Only return number of templates loaded in memory.
		 *
		 * @return
		 * Number of enrollment set templates.
		 *
		 * @warning
		 * **Must** call load() before this method to get an accurate
		 * count.
		 */
		uint64_t
		size(
		    bool inMem)
		    const;

	private:
		/** Path provided from ELFT API. */
		const std::filesystem::path databaseDirectory{};
		/** List of all keys and how to read. */
		std::unordered_map<std::string, MSUEnrollDBEntry> diskDB{};
		/** Partial database read into memory. */
		std::unordered_map<std::string, RolledFPTemplate> memDB{};

		/** Initialize MSUEnrollDB.diskDB. */
		void
		initDiskDB();
	};
}

#endif /* ELFT_MSUIMPL_ENROLLDB_H_ */
