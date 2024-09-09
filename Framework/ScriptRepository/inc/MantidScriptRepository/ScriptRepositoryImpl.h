// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ScriptRepository.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidScriptRepository/DllConfig.h"
#include <json/value.h>
#include <map>

namespace Mantid {
namespace API {

void writeJsonFile(const std::string &filename, const Json::Value &json, const std::string &error);

Json::Value readJsonFile(const std::string &filename, const std::string &error);

void writeStringFile(const std::string &filename, const std::string &stringToWrite, const std::string &error);

bool fileExists(const std::string &filename);

/** Implementation of Mantid::API::ScriptRepository

    This implementation relies on the definition of the Script Repository
   WebServer.

    @todo Describe better the implementation

 */
class SCRIPT_DLL_EXPORT ScriptRepositoryImpl : public ScriptRepository {
  /**
     Keep the usefull information for each entry of the repository.
   */
  class RepositoryEntry {
  public:
    /// Indicate if the file is presented at the central repository.
    bool remote;
    /// Indicate if the file is presented locally
    bool local;
    /// This entry is a directory?
    bool directory;
    /// For the local files, get the DateAndTime reported by the operative
    /// system
    /// or defaultTime if not available.
    Types::Core::DateAndTime current_date;
    /// For the files that were downloaded, get the DateAndTime reported when
    /// they
    /// were created.
    Types::Core::DateAndTime downloaded_date;
    /// For the remote files, get the DateAndTime of the last revision.
    Types::Core::DateAndTime pub_date;
    /// Description of the files.
    std::string description;
    /// The version downloaded of this file
    Types::Core::DateAndTime downloaded_pubdate;
    /// Indicate if this file should be updated automatically.
    bool auto_update;
    /// Identify the author of this file.
    std::string author;
    /// status of the current entry
    SCRIPTSTATUS status;
    /// provide a constructor, to set the default values.
    RepositoryEntry()
        : remote(false), local(false), directory(false), current_date(Types::Core::DateAndTime::defaultTime()),
          downloaded_date(Types::Core::DateAndTime::defaultTime()), pub_date(Types::Core::DateAndTime::defaultTime()),
          description(""), downloaded_pubdate(Types::Core::DateAndTime::defaultTime()), auto_update(false), author(""),
          status(BOTH_UNCHANGED) {};
  };

  using Repository = std::map<std::string, RepositoryEntry>;

  Repository repo;

public:
  ScriptRepositoryImpl(const std::string &local_rep = std::string(), const std::string &remote = std::string());

  void connect(const std::string &server) override;

  void install(const std::string &path) override;

  ScriptInfo info(const std::string &input_path) override;
  const std::string &description(const std::string &input_path) override;

  std::vector<std::string> listFiles() override;

  void download(const std::string &input_path) override;

  SCRIPTSTATUS fileStatus(const std::string &input_path) override;

  void upload(const std::string &file_path, const std::string &comment, const std::string &author,
              const std::string &email) override;
  // remove file from the central repository and from local folder
  void remove(const std::string &file_path, const std::string &comment, const std::string &author,
              const std::string &email) override;

  /* Return true if there is a local repository installed*/
  bool isValid() override;

  void setValid(const bool valid) override;

  std::vector<std::string> check4Update() override;

  void setIgnorePatterns(const std::string &patterns) override;

  std::string ignorePatterns() override;

  int setAutoUpdate(const std::string &input_path, bool option = true) override;

  /// @deprecated Should avoid this, it is not in the design file.
  std::string localRepository() override { return local_repository; }

  virtual void doDownloadFile(const std::string &url_file, const std::string &local_file_path = "");
  // convenient method to allow to perform the unit tests on remove files.
  virtual std::string doDeleteRemoteFile(const std::string &url, const std::string &file_path,
                                         const std::string &author, const std::string &email,
                                         const std::string &comment);

protected:
  void parseCentralRepository(Repository &repo);

  void parseLocalRepository(Repository &repo);

  void parseDownloadedEntries(Repository &repo);

  void ensureValidRepository();

  bool isEntryValid(const std::string &path);

  /// Path of the local repository.
  std::string local_repository;
  /// URL for the remote repository, usually:
  std::string remote_url;
  /// URL for the upload
  std::string remote_upload;

private:
  void recursiveParsingDirectories(const std::string &path, Repository &repo);

  std::string convertPath(const std::string &path);

  /*    /// Used to throw when a local repository is mal-formed.
  ScriptRepoException invalidRepository();

  ScriptRepoException systemException(const std::string info = std::string(),
                                   const std::string file = std::string(),
                                   int line = -1);




  enum FILEINFOSUPPORT{READMEFILE, PYTHONFILE};
  std::string processInfo(const std::string path, FILEINFOSUPPORT filetype); */

private:
  static std::string printStatus(SCRIPTSTATUS st);
  void download_directory(const std::string &);
  void download_file(const std::string &, RepositoryEntry &);
  void updateLocalJson(const std::string &, const RepositoryEntry &);
  void updateRepositoryJson(const std::string &, const RepositoryEntry &);

  /// flag that indicate a valid repository
  bool m_valid;

  std::string ignoreregex;

  std::string getParentFolder(const std::string &file);
};

} // namespace API
} // namespace Mantid
