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

/**
 * Abort a previously submitted job
 *
 * @param ID of the job to abort (as produced by submitRemoteJob())
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource.
 */
void MantidWebServiceAPIJobManager::abortRemoteJob(const std::string &jobID) {
  std::istream &respStream =
    httpGet("/abort", std::string("JobID=") + jobID);
  if (lastStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

/**
 * Authenticate to the remote compute resource
 *
 * @param username name of the user to authenticate as
 * @param password password associated with the specified user
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource.
 */
void MantidWebServiceAPIJobManager::authenticate(const std::string &username,
                                                 const std::string &password) {
  MantidWebServiceAPIHelper helper;

  std::istream &respStream =
      httpGet("/authenticate", "", username, password);
  if (lastStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

/**
 * Download a file from a remote compute resource
 *
 * @param transactionID ID of the transaction that owns the file
 *
 * @param remoteFileName name of the file on the remote machine. (Filename only;
 * no path)
 *
 * @param localFileName full pathname on the local machine where the downloaded
 * file should be saved.
 *
 * @throws std::runtime_error if there are file I/O issues or any
 * issues in the communication with the (remote) compute resource.
 */
void MantidWebServiceAPIJobManager::downloadRemoteFile(
    const std::string &transactionID, const std::string &remoteFileName,
    const std::string &localFileName) {

  std::istream &respStream =
      httpGet("/download", std::string("TransID=") + transactionID +
                                        "&File=" + remoteFileName);

  if (lastStatus() == Poco::Net::HTTPResponse::HTTP_OK) {

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

/**
 * Query a remote compute resource for all jobs the user has submitted
 *
 * @return information for all the jobs found. Note that the date/time
 * fields (submission, start, completion) are optional and may not be
 * provided
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource.
 */
std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo>
MantidWebServiceAPIJobManager::queryAllRemoteJobs() const {

  std::istream &respStream = httpGet(std::string("/query"));
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

  if (Poco::Net::HTTPResponse::HTTP_OK != lastStatus()) {
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
    info.completionTime = DateAndTime(completionDates[i]);
    result.push_back(info);
  }

  return result;
}

/**
 * Retrieve a list of the files from a remote compute resource.
 *
 * @param transactionID ID of the transaction who’s files we want to list. Must
 * have been created with startRemoteTransaction()
 *
 * @return names of all the files that were found
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource.
 */
std::vector<std::string> MantidWebServiceAPIJobManager::queryRemoteFile(
    const std::string &transactionID) const {

  std::istream &respStream =
      httpGet("/files", std::string("TransID=") + transactionID);
  JSONObject resp;
  initFromStream(resp, respStream);
  std::vector<std::string> filenames;
  if (lastStatus() == Poco::Net::HTTPResponse::HTTP_OK) {

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

/**
 * Query a remote compute resource for a specific job
 *
 * @return job information. Note that the date/time information (submission,
 * start, completion) is optional and may not be provided
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource.
 */
Mantid::API::IRemoteJobManager::RemoteJobInfo
MantidWebServiceAPIJobManager::queryRemoteJob(const std::string &jobID) const {
  std::istream &respStream =
      httpGet("/query", std::string("JobID=") + jobID);
  JSONObject resp;
  initFromStream(resp, respStream);

  if (Poco::Net::HTTPResponse::HTTP_OK != lastStatus()) {
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }

  if (resp[jobID].getType() != JSONValue::OBJECT) {
    throw(std::runtime_error("Expected value not found in return stream.  "
                             "Has the client/server protocol changed?!?"));
  }

  Mantid::API::IRemoteJobManager::RemoteJobInfo info;

  JSONObject status;
  resp[jobID].getValue(status);

  std::string value;
  status["JobStatus"].getValue(value);
  info.status = value;

  status["JobName"].getValue(value);
  info.name = value;

  status["ScriptName"].getValue(value);
  info.runnableName = value;

  status["TransID"].getValue(value);
  info.transactionID = value;

  // The time stuff is actually an optional extension.  We could check the
  // info
  // URL and see if the server implements it, but it's easier to just look in
  // the output and see if the values are there...
  if (status.find("SubmitDate") != status.end()) {
    status["SubmitDate"].getValue(value);
    info.submitDate = DateAndTime(value);

    status["StartDate"].getValue(value);
    info.startDate = DateAndTime(value);

    status["CompletionDate"].getValue(value);
    info.completionTime = DateAndTime(value);
  }

  return info;
}

/**
 * Start a job transaction on a remote compute resource.
 *
 * @return ID of the new transaction
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource.
 */
std::string MantidWebServiceAPIJobManager::startRemoteTransaction() {
  std::istream &respStream = httpGet("/transaction", "Action=Start");
  JSONObject resp;
  initFromStream(resp, respStream);

  if (Poco::Net::HTTPResponse::HTTP_OK != lastStatus()) {
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
  std::string transId;
  resp["TransID"].getValue(transId);
  g_log.information() << "Transaction ID " << transId << " started."
                      << std::endl;

  return transId;
}

/**
 * Stop a job transaction on a remote compute resource.
 *
 * @param transactionID ID string returned when the transaction was created with
 * startRemoteTransaction()
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource.
 */
void MantidWebServiceAPIJobManager::stopRemoteTransaction(
    const std::string &transactionID) {
  std::string transId = transactionID;
  std::istream &respStream = httpGet(
      "/transaction", std::string("Action=Stop&TransID=") + transId);

  if (lastStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
    g_log.information() << "Transaction ID " << transId << " stopped."
                        << std::endl;
  } else {
    JSONObject resp;
    initFromStream(resp, respStream);
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }
}

/**
 * Submit a job, which in this context means a Mantid Python script
 *
 * @param transactionID transaction ID to associate with this job (obtained with
 * startRemoteTransaction())
 *
 * @param runnable name of the runnable (Python script that will be executed)
 *
 * @param param content of the Python script (as plain text, the actual python
 * code to execute)
 *
 * @param taskName a shot name to give to the job on the compute resource
 * @param numNodes number of computing nodes to request
 * @param coresPerNode number of cores to use in every node
 *
 * @return an ID string for this job
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource.
 */
std::string MantidWebServiceAPIJobManager::submitRemoteJob(
    const std::string &transactionID, const std::string &runnable,
    const std::string &param, const std::string &taskName, const int numNodes,
    const int coresPerNode) {

  MantidWebServiceAPIHelper::PostDataMap postData;

  postData["TransID"] = transactionID;
  postData["NumNodes"] = boost::lexical_cast<std::string>(numNodes);
  postData["CoresPerNode"] = boost::lexical_cast<std::string>(coresPerNode);

  postData["ScriptName"] = runnable;
  postData[runnable] = param;

  // Job name is optional
  std::string jobName = taskName;
  if (jobName.length() > 0) {
    postData["JobName"] = jobName;
  }

  std::istream &respStream = httpPost("/submit", postData);
  JSONObject resp;
  initFromStream(resp, respStream);
  if (Poco::Net::HTTPResponse::HTTP_CREATED != lastStatus()) {
    std::string errMsg;
    resp["Err_Msg"].getValue(errMsg);
    throw(std::runtime_error(errMsg));
  }

  std::string jobId;
  resp["JobID"].getValue(jobId);
  g_log.information() << "Job submitted.  Job ID =  " << jobId << std::endl;

  return jobId;
}

/**
 * Uploads a file to the (remote) compute resource
 *
 * @param transactionID The transaction the file will be associated
 * with. It must have been created with startRemoteTransaction
 *
 * @param remoteFileName name to save the file as on the remote computer.
 * (Filename only; no path information)
 *
 * @param localFileName full pathname (on the local machine) of the file to
 * upload
 *
 * @throws std::runtime_error if there are issues in the communication with the
 * (remote) compute resource, or file I/O issues.
 */
void MantidWebServiceAPIJobManager::uploadRemoteFile(
    const std::string &transactionID, const std::string &remoteFileName,
    const std::string &localFileName) {
  MantidWebServiceAPIHelper::PostDataMap postData;
  postData["TransID"] = transactionID;

  std::ifstream infile(localFileName.c_str());
  if (infile.good()) {
    // Yes, we're reading the entire file into memory.  Obviously, this is only
    // feasible for fairly small files...
    MantidWebServiceAPIHelper::PostDataMap fileData;
    fileData[remoteFileName] =
        std::string(std::istreambuf_iterator<char>(infile),
                    std::istreambuf_iterator<char>());
    infile.close();

    std::istream &respStream = httpPost("/upload", postData, fileData);
    if (lastStatus() ==
        Poco::Net::HTTPResponse::HTTP_CREATED) // Upload returns a "201 -
                                               // Created" code on success
    {
      g_log.information() << "Uploaded '" << remoteFileName << "' to '"
                          << localFileName << "'" << std::endl;
    } else {
      JSONObject resp;
      initFromStream(resp, respStream);
      std::string errMsg;
      resp["Err_Msg"].getValue(errMsg);
      throw(std::runtime_error(errMsg));
    }
  } else {
    throw(std::runtime_error(std::string("Failed to open " + localFileName)));
  }
}

} // end namespace RemoteJobManagers
} // end namespace Mantid
