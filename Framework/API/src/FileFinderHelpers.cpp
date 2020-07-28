#include "MantidAPI/FileFinderHelpers.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"

#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/SAX/Attributes.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/SAXParser.h>
#include <boost/regex.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::Types::Core;
using namespace Poco::XML;

namespace {
/// static logger object
Mantid::Kernel::Logger g_log("FileFinderHelpers");

// used to terminate SAX process
class DummyException {
public:
  std::string m_validFrom;
  std::string m_validTo;
  DummyException(const std::string &validFrom, const std::string &validTo)
      : m_validFrom(validFrom), m_validTo(validTo) {}
};
// SAX content handler for grapping stuff quickly from IDF
class myContentHandler : public Poco::XML::ContentHandler {
  void startElement(const XMLString & /*uri*/, const XMLString &localName,
                    const XMLString & /*qname*/,
                    const Attributes &attrList) override {
    if (localName == "instrument" || localName == "parameter-file") {
      throw DummyException(
          static_cast<std::string>(attrList.getValue("", "valid-from")),
          static_cast<std::string>(attrList.getValue("", "valid-to")));
    }
  }
  void endElement(const XMLString & /*uri*/, const XMLString & /*localName*/,
                  const XMLString & /*qname*/) override {}
  void startDocument() override {}
  void endDocument() override {}
  void characters(const XMLChar /*ch*/[], int /*start*/,
                  int /*length*/) override {}
  void endPrefixMapping(const XMLString & /*prefix*/) override {}
  void ignorableWhitespace(const XMLChar /*ch*/[], int /*start*/,
                           int /*length*/) override {}
  void processingInstruction(const XMLString & /*target*/,
                             const XMLString & /*data*/) override {}
  void setDocumentLocator(const Locator * /*loc*/) override {}
  void skippedEntity(const XMLString & /*name*/) override {}
  void startPrefixMapping(const XMLString & /*prefix*/,
                          const XMLString & /*uri*/) override {}
};
} // namespace

