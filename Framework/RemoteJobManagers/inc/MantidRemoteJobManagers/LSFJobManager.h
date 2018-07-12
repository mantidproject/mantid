#ifndef MANTID_REMOTEJOBMANAGERS_LSFJOBMANAGER_H
#define MANTID_REMOTEJOBMANAGERS_LSFJOBMANAGER_H

#include <map>

#include "MantidAPI/IRemoteJobManager.h"

#include <Poco/URI.h>

namespace Mantid {
namespace RemoteJobManagers {
/**
LSFJobManager implements a remote job manager that interacts with the
Platform LSF web service. This is in principle a generic Platform LSF
web service, but for the time being it has been tested only against
the SCARF cluster (STFC, ISIS facility). Note that there is no
implementation of the authenticate method, as SCARF uses a very
particular authentication (specific URL and script) and because of
that this class has not been tested against any web service with
standard Platform LSF authentication mechanism. All other methods can
be expected to be usable for other LSF based systems with no or very
little modification.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LSFJobManager : public Mantid::API::IRemoteJobManager {
public:
  /// We currently do not have a (tested) implementation of authenticate for LSF
  /// Platform
  void authenticate(const std::string &username,
                    const std::string &password) override = 0;

  /// Most likely the logout method specific to SCARFLSF would be valid here
  void logout(const std::string &username) override = 0;

  void abortRemoteJob(const std::string &jobID) override;

  std::string
  submitRemoteJob(const std::string &transactionID, const std::string &runnable,
                  const std::string &param, const std::string &taskName = "",
                  const int numNodes = 0, const int coresPerNode = 0) override;

  void downloadRemoteFile(const std::string &transactionID,
                          const std::string &remoteFileName,
                          const std::string &localFileName) override;

  std::vector<RemoteJobInfo> queryAllRemoteJobs() const override;

  std::vector<std::string>
  queryRemoteFile(const std::string &transactionID) const override;

  RemoteJobInfo queryRemoteJob(const std::string &jobID) const override;

  std::string startRemoteTransaction() override;

  void stopRemoteTransaction(const std::string &transactionID) override;

  void uploadRemoteFile(const std::string &transactionID,
                        const std::string &remoteFileName,
                        const std::string &localFileName) override;

protected:
  /// define the "application type" (or "submission form" in the LSF web portal)
  virtual std::string guessJobSubmissionAppName(const std::string &runnablePath,
                                                const std::string &jobOptions);

  using StringToStringMap = std::map<std::string, std::string>;

  /// method that deals with the actual HTTP(S) connection (convenient to
  /// mock up all inet messaging)
  virtual int doSendRequestGetResponse(
      const Poco::URI &uri, std::ostream &rss,
      const StringToStringMap &headers = StringToStringMap(),
      const std::string &method = std::string(),
      const std::string &body = "") const;

  /// make a map of HTTP headers to prepare a request
  StringToStringMap makeHeaders(const std::string &contentType = "",
                                const std::string &token = "",
                                const std::string &acceptType = "") const;

  /// make a full URI by appending components/segments
  Poco::URI makeFullURI(const Poco::URI &base, const std::string &path,
                        const std::string &pathParam = "") const;

  /// has this transaction being started (and not stopped)?
  bool findTransaction(const std::string &id) const;
  void addJobInTransaction(const std::string &jobID);

  // cookie obtained after logging in
  struct Token {
    Token(const std::string &u, const std::string &t)
        : m_url(u), m_token_str(t){};
    Poco::URI m_url;
    std::string m_token_str;
  };

  using UsernameToken = std::pair<std::string, Token>;

  // store for username-token pairs
  static std::map<std::string, Token> g_tokenStash;

  /// Minimal representation of a transaction: an ID and a list of job IDs
  struct Transaction {
    Transaction() : stopped(false), jobIDs() {}
    bool stopped;
    std::vector<std::string> jobIDs;
  };

  /// Minimal store for transaction information
  static std::map<std::string, Transaction> g_transactions;

  // HTTP specifics for SCARF (IBM LSF PAC)
  static std::string g_acceptType;

  /// to login
  static std::string g_loginBaseURL;
  static std::string g_loginPath;
  /// to abort/kill/cancel a job identified by id
  static std::string g_killPathBase;
  /// to query the status of all (available) jobs
  static std::string g_allJobsStatusPath;
  /// to query status of jobs by id
  static std::string g_jobIdStatusPath;
  /// to upload files to the remote compute resource
  static std::string g_uploadPath;
  /// to submit jobs
  static std::string g_submitPath;
  /// to download one file (by name)
  static std::string g_downloadOneBasePath;
  /// to download all job files (normally the job id is appended)
  static std::string g_downloadAllJobFilesBasePath;

private:
  /// TODO: this could well go to an LSFHelper class

  /// fill in output properties with job status and info
  std::vector<IRemoteJobManager::RemoteJobInfo>
  genOutputStatusInfo(const std::string &resp,
                      const std::string &jobIDFilter = std::string()) const;

  void getOneJobFile(const std::string &jobId, const std::string &remotePath,
                     const std::string &localPath, const Token &t);

  void getAllJobFiles(const std::string &jobId, const std::string &localDir,
                      const Token &t);

  const std::string checkDownloadOutputFile(const std::string &localPath,
                                            const std::string &fname) const;

  void encodeParam(std::string &body, const std::string &boundary,
                   const std::string &paramName, const std::string &paramVal);

  std::string
  buildSubmitBody(const std::string &appName, const std::string &boundary,
                  const std::string &inputFile, const std::string &inputArgs,
                  const std::string &jobName = std::string(),
                  const int numNodes = 0, const int coresPerNode = 0);

  std::string buildUploadBody(const std::string &boundary,
                              const std::string &destDir,
                              const std::string &filename);

  const std::string filterPACFilename(const std::string &PACName) const;

  std::string extractPACErrMsg(const std::string &response) const;
};

} // namespace RemoteJobManagers
} // namespace Mantid

#endif // MANTID_REMOTEJOBMANAGERS_LSFJOBMANAGER_H
