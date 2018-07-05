#include "MantidDataHandling/DownloadInstrument.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/GitHubApiHelper.h"

// boost
#include <boost/algorithm/string/predicate.hpp>

// Poco
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>
// Visual Studio complains with the inclusion of Poco/FileStream
// disabling this warning.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable : 4250)
#include <Poco/FileStream.h>
#include <Poco/NullStream.h>
#include <Winhttp.h>
#pragma warning(pop)
#else
#include <Poco/FileStream.h>
#include <Poco/NullStream.h>
#include <cstdlib>
#endif

// jsoncpp
#include <json/json.h>

// std
#include <fstream>

namespace Mantid {
namespace DataHandling {
using namespace Kernel;
using namespace Poco::Net;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DownloadInstrument)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
DownloadInstrument::DownloadInstrument() : m_proxyInfo() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string DownloadInstrument::name() const {
  return "DownloadInstrument";
}

/// Algorithm's version for identification. @see Algorithm::version
int DownloadInstrument::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DownloadInstrument::category() const {
  return "DataHandling\\Instrument";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string DownloadInstrument::summary() const {
  return "Checks the Mantid instrument repository against the local "
         "instrument files, and downloads updates as appropriate.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DownloadInstrument::init() {
  using Kernel::Direction;

  declareProperty("ForceUpdate", false, "Ignore cache information");
  declareProperty("FileDownloadCount", 0,
                  "The number of files downloaded by this algorithm",
                  Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DownloadInstrument::exec() {
  StringToStringMap fileMap;
  setProperty("FileDownloadCount", 0);
  try {
    fileMap = processRepository();
  } catch (Mantid::Kernel::Exception::InternetError &ex) {
    std::string errorText(ex.what());
    if (errorText.find("rate limit") != std::string::npos) {
      g_log.notice() << "Instrument Definition Update: " << errorText << '\n';
    } else {
      // log the failure at Notice Level
      g_log.notice("Internet Connection Failed - cannot update instrument "
                   "definitions.");
      // log this error at information level
      g_log.information() << errorText << '\n';
    }
    return;
  }

  if (fileMap.empty()) {
    g_log.notice("All instrument definitions up to date");
  } else {
    std::string s = (fileMap.size() > 1) ? "s" : "";
    g_log.notice() << "Downloading " << fileMap.size() << " file" << s
                   << " from the instrument repository\n";
  }

  for (auto &itMap : fileMap) {
    // download a file
    doDownloadFile(itMap.first, itMap.second);
    if (boost::algorithm::ends_with(itMap.second, "Facilities.xml")) {
      g_log.notice("A new Facilities.xml file has been downloaded, this will "
                   "take effect next time Mantid is started.");
    }
  }

  setProperty("FileDownloadCount", static_cast<int>(fileMap.size()));
}

DownloadInstrument::StringToStringMap DownloadInstrument::processRepository() {
  // get the instrument directories
  auto instrumentDirs =
      Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories();
  Poco::Path installPath(instrumentDirs.back());
  installPath.makeDirectory();
  Poco::Path localPath(instrumentDirs[0]);
  localPath.makeDirectory();

  // get the date of the local github.json file if it exists
  Poco::Path gitHubJson(localPath, "github.json");
  Poco::File gitHubJsonFile(gitHubJson);
  Poco::DateTime gitHubJsonDate(1900, 1, 1);
  bool forceUpdate = this->getProperty("ForceUpdate");
  if ((!forceUpdate) && gitHubJsonFile.exists() && gitHubJsonFile.isFile()) {
    gitHubJsonDate = gitHubJsonFile.getLastModified();
  }

  // get the file list from github
  StringToStringMap headers;
  headers.emplace("if-modified-since",
                  Poco::DateTimeFormatter::format(
                      gitHubJsonDate, Poco::DateTimeFormat::HTTP_FORMAT));
  std::string gitHubInstrumentRepoUrl =
      ConfigService::Instance().getString("UpdateInstrumentDefinitions.URL");
  if (gitHubInstrumentRepoUrl.empty()) {
    throw std::runtime_error(
        "Property UpdateInstrumentDefinitions.URL is not defined, "
        "this should point to the location of the instrument "
        "directory in the github API "
        "e.g. "
        "https://api.github.com/repos/mantidproject/mantid/contents/"
        "instrument.");
  }
  StringToStringMap fileMap;
  try {
    doDownloadFile(gitHubInstrumentRepoUrl, gitHubJson.toString(), headers);
  } catch (Exception::InternetError &ex) {
    if (ex.errorCode() == InternetHelper::HTTP_NOT_MODIFIED) {
      // No changes since last time
      return fileMap;
    } else {
      throw;
    }
  }

  // update local repo files
  Poco::Path installRepoFile(localPath, "install.json");
  StringToStringMap installShas = getFileShas(installPath.toString());
  Poco::Path localRepoFile(localPath, "local.json");
  StringToStringMap localShas = getFileShas(localPath.toString());

  // Parse the server JSON response
  Json::Reader reader;
  Json::Value serverContents;
  Poco::FileStream fileStream(gitHubJson.toString(), std::ios::in);
  if (!reader.parse(fileStream, serverContents)) {
    throw std::runtime_error("Unable to parse server JSON file \"" +
                             gitHubJson.toString() + "\"");
  }
  fileStream.close();

  std::unordered_set<std::string> repoFilenames;

  for (auto &serverElement : serverContents) {
    std::string name = serverElement.get("name", "").asString();
    repoFilenames.insert(name);
    Poco::Path filePath(localPath, name);
    if (filePath.getExtension() != "xml")
      continue;
    std::string sha = serverElement.get("sha", "").asString();
    std::string htmlUrl =
        getDownloadableRepoUrl(serverElement.get("html_url", "").asString());

    // Find shas
    std::string localSha = getValueOrDefault(localShas, name, "");
    std::string installSha = getValueOrDefault(installShas, name, "");
    // Different sha1 on github cf local and global
    // this will also catch when file is only present on github (as local sha
    // will be "")
    if ((sha != installSha) && (sha != localSha)) {
      fileMap.emplace(htmlUrl,
                      filePath.toString()); // ACTION - DOWNLOAD to localPath
    } else if ((!localSha.empty()) && (sha == installSha) &&
               (sha != localSha)) // matches install, but different local
    {
      fileMap.emplace(
          htmlUrl,
          filePath.toString()); // ACTION - DOWNLOAD to localPath and overwrite
    }
  }

  // remove any .xml files from the local appdata directory that are not present
  // in the remote instrument repo
  removeOrphanedFiles(localPath.toString(), repoFilenames);

  return fileMap;
}

/**
 *
 * @param mapping A map of string keys to string values
 * @param key A string representing a key
 * @param defaultValue A default to return if the key is not present
 * @return The value of the key or the default if the key does not exist
 */
std::string DownloadInstrument::getValueOrDefault(
    const DownloadInstrument::StringToStringMap &mapping,
    const std::string &key, const std::string &defaultValue) const {
  auto element = mapping.find(key);
  return (element != mapping.end()) ? element->second : defaultValue;
}

/** Creates or updates the json file of a directories contents
 * @param directoryPath The path to the directory to catalog
 * @return A map of file names to sha1 values
 **/
DownloadInstrument::StringToStringMap
DownloadInstrument::getFileShas(const std::string &directoryPath) {
  StringToStringMap filesToSha;
  try {
    using Poco::DirectoryIterator;
    DirectoryIterator end;
    for (DirectoryIterator it(directoryPath); it != end; ++it) {
      const auto &entryPath = Poco::Path(it->path());
      if (entryPath.getExtension() != "xml")
        continue;
      std::string sha1 = ChecksumHelper::gitSha1FromFile(entryPath.toString());
      // Track sha1
      filesToSha.emplace(entryPath.getFileName(), sha1);
    }
  } catch (Poco::Exception &ex) {
    g_log.error() << "DownloadInstrument: failed to parse the directory: "
                  << directoryPath << " : " << ex.className() << " : "
                  << ex.displayText() << '\n';
    // silently ignore this exception.
  } catch (std::exception &ex) {
    std::stringstream ss;
    ss << "unknown exception while checking local file system. " << ex.what()
       << ". Input = " << directoryPath;
    throw std::runtime_error(ss.str());
  }

  return filesToSha;
}

/** removes any .xml files in a directory that are not in filenamesToKeep
 * @param directoryPath the directory to work in
 * @param filenamesToKeep a set of filenames to keep
 * @returns the number of files removed
 **/
size_t DownloadInstrument::removeOrphanedFiles(
    const std::string &directoryPath,
    const std::unordered_set<std::string> &filenamesToKeep) const {
  // hold files to delete in a set so we don't remove files while iterating over
  // the directory.
  std::vector<std::string> filesToDelete;

  try {
    using Poco::DirectoryIterator;
    DirectoryIterator end;
    for (DirectoryIterator it(directoryPath); it != end; ++it) {
      const auto &entryPath = Poco::Path(it->path());
      if (entryPath.getExtension() != "xml")
        continue;
      if (filenamesToKeep.find(entryPath.getFileName()) ==
          filenamesToKeep.end()) {
        g_log.debug() << "File not found in remote instrument repository, will "
                         "be deleted: "
                      << entryPath.getFileName() << '\n';
        filesToDelete.push_back(it->path());
      }
    }
  } catch (Poco::Exception &ex) {
    g_log.error() << "DownloadInstrument: failed to list the directory: "
                  << directoryPath << " : " << ex.className() << " : "
                  << ex.displayText() << '\n';
    // silently ignore this exception.
  } catch (std::exception &ex) {
    std::stringstream ss;
    ss << "unknown exception while checking local file system. " << ex.what()
       << ". Input = " << directoryPath;
    throw std::runtime_error(ss.str());
  }

  // delete any identified files
  try {
    for (const auto &filename : filesToDelete) {
      Poco::File file(filename);
      file.remove();
    }
  } catch (Poco::Exception &ex) {
    g_log.error() << "DownloadInstrument: failed to delete file: "
                  << ex.className() << " : " << ex.displayText() << '\n';
    // silently ignore this exception.
  } catch (std::exception &ex) {
    std::stringstream ss;
    ss << "unknown exception while deleting file: " << ex.what();
    throw std::runtime_error(ss.str());
  }

  g_log.debug() << filesToDelete.size() << " Files deleted.\n";

  return filesToDelete.size();
}

/** Converts a github file page to a downloadable url for the file.
 * @param filename a github file page url
 * @returns a downloadable url for the file
 **/
const std::string
DownloadInstrument::getDownloadableRepoUrl(const std::string &filename) const {
  return filename + "?raw=1";
}

/** Download a url and fetch it inside the local path given.
This calls Kernel/InternetHelper, but is wrapped in this method to allow mocking
in the unit tests.

@param urlFile : The url to download the contents of
@param localFilePath [optional] : Provide the destination of the file downloaded
at the url_file.
the connection and the download was done correctly.
@param headers [optional] : A key value pair map of any additional headers to
include in the request.
@exception Mantid::Kernel::Exception::InternetError : For any unexpected
behaviour.
*/
int DownloadInstrument::doDownloadFile(const std::string &urlFile,
                                       const std::string &localFilePath,
                                       const StringToStringMap &headers) {
  Poco::File localFile(localFilePath);
  if (localFile.exists()) {
    if (!localFile.canWrite()) {
      std::stringstream msg;
      msg << "Cannot write file \"" << localFilePath << "\"";
      throw std::runtime_error(msg.str());
    }
  } else {
    localFile = Poco::File(Poco::Path(localFilePath).parent().toString());
    if (!localFile.canWrite()) {
      std::stringstream msg;
      msg << "Cannot write file \"" << localFilePath << "\"";
      throw std::runtime_error(msg.str());
    }
  }

  int retStatus = 0;
  GitHubApiHelper inetHelper;
  inetHelper.headers().insert(headers.begin(), headers.end());
  retStatus = inetHelper.downloadFile(urlFile, localFilePath);
  return retStatus;
}

} // namespace DataHandling
} // namespace Mantid
