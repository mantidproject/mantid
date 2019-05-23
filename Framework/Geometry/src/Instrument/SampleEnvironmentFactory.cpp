// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/SampleEnvironmentFactory.h"
#include "MantidGeometry/Instrument/SampleEnvironmentSpecParser.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Material.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace Mantid {
namespace Geometry {

//------------------------------------------------------------------------------
// Anonyomous
//------------------------------------------------------------------------------
namespace {
// static logger object
Mantid::Kernel::Logger g_log("SampleEnvironment");

// Typedef for cache
using SampleEnvironmentSpecCache =
    std::unordered_map<std::string, SampleEnvironmentSpec_uptr>;

/**
 * If it doesn't exist create the static cache, otherwise return a reference to
 * it
 * @return A reference to the static cache
 */
SampleEnvironmentSpecCache &retrieveSpecCache() {
  static SampleEnvironmentSpecCache cache;
  return cache;
}

/**
 * Create a key for the cache
 * @param facility Name of facility
 * @param instrument Name of instrument
 * @param specName Name of spec
 * @return A key for the cache
 */
std::string createCacheKey(const std::string &facility,
                           const std::string &instrument,
                           const std::string &specName) {
  return facility + "/" + instrument + "/" + specName;
}
} // namespace

//------------------------------------------------------------------------------
// SampleEnvironmentFactory
//------------------------------------------------------------------------------
/**
 * Constructor accepting a pointer to a finder object
 * @param specFinder A reference to an object used to retrieve find a given
 * specification.
 */
SampleEnvironmentFactory::SampleEnvironmentFactory(
    ISampleEnvironmentSpecFinder_uptr specFinder)
    : m_finder(std::move(specFinder)) {}

/**
 * Create a new SampleEnvironment instance from the given specification and can.
 * SampleEnvironmentSpec
 * @param facility Name of facility
 * @param instrument Full name of the instrument
 * @param specName The name of a specification
 * @param canName The name of a can within the spec
 * @return A new instance of the given environment
 */
SampleEnvironment_uptr SampleEnvironmentFactory::create(
    const std::string &facility, const std::string &instrument,
    const std::string &specName, const std::string &canName) {
  assert(m_finder);
  auto &specCache = retrieveSpecCache();
  auto cacheKey = createCacheKey(facility, instrument, specName);

  SampleEnvironmentSpec *spec(nullptr);
  auto iter = specCache.find(cacheKey);
  if (iter != specCache.end()) {
    spec = iter->second.get();
  } else {
    auto specUPtr = m_finder->find(facility, instrument, specName);
    spec = specUPtr.get();
    specCache.emplace(cacheKey, std::move(specUPtr));
  }
  return spec->buildEnvironment(canName);
}

/**
 * @return the number of cache entries
 */
size_t SampleEnvironmentFactory::cacheSize() const {
  return retrieveSpecCache().size();
}

/**
 * Clear the cache of SampleEnvironmentSpec objects
 */
void SampleEnvironmentFactory::clearCache() { retrieveSpecCache().clear(); }

//------------------------------------------------------------------------------
// SampleEnvironmentSpecFileFinder
//------------------------------------------------------------------------------

/**
 * Constructor accepting a list of directories to search.
 * @param directories A list of directories that form the root of the search
 * path. Each entry will be appended with /facility/instrument/ and this
 * directory
 * then forms the path to find the named spec
 * @throws std::invalid_argument if the list is empty
 */
SampleEnvironmentSpecFileFinder::SampleEnvironmentSpecFileFinder(
    const std::vector<std::string> &directories)
    : m_rootDirs(directories) {
  if (m_rootDirs.empty()) {
    throw std::invalid_argument(
        "SampleEnvironmentSpecFileFinder() - Empty directory search list.");
  }
}

/**
 * Find a named specification in a file.
 * @param facility Name of facility
 * @param instrument Full name of the instrument
 * @param name The name of the spec
 * @return The SampleEnvironmentSpec_uptr
 */
SampleEnvironmentSpec_uptr
SampleEnvironmentSpecFileFinder::find(const std::string &facility,
                                      const std::string &instrument,
                                      const std::string &name) const {
  using Poco::File;
  using Poco::Path;

  Path relpath_instr(facility);
  relpath_instr.append(instrument).append(name + m_fileext);

  Path relpath_facil(facility);
  relpath_facil.append(name + m_fileext);

  // check for the instrument environment, then facility environment
  for (const auto rel_path : {relpath_instr, relpath_facil}) {
    for (const auto &prefixStr : m_rootDirs) {
      Path prefix(prefixStr);
      // Ensure the path is a directory (note that this does not create it!)
      prefix.makeDirectory();
      File fullpath(Poco::Path(prefix, rel_path));
      if (fullpath.exists()) {
        g_log.debug() << "Found environment at \"" << fullpath.path() << "\"\n";
        return parseSpec(name, fullpath.path());
      } else {
        g_log.debug() << "Failed to find environment at \"" << fullpath.path()
                      << "\"\n";
      }
    }
  }

  // no match if we get here
  std::ostringstream msg;
  msg << "Unable to find sample environment file '" << name
      << "' for facility '" << facility << "' and instrument '" << instrument
      << "'";
  throw std::runtime_error(msg.str());
}

/**
 * Parses the specification from the given file
 * @param name The name of the specification
 * @param filename Assumed to be an absolute path to an existing specification
 * @return A parse specification
 */
SampleEnvironmentSpec_uptr
SampleEnvironmentSpecFileFinder::parseSpec(const std::string &name,
                                           const std::string &filename) const {
  std::ifstream reader(filename, std::ios_base::in);
  if (!reader) {
    throw std::runtime_error(
        "SampleEnvironmentSpecFileFinder() - Error accessing file '" +
        filename + "'");
  }
  SampleEnvironmentSpecParser parser;
  return parser.parse(name, reader);
}

} // namespace Geometry
} // namespace Mantid
