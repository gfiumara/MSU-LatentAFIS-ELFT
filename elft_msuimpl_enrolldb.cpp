/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#include <fstream>

#include <elft_msuimpl_enrolldb.h>

ELFT::MSUEnrollDB::MSUEnrollDB(
    const std::filesystem::path &databaseDirectory) :
    databaseDirectory{databaseDirectory}
{

}

RolledFPTemplate
ELFT::MSUEnrollDB::read(
    const std::string &key,
    const PQ::Matcher &algorithm,
    const bool forceFromDisk)
    const
{
	MSUEnrollDBEntry entry{};
	try {
		entry = this->diskDB.at(key);
	} catch (const std::out_of_range &) {
		return {};
	}

	if (!forceFromDisk && entry.inMem) {
		try {
			return (this->memDB.at(key));
		} catch (const std::out_of_range&) {
			/* Read from disk below */
		}
	}

	std::ifstream archive{this->databaseDirectory / "archive",
	    std::ifstream::binary};
	if (!archive)
		throw std::runtime_error{"Could not open archive"};

	archive.seekg(entry.offset);
	if (!archive)
		throw std::runtime_error{"Could not seek archive"};
	if (entry.length < 0 || static_cast<uint64_t>(entry.length) >
	    std::numeric_limits<std::vector<uint8_t>::size_type>::max())
		throw std::runtime_error{"Invalid template length"};

	std::vector<uint8_t> buf(static_cast<std::vector<uint8_t>::size_type>(
	    entry.length));
	archive.read(reinterpret_cast<char *>(buf.data()), entry.length);
	if (!archive)
		throw std::runtime_error{"Could not read archive"};

	RolledFPTemplate exemplar{};
	algorithm.load_FP_template(buf, exemplar);
	return (exemplar);
}

void
ELFT::MSUEnrollDB::initDiskDB()
{
	std::ifstream manifest{this->databaseDirectory / "manifest"};
	if (!manifest)
		throw std::runtime_error{"Could not open manifest"};

	std::string key{};
	MSUEnrollDBEntry entry{};
	while (manifest >> key >> entry.length >> entry.offset)
		this->diskDB[key] = entry;
}

void
ELFT::MSUEnrollDB::load(
    const uint64_t maxMemoryUsage,
    const PQ::Matcher &algorithm)
{
	/* Load all offsets */
	this->initDiskDB();

	/* Number of enrollment database templates */
	const auto numTmpls{this->diskDB.size()};

	std::intmax_t remainingBytes{};
	if (static_cast<std::intmax_t>(maxMemoryUsage) >
	    std::numeric_limits<std::intmax_t>::max())
		remainingBytes = std::numeric_limits<std::intmax_t>::max();
	else
		remainingBytes = static_cast<std::intmax_t>(maxMemoryUsage);

	/* Rough initial assumption */
	remainingBytes -= static_cast<decltype(remainingBytes)>(
	    sizeof(decltype(this->diskDB)) +
	    (numTmpls * sizeof(std::string)) +
	    (numTmpls * sizeof(MSUEnrollDBEntry)));

	for (auto &[key, entry] : this->diskDB) {
		/* Assume data structure uses 1.2x on-disk storage */
		remainingBytes -= static_cast<std::streamoff>(std::ceil(
		    1.2 * static_cast<float>(entry.length)));
		remainingBytes -= static_cast<decltype(remainingBytes)>(
		    sizeof(std::string);

		/* Can't fit anything more in RAM, stop loading. */
		if (remainingBytes <= 0)
			break;

		/* Still room in RAM. Read this template. */
		try {
			this->memDB[key] = this->read(key, algorithm, true);
		} catch (const std::exception &) {
			/* Can't parse template. Skip */
			continue;
		}
		entry.inMem = true;
	}
}

uint64_t
ELFT::MSUEnrollDB::size(
    bool inMem)
    const
{
	if (inMem)
		return (this->memDB.size());

	return (this->diskDB.size());
}