namespace Mantid::API {

/** A given instrument may have multiple definition files associated with it.
 *This method returns a file name which identifies a given instrument definition
 *for a given instrument.
 *The instrument geometry can be loaded from either a ".xml" file (old-style
 *IDF) or a ".hdf5/.nxs" file (new-style nexus).
 *The filename is required to be of the form InstrumentName + _Definition +
 *Identifier + extension. The identifier then is the part of a filename that
 *identifies the instrument definition valid at a given date.
 *
 *  If several instrument files files are valid at the given date the file with
 *the most recent from date is selected. If no such files are found the file
 *with the latest from date is selected.
 *
 *  If no file is found for the given instrument, an empty string is returned.
 *
 *  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
 *  @param date :: ISO 8601 date
 *  @return full path of instrument geometry file
 *
 * @throws Exception::NotFoundError If no valid instrument definition filename
 *is found
 */
std::string
FileFinderHelpers::getInstrumentFilename(const std::string &instrumentName,
                                         const std::string &date) {
  const std::vector<std::string> validFormats = {"xml", "nxs", "hdf5"};
  g_log.debug() << "Looking for instrument file for " << instrumentName
                << " that is valid on '" << date << "'\n";
  // Lookup the instrument (long) name
  const std::string instrument(
      Kernel::ConfigService::Instance().getInstrument(instrumentName).name());

  // Get the instrument directories for instrument file search
  const std::vector<std::string> &directoryNames =
      Kernel::ConfigService::Instance().getInstrumentDirectories();

  // matching files sorted with newest files coming first
  const std::vector<std::string> matchingFiles = getResourceFilenames(
      instrument + "_Definition", validFormats, directoryNames, date);
  std::string instFile;
  if (!matchingFiles.empty()) {
    instFile = matchingFiles[0];
    g_log.debug() << "Instrument file selected is " << instFile << '\n';
  } else {
    g_log.debug() << "No instrument file found\n";
  }
  return instFile;
}

/// Search the directory for the Parameter IDF file and return full path name if
/// found, else return "".
//  directoryName must include a final '/'.
std::string FileFinderHelpers::getFullPathParamIDF(std::string instName,
                                                   const std::string &dirHint) {

  constexpr auto lookupFile = [](const std::string &dir,
                                 const std::string &filename) {
    Poco::Path directoryPath(dir);
    directoryPath.makeDirectory();
    // Remove the path from the filename
    Poco::Path filePath(filename);
    const std::string &instrumentFile = filePath.getFileName();

    // First check whether there is a parameter file whose name is the same as
    // the IDF file, but with 'Parameters' instead of 'Definition'.
    std::string definitionPart("_Definition");
    const std::string::size_type prefix_end(
        instrumentFile.find(definitionPart));
    const std::string::size_type suffix_start =
        prefix_end + definitionPart.length();
    // Get prefix and leave case sensitive
    std::string prefix = instrumentFile.substr(0, prefix_end);
    // Make suffix ensuring it has positive length
    std::string suffix = ".xml";
    if (suffix_start < instrumentFile.length()) {
      suffix = instrumentFile.substr(suffix_start, std::string::npos);
    }

    // Assemble parameter file name
    std::string fullPathParamIDF =
        directoryPath.setFileName(prefix + "_Parameters" + suffix).toString();
    if (!Poco::File(fullPathParamIDF)
             .exists()) { // No such file exists, so look
                          // for file based on instrument
                          // ID
                          // given by the prefix
      fullPathParamIDF =
          directoryPath.setFileName(prefix + "_Parameters.xml").toString();
    }

    if (!Poco::File(fullPathParamIDF)
             .exists()) { // No such file exists, indicate
                          // none found in this directory.
      fullPathParamIDF = "";
    }

    return fullPathParamIDF;
  };

  // All inst names are stored as capitals currently,
  // so we need to also do this for case-sensitive filesystems
  std::transform(instName.begin(), instName.end(), instName.begin(), ::toupper);

  // Try the hinted dir first
  if (!dirHint.empty()) {
    const std::string result = lookupFile(dirHint, instName);
    if (!result.empty()) {
      return result;
    }
  }

  Kernel::ConfigServiceImpl &configService = Kernel::ConfigService::Instance();
  std::vector<std::string> directoryNames =
      configService.getInstrumentDirectories();

  for (const auto &dirName : directoryNames) {
    // This will iterate around the directories from user ->etc ->install, and
    // find the first beat file
    const std::string result = lookupFile(dirName, instName);
    if (!result.empty()) {
      std::cout << "Found: " << result << '\n';
      return result;
    }
  }

  std::cout << "Found Nothing \n";
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
std::vector<std::string> FileFinderHelpers::getResourceFilenames(
    const std::string &prefix, const std::vector<std::string> &fileFormats,
    const std::vector<std::string> &directoryNames, const std::string &date) {

  if (date.empty()) {
    // Just use the current date
    g_log.debug() << "No date specified, using current date and time.\n";
    const std::string now =
        Types::Core::DateAndTime::getCurrentTime().toISO8601String();
    // Recursively call this method, but with all parameters.
    return FileFinderHelpers::getResourceFilenames(prefix, fileFormats,
                                                   directoryNames, now);
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

  const boost::regex regex(prefix + ".*\\." + allFileFormats,
                           boost::regex_constants::icase);
  Poco::DirectoryIterator end_iter;
  DateAndTime d(date);

  DateAndTime refDate("1900-01-31 23:59:00"); // used to help determine the most
  // recently starting file, if none match
  DateAndTime refDateGoodFile(
      "1900-01-31 23:59:00"); // used to help determine the most recently

  // Two files could have the same `from` date so multimap is required.
  // Sort with newer dates placed at the beginning
  std::multimap<DateAndTime, std::string, std::greater<DateAndTime>>
      matchingFiles;
  bool foundFile = false;
  std::string mostRecentFile; // path to the file with most recent "valid-from"
  for (const auto &directoryName : directoryNames) {
    // Iterate over the directories from user ->etc ->install, and find the
    // first beat file
    for (Poco::DirectoryIterator dir_itr(directoryName); dir_itr != end_iter;
         ++dir_itr) {

      const auto &filePath = dir_itr.path();
      if (!filePath.isFile())
        continue;

      const std::string &l_filenamePart = filePath.getFileName();
      if (regex_match(l_filenamePart, regex)) {
        const auto &pathName = filePath.toString();
        g_log.debug() << "Found file: '" << pathName << "'\n";

        std::string validFrom, validTo;
        getValidFromTo(pathName, validFrom, validTo);
        g_log.debug() << "File '" << pathName << " valid dates: from '"
                      << validFrom << "' to '" << validTo << "'\n";
        // Use default valid "from" and "to" dates if none were found.
        DateAndTime to, from;
        if (validFrom.length() > 0)
          from.setFromISO8601(validFrom);
        else
          from = refDate;
        if (validTo.length() > 0)
          to.setFromISO8601(validTo);
        else
          to.setFromISO8601("2100-01-01T00:00:00");

        if (from <= d && d <= to) {
          foundFile = true;
          matchingFiles.insert(
              std::pair<DateAndTime, std::string>(from, pathName));
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

    std::transform(matchingFiles.begin(), matchingFiles.end(),
                   std::back_inserter(pathNames),
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
void FileFinderHelpers::getValidFromTo(const std::string &IDFfilename,
                                       std::string &outValidFrom,
                                       std::string &outValidTo) {
  SAXParser pParser;
  // Create on stack to ensure deletion. Relies on pParser also being local
  // variable.
  myContentHandler conHand;
  pParser.setContentHandler(&conHand);

  try {
    pParser.parse(IDFfilename);
  } catch (DummyException &e) {
    outValidFrom = e.m_validFrom;
    outValidTo = e.m_validTo;
  } catch (...) {
    // should throw some sensible here
  }
}

} // Namespace Mantid::API