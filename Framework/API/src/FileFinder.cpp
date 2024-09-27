// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IArchiveSearch.h"
#include "MantidAPI/ISISInstrumentDataCache.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Strings.h"

#include "MantidKernel/StringTokenizer.h"
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <cctype>

#include <boost/algorithm/string.hpp>

#include <filesystem>
#include <json/value.h>

namespace {
/// static logger object
Mantid::Kernel::Logger g_log("FileFinder");

/**
 * Unary predicate for use with remove_if.  Checks for the existance of
 * a "*" wild card in the file extension string passed to it.
 *
 * @param ext :: the extension to check.
 *
 * @returns true if extension contains a "*", else false.
 */
bool containsWildCard(const std::string &ext) { return std::string::npos != ext.find('*'); }

bool isASCII(const std::string &str) {
  return !std::any_of(str.cbegin(), str.cend(), [](char c) { return static_cast<unsigned char>(c) > 127; });
}

} // namespace

namespace Mantid::API {
using std::string;

// this allowed string could be made into an array of allowed, currently used
// only by the ISIS SANS group
const std::string FileFinderImpl::ALLOWED_SUFFIX = "-add";
//----------------------------------------------------------------------
// Public member functions
//----------------------------------------------------------------------
/**
 * Default constructor
 */
FileFinderImpl::FileFinderImpl() {
  // Make sure plugins are loaded
  FrameworkManager::Instance().loadPlugins();

// determine from Mantid property how sensitive Mantid should be
#ifdef _WIN32
  m_globOption = Poco::Glob::GLOB_DEFAULT;
#else
  setCaseSensitive(Kernel::ConfigService::Instance().getValue<bool>("filefinder.casesensitive").value_or(false));
#endif
}

/**
 * Option to set if file finder should be case sensitive
 * @param cs :: If true then set to case sensitive
 */
void FileFinderImpl::setCaseSensitive(const bool cs) {
  if (cs)
    m_globOption = Poco::Glob::GLOB_DEFAULT;
  else
    m_globOption = Poco::Glob::GLOB_CASELESS;
}

/**
 * Option to get if file finder should be case sensitive
 * @return cs :: If case sensitive return true, if not case sensitive return
 * false
 */
bool FileFinderImpl::getCaseSensitive() const { return (m_globOption == Poco::Glob::GLOB_DEFAULT); }

/**
 * Return the full path to the file given its name
 * @param filename :: A file name (without path) including extension
 * @param ignoreDirs :: If true, directories that match are skipped unless the
 * path given is already absolute
 * @return The full path if the file exists and can be found in one of the
 * search locations
 *  or an empty string otherwise.
 */

std::string FileFinderImpl::getFullPath(const std::string &filename, const bool ignoreDirs) const {
  return Kernel::ConfigService::Instance().getFullPath(filename, ignoreDirs, m_globOption);
}

/** Run numbers can be followed by an allowed string. Check if there is
 *  one, remove it from the name and return the string, else return empty
 *  @param userString run number that may have a suffix
 *  @return the suffix, if there was one
 */
std::string FileFinderImpl::extractAllowedSuffix(std::string &userString) const {
  if (userString.find(ALLOWED_SUFFIX) == std::string::npos) {
    // short cut processing as normally there is no suffix
    return "";
  }

  // ignore any file extension in checking if a suffix is present
  Poco::Path entry(userString);
  std::string noExt(entry.getBaseName());
  const size_t repNumChars = ALLOWED_SUFFIX.size();
  if (noExt.find(ALLOWED_SUFFIX) == noExt.size() - repNumChars) {
    userString.replace(userString.size() - repNumChars, repNumChars, "");
    return ALLOWED_SUFFIX;
  }
  return "";
}

/**
 * Return the InstrumentInfo as determined from the hint.
 *
 * @param hint :: The name hint.
 * @param returnDefaultIfNotFound :: Flag to control return. May throw exception if set to false.
 * @return This will return the default instrument if it cannot be determined.
 */
const Kernel::InstrumentInfo FileFinderImpl::getInstrument(const string &hint,
                                                           const bool returnDefaultIfNotFound) const {
  if ((!hint.empty()) && (!isdigit(hint[0]))) {
    string instrName(hint);
    Poco::Path path(instrName);
    instrName = path.getFileName();
    if ((instrName.find("PG3") == 0) || (instrName.find("pg3") == 0)) {
      instrName = "PG3";
    }
    // We're extending this nasty hack to accomodate data archive searching for
    // SANS2D.
    // While this certainly shouldn't be considered good practice, #7515 exists
    // to
    // completely redesign FileFinder -- this quick fix will have to do until
    // all this
    // code gets an overhaul as part of that ticket.  Please think twice before
    // adding
    // any more instruments to this list.
    else if ((instrName.find("SANS2D") == 0) || (instrName.find("sans2d") == 0)) {
      instrName = "SANS2D";
    } else {
      // go forwards looking for the run number to start
      {
        const auto it = std::find_if(instrName.begin(), instrName.end(), isdigit);
        const auto nChars = std::distance(instrName.begin(), it);
        instrName.resize(nChars);
      }

      // go backwards looking for the instrument name to end - gets around
      // delimiters
      if (!instrName.empty()) {
        const auto it = std::find_if(instrName.rbegin(), instrName.rend(), isalpha);
        const auto nChars = std::distance(it, instrName.rend());
        instrName.resize(nChars);
      }
    }
    try {
      const Kernel::InstrumentInfo instrument = Kernel::ConfigService::Instance().getInstrument(instrName);
      return instrument;
    } catch (Kernel::Exception::NotFoundError &e) {
      g_log.debug() << e.what() << "\n";
      if (!returnDefaultIfNotFound) {
        throw e;
      }
    }
  }
  return Kernel::ConfigService::Instance().getInstrument();
}

/**
 * Extracts the instrument name and run number from a hint
 * @param hint :: The name hint
 * @return A pair of instrument name and run number
 */
std::pair<std::string, std::string> FileFinderImpl::toInstrumentAndNumber(const std::string &hint) const {
  g_log.debug() << "toInstrumentAndNumber(" << hint << ")\n";
  std::string instrPart;
  std::string runPart;

  if (isdigit(hint[0])) {
    instrPart = Kernel::ConfigService::Instance().getInstrument().shortName();
    runPart = hint;
  } else {
    /// Find the last non-digit as the instrument name can contain numbers
    std::string::const_reverse_iterator it = std::find_if(hint.rbegin(), hint.rend(), std::not_fn(isdigit));
    // No non-digit or all non-digits
    if (it == hint.rend() || it == hint.rbegin()) {
      throw std::invalid_argument("Malformed hint to FileFinderImpl::makeFileName: " + hint);
    }
    std::string::size_type nChars = std::distance(it, hint.rend());

    // Add in special test for PG3
    if (boost::algorithm::istarts_with(hint, "PG3")) {
      instrPart = "PG3";
      nChars = instrPart.length();
    }
    // Another nasty check for SANS2D.  Will do until FileFinder redesign.
    else if (boost::algorithm::istarts_with(hint, "SANS2D")) {
      instrPart = "SANS2D";
      nChars = instrPart.length();
    } else {
      instrPart = hint.substr(0, nChars);
    }

    runPart = hint.substr(nChars);
  }

  unsigned int irunPart(0);
  try {
    irunPart = boost::lexical_cast<unsigned int>(runPart);
  } catch (boost::bad_lexical_cast &) {
    std::ostringstream os;
    os << "Cannot convert '" << runPart << "' to run number.";
    throw std::invalid_argument(os.str());
  }
  Kernel::InstrumentInfo instr = Kernel::ConfigService::Instance().getInstrument(instrPart);
  size_t nZero = instr.zeroPadding(irunPart);
  // remove any leading zeros in case there are too many of them
  std::string::size_type i = runPart.find_first_not_of('0');
  runPart.erase(0, i);
  while (runPart.size() < nZero)
    runPart.insert(0, "0");
  if (runPart.size() > nZero && nZero != 0) {
    throw std::invalid_argument("Run number does not match instrument's zero padding");
  }

  instrPart = instr.filePrefix(irunPart);

  return std::make_pair(instrPart, runPart);
}

/**
 * Make a data file name (without extension) from a hint. The hint can be either
 * a run number or
 * a run number prefixed with an instrument name/short name. If the instrument
 * name is absent the default one is used.
 * @param hint :: The name hint
 * @param instrument :: The current instrument object
 * @return The file name
 * @throw NotFoundError if a required default is not set
 * @throw std::invalid_argument if the argument is malformed or run number is
 * too long
 */
std::string FileFinderImpl::makeFileName(const std::string &hint, const Kernel::InstrumentInfo &instrument) const {
  if (hint.empty())
    return "";

  std::string filename(hint);
  const std::string suffix = extractAllowedSuffix(filename);
  const std::string shortName = instrument.shortName();
  std::string delimiter = instrument.delimiter();

  // see if starts with the provided instrument name
  if (filename.substr(0, shortName.size()) == shortName) {
    filename = filename.substr(shortName.size());
    if ((!delimiter.empty()) && (filename.substr(0, delimiter.size()) == delimiter))
      filename = filename.substr(delimiter.size());

    filename = shortName + filename;
  }

  auto [instrumentName, runNumber] = toInstrumentAndNumber(filename);

  // delimiter and suffix might be empty strings
  filename = instrumentName + delimiter + runNumber + suffix;
  return filename;
}

/**
 * Determine the extension from a filename.
 *
 * @param filename The filename to get the extension from.
 * @param exts The list of extensions to try before giving up and
 * using the default: whatever happens after the '.'.
 *
 * @return The extension. If one isn't determined it is an empty string.
 */
std::string FileFinderImpl::getExtension(const std::string &filename, const std::vector<std::string> &exts) const {
  g_log.debug() << "getExtension(" << filename << ", exts[" << exts.size() << "])\n";

  // go through the list of supplied extensions
  for (const auto &ext : exts) {
    std::string extension = toUpper(ext);
    if (extension.rfind('*') == extension.size() - 1) // there is a wildcard at play
    {
      extension.resize(extension.rfind('*'));
    }

    std::size_t found = toUpper(filename).rfind(extension);
    if (found != std::string::npos) {
      g_log.debug() << "matched extension \"" << extension << "\" based on \"" << ext << "\"\n";
      return filename.substr(found); // grab the actual extensions found
    }
  }

  g_log.debug() << "Failed to find extension. Just using last \'.\'\n";
  std::size_t pos = filename.find_last_of('.');
  if (pos != std::string::npos) {
    return filename.substr(pos);
  }

  // couldn't find an extension
  return "";
}

std::vector<IArchiveSearch_sptr> FileFinderImpl::getArchiveSearch(const Kernel::FacilityInfo &facility) {
  std::vector<IArchiveSearch_sptr> archs;

  // get the searchive option from config service and format it
  std::string archiveOpt = Kernel::ConfigService::Instance().getString("datasearch.searcharchive");
  std::transform(archiveOpt.begin(), archiveOpt.end(), archiveOpt.begin(), tolower);

  // if it is turned off, not specified, or the facility doesn't have
  // IArchiveSearch defined, return an empty vector
  if (archiveOpt.empty() || archiveOpt == "off" || facility.archiveSearch().empty())
    return archs;

  // determine if the user wants archive search for this facility
  auto createArchiveSearch = bool(archiveOpt == "all");

  // then see if the facility name appears in the list or if we just want the
  // default facility
  if (!createArchiveSearch) {
    std::string faciltyName = facility.name();
    std::transform(faciltyName.begin(), faciltyName.end(), faciltyName.begin(), tolower);
    if (archiveOpt == "on") { // only default facilty
      std::string defaultFacility = Kernel::ConfigService::Instance().getString("default.facility");
      std::transform(defaultFacility.begin(), defaultFacility.end(), defaultFacility.begin(), tolower);
      createArchiveSearch = bool(faciltyName == defaultFacility);
    } else { // everything in the list
      createArchiveSearch = bool(archiveOpt.find(faciltyName) != std::string::npos);
    }
  }

  // put together the list of IArchiveSearch to use
  if (createArchiveSearch) {
    for (const auto &facilityname : facility.archiveSearch()) {
      g_log.debug() << "get archive search for the facility..." << facilityname << "\n";
      archs.emplace_back(ArchiveSearchFactory::Instance().create(facilityname));
    }
  }
  return archs;
}

/**
 * Find a path to a single file from a hint.
 * @param hintstr :: hint string to look for filename.
 * @param extensionsProvided :: Vector of aditional file extensions to consider. Optional.
 *                  If not provided, facility extensions used.
 * @param useOnlyExtensionsProvided :: Optional bool.
 *                  If it's true (and extensionsProvided is not empty),
 *                  search for the file using extensionsProvided only.
 *                  If it's false, use extensionsProvided AND facility extensions.
 * @return A vector of full paths or empty vector
 */
const API::Result<std::string> FileFinderImpl::findRun(const std::string &hintstr,
                                                       const std::vector<std::string> &extensionsProvided,
                                                       const bool useOnlyExtensionsProvided) const {
  std::string hint = Kernel::Strings::strip(hintstr);
  g_log.debug() << "vector findRun(\'" << hint << "\', exts[" << extensionsProvided.size() << "])\n";

  // if partial filename or run number is not supplied, return here
  if (hint.empty())
    return API::Result<std::string>("", "File not found.");

  // if it looks like a full filename just do a quick search for it
  Poco::Path hintPath(hint);
  if (!hintPath.getExtension().empty()) {
    // check in normal search locations
    g_log.debug() << "hintPath is not empty, check in normal search locations"
                  << "\n";
    std::string path = getFullPath(hint);
    if (!path.empty()) {
      try {
        if (Poco::File(path).exists()) {
          g_log.information() << "found path = " << path << '\n';
          return API::Result<std::string>(path);
        }
      } catch (Poco::Exception &) {
      }
    } else {
      g_log.debug() << "Unable to find files via directory search with the "
                       "filename that looks like a full filename"
                    << "\n";
    }
  }

  // get instrument and facility
  const Kernel::InstrumentInfo instrument = this->getInstrument(hint);
  const Kernel::FacilityInfo &facility = instrument.facility();
  // get facility extensions
  const std::vector<std::string> facilityExtensions = facility.extensions();

  // Do we need to try and form a filename from our preset rules
  std::string filename(hint);
  std::string extension = getExtension(hint, facilityExtensions);
  if (!facilityExtensions.empty())
    filename = hint.substr(0, hint.rfind(extension));
  if (hintPath.depth() == 0) {
    try {
      if (!facility.noFilePrefix()) {
        filename = makeFileName(filename, instrument);
      }
    } catch (std::invalid_argument &) {
      if (filename.length() >= hint.length()) {
        g_log.information() << "Could not form filename from standard rules '" << filename << "'\n";
      }
    }
  }

  if (filename.empty())
    return API::Result<std::string>("", "File not found");

  // Look first at the original filename then for case variations. This is
  // important
  // on platforms where file names ARE case sensitive.
  // Sorry for the duplication, a last minute fix was required. Ticket #6419 is
  // tasked with a redesign of
  // the whole file finding concept.

  std::set<std::string> filenames;
  filenames.insert(filename);
  if (!getCaseSensitive()) {
    std::string transformed(filename);
    std::transform(filename.begin(), filename.end(), transformed.begin(), toupper);
    filenames.insert(transformed);
    std::transform(filename.begin(), filename.end(), transformed.begin(), tolower);
    filenames.insert(transformed);
  }

  // Merge the extensions & throw out duplicates
  // On Windows throw out ones that only vary in case
  std::vector<std::string> extensionsToSearch;
  extensionsToSearch.reserve(1 + extensionsProvided.size() + facilityExtensions.size());

  if (useOnlyExtensionsProvided) {
    getUniqueExtensions(extensionsProvided, extensionsToSearch);

  } else {
    if (!extension.empty()) {
      extensionsToSearch.emplace_back(extension);

    } else {
      getUniqueExtensions(extensionsProvided, extensionsToSearch);
      getUniqueExtensions(facilityExtensions, extensionsToSearch);
    }
  }

  // determine which archive search facilities to use
  std::vector<IArchiveSearch_sptr> archs = getArchiveSearch(facility);

  auto path = getPath(archs, filenames, extensionsToSearch);
  if (path) {
    g_log.information() << "Found path = " << path << '\n';
    return path;
  }
  // Path not found

  // If only looked for extension in filename
  if (!useOnlyExtensionsProvided && extensionsToSearch.size() == 1) {

    extensionsToSearch.pop_back(); // No need to search for missing extension again
    getUniqueExtensions(extensionsProvided, extensionsToSearch);
    getUniqueExtensions(facilityExtensions, extensionsToSearch);

    g_log.warning() << "Extension ['" << extension << "'] not found.\n";
    g_log.warning() << "Searching for other facility extensions." << std::endl;

    path = getPath(archs, filenames, extensionsToSearch);
    if (path) {
      g_log.information() << "Found path = " << path << '\n';
      return path;
    }
  }
  g_log.information() << "Unable to find run with hint " << hint << "\n";
  return API::Result<std::string>("", path.errors());
}

/**
 * Given a set of already determined extensions and new extensions,
 * create a set of all extensions.
 * If not in an extension-is-case-sensitive environment, only add the
 * lower case OR upper case version of the extension
 * @param extensionsToAdd :: a vector of extensions to add
 * @param uniqueExts :: a vector of currently included extensions
 */
void FileFinderImpl::getUniqueExtensions(const std::vector<std::string> &extensionsToAdd,
                                         std::vector<std::string> &uniqueExts) const {
  const bool isCaseSensitive = getCaseSensitive();
  for (const auto &cit : extensionsToAdd) {
    std::string transformed(cit);
    if (!isCaseSensitive) {
      std::transform(cit.begin(), cit.end(), transformed.begin(), tolower);
    }
    const auto searchItr = std::find(uniqueExts.begin(), uniqueExts.end(), transformed);
    if (searchItr == uniqueExts.end()) {
      uniqueExts.emplace_back(transformed);
    }
  }
}

/**
 * Performs validation on the search text entered into the File Finder. It will
 * return an error message if a problem is found.
 * @param searchText :: The text to validate.
 * @return An error message if something is invalid.
 */
std::string FileFinderImpl::validateRuns(const std::string &searchText) const {
  if (!isASCII(searchText))
    return "An unsupported non-ASCII character was found in the search text.";
  return "";
}

/**
 * Find a list of files file given a hint. Calls findRun internally.
 * @param hintstr :: Comma separated list of hints to findRun method.
 *  Can also include ranges of runs, e.g. 123-135 or equivalently 123-35.
 *  Only the beginning of a range can contain an instrument name.
 * @param extensionsProvided :: Vector of allowed file extensions. Optional.
 *                If provided, this provides the only extensions searched for.
 *                If not provided, facility extensions used.
 * @param useOnlyExtensionsProvided:: Optional bool. If it's true (and exts is not empty),
                           search the for the file using exts only.
                           If it's false, use exts AND facility extensions.
 * @return A vector of full paths or empty vector
 * @throw std::invalid_argument if the argument is malformed
 * @throw Exception::NotFoundError if a file could not be found
 */
std::vector<std::string> FileFinderImpl::findRuns(const std::string &hintstr,
                                                  const std::vector<std::string> &extensionsProvided,
                                                  const bool useOnlyExtensionsProvided) const {
  auto const error = validateRuns(hintstr);
  if (!error.empty())
    throw std::invalid_argument(error);

  std::string hint = Kernel::Strings::strip(hintstr);
  g_log.debug() << "findRuns hint = " << hint << "\n";
  std::vector<std::string> res;
  Mantid::Kernel::StringTokenizer hints(
      hint, ",", Mantid::Kernel::StringTokenizer::TOK_TRIM | Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  static const boost::regex digits("[0-9]+");
  auto h = hints.begin();

  std::string instrSName;
  for (; h != hints.end(); ++h) {
    // Quick check for a filename
    bool fileSuspected = false;
    // Assume if the hint contains either a "/" or "\" it is a filename..
    if ((*h).find("\\") != std::string::npos) {
      fileSuspected = true;
    }
    if ((*h).find("/") != std::string::npos) {
      fileSuspected = true;
    }
    if ((*h).find(ALLOWED_SUFFIX) != std::string::npos) {
      fileSuspected = true;
    }

    Mantid::Kernel::StringTokenizer range(
        *h, "-", Mantid::Kernel::StringTokenizer::TOK_TRIM | Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    if ((range.count() > 2) && (!fileSuspected)) {
      throw std::invalid_argument("Malformed range of runs: " + *h);
    } else if ((range.count() == 2) && (!fileSuspected)) {
      std::pair<std::string, std::string> p1 = toInstrumentAndNumber(range[0]);
      if (boost::algorithm::istarts_with(hint, "PG3")) {
        instrSName = "PG3";
      }
      std::string run = p1.second;
      size_t nZero = run.size(); // zero padding
      if (range[1].size() > nZero) {
        throw std::invalid_argument("Malformed range of runs: " + *h +
                                    ". The end of string value is longer than "
                                    "the instrument's zero padding");
      }
      auto runNumber = boost::lexical_cast<int>(run);
      std::string runEnd = run;
      // Adds zero padding to end of range.
      runEnd.replace(runEnd.end() - range[1].size(), runEnd.end(), range[1]);

      // Throw if runEnd contains something else other than a digit.
      if (!boost::regex_match(runEnd, digits))
        throw std::invalid_argument("Malformed range of runs: Part of the run "
                                    "has a non-digit character in it.");

      auto runEndNumber = boost::lexical_cast<int>(runEnd);
      if (runEndNumber < runNumber) {
        throw std::invalid_argument("Malformed range of runs: " + *h);
      }
      std::string previousPath, previousExt;
      for (int irun = runNumber; irun <= runEndNumber; ++irun) {
        run = std::to_string(irun);
        while (run.size() < nZero)
          run.insert(0, "0");

        // Quick check if file can be created from previous successfully found
        // path/extension
        if (!previousPath.empty() && !previousExt.empty()) {
          try {
            const Poco::File file(previousPath + p1.first + run + previousExt);
            if (file.exists()) {
              res.emplace_back(file.path());
              continue;
            }
          } catch (...) {
            // Clear cached path and extension
            previousPath = previousExt = "";
          }
        }

        std::string path;
        if (boost::algorithm::istarts_with(hint, "PG3")) {
          path = findRun(instrSName + run, extensionsProvided, useOnlyExtensionsProvided).result();
        } else {
          path = findRun(p1.first + run, extensionsProvided, useOnlyExtensionsProvided).result();
        }

        if (!path.empty()) {
          // Cache successfully found path and extension
          auto tempPath = Poco::Path(path);
          previousExt = "." + tempPath.getExtension();
          previousPath = tempPath.makeParent().toString();
          res.emplace_back(path);
        } else {
          throw Kernel::Exception::NotFoundError("Unable to find file:", run);
        }
      }
    } else {
      std::string path;
      // Special check for "PG3", to cope with situation like '48314,48316'.
      if (boost::algorithm::istarts_with(hint, "PG3")) {
        if (h == hints.begin()) {
          instrSName = "PG3";
          path = findRun(*h, extensionsProvided, useOnlyExtensionsProvided).result();
        } else {
          path = findRun(instrSName + *h, extensionsProvided, useOnlyExtensionsProvided).result();
        }
      } else {
        path = findRun(*h, extensionsProvided, useOnlyExtensionsProvided).result();
      }
      if (!path.empty()) {
        res.emplace_back(path);
      } else {
        throw Kernel::Exception::NotFoundError("Unable to find file:", *h);
      }
    }
  }

  return res;
}

const API::Result<std::string>
FileFinderImpl::getISISInstrumentDataCachePath(const std::string &cachePathToSearch,
                                               const std::set<std::string> &filenames,
                                               const std::vector<std::string> &exts) const {
  std::string errors;
  auto dataCache = API::ISISInstrumentDataCache(cachePathToSearch);

  for (const auto &filename : filenames) {

    std::string parentDirPath;

    try {
      parentDirPath = dataCache.getFileParentDirectoryPath(filename);

    } catch (const std::invalid_argument &e) {
      errors += "Data cache: " + std::string(e.what());
      return API::Result<std::string>("", errors);

    } catch (const Json::Exception &e) {
      errors += "Data cache: Failed parsing to JSON: " + std::string(e.what()) +
                "Error likely due to accessing instrument index file while it was being updated on IDAaaS.";
      return API::Result<std::string>("", errors);
    }

    if (!std::filesystem::exists(parentDirPath)) {
      errors += "Data cache: Directory not found: " + parentDirPath;
      return API::Result<std::string>("", errors);
    }

    for (const auto &ext : exts) {
      std::filesystem::path filePath(parentDirPath + '/' + filename + ext);

      try { // Catches error for permission denied
        if (std::filesystem::exists(filePath)) {
          return API::Result<std::string>(filePath.string());
        }
      } catch (const std::filesystem::filesystem_error &e) {
        errors += "Data cache: " + std::string(e.what());
        return API::Result<std::string>("", errors);
      }
    }
    errors += "Data cache: " + filename + " not found in " + parentDirPath;
  }
  return API::Result<std::string>("", errors);
}

/**
 * Return the path to the file found in archive
 * @param archs :: A list of archives to search
 * @param filenames :: A list of filenames (without extensions) to pass to the
 * archive
 * @param exts :: A list of extensions to check for in turn against each file
 * @return The full path if the file exists and can be found in one of the
 * search locations
 *  or an empty string otherwise.
 */
const API::Result<std::string> FileFinderImpl::getArchivePath(const std::vector<IArchiveSearch_sptr> &archs,
                                                              const std::set<std::string> &filenames,
                                                              const std::vector<std::string> &exts) const {
  g_log.debug() << "getArchivePath([IArchiveSearch_sptr], [ ";
  for (const auto &iter : filenames)
    g_log.debug() << iter << " ";
  g_log.debug() << "], [ ";
  for (const auto &iter : exts)
    g_log.debug() << iter << " ";
  g_log.debug() << "])\n";

  string errors = "";
  for (const auto &arch : archs) {
    try {
      g_log.debug() << "Getting archive path for requested files\n";
      auto path = arch->getArchivePath(filenames, exts);
      if (path)
        return path;
      else
        errors += path.errors();
    } catch (...) {
    }
  }
  return API::Result<std::string>("", errors);
}

/**
 * Return the full path to the file given its name, checking local directories
 * first.
 * @param archs :: A list of archives to search
 * @param filenames :: A list of filenames (without extensions) to pass to the
 * archive
 * @param exts :: A list of extensions to check for in turn against each file
 * @return The full path if the file exists and can be found in one of the
 * search locations
 *  or an empty string otherwise.
 */
const API::Result<std::string> FileFinderImpl::getPath(const std::vector<IArchiveSearch_sptr> &archs,
                                                       const std::set<std::string> &filenames,
                                                       const std::vector<std::string> &exts) const {
  std::string path;

  std::vector<std::string> extensions;
  extensions.assign(exts.begin(), exts.end());

  // Remove wild cards.
  extensions.erase(std::remove_if(extensions.begin(), extensions.end(), containsWildCard), extensions.end());

  const std::vector<std::string> &searchPaths = Kernel::ConfigService::Instance().getDataSearchDirs();

  // Before we try any globbing, make sure we exhaust all reasonable attempts at
  // constructing the possible filename.
  // Avoiding the globbing of getFullPath() for as long as possible will help
  // performance when calling findRuns()
  // with a large range of files, especially when searchPaths consists of
  // folders containing a large number of runs.
  for (const auto &extension : extensions) {
    for (const auto &filename : filenames) {
      for (const auto &searchPath : searchPaths) {
        try {
          const Poco::Path filePath(searchPath, filename + extension);
          const Poco::File file(filePath);
          if (file.exists())
            return API::Result<std::string>(filePath.toString());

        } catch (Poco::Exception &) { /* File does not exist, just carry on. */
        }
      }
    }
  }

  for (const auto &extension : extensions) {
    for (const auto &filename : filenames) {
      path = getFullPath(filename + extension);
      try {
        if (!path.empty() && Poco::File(path).exists()) {
          g_log.debug() << "path returned from getFullPath() = " << path << '\n';
          return API::Result<std::string>(path);
        }
      } catch (std::exception &e) {
        g_log.error() << "Cannot open file " << path << ": " << e.what() << '\n';
        return API::Result<std::string>("", "Cannot open file.");
      }
    }
  }

  // Search data cache
  string errors;
  std::filesystem::path cachePathToSearch(Kernel::ConfigService::Instance().getString("datacachesearch.directory"));
  // Only expect to find path to data cache on IDAaaS
  if (std::filesystem::exists(cachePathToSearch)) {

    API::Result<std::string> cacheFilePath =
        getISISInstrumentDataCachePath(cachePathToSearch.string(), filenames, exts);

    if (cacheFilePath) {
      return cacheFilePath;
    }
    errors += cacheFilePath.errors();

  } else {
    g_log.debug() << "Data cache directory not found, proceeding with the search." << std::endl;
    errors += "Could not find data cache directory: " + cachePathToSearch.string() + '\n';
  }

  // Search the archive
  if (!archs.empty()) {
    g_log.debug() << "Search the archives\n";
    const auto archivePath = getArchivePath(archs, filenames, exts);
    if (archivePath) {
      try {
        if (Poco::File(archivePath.result()).exists())
          return archivePath;
      } catch (std::exception &e) {
        g_log.error() << "Cannot open file " << archivePath << ": " << e.what() << '\n';
        return API::Result<std::string>("", "Cannot open file.");
      }
    } else
      errors += archivePath.errors();

  } // archs
  return API::Result<std::string>("", errors);
}

std::string FileFinderImpl::toUpper(const std::string &src) const {
  std::string result = src;
  std::transform(result.begin(), result.end(), result.begin(), toupper);
  return result;
}

} // namespace Mantid::API
