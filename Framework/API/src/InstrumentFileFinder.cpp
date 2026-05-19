#include "MantidAPI/InstrumentFileFinder.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"

#include <Poco/SAX/Attributes.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/SAXParser.h>
#include <boost/algorithm/string/find.hpp>
#include <boost/regex.hpp>
#include <filesystem>

#include <string>
#include <utility>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::Types::Core;
using namespace Poco::XML;

namespace {
/// static logger object
Mantid::Kernel::Logger g_log("InstrumentFileFinder");

// used to terminate SAX process
class DummyException {
public:
  std::string m_validFrom;
  std::string m_validTo;
  DummyException(std::string validFrom, std::string validTo)
      : m_validFrom(std::move(validFrom)), m_validTo(std::move(validTo)) {}
};
// SAX content handler for grapping stuff quickly from IDF
class myContentHandler : public Poco::XML::ContentHandler {
  void startElement(const XMLString & /*uri*/, const XMLString &localName, const XMLString & /*qname*/,
                    const Attributes &attrList) override {
    if (localName == "instrument" || localName == "parameter-file") {
      throw DummyException(static_cast<std::string>(attrList.getValue("", "valid-from")),
                           static_cast<std::string>(attrList.getValue("", "valid-to")));
    }
  }
  void endElement(const XMLString & /*uri*/, const XMLString & /*localName*/, const XMLString & /*qname*/) override {}
  void startDocument() override {}
  void endDocument() override {}
  void characters(const XMLChar /*ch*/[], int /*start*/, int /*length*/) override {}
  void endPrefixMapping(const XMLString & /*prefix*/) override {}
  void ignorableWhitespace(const XMLChar /*ch*/[], int /*start*/, int /*length*/) override {}
  void processingInstruction(const XMLString & /*target*/, const XMLString & /*data*/) override {}
  void setDocumentLocator(const Locator * /*loc*/) override {}
  void skippedEntity(const XMLString & /*name*/) override {}
  void startPrefixMapping(const XMLString & /*prefix*/, const XMLString & /*uri*/) override {}
};
} // namespace

