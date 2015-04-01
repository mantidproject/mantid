#include "MantidKernel/Logger.h"
#include "MantidRemoteJobManagers/MantidWebServiceAPIHelper.h"
#include "MantidRemoteJobManagers/MantidWebServiceAPIJobManager.h"
#include "MantidRemoteJobManagers/SimpleJSON.h"

namespace Mantid {
namespace RemoteJobManagers {

// Register the manager into the RemoteJobManagerFactory
// TODO Factory TODO
// DECLARE_REMOTEJOBMANAGER(MantidWebServiceAPIJobManager);

namespace {
// static logger object
Mantid::Kernel::Logger g_log("MantidWebServiceAPIJobManager");
}

using namespace Mantid::Kernel;

void MantidWebServiceAPIJobManager::abortRemoteJob(const std::string &jobID) {
  std::istream &respStream =
      m_helper.httpGet("/abort", std::string("JobID=") + JobID);

  if (m_helper.lastStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

void MantidWebServiceAPIJobManager::authenticate(std::string &username,
                                                 std::string &password) {
  MantidWebServiceAPIHelper helper;

  std::istream &respStream =
      m_helper.httpGet("/authenticate", "", username, password);
  if (m_helper.lastStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

void MantidWebServiceAPIJobManager::downloadRemoteFile(
    const std::string &transactionID, const std::string &remoteFileName,
    const std::string &localFileName) {

  std::istream &respStream = m_helper.httpGet(
      "/download", std::string("TransID=") + getPropertyValue("TransactionID") +
                       "&File=" + getPropertyValue("RemoteFileName"));

  if (m_helper.lastStatus() == Poco::Net::HTTPResponse::HTTP_OK) {

    std::string localFileName = getPropertyValue("LocalFileName");
    std::ofstream outfile(localFileName.c_str());
    if (outfile.good()) {
      outfile << respStream.rdbuf();
      outfile.close();
      g_log.information() << "Downloaded '"
                          << getPropertyValue("RemoteFileName") << "' to '"
                          << getPropertyValue("LocalFileName") << "'"
                          << std::endl;
    } else {
      throw(std::runtime_error(
          std::string("Failed to open " + getPropertyValue("LocalFileName"))));
    }
  } else {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

std::vector<IRemoteJobManager::RemoteJobInfo>
MantidWebServiceAPIJobManager::queryAllRemoteJobs() const {

  TODO

  return std::vector<IRemoteJobManager::RemoteJobInfo>();
}

std::vector<std::string> MantidWebServiceAPIJobManager::queryRemoteFile(
    const std::string &transactionID) const {

  std::istream &respStream = m_helper.httpGet(
      "/files", std::string("TransID=") + transactionID);
  JSONObject resp;
  initFromStream(resp, respStream);
  std::vector<std::string> filenames;
  if (m_helper.lastStatus() == Poco::Net::HTTPResponse::HTTP_OK) {

    JSONArray files;
    std::string oneFile;
    resp["Files"].getValue(files);
    for (unsigned int i = 0; i < files.size(); i++) {
      files[i].getValue(oneFile);
      filenames.push_back(oneFile);
    }

    setProperty("FileNames", filenames);
  } else {
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }

  return filenames;
}

IRemoteJobManager::RemoteJobInfo
MantidWebServiceAPIJobManager::queryRemoteJob(const std::string &jobID) const {

  return IRemoteJobManager::RemoteJobInfo();
}

std::string MantidWebServiceAPIJobManager::startRemoteTransaction() {
  return "";
}

void MantidWebServiceAPIJobManager::stopRemoteTransaction(
    const std::string &transactionID) {}

std::string MantidWebServiceAPIJobManager::submitRemoteJob(
    const std::string &transactionID, const std::string &runnable,
    const std::string &param, const std::string &taskName, const int numNodes,
    const int coresPerNode) {

  return "";
}

void MantidWebServiceAPIJobManager::uploadRemoteFile(
    const std::string &transactionID, const std::string &remoteFileName,
    const std::string &localFileName) {}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
