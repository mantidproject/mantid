// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/CheckMantidVersion.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/GitHubApiHelper.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Strings.h"

#include "MantidKernel/StringTokenizer.h"
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>

// jsoncpp
#include <json/json.h>

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CheckMantidVersion)

/// Algorithms name for identification. @see Algorithm::name
const std::string CheckMantidVersion::name() const {
  return "CheckMantidVersion";
}

/// Algorithm's version for identification. @see Algorithm::version
int CheckMantidVersion::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CheckMantidVersion::category() const {
  return "Utility\\Development";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CheckMantidVersion::summary() const {
  return "Checks if there is a more recent version of Mantid available using "
         "the Github API";
}

/** Initialize the algorithm's properties.
 */
void CheckMantidVersion::init() {
  declareProperty("CurrentVersion", "", Direction::Output);
  declareProperty("MostRecentVersion", "", Direction::Output);
  declareProperty("IsNewVersionAvailable", false,
                  "True if a newer version is available, otherwise false",
                  Direction::Output);
}

/** Execute the algorithm.
 */
void CheckMantidVersion::exec() {
  std::string currentVersion = getCurrentVersion();
  setProperty("CurrentVersion", currentVersion);
  std::string mostRecentVersion;

  std::string gitHubReleaseUrl = ConfigService::Instance().getString(
      "CheckMantidVersion.GitHubReleaseURL");
  if (gitHubReleaseUrl.empty()) {
    gitHubReleaseUrl =
        "https://api.github.com/repos/mantidproject/mantid/releases/latest";
  }
  std::string downloadUrl =
      ConfigService::Instance().getString("CheckMantidVersion.DownloadURL");
  if (downloadUrl.empty()) {
    downloadUrl = "http://download.mantidproject.org";
  }

  std::string json;
  try {
    json = getVersionsFromGitHub(gitHubReleaseUrl);
  } catch (Exception::InternetError &ex) {
    if (ex.errorCode() == InternetHelper::HTTP_NOT_MODIFIED) {
      // No changes since last release
      // mostRecentVersion = getCurrentVersion();
      mostRecentVersion =
          "No new versions since " + std::string(MantidVersion::releaseDate());
    } else {
      // any other exception just log quietly and return
      g_log.debug("Cannot get latest version details from " + gitHubReleaseUrl);
      g_log.debug("The address can be changed using the property "
                  "CheckMantidVersion.GitHubReleaseURL");
      g_log.debug(ex.what());
      return;
    }
  }

  bool isNewVersionAvailable = false;
  if (!json.empty()) {
    Json::Reader r;
    Json::Value root;
    bool parseOK = r.parse(json, root);
    if (!parseOK) {
      // just warning. The parser is able to get relevant info even if there are
      // formatting issues like missing quotes or brackets.
      g_log.warning() << "Error found when parsing version information "
                         "retrieved from GitHub as a JSON string. "
                         "Error trying to parse this JSON string: "
                      << json << "\n. Parsing error details: "
                      << r.getFormattedErrorMessages() << '\n';
    }

    std::string gitHubVersionTag;
    try {
      gitHubVersionTag = root["tag_name"].asString();
    } catch (std::runtime_error &re) {
      g_log.error()
          << "Error while trying to get the field 'tag_name' from "
             "the version information retrieved from GitHub. This "
             "algorithm cannot continue and will stop now. Error details: "
          << re.what() << '\n';

      mostRecentVersion = "Could not get information from GitHub";
      setProperty("MostRecentVersion", mostRecentVersion);
      setProperty("IsNewVersionAvailable", isNewVersionAvailable);
      return;
    }

    mostRecentVersion = cleanVersionTag(gitHubVersionTag);
    isNewVersionAvailable =
        isVersionMoreRecent(currentVersion, mostRecentVersion);
    if (isNewVersionAvailable) {
      // output a notice level log
      g_log.notice("A new version of Mantid(" + mostRecentVersion +
                   ") is available for download from " + downloadUrl);
    }
  }

  g_log.information("Current Mantid Version: " + currentVersion);
  g_log.information("Most Recent Mantid Version: " + mostRecentVersion);

  setProperty("MostRecentVersion", mostRecentVersion);
  setProperty("IsNewVersionAvailable", isNewVersionAvailable);
}

/** Cleans the tag name from github to make it similar to that from
 * MantidVersion
 * @param versionTag the version tag that needs cleaning
 * @returns a clean string
 */
std::string
CheckMantidVersion::cleanVersionTag(const std::string &versionTag) const {
  std::string retVal = versionTag;

  retVal = Strings::replaceAll(retVal, "v", "");
  retVal = Strings::replaceAll(retVal, "V", "");
  retVal = Strings::strip(retVal);

  return retVal;
}

/** splits a . separated version string into a vector of integers
 * @param versionString Something like "2.3.4"
 * @returns a vector of [2,3,4]
 */
std::vector<int>
CheckMantidVersion::splitVersionString(const std::string &versionString) const {
  std::vector<int> retVal;
  Mantid::Kernel::StringTokenizer tokenizer(
      versionString, ".",
      Mantid::Kernel::StringTokenizer::TOK_TRIM |
          Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  auto h = tokenizer.begin();

  for (; h != tokenizer.end(); ++h) {
    try {
      auto part = boost::lexical_cast<int>(*h);
      retVal.push_back(part);
    } catch (const boost::bad_lexical_cast &) {
      g_log.error("Failed to convert the following string to an integer '" +
                  *h + "' as part of CheckMantidVersion::splitVersionString");
      retVal.push_back(0);
    }
  }
  return retVal;
}

/** Compare two version strings, tests if the gitHubVersion is more recent
 * @param localVersion Something like "2.3.4"
 * @param gitHubVersion Something like "2.3.4"
 * @returns True if gitHubVersion is more recent
 */
bool CheckMantidVersion::isVersionMoreRecent(
    const std::string &localVersion, const std::string &gitHubVersion) const {
  auto localVersionParts = splitVersionString(localVersion);
  auto gitHubVersionParts = splitVersionString(gitHubVersion);

  for (size_t i = 0; i < gitHubVersionParts.size(); i++) {
    // sanity check
    if (i >= localVersionParts.size()) {
      // ran out of items to compare
      break;
    }

    // the revision number needs to be handled separately
    if (i == 2) {
      if (localVersionParts[i] > 2000) {
        // this is a date string, nightly build
        // state that the local version is up to date
        return false;
      }
    }
    if (gitHubVersionParts[i] > localVersionParts[i]) {
      return true;
    }
    if (gitHubVersionParts[i] < localVersionParts[i]) {
      return false;
    }
  }
  return false;
}

/** Gets the version json for the most recent release from gitHub

@param url : The url to use
@exception Mantid::Kernel::Exception::InternetError : For any unexpected
behaviour.
*/
std::string CheckMantidVersion::getVersionsFromGitHub(const std::string &url) {

  Kernel::GitHubApiHelper inetHelper;
  std::ostringstream os;
  int tzd = 0;

  inetHelper.addHeader(
      "if-modified-since",
      Poco::DateTimeFormatter::format(
          Poco::DateTimeParser::parse(MantidVersion::releaseDate(), tzd),
          Poco::DateTimeFormat::HTTP_FORMAT));
  inetHelper.sendRequest(url, os);
  std::string retVal = os.str();

  return retVal;
}
/** Gets the version of this Mantid
@returns a string of the form "1.2.3[.4]"
*/
std::string CheckMantidVersion::getCurrentVersion() const {
  return Mantid::Kernel::MantidVersion::version();
}

} // namespace DataHandling
} // namespace Mantid
