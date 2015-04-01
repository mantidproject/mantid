#ifndef MANTID_REMOTEJOBMANAGERS_LSFJOBMANAGER_H
#define MANTID_REMOTEJOBMANAGERS_LSFJOBMANAGER_H

#include <map>

#include "MantidAPI/IRemoteJobManager.h"

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
  virtual void authenticate(const std::string &username,
                            const std::string &password) = 0;

  virtual void abortRemoteJob(const std::string &jobID);

  virtual std::string
  submitRemoteJob(const std::string &transactionID, const std::string &runnable,
                  const std::string &param, const std::string &taskName = "",
                  const int numNodes = 0, const int coresPerNode = 0);

  virtual void downloadRemoteFile(const std::string &transactionID,
                                  const std::string &remoteFileName,
                                  const std::string &localFileName);

  virtual std::vector<RemoteJobInfo> queryAllRemoteJobs() const;

  virtual std::vector<std::string>
  queryRemoteFile(const std::string &transactionID) const;

  virtual RemoteJobInfo queryRemoteJob(const std::string &jobID) const;

  virtual std::string startRemoteTransaction();

  virtual void stopRemoteTransaction(const std::string &transactionID);

  virtual void uploadRemoteFile(const std::string &transactionID,
                                const std::string &remoteFileName,
                                const std::string &localFileName);

protected:
  typedef std::map<std::string, std::string> StringToStringMap;

  /// method that deals with the actual HTTP(S) connection (convenient to
  /// mock up all inet messaging)
  virtual int doSendRequestGetResponse(
      const std::string &url, std::ostream &response,
      const StringToStringMap &headers = StringToStringMap(),
      const std::string &method = std::string(),
      const std::string &body = "") const;

  /// has this transaction being started (and not stopped)?
  bool findTransaction(const std::string &id) const;
  void addJobInTransaction(std::string &jobID);

  // cookie obtained after logging in
  struct Token {
    Token(std::string &u, std::string &t) : m_url(u), m_token_str(t){};
    std::string m_url;
    std::string m_token_str;
  };

  typedef std::pair<std::string, Token> UsernameToken;

  // store for username-token pairs
  std::map<std::string, Token> m_tokenStash;

  /// Minimal representation of a transaction: an ID and a list of job IDs
  struct Transaction {
    Transaction() : stopped(false), jobIDs() {}
    bool stopped;
    std::vector<std::string> jobIDs;
  };

  /// Minimal store for transaction information
  std::map<std::string, Transaction> m_transactions;

  // HTTP specifics for SCARF (IBM LSF PAC)
  static std::string m_acceptType;

  /// to login
  static std::string m_loginBaseURL;
  static std::string m_loginPath;
  /// to abort/kill/cancel a job identified by id
  static std::string m_killPathBase;
  /// to query the status of all (available) jobs
  static std::string m_allJobsStatusPath;
  /// to query status of jobs by id
  static std::string m_jobIdStatusPath;
  /// to upload files to the remote compute resource
  static std::string m_uploadPath;
  /// to submit jobs
  static std::string m_submitPath;
  /// to download one file (by name)
  static std::string m_downloadOneBasePath;
  /// to download all job files (normally the job id is appended)
  static std::string m_downloadAllJobFilesBasePath;

private:
  /// TODO: this could well go to an LSFHelper class

  /// fill in output properties with job status and info
  std::vector<IRemoteJobManager::RemoteJobInfo>
  genOutputStatusInfo(const std::string &resp,
                      const std::string &jobID = std::string()) const;

  void getOneJobFile(const std::string &jobId, const std::string &remotePath,
                     const std::string &localPath, const Token &t);

  void getAllJobFiles(const std::string &jobId, const std::string &localDir,
                      const Token &t);

  const std::string checkDownloadOutputFile(const std::string &localPath,
                                            const std::string &fname) const;

  void encodeParam(std::string &body, const std::string &boundary,
                   const std::string &paramName, const std::string &paramVal);

  std::string buildSubmitBody(const std::string &appName,
                              const std::string &boundary,
                              const std::string &inputFile,
                              const std::string &inputArgs,
                              const std::string &jobName = std::string(),
                              const int numNodes = 0,
                              const int coresPerNode = 0);

  std::string buildUploadBody(const std::string &boundary,
                              const std::string &destDir,
                              const std::string &filename);

  const std::string filterPACFilename(const std::string PACName) const;

  std::string extractPACErrMsg(const std::string &response) const;
};

} // namespace RemoteJobManagers
} // namespace Mantid

#endif // MANTID_REMOTEJOBMANAGERS_LSFJOBMANAGER_H
