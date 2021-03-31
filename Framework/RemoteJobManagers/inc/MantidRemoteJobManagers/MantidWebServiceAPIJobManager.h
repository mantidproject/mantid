// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IRemoteJobManager.h"
#include "MantidRemoteJobManagers/MantidWebServiceAPIHelper.h"

namespace Mantid {
namespace RemoteJobManagers {
/**
MantidWebServiceAPIJobManager implements a remote job manager that
knows how to talk to the Mantid web service / job submission API
(http://www.mantidproject.org/Remote_Job_Submission_API). This is
being used for example for the Fermi cluster at SNS.
*/
class DLLExport MantidWebServiceAPIJobManager : public Mantid::API::IRemoteJobManager {
public:
  void authenticate(const std::string &username, const std::string &password) override;

  void logout(const std::string &username) override;

  std::string submitRemoteJob(const std::string &transactionID, const std::string &runnable, const std::string &param,
                              const std::string &taskName = "", const int numNodes = 1,
                              const int coresPerNode = 1) override;

  void downloadRemoteFile(const std::string &transactionID, const std::string &remoteFileName,
                          const std::string &localFileName) override;

  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> queryAllRemoteJobs() const override;

  std::vector<std::string> queryRemoteFile(const std::string &transactionID) const override;

  Mantid::API::IRemoteJobManager::RemoteJobInfo queryRemoteJob(const std::string &jobID) const override;

  std::string startRemoteTransaction() override;

  void stopRemoteTransaction(const std::string &transactionID) override;

  void abortRemoteJob(const std::string &jobID) override;

  void uploadRemoteFile(const std::string &transactionID, const std::string &remoteFileName,
                        const std::string &localFileName) override;

protected:
  /// Use the helper for these operations
  virtual std::istream &httpGet(const std::string &path, const std::string &query_str = "",
                                const std::string &username = "", const std::string &password = "") const {
    return m_helper.httpGet(path, query_str, username, password);
  }

  virtual std::istream &
  httpPost(const std::string &path, const MantidWebServiceAPIHelper::PostDataMap &postData,
           const MantidWebServiceAPIHelper::PostDataMap &fileData = MantidWebServiceAPIHelper::PostDataMap(),
           const std::string &username = "", const std::string &password = "") const {
    return m_helper.httpPost(path, postData, fileData, username, password);
  }

  virtual Poco::Net::HTTPResponse::HTTPStatus lastStatus() const { return m_helper.lastStatus(); }

  void clearSessionCookies() { m_helper.clearSessionCookies(); }

private:
  MantidWebServiceAPIHelper m_helper;
};

} // namespace RemoteJobManagers
} // namespace Mantid
