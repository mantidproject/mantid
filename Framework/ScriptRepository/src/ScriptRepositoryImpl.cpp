// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// from mantid
#include "MantidScriptRepository/ScriptRepositoryImpl.h"
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidJson/Json.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/NetworkProxy.h"
#include "MantidKernel/ProxyInfo.h"
#include <filesystem>
#include <unordered_set>
#include <utility>

using Mantid::Kernel::ConfigService;
using Mantid::Kernel::ConfigServiceImpl;
using Mantid::Kernel::Logger;
using Mantid::Types::Core::DateAndTime;

// from poco
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Net/NetException.h>
#include <Poco/Path.h>
#include <Poco/TemporaryFile.h>
#include <Poco/URI.h>
/*#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
*/
#include "Poco/Net/FilePartSource.h"
#include <Poco/Net/HTMLForm.h>

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
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/StreamCopier.h>

// from boost
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

#include <json/json.h>

namespace Mantid::API {
namespace {
/// static logger
Kernel::Logger g_log("ScriptRepositoryImpl");
} // namespace

/// Default timeout
int DEFAULT_TIMEOUT_SEC = 30;

const char *timeformat = "%Y-%b-%d %H:%M:%S";

const char *emptyURL = "The initialization failed because no URL was given that points "
                       "to the central repository.\nThis entry should be defined at the "
                       "properties file, "
                       "at ScriptRepository";

/**
Write json object to file
*/
void writeJsonFile(const std::string &filename, const Json::Value &json, const std::string &error) {
  Poco::FileOutputStream filestream(filename);
  if (!filestream.good()) {
    g_log.error() << error << '\n';
  }
  filestream << Mantid::JsonHelpers::jsonToString(json, " ");
  filestream.close();
}

/**
Read json object from file
*/
Json::Value readJsonFile(const std::string &filename, const std::string &error) {
  Poco::FileInputStream fileStream(filename);
  if (!fileStream.good()) {
    g_log.error() << error << '\n';
  }

  ::Json::CharReaderBuilder readerBuilder;
  Json::Value read;
  std::string errors;
  Json::parseFromStream(readerBuilder, fileStream, &read, &errors);
  if (errors.size() != 0) {
    throw ScriptRepoException("Bad JSON string from file: " + filename + ". " + error);
  }
  fileStream.close();

  return read;
}

/**
Write string to file
*/
void writeStringFile(const std::string &filename, const std::string &stringToWrite, const std::string &error) {
  Poco::FileStream filestream(filename);
  if (!filestream.good()) {
    g_log.error() << error << '\n';
  }
  filestream << stringToWrite;
  filestream.close();
}

/**
Test if a file with this filename already exists
*/
bool fileExists(const std::string &filename) {
  Poco::File test_file(filename);
  return test_file.exists();
}

DECLARE_SCRIPTREPOSITORY(ScriptRepositoryImpl)
/**
 The main information that ScriptrepositoryImpl needs to be able
 to operate are where the local repository is (or will be), and
 the url for the mantid web server.

 Usually these values are available at the Mantid properties files,
 so, it is possible to construct the ScriptrepositoryImpl without
 parameters.

 But, for flexibility reasons, (for example, testing with other
 repositories), a more general constructor is provided.

 In case a string is passed to the constructor different from the
 default one, it will have precedence, but it will not override what
 is defined by the Mantid properties files. These values will be valid
 only for that instance.

 Currently, two properties are defined: ScriptLocalRepository, and
 ScriptRepository.

 @code
 // get ScriptRepository and ScriptLocalRepository values from Mantid Config
 Service
 ScriptrepositoryImpl sharing();
 // apply given values
 ScriptrepositoryImpl sharing("/tmp/gitrep",
 "https://repository.mantidproject.com");
 @endcode
 */
ScriptRepositoryImpl::ScriptRepositoryImpl(const std::string &local_rep, const std::string &remote) : m_valid(false) {
  // get the local path and the remote path
  std::string loc, rem;
  const ConfigServiceImpl &config = ConfigService::Instance();
  remote_upload = config.getString("UploaderWebServer");
  if (local_rep.empty() || remote.empty()) {
    loc = config.getString("ScriptLocalRepository");
    rem = config.getString("ScriptRepository");
  } else {
    local_repository = local_rep;
    remote_url = remote;
  }
  // the parameters given from the constructor have precedence
  if (local_rep.empty())
    local_repository = loc;
  else
    local_repository = local_rep;

  if (remote.empty())
    remote_url = rem;
  else
    remote_url = remote;

  // empty remote url is not allowed
  if (remote_url.empty()) {
    g_log.error() << emptyURL << '\n';
    throw ScriptRepoException(emptyURL, "Constructor Failed: remote_url.empty");
  }

  if (remote_url.back() != '/')
    remote_url.append("/");

  // if no folder is given, the repository is invalid.
  if (local_repository.empty())
    return;

  if (local_repository.back() != '/')
    local_repository.append("/");

  g_log.debug() << "ScriptRepository creation pointing to " << local_repository << " and " << remote_url << "\n";

  // check if the repository is valid.

  // parsing the ignore pattern
  std::string ignore = ignorePatterns();
  boost::replace_all(ignore, "/", "\\/");
  boost::replace_all(ignore, ";", "|");
  boost::replace_all(ignore, ".", "\\.");
  boost::replace_all(ignore, "*", ".*");
  ignoreregex = std::string("(").append(ignore).append(")");

  // A valid repository must pass 3 tests:
  //  - An existing folder
  //  - This folder must have the .repository.json file
  //  - This folder must have the .local.json file
  // These tests will be done with Poco library

  Poco::Path local(local_repository);

  std::string aux_local_rep;
  if (local.isRelative()) {
    aux_local_rep = std::string(Poco::Path::current()).append(local_repository);
    local_repository = aux_local_rep;
  }

  try { // tests 1 and 2
    {
      Poco::File local_rep_dir(local);
      std::string repository_json = std::string(local_repository).append(".repository.json");
      Poco::File rep_json(repository_json);
      if (!local_rep_dir.exists() || !rep_json.exists()) {
        g_log.information() << "ScriptRepository was not installed at " << local_repository << '\n';
        return; // this is an invalid repository, because it was not created
                // (installed)
      }
    }
    // third test
    {
      std::string repository_json = std::string(local_repository).append(".local.json");
      Poco::File rep_json(repository_json);
      if (!rep_json.exists()) {
        g_log.error() << "Corrupted ScriptRepository at " << local_repository
                      << ". Please, remove this folder, and install "
                         "ScriptRepository again\n";
      }
    }
  } catch (Poco::FileNotFoundException & /*ex*/) {
    g_log.error() << "Testing the existence of repository.json and local.json failed\n";
    return;
  }

  // this is necessary because in windows, the absolute path is given
  // with \ slash.
  boost::replace_all(local_repository, "\\", "/");
  if (local_repository.back() != '/')
    local_repository.append("/");

  repo.clear();
  m_valid = true;
}

/**
 Check the connection with the server through the ::doDownloadFile method.
 @path server : The url that will be used to connect.
 */
void ScriptRepositoryImpl::connect(const std::string &server) { doDownloadFile(server); }

/** Implements the ScriptRepository::install method.

 The instalation consists of:

 - creation of the folder for the ScriptRepository (if it does not exists).
 - download of the repository.json file (Make it hidden)
 - creation of the local.json file. (Make if hidden)

 The installation will also upate the ScriptLocalRepository setting, if
 necessary,
 to match the given path.

 If it success, it will change the status of the ScriptRepository as valid.

 @note Any directory may be given, from existing directories a new directory.
 If an existing directory is given, the installation will install the two
 necessary
 files to deal with this folder as a ScriptRepository.


 @param path : Path for a folder inside the local machine.


 */
void ScriptRepositoryImpl::install(const std::string &path) {
  using Poco::DirectoryIterator;
  if (remote_url.empty()) {
    std::stringstream ss;
    ss << "ScriptRepository is configured to download from a invalid URL "
          "(empty URL)."
       << "\nThis URL comes from the property file and it is called "
          "ScriptRepository.";
    throw ScriptRepoException(ss.str());
  }
  std::string folder = std::string(path);
  Poco::File repository_folder(folder);
  std::string rep_json_file = std::string(path).append("/.repository.json");
  std::string local_json_file = std::string(path).append("/.local.json");
  if (!repository_folder.exists()) {
    repository_folder.createDirectories();
  }

  // install the two files inside the given folder
  g_log.debug() << "ScriptRepository attempt to doDownload file " << path << '\n';
  // download the repository json
  doDownloadFile(std::string(remote_url).append("repository.json"), rep_json_file);
  g_log.debug() << "ScriptRepository downloaded repository information\n";
  // creation of the instance of local_json file
  if (!fileExists(local_json_file)) {
    writeStringFile(local_json_file, "{\n}", "ScriptRepository failed to create local repository");
    g_log.debug() << "ScriptRepository created the local repository information\n";
  }

#if defined(_WIN32) || defined(_WIN64)
  // set the .repository.json and .local.json hidden
  SetFileAttributes(local_json_file.c_str(), FILE_ATTRIBUTE_HIDDEN);
  SetFileAttributes(rep_json_file.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

  // save the path to the config service
  //
  ConfigServiceImpl &config = ConfigService::Instance();
  std::string loc = config.getString("ScriptLocalRepository");
  if (loc != path) {
    config.setString("ScriptLocalRepository", path);
    config.saveConfig(config.getUserFilename());
  }

  local_repository = path;
  // this is necessary because in windows, the absolute path is given
  // with \ slash.
  boost::replace_all(local_repository, "\\", "/");
  if (local_repository.back() != '/')
    local_repository.append("/");

  m_valid = true;
}

void ScriptRepositoryImpl::ensureValidRepository() {
  if (!isValid()) {
    std::stringstream ss;
    ss << "ScriptRepository is not installed correctly. The current path for "
          "ScriptRepository is "
       << local_repository
       << " but some important files that are required are corrupted or not "
          "present."
       << "\nPlease, re-install the ScriptRepository!\n"
       << "Hint: if you have a proper installation in other path, check the "
          "property ScriptLocalRepository "
       << "at the Mantid.user.properties and correct it if necessary.";
    throw ScriptRepoException(ss.str(), "CORRUPTED");
  }
}

/** Implements ScriptRepository::info

 For each entry inside the repository, there are some usefull information, that
 are stored in
 a ::ScriptInfo struct.

 Use this method, to get information about the description, last modified date,
 the auto update
 flag and the author.

 @param input_path: The path (relative or absolute) to the file/folder entry,

 @return ScriptInfo with the information of this file/folder.

 @note: This method requires that ::listFiles was executed at least once.

 */
ScriptInfo ScriptRepositoryImpl::info(const std::string &input_path) {
  ensureValidRepository();
  std::string path = convertPath(input_path);
  ScriptInfo info;
  try {
    const RepositoryEntry &entry = repo.at(path);
    info.author = entry.author;
    info.pub_date = entry.pub_date;
    info.auto_update = entry.auto_update;
    info.directory = entry.directory;
  } catch (const std::out_of_range &ex) {
    std::stringstream ss;
    ss << "The file \"" << input_path << "\" was not found inside the repository!";
    throw ScriptRepoException(ss.str(), ex.what());
  }
  return info;
}

const std::string &ScriptRepositoryImpl::description(const std::string &input_path) {
  ensureValidRepository();
  std::string path = convertPath(input_path);
  try {
    const RepositoryEntry &entry = repo.at(path);
    return entry.description;
  } catch (const std::out_of_range &ex) {
    std::stringstream ss;
    ss << "The file \"" << input_path << "\" was not found inside the repository!";
    throw ScriptRepoException(ss.str(), ex.what());
  }
}

/**
 Implement the ScriptRepository::listFiles.

 It will fill up the ScriptRepositoryImpl::Repository variable in order to
 provide
 information about the status of the file as well.

 In order to list all the values from the repository, it uses three methods:

 - ::parseCentralRepository
 - ::parseDonwloadedEntries
 - ::parseLocalRepository

 After this, it will perform a reverse iteration on all the entries of the
 repository
 in order to evaluate the status (::findStatus) of every file, and will also get
 the
 status of every directory, by accumulating the influency of every directory.

 The listFiles will list:
 - all files in the central repository
 - all files in the local repository

 @return It returns a list of all the files and directories (the relative path
 inside the repository).
 */
std::vector<std::string> ScriptRepositoryImpl::listFiles() {
  ensureValidRepository();

  repo.clear();
  assert(repo.empty());
  try {
    parseCentralRepository(repo);
    parseLocalRepository(repo);
    parseDownloadedEntries(repo);
    // it will not catch ScriptRepositoryExc, because, this means, that it was
    // already processed.
    // it will proceed in this situation.
  } catch (Poco::Exception &ex) {
    g_log.error() << "ScriptRepository failed to list all entries inside the "
                     "repository. Details: "
                  << ex.className() << ":> " << ex.displayText() << '\n';
  } catch (std::exception &ex) {
    g_log.error() << "ScriptRepository failed to list all entries inside the "
                     "repository. Details: "
                  << ex.what() << '\n';
  }
  std::vector<std::string> out(repo.size());
  size_t i = repo.size();

  // evaluate the status for all entries
  // and also fill up the output vector (in reverse order)
  Mantid::API::SCRIPTSTATUS acc_status = Mantid::API::BOTH_UNCHANGED;
  std::string last_directory;
  for (auto it = repo.rbegin(); it != repo.rend(); ++it) {
    // for every entry, it takes the path and RepositoryEntry
    std::string entry_path = it->first;
    RepositoryEntry &entry = it->second;
    // g_log.debug() << "Evaluating the status of " << entry_path << '\n';
    // fill up the output vector
    out[--i] = it->first;
    // g_log.debug() << "inserting file: " << it->first << '\n';

    // for the directories, update the status of this directory
    if (entry.directory) {
      entry.status = acc_status;
      if (!entry.remote)
        entry.status = Mantid::API::LOCAL_ONLY;
      last_directory = entry_path;
    } else {
      // for the files, it evaluates the status of this file

      if (entry.local && !entry.remote) {
        // entry local only
        entry.status = LOCAL_ONLY;
      } else if (!entry.local && entry.remote) {
        // entry remote only
        entry.status = REMOTE_ONLY;
      } else {
        // there is no way of not being remote nor local!

        // entry is local and is remote
        // the following status are available:
        // BOTH_CHANGED, BOTH_UNCHANGED, REMOTE_CHANGED, LOCAL_CHANGED.
        enum CHANGES { UNCH = 0, REMO = 0X1, LOC = 0X2, BOTH = 0X3 };
        int st = UNCH;
        // the file is local_changed, if the date of the current file is
        // diferent
        // from the downloaded one.
        if (entry.current_date != entry.downloaded_date)
          st |= LOC;
        // the file is remote_changed if the date of the pub_date file is
        // diferent from the local downloaded pubdate.
        if (entry.pub_date > entry.downloaded_pubdate)
          st |= REMO;

        switch (st) {
        case UNCH:
          entry.status = BOTH_UNCHANGED;
          break;
        case REMO:
          entry.status = REMOTE_CHANGED;
          break;
        case LOC:
          entry.status = LOCAL_CHANGED;
          break;
        case BOTH:
        default:
          entry.status = BOTH_CHANGED;
          break;
        } // end switch

      } // end evaluating the file status

    } // end dealing with files

    // is this entry a child of the last directory?
    if (!last_directory.empty()) {
      if (entry_path.find(last_directory) == std::string::npos) {
        // no, this entry is not a child of the last directory
        // restart the status
        acc_status = Mantid::API::BOTH_UNCHANGED;
      }
    }

    // update the status of the parent directory:
    // the strategy here is to compare binary the current status with the
    // acc_state
    switch (acc_status | entry.status) {
    // pure matching, meaning that the matching is done with the same state
    // or with BOTH_UNCHANGED (neutral)
    case BOTH_UNCHANGED: // BOTH_UNCHANGED IS 0, so only 0|0 match this option
    case REMOTE_ONLY:    // REMOTE_ONLY IS 0x01, so only 0|0x01 and 0x01|0x01 match
                         // this option
    case LOCAL_ONLY:
    case LOCAL_CHANGED:
    case REMOTE_CHANGED:
      acc_status = static_cast<SCRIPTSTATUS>(acc_status | entry.status);
      break;
    case LOCAL_ONLY | LOCAL_CHANGED:
      acc_status = LOCAL_CHANGED;
      break;
    case REMOTE_ONLY | REMOTE_CHANGED:
      acc_status = REMOTE_CHANGED;
      break;
    default:
      acc_status = BOTH_CHANGED;
      break;
    }
  }

  return out;
}

/**
 Implements the ScriptRepository::download.

 @note Require that ::listFiles been called at least once.

 The download is able to download files or directories. Internally,
 it will assign the job to the ::download_diretory or ::download_file.

 This method, just ensure that the entry is valid (wich means,
 it is inside the repository).

 @param input_path: The path for the file/folder to be downloaded.

 @note As a result of the download a new file, the local repository
 information .local.repository will be changed.

 */

void ScriptRepositoryImpl::download(const std::string &input_path) {
  ensureValidRepository();
  std::string file_path = convertPath(input_path);
  try {
    RepositoryEntry &entry = repo.at(file_path);
    if (entry.directory)
      download_directory(file_path);
    else
      download_file(file_path, entry);
  } catch (const std::out_of_range &ex) {
    // fixme: readable exception
    throw ScriptRepoException(ex.what());
  }
}

/**
 Go recursively to download all the children of an input directory.

 @param directory_path : the path for the directory.
 */
void ScriptRepositoryImpl::download_directory(const std::string &directory_path) {
  std::string directory_path_with_slash = std::string(directory_path).append("/");
  bool found = false;
  for (auto &entry : repo) {
    // skip all entries that are not children of directory_path
    // the map will list the entries in alphabetical order, so,
    // when it first find the directory, it will list all the
    // childrens of this directory, and them,
    // it will list other things, so we can, break the loop
    if (entry.first.find(directory_path) != 0) {
      if (found)
        break; // for the sake of performance
      else
        continue;
    }
    found = true;
    if (entry.first != directory_path && entry.first.find(directory_path_with_slash) != 0) {
      // it is not a children of this entry, just similar. Example:
      // TofConverter/README
      // TofConverter.py
      // these two pass the first test, but will not pass this one.
      found = false;
      continue;
    }
    // now, we are dealing with the children of directory path
    if (!entry.second.directory)
      download_file(entry.first, entry.second);
    else {
      // download the directory.

      // we will not download the directory, but create one with the
      // same name, and update the local json

      Poco::File dir(std::string(local_repository).append(entry.first));
      dir.createDirectories();

      entry.second.status = BOTH_UNCHANGED;
      entry.second.downloaded_date = DateAndTime(Poco::DateTimeFormatter::format(dir.getLastModified(), timeformat));
      entry.second.downloaded_pubdate = entry.second.pub_date;
      updateLocalJson(entry.first, entry.second);

    } // end downloading directory
      // update the status
    entry.second.status = BOTH_UNCHANGED; // update this entry
  } // end interaction with all entries
}

/**
 Download the real file from the remote_url.

 @todo describe better this method.
 */
void ScriptRepositoryImpl::download_file(const std::string &file_path, RepositoryEntry &entry) {
  SCRIPTSTATUS state = entry.status;
  // if we have the state, this means that the entry is available
  if (state == LOCAL_ONLY || state == LOCAL_CHANGED) {
    std::stringstream ss;
    ss << "The file " << file_path << " can not be download because it has only local changes."
       << " If you want, please, publish this file uploading it";
    throw ScriptRepoException(ss.str());
  }

  if (state == BOTH_UNCHANGED)
    // instead of throwing exception, silently assumes that the download was
    // done.
    return;

  // download the file
  std::string url_path = std::string(remote_url).append(file_path);
  Poco::TemporaryFile tmpFile;
  doDownloadFile(url_path, tmpFile.path());

  std::string local_path = std::string(local_repository).append(file_path);
  g_log.debug() << "ScriptRepository download url_path: " << url_path << " to " << local_path << '\n';

  std::string dir_path;

  try {

    if (state == BOTH_CHANGED) {
      // make a back up of the local version
      Poco::File f(std::string(local_repository).append(file_path));
      std::string bck = std::string(f.path()).append("_bck");
      g_log.notice() << "The current file " << f.path() << " has some local changes"
                     << " so, a back up copy will be created at " << bck << '\n';
      f.copyTo(bck);
    }

    // ensure that the path to the local_path exists
    size_t slash_pos = local_path.rfind('/');
    Poco::File file_out(local_path);
    if (slash_pos != std::string::npos) {
      dir_path = std::string(local_path.begin(), local_path.begin() + slash_pos);
      if (!dir_path.empty()) {
        Poco::File dir_parent(dir_path);
        if (!dir_parent.exists()) {
          dir_parent.createDirectories();
        }
      } // dir path is empty
    }

    if (!file_out.exists())
      file_out.createFile();

    tmpFile.copyTo(local_path);

  } catch (Poco::FileAccessDeniedException &) {
    std::stringstream ss;
    ss << "You cannot create file at " << local_path << ". Not downloading ...";
    throw ScriptRepoException(ss.str());
  }

  {
    Poco::File local(local_path);
    entry.downloaded_date = DateAndTime(Poco::DateTimeFormatter::format(local.getLastModified(), timeformat));
    entry.downloaded_pubdate = entry.pub_date;
    entry.status = BOTH_UNCHANGED;
  }

  // Update pythonscripts.directories if necessary
  // (TEST_DOWNLOAD_ADD_FOLDER_TO_PYTHON_SCRIPTS)
  if (!dir_path.empty()) {
    const char *python_sc_option = "pythonscripts.directories";
    ConfigServiceImpl &config = ConfigService::Instance();
    std::string python_dir = config.getString(python_sc_option);
    if (python_dir.find(dir_path) == std::string::npos) {
      // this means that the directory is not inside the
      // pythonscripts.directories
      // add to the repository
      python_dir.append(";").append(dir_path);
      config.setString(python_sc_option, python_dir);
      config.saveConfig(config.getUserFilename());

      // the previous code make the path available for the following
      // instances of Mantid, but, for the current one, it is necessary
      // do add to the python path...
    }
  }

  updateLocalJson(file_path, entry); /// FIXME: performance!
  g_log.debug() << "ScriptRepository download " << local_path << " success!\n";
}

/**
 @todo Describe
 */
SCRIPTSTATUS ScriptRepositoryImpl::fileStatus(const std::string &input_path) {
  /// @todo: implement the trigger method to know it we need to revised the
  ///        directories trees.
  ensureValidRepository();
  std::string file_path = convertPath(input_path);
  // g_log.debug() << "Attempt to ask for the status of "<< file_path <<
  // '\n';
  try {
    const RepositoryEntry &entry = repo.at(file_path);
    return entry.status;
  } catch (const std::out_of_range &ex) {
    std::stringstream ss;
    ss << "The file \"" << input_path << "\" was not found inside the repository!";
    throw ScriptRepoException(ss.str(), ex.what());
  }
  // this line will never be executed, just for avoid compiler warnings.
  return BOTH_UNCHANGED;
}

/**
 * Uploads one file to the ScriptRepository web server, pushing, indirectly, to
 *the
 * git repository. It will send in a POST method, the file and the following
 *fields:
 *  - author : Will identify the author of the change
 *  - email:  Will identify the email of the author
 *  - comment: Description of the nature of the file or of the update
 *
 * It will them upload to the URL pointed to UploaderWebServer. It will them
 *receive a json file
 * with some usefull information about the success or failure of the attempt to
 *upload.
 * In failure, it will be converted to an appropriated ScriptRepoException.
 */
void ScriptRepositoryImpl::upload(const std::string &file_path, const std::string &comment, const std::string &author,
                                  const std::string &email)

{
  using namespace Poco::Net;
  try {
    g_log.notice() << "ScriptRepository uploading " << file_path << " ...\n";

    Kernel::InternetHelper inetHelper;

    HTMLForm form(HTMLForm::ENCODING_MULTIPART);

    // add the fields author, email and comment
    form.add("author", author);
    form.add("mail", email);
    form.add("comment", comment);

    // deal with the folder
    std::string relative_path = convertPath(file_path);
    std::string absolute_path = local_repository + relative_path;
    std::string folder = "./";
    size_t pos = relative_path.rfind('/');
    if (pos != std::string::npos)
      folder += std::string(relative_path.begin(), relative_path.begin() + pos);
    if (folder.back() != '/')
      folder += "/";
    g_log.information() << "Uploading to folder: " << folder << '\n';
    form.add("path", folder);

    // inserting the file
    auto m_file = new FilePartSource(absolute_path);
    form.addPart("file", m_file);

    inetHelper.setBody(form);
    std::stringstream server_reply;

    Kernel::InternetHelper::HTTPStatus status{Kernel::InternetHelper::HTTPStatus::BAD_REQUEST};
    try {
      status = inetHelper.sendRequest(remote_upload, server_reply);
    } catch (Kernel::Exception::InternetError &ie) {
      status = static_cast<Kernel::InternetHelper::HTTPStatus>(ie.errorCode());
    }

    g_log.information() << "ScriptRepository upload status: " << static_cast<int>(status) << '\n';
    std::stringstream answer;
    { // remove the status message from the end of the reply, in order not to
      // get exception from the read_json parser
      std::string server_reply_str;
      server_reply_str = server_reply.str();
      const size_t lastBrace = server_reply_str.rfind('}');
      if (lastBrace != std::string::npos)
        answer << std::string(server_reply_str.begin(), server_reply_str.begin() + lastBrace + 1);
      else
        answer << server_reply_str;
    }
    g_log.debug() << "Form Output: " << answer.str() << '\n';

    std::string messageInfo;
    std::string detail;
    std::string published_date;

    Json::Value pt;
    auto answerString = answer.str();
    if (!Mantid::JsonHelpers::parse(answerString, &pt)) {
      throw ScriptRepoException("Bad answer from the Server");
    }
    messageInfo = pt.get("message", "").asString();
    detail = pt.get("detail", "").asString();
    published_date = pt.get("pub_date", "").asString();
    std::string cmd = pt.get("shell", "").asString();
    if (!cmd.empty())
      detail.append("\nFrom Command: ").append(cmd);

    if (messageInfo == "success") {
      g_log.notice() << "ScriptRepository:" << file_path << " uploaded!\n";

      // update the file
      RepositoryEntry &entry = repo.at(file_path);
      {
        Poco::File local(absolute_path);
        entry.downloaded_date = DateAndTime(Poco::DateTimeFormatter::format(local.getLastModified(), timeformat));
        // update the pub_date and downloaded_pubdate with the pub_date given by
        // the upload.
        // this ensures that the status will be correctly defined.
        if (!published_date.empty())
          entry.pub_date = DateAndTime(published_date);
        entry.downloaded_pubdate = entry.pub_date;
        entry.status = BOTH_UNCHANGED;
      }
      g_log.information() << "ScriptRepository update local json \n";
      updateLocalJson(file_path, entry); /// FIXME: performance!

      // add the entry to the repository.json. The
      // repository.json should change at the
      // remote repository, and we could just download the new one, but
      // we can not rely on the server updating it fast enough.
      // So add to the file locally to avoid race condition.
      RepositoryEntry &remote_entry = repo.at(file_path);
      if (!published_date.empty())
        remote_entry.pub_date = DateAndTime(published_date);
      remote_entry.status = BOTH_UNCHANGED;
      g_log.debug() << "ScriptRepository updating repository json \n";
      updateRepositoryJson(file_path, remote_entry);

    } else
      throw ScriptRepoException(messageInfo, detail);

  } catch (Poco::Exception &ex) {
    throw ScriptRepoException(ex.displayText(), ex.className());
  }
}

/*
 * Adds an entry to .repository.json
 * This is necessary when uploading a file to keep .repository.json and
 * .local.json in sync, and thus display correct file status in the GUI.
 * Requesting an updated .repository.json from the server is not viable
 * at such a time as it would create a race condition.
 * @param path: relative path of uploaded file
 * @param entry: the entry to add to the json file
 */
void ScriptRepositoryImpl::updateRepositoryJson(const std::string &path, const RepositoryEntry &entry) {

  Json::Value repository_json;
  std::string filename = std::string(local_repository).append(".repository.json");
  repository_json = readJsonFile(filename, "Error reading .repository.json file");

  if (!repository_json.isMember(path)) {
    // Create Json value for entry
    Json::Value entry_json;
    entry_json["author"] = entry.author;
    entry_json["description"] = entry.description;
    entry_json["directory"] = (entry.directory ? "true" : "false");
    entry_json["pub_date"] = entry.pub_date.toFormattedString();

    // Add Json value for entry to repository Json value
    repository_json[path] = entry_json;
  }

  g_log.debug() << "Update LOCAL JSON FILE\n";
#if defined(_WIN32) || defined(_WIN64)
  // set the .repository.json and .local.json not hidden to be able to edit it
  SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif
  writeJsonFile(filename, repository_json, "Error writing .repository.json file");
#if defined(_WIN32) || defined(_WIN64)
  // set the .repository.json and .local.json hidden
  SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}

/**
 * Delete one file from the local and the central ScriptRepository
 * It will send in a POST method, with the file path to find the path :
 *  - author : Will identify the author of the change
 *  - email:  Will identify the email of the author
 *  - comment: Description of the nature of the file or of the update
 *
 * It will them send the request to the URL pointed to UploaderWebServer,
 *changing the word
 * publish to remove. For example:
 *
 * https://upload.mantidproject.org/scriptrepository/payload/remove
 *
 * The server will them create a git commit deleting the file. And will reply
 *with a json string
 * with some usefull information about the success or failure of the attempt to
 *delete.
 * In failure, it will be converted to an appropriated ScriptRepoException.
 *
 * Requirements: in order to be allowed to delete files from the central
 *repository,
 * it is required that the state of the file must be BOTH_UNCHANGED or
 *LOCAL_CHANGED.
 *
 * @param file_path: The path (relative to the repository) or absolute to
 *identify the file to remove
 * @param comment: justification to remove this file (will be used as git commit
 *message)
 * @param author: identification of the requester for deleting wich must be the
 *author of the file as well
 * @param email: email of the requester
 *
 * @exception ScriptRepoException justifying the reason to failure.
 *
 * @note only local files can be removed.
 */
void ScriptRepositoryImpl::remove(const std::string &file_path, const std::string &comment, const std::string &author,
                                  const std::string &email) {
  std::string relative_path = convertPath(file_path);

  // get the status, because only local files can be removed
  SCRIPTSTATUS status = fileStatus(relative_path);
  std::stringstream ss;
  bool raise_exc = false;
  switch (status) {
  case REMOTE_ONLY:
    ss << "You are not allowed to remove files from the repository that you "
          "have not installed and you are not the owner";
    raise_exc = true;
    break;
  case REMOTE_CHANGED:
  case BOTH_CHANGED:
    ss << "There is a new version of this file, so you can not remove it from "
          "the repository before checking it out. Please download the new "
          "version, and if you still wants to remove, do it afterwards";
    raise_exc = true;
    break;
  case LOCAL_ONLY:
    ss << "This operation is to remove files from the central repository. "
       << "\nTo delete files or folders from your local folder, please, do it "
          "through your operative system,"
       << "using your local installation folder at " << local_repository;
    raise_exc = true;
  default:
    break;
  }
  if (raise_exc)
    throw ScriptRepoException(ss.str());

  g_log.information() << "ScriptRepository deleting " << file_path << " ...\n";

  {
    // request to remove the file from the central repository

    RepositoryEntry &entry = repo.at(relative_path);

    if (entry.directory)
      throw ScriptRepoException("You can not remove folders recursively from "
                                "the central repository.");

    // prepare the request, and call doDeleteRemoteFile to request the server to
    // remove the file
    std::stringstream answer;
    answer << doDeleteRemoteFile(remote_upload, file_path, author, email, comment);
    g_log.debug() << "Answer from doDelete: " << answer.str() << '\n';

    // analyze the answer from the server, to see if the file was removed or
    // not.
    std::string messageInfo;
    std::string detail;
    Json::Value answer_json;
    auto answerString = answer.str();
    if (!Mantid::JsonHelpers::parse(answerString, &answer_json)) {
      throw ScriptRepoException("Bad answer from the Server");
    }

    messageInfo = answer_json.get("message", "").asString();
    detail = answer_json.get("detail", "").asString();
    std::string cmd = answer_json.get("shell", "").asString();

    if (!cmd.empty())
      detail.append("\nFrom Command: ").append(cmd);

    g_log.debug() << "Checking if success info=" << messageInfo << '\n';
    // check if the server removed the file from the central repository
    if (messageInfo != "success")
      throw ScriptRepoException(messageInfo, detail); // no

    g_log.notice() << "ScriptRepository " << file_path << " removed from central repository\n";

    // delete the entry from the repository.json. In reality, the
    // repository.json should change at the
    // remote repository, and we could only download the new one, but,
    // practically, at the server, it will
    // take sometime to be really removed, so, for practical reasons, this is
    // dealt with locally.
    //
    {
      std::string filename = std::string(local_repository).append(".repository.json");

      Json::Value pt = readJsonFile(filename, "Error reading .repository.json file");
      pt.removeMember(relative_path); // remove the entry
#if defined(_WIN32) || defined(_WIN64)
      // set the .repository.json and .local.json not hidden (to be able to
      // edit it)
      SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif
      writeJsonFile(filename, pt, "Error writing .repository.json file");
#if defined(_WIN32) || defined(_WIN64)
      // set the .repository.json and .local.json hidden
      SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
    }

    // update the repository list variable
    // now, it is local_only and it is not inside remote.
    // this is necessary for the strange case, where removing locally may fail.

    entry.status = LOCAL_ONLY;
    entry.remote = false;

  } // file removed on central repository
}

/** Implements the request to the server to delete one file. It is created as a
 *virtual protected member
 * to allow creating unittest mocking the dependency on the internet connection.
 *This method requires
 * internet connection.
 *
 * @param url: url from the server that serves the request of removing entries
 * @param file_path: relative path to the file inside the repository
 * @param author: requester
 * @param email: email from author
 * @param comment: to be converted in git commit
 * @return The answer from the server (json string)
 *
 * The server requires that the path, author, email and comment be given in
 *order to create the commit
 * for the git repository. Besides, it will ensure that the author and email are
 *the same to the author
 * and email for the last commit, in order not to allow deleting files that
 *others are owner.
 *
 */
std::string ScriptRepositoryImpl::doDeleteRemoteFile(const std::string &url, const std::string &file_path,
                                                     const std::string &author, const std::string &email,
                                                     const std::string &comment) {
  using namespace Poco::Net;
  std::stringstream answer;
  try {
    g_log.debug() << "Receive request to delete file " << file_path << " using " << url << '\n';

    Kernel::InternetHelper inetHelper;

    // fill up the form required from the server to delete one file, with the
    // fields
    // path, author, comment, email
    HTMLForm form;
    form.add("author", author);
    form.add("mail", email);
    form.add("comment", comment);
    form.add("file_n", file_path);

    // send the request to the server
    inetHelper.setBody(form);
    std::stringstream server_reply;
    Kernel::InternetHelper::HTTPStatus status{Kernel::InternetHelper::HTTPStatus::BAD_REQUEST};
    try {
      status = inetHelper.sendRequest(url, server_reply);
    } catch (Kernel::Exception::InternetError &ie) {
      status = static_cast<Kernel::InternetHelper::HTTPStatus>(ie.errorCode());
    }

    g_log.debug() << "ScriptRepository delete status: " << static_cast<int>(status) << '\n';

    {
      // get the answer from the server
      std::string server_reply_str;
      server_reply_str = server_reply.str();
      // remove the status message from the end of the reply,
      // in order not to get exception from the read_json parser
      size_t pos = server_reply_str.rfind('}');
      if (pos != std::string::npos)
        answer << std::string(server_reply_str.begin(), server_reply_str.begin() + pos + 1);
      else
        answer << server_reply_str;
    }
    g_log.debug() << "Form Output: " << answer.str() << '\n';
  } catch (Poco::Exception &ex) {
    throw ScriptRepoException(ex.displayText(), ex.className());
  }
  return answer.str();
}

/** The ScriptRepositoryImpl is set to be valid when the local repository path
 points to a valid folder that has also the .repository.json and .local.json
 files.

 An invalid repository accepts only the ::install method.
 */
bool ScriptRepositoryImpl::isValid() {
  if (!checkLocalInstallIsPresent()) {
    m_valid = false;
  };
  return m_valid;
}

bool ScriptRepositoryImpl::checkLocalInstallIsPresent() {
  const auto local_json = std::filesystem::path(local_repository) / ".local.json";
  const auto repository_json = std::filesystem::path(local_repository) / ".repository.json";
  if (!std::filesystem::exists(local_json) || !std::filesystem::exists(repository_json)) {
    return false;
  }
  return true;
}

/**
 * Implements ScriptRepository::check4Update. It downloads the file
 *repository.json
 * from the central repository and call the listFiles again in order to inspect
 *the current
 * state of every entry inside the local repository. For the files marked as
 *AutoUpdate, if there
 * is a new version of these files, it downloads the file. As output, it
 *provides a list of
 * all files that were downloaded automatically.
 *
 *  @return List of all files automatically downloaded.
 */
std::vector<std::string> ScriptRepositoryImpl::check4Update() {
  g_log.debug() << "ScriptRepositoryImpl checking for update\n";
  // download the new repository json file
  // download the repository json
  std::string rep_json_file = std::string(local_repository).append(".repository.json");
  std::string backup = std::string(rep_json_file).append("_backup");
  {
    Poco::File f(rep_json_file);
    f.moveTo(backup);
  }
  try {
    g_log.debug() << "Download information from the Central Repository status\n";
    doDownloadFile(std::string(remote_url).append("repository.json"), rep_json_file);
  } catch (...) {
    // restore file
    Poco::File f(backup);
    f.moveTo(rep_json_file);
    throw;
  }

  // remote backup
  {
    Poco::File bak(backup);
    bak.remove();
  }

#if defined(_WIN32) || defined(_WIN64)
  // set the .repository.json and .local.json hidden
  SetFileAttributes(rep_json_file.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

  // re list the files
  g_log.debug() << "Check the status of all files again\n";
  listFiles();
  std::vector<std::string> output_list;
  // look for all the files in the list, to check those that
  // has the auto_update and check it they have changed.
  for (auto &file : repo) {
    if (file.second.auto_update) {
      // THE SAME AS it->status in (REMOTE_CHANGED, BOTH_CHANGED)
      if (file.second.status & REMOTE_CHANGED) {
        download(file.first);
        output_list.emplace_back(file.first);
        g_log.debug() << "Update file " << file.first << " to more recently version available\n";
      }
    }
  }
  g_log.debug() << "ScriptRepositoryImpl::checking for update finished\n";
  return output_list;
}

/**
 @todo describe
 */
void ScriptRepositoryImpl::setIgnorePatterns(const std::string &patterns) {
  ConfigServiceImpl &config = ConfigService::Instance();
  std::string ignore = config.getString("ScriptRepositoryIgnore");
  if (ignore != patterns) {
    config.setString("ScriptRepositoryIgnore", patterns);
    config.saveConfig(config.getUserFilename());
    std::string newignore = patterns;
    boost::replace_all(ignore, "/", "\\/");
    boost::replace_all(newignore, ";", "|");
    boost::replace_all(newignore, ".", "\\.");
    boost::replace_all(newignore, "*", ".*");
    ignoreregex = std::string("(").append(newignore).append(")");
  }
}
/**
 @todo describe
 */
std::string ScriptRepositoryImpl::ignorePatterns() {
  const ConfigServiceImpl &config = ConfigService::Instance();
  return config.getString("ScriptRepositoryIgnore", false);
}

/**
 * Configure the AutoUpdate, in order to be able to check if the user selected
 * to update this entry.
 * @param input_path : the path that identifies the entry
 * @param option: true or false to indicate if it is set to auto update.
 *
 * These configurations will be used at check4update, to download all entries
 *that
 * are set to auto update.
 */
int ScriptRepositoryImpl::setAutoUpdate(const std::string &input_path, bool option) {
  ensureValidRepository();
  const std::string path = convertPath(input_path);
  std::vector<std::string> filesToUpdate;
  for (auto it = repo.rbegin(); it != repo.rend(); ++it) {
    // for every entry, it takes the path and RepositoryEntry
    std::string entryPath = it->first;
    const RepositoryEntry &entry = it->second;
    if (entryPath.compare(0, path.size(), path) == 0 && entry.status != REMOTE_ONLY && entry.status != LOCAL_ONLY)
      filesToUpdate.emplace_back(entryPath);
  }

  try {
    for (const auto &fileToUpdate : filesToUpdate) {
      RepositoryEntry &entry = repo.at(fileToUpdate);
      entry.auto_update = option;
      updateLocalJson(fileToUpdate, entry);
    }
  } catch (const std::out_of_range &ex) {
    // fixme: readable exception
    throw ScriptRepoException(ex.what());
  }
  return static_cast<int>(filesToUpdate.size());
}

/** Download a url and fetch it inside the local path given.

 Provide a clear separation between the logic behind the ScriptRepositoryImpl
 and
 the Mantid Web Service. This is the only method for the downloading and update
 that performs a real connection to the Mantid Web Service.

 This method was present at the Script Repository Design, as an strategy to
 perform
 unit tests, but also, helps the definition of a clear separation of the logic
 and
 organization of the ScriptRepository, from the conneciton to the Mantid Web
 service,
 making it more decoupled.

 @param url_file: Define a valid URL for the file to be downloaded. Eventually,
 it may give
 any valid http path. For example:

 url_file = "http://www.google.com"

 url_file = "http://mantidweb/repository/README.md"

 The result is to connect to the http server, and request the path given.

 The answer, will be inserted at the local_file_path.

 @param local_file_path [optional] : Provide the destination of the file
 downloaded at the url_file.
 If an empty string is provided (default value), it will discard the result, but
 it will ensure that
 the connection and the download was done correctly.

 @exception ScriptRepoException: For any unexpected behavior.
 */
void ScriptRepositoryImpl::doDownloadFile(const std::string &url_file, const std::string &local_file_path) {
  g_log.debug() << "DoDownloadFile : " << url_file << " to file: " << local_file_path << '\n';

  // get the information from url_file
  std::string path(url_file);
  if (path.empty())
    path = "/";
  std::string given_path;
  if (path.find("/scriptrepository") != std::string::npos)
    given_path = std::string(path.begin() + 18,
                             path.end()); // remove the "/scriptrepository/" from the path
  else
    given_path = path;
  // Configure Poco HTTP Client Session
  try {
    Kernel::InternetHelper inetHelper;
    auto timeoutConfigVal = ConfigService::Instance().getValue<int>("network.scriptrepo.timeout");
    int timeout = timeoutConfigVal.value_or(DEFAULT_TIMEOUT_SEC);
    inetHelper.setTimeout(timeout);

    const auto status = inetHelper.downloadFile(url_file, local_file_path);
    g_log.debug() << "Answer from server: " << static_cast<int>(status) << '\n';
  } catch (Kernel::Exception::InternetError &ie) {
    std::stringstream exceptionInfo;
    exceptionInfo << "Failed to download " << given_path << " from "
                  << "<a href=\"" << url_file << "\">" << url_file << "</a>.\n";
    throw ScriptRepoException(exceptionInfo.str(), ie.what());
  }
}

/**
 @todo describe
 */
void ScriptRepositoryImpl::parseCentralRepository(Repository &repo) {
  std::string filename = std::string(local_repository).append(".repository.json");
  try {
    Json::Value pt = readJsonFile(filename, "Error reading .repository.json file");

    // This is looping through the member name list rather than using
    // Json::ValueIterator
    // as a workaround for a bug in the JsonCpp library (Json::ValueIterator is
    // not exported)
    Json::Value::Members member_names = pt.getMemberNames();
    for (const auto &filepath : member_names) {
      if (!isEntryValid(filepath))
        continue;
      Json::Value entry_json = pt.get(filepath, "");
      RepositoryEntry &entry = repo[filepath];
      entry.remote = true;
      entry.directory = entry_json.get("directory", false).asBool();
      entry.pub_date = DateAndTime(entry_json.get("pub_date", "").asString());
      entry.description = entry_json.get("description", "").asString();
      entry.author = entry_json.get("author", "").asString();
      entry.status = BOTH_UNCHANGED;
    }

  } catch (std::exception &ex) {
    std::stringstream ss;
    ss << "RuntimeError: checking database >> " << ex.what();
    g_log.error() << "ScriptRepository: " << ss.str() << ". Input: " << filename << '\n';
    throw ScriptRepoException(ss.str(), filename);
  } catch (...) {
    g_log.error() << "FATAL Unknown error (checking database): " << filename << '\n';
    throw;
  }
}
/**
 @todo describe
 */
void ScriptRepositoryImpl::parseLocalRepository(Repository &repo) {
  recursiveParsingDirectories(local_repository, repo);
}
/**
 This method will parse through all the entries inside the local.json
 file to get the information about the downloaded date and the version
 of the downloaded file. This information will be used to extract the
 status of the file entry.

 All the entries should be already created before, because, if the entry
 was once downloaded, it should be already at the central repository,
 as well as in the local file system.

 The parseDownloadedEntries is not expected to create any new entry.
 If it finds that the entry is not set as local.

 The parseDownloadedEntries will remove all the entries that are not
 shown anymore inside the local file system or the central repository.
 This is usefull to understand that a file has been deleted.

 :param repo: Reference to the pointer so to update it with the information

 */
void ScriptRepositoryImpl::parseDownloadedEntries(Repository &repo) {
  std::string filename = std::string(local_repository).append(".local.json");
  std::vector<std::string> entries_to_delete;
  Repository::iterator entry_it;
  std::unordered_set<std::string> folders_of_deleted;

  try {
    Json::Value pt = readJsonFile(filename, "Error reading .local.json file");

    // This is looping through the member name list rather than using
    // Json::ValueIterator
    // as a workaround for a bug in the JsonCpp library (Json::ValueIterator is
    // not exported)
    Json::Value::Members member_names = pt.getMemberNames();
    for (const auto &filepath : member_names) {
      Json::Value entry_json = pt.get(filepath, "");

      entry_it = repo.find(filepath);
      if (entry_it != repo.end()) {
        // entry found, so, lets update the entry
        if (entry_it->second.local && entry_it->second.remote) {
          // this is the normal condition, the downloaded entry
          // was found at the local file system and at the remote repository

          entry_it->second.downloaded_pubdate = DateAndTime(entry_json.get("downloaded_pubdate", "").asString());
          entry_it->second.downloaded_date = DateAndTime(entry_json.get("downloaded_date", "").asString());
          std::string auto_update = entry_json.get("auto_update", "false").asString();
          entry_it->second.auto_update = (auto_update == "true"); // get().asBool() fails here on OS X

        } else {
          // if the entry was not found locally or remotely, this means
          // that this entry was deleted (remotely or locally),
          // so it should not appear at local_repository json any more
          entries_to_delete.emplace_back(filepath);
          folders_of_deleted.insert(getParentFolder(filepath));
        }
      } else {
        // this entry was never created before, so it should not
        // exist in local repository json
        entries_to_delete.emplace_back(filepath);
      }

    } // end loop FOREACH entry in local json

    // delete the entries to be deleted in json file
    if (!entries_to_delete.empty()) {

      // clear the auto_update flag from the folders if the user deleted files
      for (const auto &folder : folders_of_deleted) {
        if (!pt.isMember(folder))
          continue;

        entry_it = repo.find(folder);
        if (entry_it == repo.end())
          continue;

        if (entry_it->second.auto_update) {
          entry_it->second.auto_update = false;
          entries_to_delete.emplace_back(folder);
        }
      }

      for (auto &entry : entries_to_delete) {
        // remove this entry
        pt.removeMember(entry);
      }
#if defined(_WIN32) || defined(_WIN64)
      // set the .repository.json and .local.json not hidden (to be able to edit
      // it)
      SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif
      writeJsonFile(filename, pt, "Error writing .local.json file");
#if defined(_WIN32) || defined(_WIN64)
      // set the .repository.json and .local.json hidden
      SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
    }
  } catch (std::exception &ex) {
    std::stringstream ss;
    ss << "RuntimeError: checking downloaded entries >> " << ex.what();
    g_log.error() << "ScriptRepository: " << ss.str() << ". Input: " << filename << '\n';
    throw ScriptRepoException(ss.str(), filename);
  } catch (...) {
    g_log.error() << "FATAL Unknown error (checking downloaded entries): " << filename << '\n';
    throw;
  }
}

void ScriptRepositoryImpl::updateLocalJson(const std::string &path, const RepositoryEntry &entry) {

  std::string filename = std::string(local_repository).append(".local.json");
  Json::Value local_json = readJsonFile(filename, "Error reading .local.json file");

  if (!local_json.isMember(path)) {

    // Create new entry
    Json::Value new_entry;
    new_entry["downloaded_date"] = entry.downloaded_date.toFormattedString();
    new_entry["downloaded_pubdate"] = entry.downloaded_pubdate.toFormattedString();

    // Add new entry to repository json value
    local_json[path] = new_entry;

  } else {

    Json::Value replace_entry;
    replace_entry["downloaded_date"] = entry.downloaded_date.toFormattedString();
    replace_entry["downloaded_pubdate"] = entry.downloaded_pubdate.toFormattedString();
    replace_entry["auto_update"] = ((entry.auto_update) ? "true" : "false");

    // Replace existing entry for this file
    local_json.removeMember(path);
    local_json[path] = replace_entry;
  }

#if defined(_WIN32) || defined(_WIN64)
  // set the .repository.json and .local.json not hidden to be able to edit it
  SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif
  writeJsonFile(filename, local_json, "Error writing .local.json file");
#if defined(_WIN32) || defined(_WIN64)
  // set the .repository.json and .local.json hidden
  SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}

std::string ScriptRepositoryImpl::printStatus(SCRIPTSTATUS st) {
  switch (st) {
  case BOTH_UNCHANGED:
    return "Unchanged";
  case LOCAL_ONLY:
    return "LocalOnly";
  case LOCAL_CHANGED:
    return "LocalChanged";
  case REMOTE_ONLY:
    return "RemoteOnly";
  case REMOTE_CHANGED:
    return "RemoteChanged";
  case BOTH_CHANGED:
    return "BothChanged";
  default:
    return "FAULT: INVALID STATUS";
  }
}
/**
 @todo describe
 */
void ScriptRepositoryImpl::recursiveParsingDirectories(const std::string &path, Repository &repo) {
  using Poco::DirectoryIterator;
  DirectoryIterator end;
  try {
    for (DirectoryIterator it(path); it != end; ++it) {
      std::string entry_path = convertPath(it->path());

      if (!isEntryValid(entry_path))
        continue;

      // g_log.debug() << "RecursiveParsing: insert : " << entry_path <<
      // '\n';
      RepositoryEntry &entry = repo[entry_path];
      entry.local = true;
      entry.current_date = DateAndTime(Poco::DateTimeFormatter::format(it->getLastModified(), timeformat));
      entry.directory = it->isDirectory();
      if (it->isDirectory())
        recursiveParsingDirectories(it->path(), repo);
    }
  } catch (Poco::Exception &ex) {
    g_log.error() << "ScriptRepository: failed to parse the directory: " << path << " : " << ex.className() << " : "
                  << ex.displayText() << '\n';
    // silently ignore this exception.
    // throw ScriptRepoException(ex.displayText());
  } catch (std::exception &ex) {
    std::stringstream ss;
    ss << "unknown exception while checking local file system. " << ex.what() << ". Input = " << path;
    g_log.error() << "ScriptRepository: " << ss.str() << '\n';
    throw ScriptRepoException(ss.str());
  }
}

bool ScriptRepositoryImpl::isEntryValid(const std::string &path) {
  // g_log.debug() << "Is valid entry? " << path << '\n';
  if (path == ".repository.json")
    return false;
  if (path == ".local.json")
    return false;
  // hide everything under system folder
  if (path == "system" || path.starts_with("system/"))
    return false;

  try {
    boost::regex re1(ignoreregex);

    if (boost::regex_match(path, re1))
      return false;
    // TODO: apply the pattern ingore checking
  } catch (std::exception &ex) {
    g_log.warning() << "Pattern exception : " << ignoreregex << ": " << ex.what() << '\n';
  }
  return true;
}

std::string ScriptRepositoryImpl::getParentFolder(const std::string &file) {
  size_t pos = file.rfind('/');
  if (pos == file.npos) {
    return "";
  }

  return std::string(file.begin(), file.begin() + pos);
}

/**
 Transform the file path in a path related to the local repository.
 Set the flag file_is_local to true if the file already exists inside
 the local machine.

 For example:

 @code
 // consider the local repository at /opt/scripts_repo/
 bool flag;
 convertPath("/opt/scripts_repo/README.md", flag) // returns: README.md
 convertPath("README.md", flag) // returns: README.md
 // consider the local repository at c:\MantidInstall\scripts_repo
 convertPath("c:\MantidInstall\scripts_repo\README.md", flag)// returns
 README.md
 @endcode
 */
std::string ScriptRepositoryImpl::convertPath(const std::string &path) {
  std::vector<std::string> lookAfter;
  using Poco::Path;
  lookAfter.emplace_back(Path::current());
  //    lookAfter.emplace_back(Path::home());
  lookAfter.emplace_back(local_repository);

  Path pathFound;
  bool file_is_local;

  // try to find the given path at one of the paths at lookAfter.
  file_is_local = Path::find(lookAfter.begin(), lookAfter.end(), path, pathFound);
  // get the absolute path:
  std::string absolute_path;
  if (file_is_local)
    absolute_path = pathFound.absolute().toString();
  else
    absolute_path = path;
  // g_log.debug() << "ConvertPath: Entered: " << path << " and
  // local_repository: " << local_repository << '\n'; this is
  // necessary because in windows, the absolute path is given
  // with \ slash.
  boost::replace_all(absolute_path, "\\", "/");

  // check it the path is inside the repository:
  size_t pos = absolute_path.find(local_repository);

  if (pos == std::string::npos) {
    // the given file is not inside the local repository. It can not be
    // converted.
    return path;
  } else {
    // the path is inside the local repository
    // remove the repo_path from te absolute path
    // +1 to remove the slash /
    std::string retpath(absolute_path.begin() + pos + local_repository.size(), absolute_path.end());
    // g_log.debug() << "ConvertPath: Entered: " << path << " return: " <<
    // retpath << '\n';
    return retpath;
  }
  return path;
}

} // namespace Mantid::API
