#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidRemoteJobManagers/MantidWebServiceAPIHelper.h"
#include "MantidRemoteJobManagers/MantidWebServiceAPIJobManager.h"
#include "MantidRemoteJobManagers/SimpleJSON.h"

#include <fstream>

namespace Mantid {
namespace RemoteJobManagers {

// Register this job manager into the RemoteJobManagerFactory
DECLARE_REMOTEJOBMANAGER(MantidWebServiceAPIJobManager)

namespace {
// static logger object
Mantid::Kernel::Logger g_log("MantidWebServiceAPIJobManager");
}

using namespace Mantid::Kernel;

void MantidWebServiceAPIJobManager::abortRemoteJob(const std::string &jobID) {
  std::istream &respStream =
      m_helper.httpGet("/abort", std::string("JobID=") + jobID);

  if (m_helper.lastStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

void MantidWebServiceAPIJobManager::authenticate(const std::string &username,
                                                 const std::string &password) {
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

  std::istream &respStream =
      m_helper.httpGet("/download", std::string("TransID=") + transactionID +
                                        "&File=" + remoteFileName);

  if (m_helper.lastStatus() == Poco::Net::HTTPResponse::HTTP_OK) {

    std::ofstream outfile(localFileName.c_str());
    if (outfile.good()) {
      outfile << respStream.rdbuf();
      outfile.close();
      g_log.information() << "Downloaded '" << remoteFileName << "' to '"
                          << localFileName << "'" << std::endl;
    } else {
      throw(std::runtime_error(std::string("Failed to open " + localFileName)));
    }
  } else {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo>
MantidWebServiceAPIJobManager::queryAllRemoteJobs() const {

  std::istream &respStream = m_helper.httpGet(std::string("/query"));
  JSONObject resp;
  try {
    initFromStream(resp, respStream);
  } catch (JSONParseException &) {
    // Nobody else knows what a JSONParseException is, so rethrow as a
    // runtime_error
    throw(std::runtime_error("Error parsing data returned from the server.  "
                             "This probably indicates a server-side error of "
                             "some kind."));
  }

  if (Poco::Net::HTTPResponse::HTTP_OK != m_helper.lastStatus()) {
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }

  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> result;

  std::vector<std::string> jobIds;
  std::vector<std::string> jobStatusStrs;
  std::vector<std::string> jobNames;
  std::vector<std::string> scriptNames;
  std::vector<std::string> transIds;
  std::vector<std::string> submitDates;
  std::vector<std::string> startDates;
  std::vector<std::string> completionDates;

  JSONObject::const_iterator it = resp.begin();
  while (it != resp.end()) {
    jobIds.push_back((*it).first);
    JSONObject jobData;
    (*it).second.getValue(jobData);

    std::string value;
    jobData["JobStatus"].getValue(value);
    jobStatusStrs.push_back(value);

    jobData["JobName"].getValue(value);
    jobNames.push_back(value);

    jobData["ScriptName"].getValue(value);
    scriptNames.push_back(value);

    jobData["TransID"].getValue(value);
    transIds.push_back(value);

    // The time stuff is actually an optional extension.  We could check the
    // info
    // URL and see if the server implements it, but it's easier to just look
    // in
    // the output and see if the values are there...
    if (jobData.find("SubmitDate") != jobData.end()) {
      jobData["SubmitDate"].getValue(value);
      submitDates.push_back(value);

      jobData["StartDate"].getValue(value);
      startDates.push_back(value);

      jobData["CompletionDate"].getValue(value);
      completionDates.push_back(value);
    } else {
      // push back empty strings just so all the array properties have the
      // same
      // number of elements
      submitDates.push_back("");
      startDates.push_back("");
      completionDates.push_back("");
    }

    ++it;
  }

  // this is done here, very inefficiently, to avoid messing up the last loop
  for (size_t i = 0; i < resp.size(); ++i) {
    Mantid::API::IRemoteJobManager::RemoteJobInfo info;
    info.id = jobIds[i];
    info.status = jobStatusStrs[i];
    info.name = jobNames[i];
    info.runnableName = scriptNames[i];
    info.transactionID = transIds[i];
    info.submitDate = DateAndTime(submitDates[i]);
    info.startDate = DateAndTime(startDates[i]);
    infos.completionTime = DateAndTime(completionDates[i]);
    result.push_back(info);
  }

  return result;
}

std::vector<std::string> MantidWebServiceAPIJobManager::queryRemoteFile(
    const std::string &transactionID) const {

  std::istream &respStream =
      m_helper.httpGet("/files", std::string("TransID=") + transactionID);
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

  } else {
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }

  return filenames;
}

Mantid::API::IRemoteJobManager::RemoteJobInfo
MantidWebServiceAPIJobManager::queryRemoteJob(const std::string &jobID) const {

  return Mantid::API::IRemoteJobManager::RemoteJobInfo();
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
