#ifndef _MANTIDSCRIPTREPOSITORY_SCRIPTREPOSITORYIMPL_H_
#define _MANTIDSCRIPTREPOSITORY_SCRIPTREPOSITORYIMPL_H_

#include "MantidAPI/ScriptRepository.h"
#include "MantidKernel/DateAndTime.h"
#include <map>

#ifdef _WIN32
#if (IN_MANTID_SCRIPTREPO)
#define SCRIPT_DLL_EXPORT DLLExport
#else
#define SCRIPT_DLL_EXPORT DLLImport
#endif
#else
#define SCRIPT_DLL_EXPORT
#endif

namespace Mantid {
namespace API {

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
    Kernel::DateAndTime current_date;
    /// For the files that were downloaded, get the DateAndTime reported when
    /// they
    /// were created.
    Kernel::DateAndTime downloaded_date;
    /// For the remote files, get the DateAndTime of the last revision.
    Kernel::DateAndTime pub_date;
    /// Description of the files.
    std::string description;
    /// The version downloaded of this file
    Kernel::DateAndTime downloaded_pubdate;
    /// Indicate if this file should be updated automatically.
    bool auto_update;
    /// Identify the author of this file.
    std::string author;
    /// status of the current entry
    SCRIPTSTATUS status;
    /// provide a constructor, to set the default values.
    RepositoryEntry()
        : remote(false), local(false), directory(false),
          current_date(Kernel::DateAndTime::defaultTime()),
          downloaded_date(Kernel::DateAndTime::defaultTime()),
          pub_date(Kernel::DateAndTime::defaultTime()), description(""),
          downloaded_pubdate(Kernel::DateAndTime::defaultTime()),
          auto_update(false), author(""){};
  };

  typedef std::map<std::string, RepositoryEntry> Repository;

  Repository repo;

public:
  ScriptRepositoryImpl(const std::string &local_repository = std::string(),
                       const std::string &remote_url = std::string());

  virtual ~ScriptRepositoryImpl() throw();

  void connect(const std::string &server);

  void install(const std::string &local_path);

  ScriptInfo info(const std::string &path);
  const std::string &description(const std::string &path);

  std::vector<std::string> listFiles();

  void download(const std::string &file_path);

  SCRIPTSTATUS fileStatus(const std::string &file_path);

  void upload(const std::string &file_path, const std::string &comment,
              const std::string &author, const std::string &email);
  // remove file from the central repository and from local folder
  void remove(const std::string &file_path, const std::string &comment,
              const std::string &author, const std::string &email);

  /* Return true if there is a local repository installed*/
  bool isValid(void);

  std::vector<std::string> check4Update(void);

  void setIgnorePatterns(const std::string &patterns);

  std::string ignorePatterns(void);

  int setAutoUpdate(const std::string &path, bool option = true);

  /// @deprecated Should avoid this, it is not in the design file.
  std::string localRepository() const { return local_repository; }

  virtual void doDownloadFile(const std::string &url_file,
                              const std::string &local_file_path = "");
  // convenient method to allow to perform the unit tests on remove files.
  virtual std::string doDeleteRemoteFile(const std::string &url,
                                         const std::string &file_path,
                                         const std::string &author,
                                         const std::string &email,
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

  /// flag that indicate a valid repository
  bool valid;

  std::string ignoreregex;


  std::string getParentFolder(const std::string &entry);
};

} // namespace API
} // namespace Mantid

#endif // _MANTIDSCRIPTREPOSITORY_SCRIPTREPOSITORYIMPL_H_
