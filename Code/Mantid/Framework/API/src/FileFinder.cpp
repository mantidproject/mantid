//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IArchiveSearch.h"
#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Strings.h"

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Exception.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <cctype>
#include <algorithm>

#include <boost/algorithm/string.hpp>

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
bool containsWildCard(const std::string &ext) {
  if (std::string::npos != ext.find("*"))
    return true;
  return false;
}
}

namespace Mantid {
namespace API {
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
  std::string libpath =
      Kernel::ConfigService::Instance().getString("plugins.directory");
  if (!libpath.empty()) {
    Kernel::LibraryManager::Instance().OpenAllLibraries(libpath);
  }

// determine from Mantid property how sensitive Mantid should be
#ifdef _WIN32
  m_globOption = Poco::Glob::GLOB_DEFAULT;
#else
  std::string casesensitive =
      Mantid::Kernel::ConfigService::Instance().getString(
          "filefinder.casesensitive");
  if (boost::iequals("Off", casesensitive))
    m_globOption = Poco::Glob::GLOB_CASELESS;
  else
    m_globOption = Poco::Glob::GLOB_DEFAULT;
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
bool FileFinderImpl::getCaseSensitive() const {
  return (m_globOption == Poco::Glob::GLOB_DEFAULT);
}

/**
 * Return the full path to the file given its name
 * @param filename :: A file name (without path) including extension
 * @param ignoreDirs :: If true, directories that match are skipped unless the path given is already absolute
 * @return The full path if the file exists and can be found in one of the
 * search locations
 *  or an empty string otherwise.
 */
std::string FileFinderImpl::getFullPath(const std::string &filename, const bool ignoreDirs) const {
  std::string fName = Kernel::Strings::strip(filename);
  g_log.debug() << "getFullPath(" << fName << ")\n";
  // If this is already a full path, nothing to do
  if (Poco::Path(fName).isAbsolute())
    return fName;

  // First try the path relative to the current directory. Can throw in some
  // circumstances with extensions that have wild cards
  try {
    Poco::File fullPath(Poco::Path().resolve(fName));
    if (fullPath.exists() && (!ignoreDirs || !fullPath.isDirectory()))
      return fullPath.path();
  } catch (std::exception &) {
  }

  const std::vector<std::string> &searchPaths =
      Kernel::ConfigService::Instance().getDataSearchDirs();
  std::vector<std::string>::const_iterator it = searchPaths.begin();
  for (; it != searchPaths.end(); ++it) {
// On windows globbing is note working properly with network drives
// for example a network drive containing a $
// For this reason, and since windows is case insensitive anyway
// a special case is made for windows
#ifdef _WIN32
    if (fName.find("*") != std::string::npos) {
#endif
      Poco::Path path(*it, fName);
      Poco::Path pathPattern(path);
      std::set<std::string> files;
      Kernel::Glob::glob(pathPattern, files, m_globOption);
      if (!files.empty()) {
        Poco::File matchPath(*files.begin());
        if(ignoreDirs && matchPath.isDirectory()) {
          continue;
        }
        return *files.begin();
        
      }
#ifdef _WIN32
    } else {
      Poco::Path path(*it, fName);
      Poco::File file(path);
	  if (file.exists() && !(ignoreDirs && file.isDirectory())) {
        return path.toString();
	  }
    }
#endif
  }
  return "";
}

/** Run numbers can be followed by an allowed string. Check if there is
 *  one, remove it from the name and return the string, else return empty
 *  @param userString run number that may have a suffix
 *  @return the suffix, if there was one
 */
std::string
FileFinderImpl::extractAllowedSuffix(std::string &userString) const {
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
 * @return This will return the default instrument if it cannot be determined.
 */
const Kernel::InstrumentInfo
FileFinderImpl::getInstrument(const string &hint) const {
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
    else if ((instrName.find("SANS2D") == 0) ||
             (instrName.find("sans2d") == 0)) {
      instrName = "SANS2D";
    } else {
      // go forwards looking for the run number to start
      {
        string::const_iterator it = std::find_if(
            instrName.begin(), instrName.end(), std::ptr_fun(isdigit));
        std::string::size_type nChars = std::distance(
            static_cast<string::const_iterator>(instrName.begin()), it);
        instrName = instrName.substr(0, nChars);
      }

      // go backwards looking for the instrument name to end - gets around
      // delimiters
      if (!instrName.empty()) {
        string::const_reverse_iterator it = std::find_if(
            instrName.rbegin(), instrName.rend(), std::ptr_fun(isalpha));
        string::size_type nChars = std::distance(
            it, static_cast<string::const_reverse_iterator>(instrName.rend()));
        instrName = instrName.substr(0, nChars);
      }
    }
    try {
      const Kernel::InstrumentInfo instrument =
          Kernel::ConfigService::Instance().getInstrument(instrName);
      return instrument;
    } catch (Kernel::Exception::NotFoundError &e) {
      g_log.debug() << e.what() << "\n";
    }
  }
  return Kernel::ConfigService::Instance().getInstrument();
}

/**
 * Extracts the instrument name and run number from a hint
 * @param hint :: The name hint
 * @return A pair of instrument name and run number
 */
std::pair<std::string, std::string>
FileFinderImpl::toInstrumentAndNumber(const std::string &hint) const {
  // g_log.debug() << "toInstrumentAndNumber(" << hint << ")\n";
  std::string instrPart;
  std::string runPart;

  if (isdigit(hint[0])) {
    instrPart = Kernel::ConfigService::Instance().getInstrument().shortName();
    runPart = hint;
  } else {
    /// Find the last non-digit as the instrument name can contain numbers
    std::string::const_reverse_iterator it = std::find_if(
        hint.rbegin(), hint.rend(), std::not1(std::ptr_fun(isdigit)));
    // No non-digit or all non-digits
    if (it == hint.rend() || it == hint.rbegin()) {
      throw std::invalid_argument(
          "Malformed hint to FileFinderImpl::makeFileName: " + hint);
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
  Kernel::InstrumentInfo instr =
      Kernel::ConfigService::Instance().getInstrument(instrPart);
  size_t nZero = instr.zeroPadding(irunPart);
  // remove any leading zeros in case there are too many of them
  std::string::size_type i = runPart.find_first_not_of('0');
  runPart.erase(0, i);
  while (runPart.size() < nZero)
    runPart.insert(0, "0");
  if (runPart.size() > nZero && nZero != 0) {
    throw std::invalid_argument(
        "Run number does not match instrument's zero padding");
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
std::string
FileFinderImpl::makeFileName(const std::string &hint,
                             const Kernel::InstrumentInfo &instrument) const {
  // g_log.debug() << "makeFileName(" << hint << ", " << instrument.shortName()
  // << ")\n";
  if (hint.empty())
    return "";

  std::string filename(hint);
  const std::string suffix = extractAllowedSuffix(filename);
  const std::string shortName = instrument.shortName();
  std::string delimiter = instrument.delimiter();

  // see if starts with the provided instrument name
  if (filename.substr(0, shortName.size()) == shortName) {
    filename = filename.substr(shortName.size());
    if ((!delimiter.empty()) &&
        (filename.substr(0, delimiter.size()) == delimiter))
      filename = filename.substr(delimiter.size());

    filename = shortName + filename;
  }

  std::pair<std::string, std::string> p = toInstrumentAndNumber(filename);

  filename = p.first;
  if (!delimiter.empty()) {
    filename += delimiter;
  }
  filename += p.second;

  if (!suffix.empty()) {
    filename += suffix;
  }

  return filename;
}

/**
 * Find the file given a hint. If the name contains a dot(.) then it is assumed
 * that it is already a file stem
 * otherwise calls makeFileName internally.
 * @param hintstr :: The name hint, format: [INSTR]1234[.ext]
 * @param exts :: Optional list of allowed extensions. Only those extensions
 * found in both
 *  facilities extension list and exts will be used in the search. If an
 * extension is given in hint
 *  this argument is ignored.
 * @return The full path to the file or empty string if not found
 */
std::string FileFinderImpl::findRun(const std::string &hintstr,
                                    const std::set<std::string> &exts) const {
  std::string hint = Kernel::Strings::strip(hintstr);
  g_log.debug() << "set findRun(\'" << hintstr << "\', exts[" << exts.size()
                << "])\n";
  if (hint.empty())
    return "";
  std::vector<std::string> exts_v;
  if (!exts.empty())
    exts_v.assign(exts.begin(), exts.end());

  return this->findRun(hint, exts_v);
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
std::string
FileFinderImpl::getExtension(const std::string &filename,
                             const std::vector<std::string> &exts) const {
  g_log.debug() << "getExtension(" << filename << ", exts[" << exts.size()
                << "])\n";

  // go through the list of supplied extensions
  for (auto it = exts.begin(); it != exts.end(); ++it) {
    std::string extension = toUpper(*it);
    if (extension.rfind('*') ==
        extension.size() - 1) // there is a wildcard at play
    {
      extension = extension.substr(0, extension.rfind('*'));
    }

    std::size_t found = toUpper(filename).rfind(extension);
    if (found != std::string::npos) {
      g_log.debug() << "matched extension \"" << extension << "\" based on \""
                    << (*it) << "\"\n";
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

std::string
FileFinderImpl::findRun(const std::string &hintstr,
                        const std::vector<std::string> &exts) const {
  std::string hint = Kernel::Strings::strip(hintstr);
  g_log.debug() << "vector findRun(\'" << hint << "\', exts[" << exts.size()
                << "])\n";

  // if partial filename or run number is not supplied, return here
  if (hint.empty())
    return "";

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
          return path;
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
  const Kernel::FacilityInfo facility = instrument.facility();
  // get facility extensions
  const std::vector<std::string> facility_extensions = facility.extensions();
  // select allowed extensions
  std::vector<std::string> extensions;

  g_log.debug() << "Add facility extensions defined in the Facility.xml file"
                << "\n";
  extensions.assign(facility_extensions.begin(), facility_extensions.end());

  // initialize the archive searcher
  std::vector<IArchiveSearch_sptr> archs;
  { // hide in a local namespace so things fall out of scope
    std::string archiveOpt =
        Kernel::ConfigService::Instance().getString("datasearch.searcharchive");
    std::transform(archiveOpt.begin(), archiveOpt.end(), archiveOpt.begin(),
                   tolower);
    if (!archiveOpt.empty() && archiveOpt != "off" &&
        !facility.archiveSearch().empty()) {
      std::vector<std::string>::const_iterator it =
          facility.archiveSearch().begin();
      for (; it != facility.archiveSearch().end(); ++it) {
        g_log.debug() << "get archive search for the facility..." << *it
                      << "\n";
        archs.push_back(ArchiveSearchFactory::Instance().create(*it));
      }
    }
  }

  // Do we need to try and form a filename from our preset rules
  std::string filename(hint);
  std::string extension = getExtension(hint, extensions);
  if (!extensions.empty())
    filename = hint.substr(0, hint.rfind(extension));
  if (hintPath.depth() == 0) {
    try {
      filename = makeFileName(filename, instrument);
    } catch (std::invalid_argument &) {
      if (filename.length() >= hint.length()) {
        g_log.information() << "Could not form filename from standard rules '"
                            << filename << "'\n";
      }
    }
  }

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
    std::transform(filename.begin(), filename.end(), transformed.begin(),
                   toupper);
    filenames.insert(transformed);
    std::transform(filename.begin(), filename.end(), transformed.begin(),
                   tolower);
    filenames.insert(transformed);
  }

  // Merge the extensions & throw out duplicates
  // On Windows throw out ones that only vary in case
  std::vector<std::string> uniqueExts;
  uniqueExts.reserve(1 + exts.size() + extensions.size());
  if (!extension.empty())
    uniqueExts.push_back(extension);

  auto cend = exts.end();
  for (auto cit = exts.begin(); cit != cend; ++cit) {
    if (getCaseSensitive()) // prune case variations - this is a hack, see above
    {
      std::string transformed(*cit);
      std::transform(cit->begin(), cit->end(), transformed.begin(), tolower);
      auto searchItr =
          std::find(uniqueExts.begin(), uniqueExts.end(), transformed);
      if (searchItr != uniqueExts.end())
        continue;
      std::transform(cit->begin(), cit->end(), transformed.begin(), toupper);
      searchItr = std::find(uniqueExts.begin(), uniqueExts.end(), transformed);
      if (searchItr == uniqueExts.end())
        uniqueExts.push_back(*cit);
    } else {
      auto searchItr = std::find(uniqueExts.begin(), uniqueExts.end(), *cit);
      if (searchItr == uniqueExts.end())
        uniqueExts.push_back(*cit);
    }
  }
  cend = extensions.end();
  for (auto cit = extensions.begin(); cit != cend; ++cit) {
    if (getCaseSensitive()) // prune case variations - this is a hack, see above
    {
      std::string transformed(*cit);
      std::transform(cit->begin(), cit->end(), transformed.begin(), tolower);
      auto searchItr =
          std::find(uniqueExts.begin(), uniqueExts.end(), transformed);
      if (searchItr != uniqueExts.end())
        continue;
      std::transform(cit->begin(), cit->end(), transformed.begin(), toupper);
      searchItr = std::find(uniqueExts.begin(), uniqueExts.end(), transformed);
      if (searchItr == uniqueExts.end())
        uniqueExts.push_back(*cit);
    } else {
      auto searchItr = std::find(uniqueExts.begin(), uniqueExts.end(), *cit);
      if (searchItr == uniqueExts.end())
        uniqueExts.push_back(*cit);
    }
  }

  std::string path = getPath(archs, filenames, uniqueExts);
  if (!path.empty()) {
    g_log.information() << "found path = " << path << '\n';
    return path;
  } else {
    g_log.information() << "Unable to find run with hint " << hint << "\n";
  }

  g_log.information() << "Unable to find file path for " << hint << "\n";

  return "";
}

/**
 * Find a list of files file given a hint. Calls findRun internally.
 * @param hintstr :: Comma separated list of hints to findRun method.
 *  Can also include ranges of runs, e.g. 123-135 or equivalently 123-35.
 *  Only the beginning of a range can contain an instrument name.
 * @return A vector of full paths or empty vector
 * @throw std::invalid_argument if the argument is malformed
 * @throw Exception::NotFoundError if a file could not be found
 */
std::vector<std::string>
FileFinderImpl::findRuns(const std::string &hintstr) const {
  std::string hint = Kernel::Strings::strip(hintstr);
  g_log.debug() << "findRuns hint = " << hint << "\n";
  std::vector<std::string> res;
  Poco::StringTokenizer hints(hint, ",",
                              Poco::StringTokenizer::TOK_TRIM |
                                  Poco::StringTokenizer::TOK_IGNORE_EMPTY);
  Poco::StringTokenizer::Iterator h = hints.begin();

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

    Poco::StringTokenizer range(*h, "-",
                                Poco::StringTokenizer::TOK_TRIM |
                                    Poco::StringTokenizer::TOK_IGNORE_EMPTY);
    if ((range.count() > 2) && (!fileSuspected)) {
      throw std::invalid_argument("Malformed range of runs: " + *h);
    } else if ((range.count() == 2) && (!fileSuspected)) {
      std::pair<std::string, std::string> p1 = toInstrumentAndNumber(range[0]);
      std::string run = p1.second;
      size_t nZero = run.size(); // zero padding
      if (range[1].size() > nZero) {
        throw std::invalid_argument("Malformed range of runs: " + *h +
                                    ". The end of string value is longer than "
                                    "the instrument's zero padding");
      }
      int runNumber = boost::lexical_cast<int>(run);
      std::string runEnd = run;
      // Adds zero padding to end of range.
      runEnd.replace(runEnd.end() - range[1].size(), runEnd.end(), range[1]);

      // Throw if runEnd contains something else other than a digit.
      boost::regex digits("[0-9]+");
      if (!boost::regex_match(runEnd, digits))
        throw std::invalid_argument("Malformed range of runs: Part of the run "
                                    "has a non-digit character in it.");

      int runEndNumber = boost::lexical_cast<int>(runEnd);
      if (runEndNumber < runNumber) {
        throw std::invalid_argument("Malformed range of runs: " + *h);
      }
      for (int irun = runNumber; irun <= runEndNumber; ++irun) {
        run = boost::lexical_cast<std::string>(irun);
        while (run.size() < nZero)
          run.insert(0, "0");
        std::string path = findRun(p1.first + run);
        if (!path.empty()) {
          res.push_back(path);
        } else {
          throw Kernel::Exception::NotFoundError("Unable to find file:", run);
        }
      }
    } else {
      std::string path = findRun(*h);
      if (!path.empty()) {
        res.push_back(path);
      } else {
        throw Kernel::Exception::NotFoundError("Unable to find file:", *h);
      }
    }
  }

  return res;
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
std::string
FileFinderImpl::getArchivePath(const std::vector<IArchiveSearch_sptr> &archs,
                               const std::set<std::string> &filenames,
                               const std::vector<std::string> &exts) const {
  std::string path = "";
  std::vector<IArchiveSearch_sptr>::const_iterator it = archs.begin();
  for (; it != archs.end(); ++it) {
    try {
      path = (*it)->getArchivePath(filenames, exts);
      if (!path.empty()) {
        return path;
      }
    } catch (...) {
    }
  }
  return path;
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
std::string
FileFinderImpl::getPath(const std::vector<IArchiveSearch_sptr> &archs,
                        const std::set<std::string> &filenames,
                        const std::vector<std::string> &exts) const {
  std::string path;

  std::vector<std::string> extensions;
  extensions.assign(exts.begin(), exts.end());

  // Remove wild cards.
  extensions.erase(
      std::remove_if(extensions.begin(), extensions.end(), containsWildCard),
      extensions.end());

  const std::vector<std::string> &searchPaths =
      Kernel::ConfigService::Instance().getDataSearchDirs();

  // Before we try any globbing, make sure we exhaust all reasonable attempts at
  // constructing the possible filename.
  // Avoiding the globbing of getFullPath() for as long as possible will help
  // performance when calling findRuns()
  // with a large range of files, especially when searchPaths consists of
  // folders containing a large number of runs.
  for (auto ext = extensions.begin(); ext != extensions.end(); ++ext) {
    for (auto filename = filenames.begin(); filename != filenames.end();
         ++filename) {
      for (auto searchPath = searchPaths.begin();
           searchPath != searchPaths.end(); ++searchPath) {
        try {
          Poco::Path path(*searchPath, *filename + *ext);
          Poco::File file(path);
          if (file.exists())
            return path.toString();

        } catch (Poco::Exception &) { /* File does not exist, just carry on. */
        }
      }
    }
  }

  for (auto ext = extensions.begin(); ext != extensions.end(); ++ext) {
    std::set<std::string>::const_iterator it = filenames.begin();
    for (; it != filenames.end(); ++it) {
      path = getFullPath(*it + *ext);
      try {
        if (!path.empty() && Poco::File(path).exists()) {
          g_log.debug() << "path returned from getFullPath() = " << path
                        << '\n';
          return path;
        }
      } catch (std::exception &e) {
        g_log.error() << "Cannot open file " << path << ": " << e.what()
                      << '\n';
        return "";
      }
    }
  }

  // Search the archive
  if (archs.size() != 0) {
    g_log.debug() << "Search the archives\n";
    std::string path = getArchivePath(archs, filenames, exts);
    try {
      if (!path.empty() && Poco::File(path).exists()) {
        return path;
      }
    } catch (std::exception &e) {
      g_log.error() << "Cannot open file " << path << ": " << e.what() << '\n';
      return "";
    }

  } // archs

  return "";
}

std::string FileFinderImpl::toUpper(const std::string &src) const {
  std::string result = src;
  std::transform(result.begin(), result.end(), result.begin(), toupper);
  return result;
}

} // API
} // Mantid