namespace Mantid::API {

/** If date with only YYYY - MM - DD was provided (no time component), append midnight so
 *   DateAndTime can parse it. Otherwise just return back the same date string
 *
 *   @param date :: string of date read from file
 */
const std::string InstrumentFileFinder::getNormalisedDate(const std::string &date) {
  static const boost::regex dateOnlyRegex("\\d{4}-\\d{2}-\\d{2}");
  return boost::regex_match(date, dateOnlyRegex) ? date + "T00:00:00" : date;
};

/** This method returns a file name which finds a file which contains the given instrument name + search term
 *  and is valid at the given date.
 *
 *  The search will look for filenames which start with InstrumentName + SearchTerm and match the provided file formats.
 *
 *  If several files are found with valid names and valid date ranges, the file with the most recent "valid-from" date
 * is selected. If files are found with valid names but which do not match the date, the file with the most recent
 * "valid-from" date is selected.
 *
 *  If no file is found to have a valid name, an empty string is returned.
 *
 *  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
 *  @param date :: ISO 8601 date
 *  @param searchTerm :: Snippet expected as part of filename eg. "_Definition" or "_Parameters"
 *  @param fileFormats :: Acceptable file extensions
 *  @param dirHint :: Any non-standard directory that should be search alongside the Instrument Directories
 *  @return full path of the file or empty string
 */
std::string InstrumentFileFinder::getFilenameByInstrumentDateAndSearchTerm(const std::string &instrumentName,
                                                                           const std::string &date,
                                                                           const std::string &searchTerm,
                                                                           const std::vector<std::string> &fileFormats,
                                                                           const std::string &dirHint) {
  std::string fileType;
  if (searchTerm == "_Definition")
    fileType = "instrument file";
  else if (searchTerm == "_Parameters")
    fileType = "parameter file";
  else
    fileType = searchTerm + " file";

  g_log.debug() << "Looking for " << fileType << " for " << instrumentName << " that is valid on '" << date << "'\n";
  // Lookup the instrument (long) name, falling back to the provided name if not found in any facility
  std::string instrument;
  try {
    instrument = Kernel::ConfigService::Instance().getInstrument(instrumentName).name();
  } catch (const Kernel::Exception::NotFoundError &) {
    instrument = instrumentName;
  }

  // Build the directory search list: dirHint (if any) is checked first so that
  // parameter files co-located with the IDF in a non-standard directory (e.g.
  // unit_testing/) are preferred over files in the standard instrument dirs.
  const std::vector<std::string> &configDirs = Kernel::ConfigService::Instance().getInstrumentDirectories();
  std::vector<std::string> directoryNames;
  if (!dirHint.empty()) {
    directoryNames.push_back(dirHint);
    directoryNames.insert(directoryNames.end(), configDirs.begin(), configDirs.end());
  } else {
    directoryNames = configDirs;
  }

  // matching files sorted with newest files coming first
  const std::vector<std::string> matchingFiles =
      getResourceFilenames(instrument + searchTerm, fileFormats, directoryNames, date);
  std::string foundFile;
  if (!matchingFiles.empty()) {
    foundFile = matchingFiles[0];
    g_log.debug() << "The " << fileType << " selected is " << foundFile << '\n';
  } else {
    g_log.debug() << "No " << fileType << " found\n";
  }
  return foundFile;
}

/** A given instrument may have multiple definition files associated with it.
 *  This method returns a file name which identifies a given instrument definition
 *  for a given instrument.
 *  The instrument geometry can be loaded from either a ".xml" file (old-style
 *  IDF) or a ".hdf5/.nxs" file (new-style nexus).
 *
 *  The search will look for filenames which start with InstrumentName + _Definition and match the above extensions.
 *
 *  If several files are found with valid names and valid date ranges, the file with the most recent "valid-from" date
 * is selected. If files are found with valid names but which do not match the date, the file with the most recent
 * "valid-from" date is selected.
 *
 *  If no file is found to have a valid name, an empty string is returned.
 *
 *  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
 *  @param date :: ISO 8601 date
 *  @return full path of instrument geometry file
 */
std::string InstrumentFileFinder::getInstrumentFilename(const std::string &instrumentName, const std::string &date) {
  return getFilenameByInstrumentDateAndSearchTerm(instrumentName, date, "_Definition", {"xml", "nxs", "hdf5"});
}

/** A given instrument may also have multiple parameter files associated with it.
 *  This method returns a file name which identifies a parameter file associated with the
 *  given date for a given instrument.
 *  The parameter can be loaded from either a ".xml" file (old-style
 *  IDF) or a ".hdf5/.nxs" file (new-style nexus).
 *
 *  The search will look for filenames which start with InstrumentName + _Parameters and match the above extensions.
 *
 *  If several files are found with valid names and valid date ranges, the file with the most recent "valid-from" date
 *  is selected. If files are found with valid names but which do not match the date, the file with the most recent
 *  "valid-from" date is selected.
 *
 *  If no file is found to have a valid name, an empty string is returned.
 *
 *  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
 *  @param date :: ISO 8601 date
 *  @param dirHint :: Any non-standard directory that should be search alongside the Instrument Directories
 *  @return full path of instrument geometry file
 */
std::string InstrumentFileFinder::getParameterFilename(const std::string &instrumentName, const std::string &date,
                                                       const std::string &dirHint) {
  return getFilenameByInstrumentDateAndSearchTerm(instrumentName, date, "_Parameters", {"xml"}, dirHint);
}

/// Search the directory for the Parameter IDF file and return full path name if
/// found, else return "".
//  directoryName must include a final '/'.
std::string InstrumentFileFinder::getParameterPath(const std::string &instName, const std::string &dirHint) {
  // Remove the path from the filename, some legacy callers will pass in
  // a full path rather than a filename
  std::filesystem::path filePath(instName);
  const std::string filename = filePath.filename().string();

  // Try the hinted dir first
  if (!dirHint.empty()) {
    const std::string result = lookupIPF(dirHint, filename);
    if (!result.empty()) {
      return result;
    }
  }

  const Kernel::ConfigServiceImpl &configService = Kernel::ConfigService::Instance();
  const std::vector<std::string> directoryNames = configService.getInstrumentDirectories();

  for (const auto &dirName : directoryNames) {
    // This will iterate around the directories from user ->etc ->install, and
    // find the first beat file
    const std::string result = lookupIPF(dirName, filename);
    if (!result.empty()) {
      g_log.debug() << "Found: " << result << '\n';
      return result;
    }
  }

  g_log.debug() << "Found Nothing \n";
  return "";
}

std::string InstrumentFileFinder::lookupIPF(const std::string &dir, std::string filename) {
  const std::string ext = ".xml";
  // Remove .xml for example if abc.xml was passed
  boost::algorithm::ierase_all(filename, ext);

  const std::string suffixSeperator("_Definition");

  std::string prefix;
  std::string suffix;

  if (auto sepPos = boost::algorithm::ifind_first(filename, suffixSeperator)) {
    prefix = std::string(filename.begin(), sepPos.begin());
    suffix = std::string(sepPos.end(), filename.end());
  } else {
    prefix = filename;
  }

  std::filesystem::path directoryPath(dir);

  // Assemble parameter file name
  std::string fullPathParamIDF = (directoryPath / (prefix + "_Parameters" + suffix + ext)).string();

  if (std::filesystem::exists(fullPathParamIDF)) {
    return fullPathParamIDF;
  }

  fullPathParamIDF = (directoryPath / (prefix + "_Parameters" + ext)).string();
  if (std::filesystem::exists(fullPathParamIDF)) {
    return fullPathParamIDF;
  }

  return "";
}

/** Compile a list of files in compliance with name pattern-matching, file
 * format, and date-stamp constraints
 *
 * Ideally, the valid-from and valid-to of any valid file should encapsulate
 * argument date. If this is not possible, then the file with the most recent
 * valid-from stamp is selected.
 *
 * @param prefix :: the name of a valid file must begin with this pattern
 * @param fileFormats :: the extension of a valid file must be one of these
 * formats
 * @param directoryNames :: search only in these directories
 * @param date :: the valid-from and valid-to of a valid file should encapsulate
 * this date
 * @return list of absolute paths for each valid file
 */
std::vector<std::string> InstrumentFileFinder::getResourceFilenames(const std::string &prefix,
                                                                    const std::vector<std::string> &fileFormats,
                                                                    const std::vector<std::string> &directoryNames,
                                                                    const std::string &date) {

  if (date.empty()) {
    // Just use the current date
    g_log.debug() << "No date specified, using current date and time.\n";
    const std::string now = Types::Core::DateAndTime::getCurrentTime().toISO8601String();
    // Recursively call this method, but with all parameters.
    return InstrumentFileFinder::getResourceFilenames(prefix, fileFormats, directoryNames, now);
  }

  // Join all the file formats into a single string
  std::stringstream ss;
  ss << "(";
  for (size_t i = 0; i < fileFormats.size(); ++i) {
    if (i != 0)
      ss << "|";
    ss << fileFormats[i];
  }
  ss << ")";
  const std::string allFileFormats = ss.str();

  const boost::regex regex(prefix + ".*\\." + allFileFormats, boost::regex_constants::icase);

  // Normalise date: if only YYYY-MM-DD was provided (no time component), append midnight so
  // DateAndTime can parse it. Parameter files commonly store date-only valid-from attributes.
  const std::string normalisedDate = getNormalisedDate(date);

  DateAndTime d;
  try {
    d = DateAndTime(normalisedDate);
  } catch (const std::invalid_argument &) {
    // Some legacy data files store dates in non-ISO8601 formats.
    // In this case fall back to the current time so we select the most recent matching file.
    g_log.warning() << "Could not parse date '" << date
                    << "' as ISO8601; using current time for instrument file lookup.\n";
    d = DateAndTime::getCurrentTime();
  }

  DateAndTime refDate("1899-01-01 23:59:00"); // used to help determine the most
  // recently starting file, if none match
  DateAndTime refDateGoodFile("1899-01-01 23:59:00"); // used to help determine the most recently

  // Two files could have the same `from` date so multimap is required.
  // Sort with newer dates placed at the beginning
  std::multimap<DateAndTime, std::string, std::greater<DateAndTime>> matchingFiles;
  bool foundFile = false;
  std::string mostRecentFile; // path to the file with most recent "valid-from"
  for (const auto &directoryName : directoryNames) {
    // Iterate over the directories from user ->etc ->install, and find the
    // first beat file
    for (const auto &dir_entry : std::filesystem::directory_iterator(directoryName)) {

      const auto &filePath = dir_entry.path();
      if (!std::filesystem::is_regular_file(filePath))
        continue;

      const std::string l_filenamePart = filePath.filename().string();
      if (regex_match(l_filenamePart, regex)) {
        const std::string pathName = filePath.string();
        g_log.debug() << "Found file: '" << pathName << "'\n";

        std::string validFrom, validTo;
        getValidFromTo(pathName, validFrom, validTo);
        g_log.debug() << "File '" << pathName << " valid dates: from '" << validFrom << "' to '" << validTo << "'\n";
        // Use default valid "from" and "to" dates if none were found.
        // Normalise date-only strings (YYYY-MM-DD) to full datetimes before parsing.
        // Some legacy instrument files store dates in non-ISO8601 formats; treat them as lowest-priority
        // catch-alls (valid for all time) so they are still considered but ranked last.
        DateAndTime to, from;
        try {
          if (validFrom.length() > 0) {
            const std::string normFrom = getNormalisedDate(validFrom);
            from.setFromISO8601(normFrom);
          } else {
            from = refDate;
          }
        } catch (const std::invalid_argument &) {
          g_log.debug() << "Could not parse valid-from='" << validFrom << "' in '" << pathName
                        << "'; treating as lowest priority.\n";
          from = refDate;
        }
        try {
          if (validTo.length() > 0) {
            const std::string normTo = getNormalisedDate(validTo);
            to.setFromISO8601(normTo);
          } else {
            to.setFromISO8601("2100-01-01T00:00:00");
          }
        } catch (const std::invalid_argument &) {
          g_log.debug() << "Could not parse valid-to='" << validTo << "' in '" << pathName
                        << "'; treating as lowest priority.\n";
          to.setFromISO8601("2100-01-01T00:00:00");
        }

        if (from <= d && d <= to) {
          foundFile = true;
          matchingFiles.insert(std::pair<DateAndTime, std::string>(from, pathName));
        }
        // Consider the most recent file in the absence of matching files
        if (!foundFile && (from >= refDate)) {
          refDate = from;
          mostRecentFile = pathName;
        }
      }
    }
  }

  // Retrieve the file names only
  std::vector<std::string> pathNames;
  if (!matchingFiles.empty()) {
    pathNames.reserve(matchingFiles.size());

    std::transform(matchingFiles.begin(), matchingFiles.end(), std::back_inserter(pathNames),
                   [](const auto &elem) { return elem.second; });
  } else {
    pathNames.emplace_back(std::move(mostRecentFile));
  }

  return pathNames;
}

/** Return from an IDF the values of the valid-from and valid-to attributes
 *
 *  @param IDFfilename :: Full path of an IDF
 *  @param[out] outValidFrom :: Used to return valid-from date
 *  @param[out] outValidTo :: Used to return valid-to date
 */
void InstrumentFileFinder::getValidFromTo(const std::string &IDFfilename, std::string &outValidFrom,
                                          std::string &outValidTo) {
  SAXParser pParser;
  // Create on stack to ensure deletion. Relies on pParser also being local
  // variable.
  myContentHandler conHand;
  pParser.setContentHandler(&conHand);

  try {
    pParser.parse(IDFfilename);
  } catch (const DummyException &e) {
    outValidFrom = e.m_validFrom;
    outValidTo = e.m_validTo;
  } catch (...) {
    // should throw some sensible here
  }
}

} // Namespace Mantid::API
