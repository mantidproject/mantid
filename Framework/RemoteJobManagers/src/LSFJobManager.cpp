#include <fstream>

#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidRemoteJobManagers/LSFJobManager.h"

#include "boost/algorithm/string/replace.hpp"
#include <boost/lexical_cast.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/File.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>

namespace Mantid {
namespace RemoteJobManagers {

// Do not declare as remote job manager for the factory, this is an abstract
// class
// DECLARE_REMOTEJOBMANAGER()

namespace {
// static logger object
Mantid::Kernel::Logger g_log("LSFJobManager");
} // namespace

std::map<std::string, LSFJobManager::Token> LSFJobManager::g_tokenStash;
std::map<std::string, LSFJobManager::Transaction> LSFJobManager::g_transactions;

std::string LSFJobManager::g_acceptType = "text/plain,application/xml,text/xml";

std::string LSFJobManager::g_killPathBase =
    "webservice/pacclient/jobOperation/kill/";

std::string LSFJobManager::g_allJobsStatusPath = "webservice/pacclient/jobs?";

std::string LSFJobManager::g_jobIdStatusPath = "webservice/pacclient/jobs/";

//  The 0 at the end of the upload path is 'jobId' 0, if a jobId is given the
//  upload goes to a path relative to the job path.
std::string LSFJobManager::g_uploadPath = "webservice/pacclient/upfile/0";

std::string LSFJobManager::g_submitPath = "webservice/pacclient/submitapp";

std::string LSFJobManager::g_downloadOneBasePath = "webservice/pacclient/file/";

std::string LSFJobManager::g_downloadAllJobFilesBasePath =
    "webservice/pacclient/jobfiles/";

using namespace Mantid::Kernel;

void LSFJobManager::abortRemoteJob(const std::string &jobID) {
  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "Abort job failed because you do not seem to have logged "
        "in.");
  }
  if (jobID.empty()) {
    throw std::runtime_error(
        "Abort job failed because an empty job ID was given.");
  }

  // only support for single-user
  Token tok = g_tokenStash.begin()->second;

  const std::string token = tok.m_token_str;
  const Poco::URI fullURL = makeFullURI(tok.m_url, g_killPathBase, jobID);
  const StringToStringMap headers =
      makeHeaders(std::string("application/xml"), token, g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error(
        "Error while sending HTTP request to cancel a job: " +
        std::string(ie.what()));
  }
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    std::string resp = ss.str();
    if (std::string::npos != resp.find("<errMsg>")) {
      g_log.warning() << "Killed job with Id " << jobID
                      << " but got what looks like an "
                         "error message as response: "
                      << extractPACErrMsg(resp) << '\n';
    } else if (std::string::npos != resp.find("<actionMsg>")) {
      g_log.notice() << "Killed job with Id" << jobID << ".\n";
      g_log.debug() << "Response from server: " << resp << '\n';
    } else {
      g_log.warning() << "Killed job with Id" << jobID
                      << " but got what a response "
                         "that I do not recognize: "
                      << resp << '\n';
    }
  } else {
    throw std::runtime_error("Failed to kill job (Id: " + jobID +
                             " ) through the web "
                             "service at:" +
                             fullURL.toString() +
                             ". Please check your "
                             "existing jobs, username, and parameters.");
  }
}

/**
 * Download a file from a remote transaction/job into a local
 * directory. Note that this download as supported by LSF and in
 * particular at SCARF is job-specific: you download a file from a job
 * and not a file in the file system in general.
 *
 * @param transactionID Id of a transaction as produced by
 * startRemoteTransaction()
 *
 * @param remoteFileName File name (of a job file on the compute resource).
 * @param localFileName Local directory where to download the file(s)
 */
void LSFJobManager::downloadRemoteFile(const std::string &transactionID,
                                       const std::string &remoteFileName,
                                       const std::string &localFileName) {
  auto it = g_transactions.find(transactionID);
  if (g_transactions.end() == it)
    throw std::invalid_argument("Could not find a transaction with ID: " +
                                transactionID);

  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "File download failed. You do not seem to have logged in.");
  }

  std::vector<std::string> jobIDs = it->second.jobIDs;
  if (jobIDs.empty()) {
    throw std::runtime_error("There are no jobs in this transaction and this "
                             "job manager cannot download files when no jobs "
                             "have been submitted within a transaction.");
  }

  // only support for single-user
  Token tok = g_tokenStash.begin()->second;

  // assume that the last job is what we want
  const std::string jobId = jobIDs.back();
  if (remoteFileName.empty()) {
    // no/empty name implies we want all the files of a remote job
    getAllJobFiles(jobId, localFileName, tok);
  } else {
    // name given, so we directly download this single file
    getOneJobFile(jobId, remoteFileName, localFileName, tok);
  }
}

/**
 * Query the status of jobs running (if successful will return info on
 * jobs running for our user). Note that at least for SCARF only the
 * following information fields can be retrieved: job id, name,
 * status, and command line.
 *
 * @return status and other info for all the jobs reported
 */
std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo>
LSFJobManager::queryAllRemoteJobs() const {
  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "Job status query failed. You do not seem to have logged "
        "in.");
  }
  // only support for single-user
  Token tok = g_tokenStash.begin()->second;

  // Job query status, needs these headers:
  // headers = {'Content-Type': 'application/xml', 'Cookie': token,
  //            'Accept': ACCEPT_TYPE}
  const std::string token = tok.m_token_str;

  // Note: Poco::URI will remove the trailing '?' that LSF in
  // principle uses.  Beware of potential issues with this.
  const Poco::URI fullURL = makeFullURI(tok.m_url, g_allJobsStatusPath);
  const StringToStringMap headers =
      makeHeaders(std::string("application/xml"), token, g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error(
        "Error while sending HTTP request to query the status "
        "of jobs: " +
        std::string(ie.what()));
  }

  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> info;
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    std::string resp = ss.str();
    if (std::string::npos != resp.find("<Jobs>") &&
        std::string::npos != resp.find("<extStatus>")) {
      info = genOutputStatusInfo(resp);
      g_log.notice() << "Queried the status of jobs and stored the "
                        "information in output properties.\n";
    } else {
      g_log.warning() << "Queried the status of jobs but got what looks "
                         "like an error message as response: "
                      << resp << '\n';
    }
    g_log.notice() << "Queried job status successfully.\n";
    g_log.debug() << "Response from server: " << resp << '\n';
  } else {
    throw std::runtime_error(
        "Failed to obtain job status information through the "
        "web service at:" +
        fullURL.toString() +
        ". Please check your "
        "username, credentials, and parameters.");
  }

  return info;
}

/**
 * Get the files available for download in the most recently submitted
 * job for the transaction given as input parameter.
 *
 * @param transactionID the ID of a transaction as produced by
 * startRemoteTransaction()
 *
 * @return File names on the remote compute resource
 */
std::vector<std::string>
LSFJobManager::queryRemoteFile(const std::string &transactionID) const {
  auto it = g_transactions.find(transactionID);
  if (g_transactions.end() == it)
    throw std::invalid_argument("Could not find a transaction with ID: " +
                                transactionID);
  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "Remote file names query failed. You do not seem to have logged in.");
  }
  // only support for single-user
  Token tok = g_tokenStash.begin()->second;

  std::vector<std::string> jobIDs = it->second.jobIDs;
  if (jobIDs.empty()) {
    throw std::runtime_error("There are no jobs in this transaction and this "
                             "job manager cannot query files when no jobs "
                             "have been submitted within a transaction.");
  }

  // Downloadable files from a job, needs these headers:
  // headers = {'Content-Type': 'text/plain', 'Cookie': token, 'Accept':
  // ACCEPT_TYPE}

  // assume that the last job is what we want
  const std::string jobId = jobIDs.back();
  const std::string token = tok.m_token_str;

  const Poco::URI fullURL =
      makeFullURI(tok.m_url, g_downloadAllJobFilesBasePath, jobId);
  const StringToStringMap headers =
      makeHeaders(std::string("application/xml"), token, g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error(
        "Error while sending HTTP request to download files: " +
        std::string(ie.what()));
  }
  // what you get in this response is one line with text like this:
  // 'PAC Server*/home/isisg/scarf362/../scarf362/
  // Mantid_tomography_1_1423743450375PtlPj/417666.error*FILE*281*true;PAC
  // Server*/
  // home/isisg/scarf362/../scarf362/
  // Mantid_tomography_1_1423743450375PtlPj/417666.output*FILE*1145*true;'
  //   (the number between *FILE* and *true is the size in bytes)
  std::vector<std::string> filePACNames;
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    std::string resp = ss.str();
    // this is what indicates success/failure: presence of '/' or '\'
    if (std::string::npos != resp.find('/') ||
        std::string::npos != resp.find('\\')) {
      // you can get multiple files, as remote file names listed separated by
      // ';'
      std::string PACname;
      while (std::getline(ss, PACname, ';')) {
        filePACNames.push_back(PACname);
      }
      // Do this if you want to actually download the files
      // for (size_t i = 0; i < filePACNames.size(); i++) {
      //  getOneJobFile(jobId, filePACNames[i], localDir, t);
      //}
    }
  } else {
    throw std::runtime_error(
        "Failed to get the list of downloadable files for job (Id:" + jobId +
        " ) through "
        "the web service at:" +
        fullURL.toString() +
        ". Please check your "
        "existing jobs, username, and parameters.");
  }

  return filePACNames;
}

/**
 * Query the status of jobs running (if successful will return info on
 * jobs running for our user). Note that at least for SCARF
 * information is only produced for the following fields: job id,
 * name, status, and command line.
 *
 * @param jobId Identifier of a job as used by the job scheduler
 * (integer number).
 */
Mantid::API::IRemoteJobManager::RemoteJobInfo
LSFJobManager::queryRemoteJob(const std::string &jobID) const {
  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "Job status query failed. You do not seem to have logged "
        "in.");
  }
  if (jobID.empty()) {
    throw std::runtime_error(
        "Job status failed because an empty job ID was given.");
  }

  // only support for single-user
  Token tok = g_tokenStash.begin()->second;

  // Job query status, needs these headers:
  // headers = {'Content-Type': 'application/xml', 'Cookie': token,
  //            'Accept': ACCEPT_TYPE}
  const std::string token = tok.m_token_str;

  const Poco::URI fullURL = makeFullURI(tok.m_url, g_jobIdStatusPath);
  const StringToStringMap headers =
      makeHeaders(std::string("application/xml"), token, g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error(
        "Error while sending HTTP request to query the status "
        "of a job: " +
        std::string(ie.what()));
  }

  std::vector<IRemoteJobManager::RemoteJobInfo> info;
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    std::string resp = ss.str();
    if (std::string::npos != resp.find("<Jobs>") &&
        std::string::npos != resp.find("<extStatus>")) {
      info = genOutputStatusInfo(resp, jobID);
      g_log.notice() << "Queried job status (Id " << jobID
                     << ") and stored "
                        "information into output properties.\n";
      g_log.debug() << "Response from server: " << resp << '\n';
    } else {
      g_log.warning() << "Queried job status (Id " << jobID
                      << " ) but got what "
                         "looks like an error message as response: "
                      << resp << '\n';
    }
  } else {
    throw std::runtime_error("Failed to obtain job (Id:" + jobID +
                             " ) status "
                             "information through the web service at:" +
                             fullURL.toString() +
                             ". Please check your username, credentials, and "
                             "parameters.");
  }
  if (info.size() != 1)
    throw std::runtime_error(
        "There was an unexpected problem while retrieving status info for job "
        "with Id: " +
        jobID + " through the web service at:" + fullURL.toString() +
        ". Please check your username, credentials, and parameters");

  return info.back();
}

/**
 * LSF does not have a concept of transaction as described for example
 * in the Mantid Web Service API
 * (http://www.mantidproject.org/Remote_Job_Submission_API). There are
 * only jobs with their own ID, environment, user, etc.. So this
 * method just creates and returns one transaction ID. Subsequent
 * SubmitRemoteJob() calls will add the job IDs in this transaction.
 *
 * Often, you don't want to stop a transaction and kill all the jobs
 * that are running within it, specially if they take a long time. So
 * a typical use of transactions would be to use a single
 * startRemoteTransaction() after authenticating, and then interacts
 * with the remote compute resource within a single transaction,
 * without stopping it when the code using this job manager
 * finishes. Having more transactions when the job manager/scheduler
 * is LSF doesn't have any effect, as the environments are specific to
 * jobs and not transactions.
 *
 *
 * @return Transaction ID that becomes the current transaction (where
 * the next jobs will be included) and can be used in subsequent calls
 * to stopRemoteTransaction()
 *
 * @throw std::runtime_error if there is an issue creating the new
 * transaction.
 *
 * @throw std::runtime_error if there is another issue (like not logged in)
 */
std::string LSFJobManager::startRemoteTransaction() {
  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "Transaction start operation failed. You do not seem to have logged "
        "in.");
  }

  size_t idx = g_transactions.size();
  std::string tid = std::string("LSFTrans_") + std::to_string(idx + 1);

  auto ret = g_transactions.insert(
      std::pair<std::string, Transaction>(tid, LSFJobManager::Transaction()));

  // not inserted
  if (!ret.second)
    throw std::runtime_error("Could not create a new transaction with ID " +
                             tid);

  return tid;
}

/**
 * Stops a transaction (and kills/cancels all the jobs that were
 * started in this transaction). You don't need to stop all
 * transactions and you might probably not want to do it if you want
 * to leave remote jobs running after your local code finishes.
 *
 * @param transactionID that must have been produced by a call to
 * startRemoteTransaction()
 *
 * @throw std::invalid_argument if there is an issue with the transaction ID
 * provided
 *
 * @throw std::runtime_error if there is another issue (like not logged in)
 */
void LSFJobManager::stopRemoteTransaction(const std::string &transactionID) {
  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "Transaction stop operation failed. You do not seem to have logged "
        "in.");
  }

  auto it = g_transactions.find(transactionID);

  if (g_transactions.end() == it)
    throw std::invalid_argument("Could not find a transaction with ID: " +
                                transactionID);

  it->second.stopped = true;

  std::vector<std::string> jobs = it->second.jobIDs;
  for (auto &job : jobs) {
    abortRemoteJob(job);
  }
  g_transactions.erase(it);
}

/**
 * Submits a job to an LSF scheduler.
 *
 * @param transactionID that must have been produced by a call to
 * startRemoteTransaction()
 *
 * @param runnable Script (shell, python, etc) or executable to run
 *
 * @param param Command line parameters to the runnable
 *
 * @param taskName Name for the jobs, if empty a name will be assigned
 * automatically.
 *
 * @param numNodes Number of computing nodes. To use server defaults,
 * do not specify any number .
 *
 * @param coresPerNode Number of cores. To use server defaults, do not
 * specify any number .
 *
 * @throw std::invalid_argument if there is an issue with the transaction ID
 * provided
 */
std::string LSFJobManager::submitRemoteJob(const std::string &transactionID,
                                           const std::string &runnable,
                                           const std::string &param,
                                           const std::string &taskName,
                                           const int numNodes,
                                           const int coresPerNode) {
  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "Job submission failed. You do not seem to have logged in.");
  }
  if (!findTransaction(transactionID)) {
    throw std::invalid_argument("Could not find a transaction with ID: " +
                                transactionID);
  }
  // only support for single-user
  Token tok = g_tokenStash.begin()->second;

  // Job submit query, requires specific parameters for LSF submit
  // Example params passed to python submit utility:
  // $ pacclient.py submit --app TOMOPY_0_0_3 --param "INPUT_FILE=
  // /work/imat/webservice_test/tomopy/imat_recon_FBP.py;INPUT_ARGS=
  // /work/imat/scripts/test_;JOB_NAME=01_test_job;OUTPUT_FILE=%J.output;ERROR_FILE=
  // %J.error"
  const std::string appName = guessJobSubmissionAppName(runnable, param);
  // this gets executed (for example via 'exec' or 'python', depending on the
  // appName
  const std::string boundary = "bqJky99mlBWa-ZuqjC53mG6EzbmlxB";
  const std::string &body = buildSubmitBody(appName, boundary, runnable, param,
                                            taskName, numNodes, coresPerNode);
  // Job submit, needs these headers:
  // headers = {'Content-Type': 'multipart/mixed; boundary='+boundary,
  //                 'Accept': 'text/xml,application/xml;', 'Cookie': token,
  //                 'Content-Length': str(len(body))}
  // Content-Length is added by InternetHelper/Poco HTTP request
  const std::string token = tok.m_token_str;

  const Poco::URI fullURL = makeFullURI(tok.m_url, g_submitPath);
  const StringToStringMap headers =
      makeHeaders("multipart/mixed; boundary=" + boundary, token, g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers,
                                    Poco::Net::HTTPRequest::HTTP_POST, body);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error(
        "Error while sending HTTP request to submit a job: " +
        std::string(ie.what()));
  }
  std::string jobID;
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    std::string resp = ss.str();
    if (std::string::npos != resp.find("<errMsg>")) {
      g_log.warning()
          << "Submitted job but got a a response that seems to contain "
             "an error message : "
          << extractPACErrMsg(ss.str()) << '\n';
    } else {
      // get job id number
      const std::string idTag = "<id>";
      if (std::string::npos != resp.find(idTag)) {
        jobID = resp.substr(resp.rfind(idTag) + idTag.length());
        jobID = jobID.substr(0, jobID.find('<'));
      } else {
        // default if badly formed string returned / unable to parse ID from
        // response
        jobID = "0";
      }
      g_log.notice() << "Submitted job successfully. It got ID: " << jobID
                     << '\n';
      g_log.debug() << "Response from server after submission: " << resp
                    << '\n';
    }
  } else {
    throw std::runtime_error(
        "Failed to submit a job through the web service at:" +
        fullURL.toString() +
        ". Please check your username, credentials, and parameters.");
  }

  // In LSF the job ID must be an integer number
  try {
    int iid = boost::lexical_cast<int>(jobID);
    addJobInTransaction(jobID);
    g_log.debug() << "Submitted job, got ID: " << iid << '\n';
  } catch (std::exception &e) {
    g_log.warning()
        << "The job has been submitted but the job ID  returned does "
           "not seem well formed. Job ID string from server: '" +
               jobID +
               "'. Detailed error when tryint to interpret the code "
               "returned as an integer: " +
               e.what()
        << '\n';
  }

  return jobID;
}

/**
 * Upoads a file (for the most recently created job in the transaction given)
 *
 * @param transactionID that must have been produced by a call to
 * startRemoteTransaction()
 *
 * @param remoteFileName Name of file on the (remote) compute
 * resource. This can be a full or relative path or a simple file
 * name, depending on implementation.
 *
 * @param localFileName Path to the file to upload
 *
 * @throw std::invalid_argument if there is an issue with the transaction ID
 * provided
 */
void LSFJobManager::uploadRemoteFile(const std::string &transactionID,
                                     const std::string &remoteFileName,
                                     const std::string &localFileName) {
  if (g_tokenStash.empty()) {
    throw std::runtime_error(
        "File upload failed. You do not seem to have logged in.");
  }
  if (!findTransaction(transactionID)) {
    throw std::invalid_argument("Could not find a transaction with ID: " +
                                transactionID);
  }
  // only support for single-user
  Token tok = g_tokenStash.begin()->second;

  // File upload, needs these headers:
  // headers = {'Content-Type': 'multipart/mixed; boundary='+boundary,
  //                 'Accept': 'text/plain;', 'Cookie': token,
  //                 'Content-Length': str(len(body))}
  // Content-Length is added by InternetHelper/Poco HTTP request
  const std::string boundary = "4k89ogja023oh1-gkdfk903jf9wngmujfs95m";
  const std::string token = tok.m_token_str;

  const Poco::URI fullURL = makeFullURI(tok.m_url, g_uploadPath);
  const StringToStringMap headers =
      makeHeaders("multipart/mixed; boundary=" + boundary, token, g_acceptType);

  const std::string &body =
      buildUploadBody(boundary, remoteFileName, localFileName);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers,
                                    Poco::Net::HTTPRequest::HTTP_POST, body);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error(
        "Error while sending HTTP request to upload a file: " +
        std::string(ie.what()));
  }
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    std::string resp = ss.str();
    g_log.notice() << "Uploaded file, response from server: " << resp << '\n';
  } else {
    throw std::runtime_error(
        "Failed to upload file through the web service at:" +
        fullURL.toString() +
        ". Please check your username, credentials, "
        "and parameters.");
  }
}

/**
 * Define or guess the application name (AppName) for a job
 * submission. This is an LSF concept that is used for example to
 * define different application templates (or submission forms) when
 * using the web portal interface. In that interface, compute resource
 * admins can define application specific forms (job submission
 * templates). An application name normally comes with specific
 * (default) values for job options (like output file names, job
 * queue, processors/memory limits, etc.) and specific commands to set
 * environment variables (like additional library paths or python
 * packages paths, etc.). For example, there could be a 'Mantid' app
 * name that would be associated with the appropriate environment
 * variables, etc. on the remote compute resource.
 *
 * This generic implementation for any LSF compute resource / job
 * manager just returns the 'generic' application name which should
 * always be available but might well not be the most appropriate for
 * your particular applications. Depending on the server settings
 * you'll need to specify particular application names so that LSF
 * runs the appropriate environment configuration commands. For an
 * example of specialized definition/guess of different app names see
 * the SCARF LSF derived class.
 *
 * @param runnablePath path to the runnable (script, binary, etc.) that will
 * be run in a job submission - this can be used to guess what app name/
 * type is needed.
 *
 * @param jobOptions options submitted with the runnable (which can be
 * command line options, content of a script, etc.) - this info can also be
 * useful to define the AppName (to find out for example that a particular
 * tool version is required)
 *
 * @returns Application name (server specific, contact your
 * admin). This generic implementation ignores the inputs and always
 * returns the 'generic' app name which should be available in any LSF
 * system. This will go in the <AppName> parameter of the RESTful queries.
 */
std::string
LSFJobManager::guessJobSubmissionAppName(const std::string &runnablePath,
                                         const std::string &jobOptions) {
  UNUSED_ARG(runnablePath);
  UNUSED_ARG(jobOptions);

  return "generic";
}

/**
 * Send the HHTP(S) request required to perform one of the actions.
 *
 * @param url Full URL, including request string
 * @param rss Response body stream from the remote point
 * @param headers HTTP headers given as key-value pairs
 * @param method By default GET (Poco::Net::HTTPRequest::HTTP_POST), also
 * accepts POST (Poco::Net::HTTPRequest::HTTP_POST)
 *
 * @param body HTTP message body
 *
 * @return HTTP(S) response code
 */
int LSFJobManager::doSendRequestGetResponse(const Poco::URI &uri,
                                            std::ostream &rss,
                                            const StringToStringMap &headers,
                                            const std::string &method,
                                            const std::string &body) const {
  InternetHelper session;

  const std::string ContTypeName = "Content-Type";
  auto it = headers.find(ContTypeName);
  if (headers.end() != it) {
    session.setContentType(it->second);
  }
  session.headers() = headers;
  if (!method.empty())
    session.setMethod(method);
  if (!body.empty()) {
    session.setBody(body);
    // beware, the inet helper will set method=POST if body not empty!
    // But here, for example to download, we need a GET with non-empty body
    if (Poco::Net::HTTPRequest::HTTP_GET == method) {
      session.setMethod(method);
    }
  }

  return session.sendRequest(uri.toString(), rss);
}

/**
 * Fills in a table workspace with job status information from an LSC
 * PAC response in ~xml format. Assumes that the workspace passed is
 * empty and ready to be filled. This guarantees that a non-null (I)
 * table workspace object is returned.
 *
 * @param resp Body of an HHTP response to a status query
 * @param jobIDFilter ID of one job (empty string immplies all jobs)
 *
 * @return vector with status and related information for all jobs reported by
 * the server.
 */
std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo>
LSFJobManager::genOutputStatusInfo(const std::string &resp,
                                   const std::string &jobIDFilter) const {
  Poco::XML::DOMParser parser;
  Poco::AutoPtr<Poco::XML::Document> doc;
  try {
    doc = parser.parseString(resp);
  } catch (Poco::Exception &e) {
    throw std::runtime_error("Unable to parse response in XML format: " +
                             e.displayText());
  } catch (std::exception &e) {
    throw std::runtime_error("Unable to parse response in XML format: " +
                             std::string(e.what()));
  }

  Poco::XML::Element *pRootElem = doc->documentElement();
  if (!pRootElem || !pRootElem->hasChildNodes()) {
    g_log.error("XML response from compute resouce contains no root element.");
    throw std::runtime_error("No root element was found in XML response, "
                             "cannot parse it.");
  }

  Poco::AutoPtr<Poco::XML::NodeList> jobs =
      pRootElem->getElementsByTagName("Job");
  if (!jobs) {
    g_log.error("XML response from compute resouce contains no root element.");
    throw std::runtime_error("No root element was found in XML response, "
                             "cannot parse it.");
  }

  size_t n = jobs->length();
  if (0 == jobs->length()) {
    g_log.notice() << "Got information about 0 jobs. You may not have any jobs "
                      "currently running on the compute resource. The output "
                      "workspace will not "
                      "have any rows/information";
  }

  // This is the information that is usually available for running/recently
  // run jobs: id, name, status, command line
  std::vector<IRemoteJobManager::RemoteJobInfo> info;
  for (size_t i = 0; i < n; i++) {
    Poco::XML::Element *el = static_cast<Poco::XML::Element *>(
        jobs->item(static_cast<unsigned long>(i)));
    if (!el)
      throw std::runtime_error("Error while trying to parse job with index " +
                               std::to_string(i) +
                               "could not produce a complete table workspace.");

    info.push_back(RemoteJobInfo());

    Poco::XML::Element *id = el->getChildElement("id");
    if (id) {
      const std::string &IdStr = id->innerText();
      if (!jobIDFilter.empty() && IdStr != jobIDFilter)
        continue;

      info.back().id = IdStr;
    }

    Poco::XML::Element *name = el->getChildElement("name");
    if (name) {
      info.back().name = name->innerText();
    } else {
      info.back().name = "Unknown!";
    }

    Poco::XML::Element *status = el->getChildElement("status");
    if (status) {
      info.back().status = status->innerText();
    } else {
      info.back().status = "Unknown!";
    }

    // there is no safe simple way to extract the script/binary from command
    // lines with potentially several 'load module' and other set-environment
    // instructions. The whole command line is used below for 'Commandline'
    info.back().runnableName = "Not available";

    info.back().transactionID = "no ID";

    Poco::XML::Element *cmd = el->getChildElement("cmd");
    if (cmd) {
      info.back().cmdLine = cmd->innerText();
    } else {
      info.back().cmdLine = "Not available";
    }
  }

  return info;
}

/**
 * Adds one param to a submit request body (first argument). This is
 * part of a multipart body content.
 *
 * @param body Body string being built for an HTTP request
 * @param boundary Boundary string between parameters, for request encoding
 * @param paramName Name of a parameter, for example INPUT_FILE
 * @param paramVal Value of the parameter
 */
void LSFJobManager::encodeParam(std::string &body, const std::string &boundary,
                                const std::string &paramName,
                                const std::string &paramVal) {
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"" + paramName + "\"\r\n";
  body += "Content-Type: application/xml; charset=US-ASCII\r\n";
  body += "Content-Transfer-Encoding: 8bit\r\n";
  body += "\r\n";
  body += "<AppParam><id>" + paramName + "</id><value>" + paramVal +
          "</value><type></type></AppParam>\r\n";
}

/**
 * Tiny helper to generate an integer sequence number for the job
 * names.
 */
int seqNo() {
  static int s = 1;
  return s++;
}

/**
 * Helper method to do the somewhat ugly encoding of parameters for
 * submit requests.
 *
 * @param appName A registered app name/form form SCARF, example: TOMOPY_0_0_3
 * @param boundary Boundary string between parts of the multi-part body
 * @param inputFile Input file parameter, this file will be run
 * @param inputArgs Arguments to the command (application specific)
 *
 * @return A string ready to be used as body of a 'job submit' HTTP request
 */
std::string LSFJobManager::buildSubmitBody(
    const std::string &appName, const std::string &boundary,
    const std::string &inputFile, const std::string &inputArgs,
    const std::string &jobName, const int numNodes, const int coresPerNode) {
  // BLOCK: start and encode app name like this:
  // --bqJky99mlBWa-ZuqjC53mG6EzbmlxB
  // Content-Disposition: form-data; name="AppName"
  // Content-ID: <AppName>
  //
  // TOMOPY_0_0_3
  std::string body = "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"AppName\"\r\n"
          "Content-ID: <AppName>\r\n"
          "\r\n" +
          appName + "\r\n";

  // BLOCK: encode params head like this:
  // --bqJky99mlBWa-ZuqjC53mG6EzbmlxB
  // Content-Disposition: form-data; name="data"
  // Content-Type: multipart/mixed; boundary=_Part_1_701508.1145579811786
  // Content-ID: <data>
  //
  body += "--" + boundary + "\r\n";
  const std::string boundaryInner = "_Part_1_701508.1145579811786";
  body += "Content-Disposition: form-data; name=\"data\"\r\n";
  body += "Content-Type: multipart/mixed; boundary=" + boundaryInner + "\r\n";
  body += "Content-ID: <data>\r\n";
  body += "\r\n";

  // BLOCKS: encode params like this:
  {
    // BLOCK: encode INPUT_ARGS like this:
    // --_Part_1_701508.1145579811786
    // Content-Disposition: form-data; name="INPUT_ARGS"
    // Content-Type: application/xml; charset=US-ASCII
    // Content-Transfer-Encoding: 8bit
    // <AppParam><id>INPUT_ARGS</id><value>
    //    /work/imat/scripts/test_</value><type></type></AppParam>
    encodeParam(body, boundaryInner, "INPUT_ARGS", inputArgs);
  }
  {
    // BLOCK: encode OUTPUT_FILE like this:
    // --_Part_1_701508.1145579811786
    // Content-Disposition: form-data; name="OUTPUT_FILE"
    // Content-Type: application/xml; charset=US-ASCII
    // Content-Transfer-Encoding: 8bit
    // <AppParam><id>OUTPUT_FILE</id><value>%J.output</value>
    //    <type></type></AppParam>
    encodeParam(body, boundaryInner, "OUTPUT_FILE", "%J.output");
  }
  {
    // BLOCK: encode ERROR_FILE like this:
    // --_Part_1_701508.1145579811786
    // Content-Disposition: form-data; name="ERROR_FILE"
    // Content-Type: application/xml; charset=US-ASCII
    // Content-Transfer-Encoding: 8bit
    // <AppParam><id>ERROR_FILE</id><value>%J.error</value>
    //    <type></type></AppParam>
    encodeParam(body, boundaryInner, "ERROR_FILE", "%J.error");
  }
  {
    // BLOCK: encode JOB_NAME like this:
    // --_Part_1_701508.1145579811786
    // Content-Disposition: form-data; name="JOB_NAME"
    // Content-Type: application/xml; charset=US-ASCII
    // Content-Transfer-Encoding: 8bit
    // <AppParam><id>JOB_NAME</id><value>foo</value><type></type></AppParam>
    std::string name;
    if (jobName.empty()) {
      name = "Mantid_job_" + std::to_string(seqNo());
    } else {
      name = jobName;
    }
    encodeParam(body, boundaryInner, "JOB_NAME", name);
  }
  {
    // BLOCK: encode INPUT_FILE (this is what will be run,
    //        if appName=TOMOPY_0_0_3) like this:
    // --_Part_1_701508.1145579811786
    // Content-Disposition: form-data; name="INPUT_FILE"
    // Content-Type: application/xml; charset=US-ASCII
    // Content-Transfer-Encoding: 8bit
    // <AppParam><id>INPUT_FILE</id><value>
    //    /work/imat/webservice_test/tomopy/imat_recon_FBP.py</value>
    //    <type></type></AppParam>
    encodeParam(body, boundaryInner, "INPUT_FILE", inputFile);
  }
  {
    // BLOCK: encode MIN_NUM_CPU, MAX_NUM_CPU, and PROC_PRE_HOST like this:
    // --_Part_1_701508.1145579811786
    // Content-Disposition: form-data; name="JOB_NAME"
    // Content-Type: application/xml; charset=US-ASCII
    // Content-Transfer-Encoding: 8bit
    // <AppParam><id>MIN_NUM_CPU</id><value>1</value><type></type></AppParam>
    if (0 != numNodes) {
      encodeParam(body, boundaryInner, "MIN_NUM_CPU", "1");
      encodeParam(body, boundaryInner, "MAX_NUM_CPU", std::to_string(numNodes));
    }
    if (0 != coresPerNode) {
      encodeParam(body, boundaryInner, "PROC_PRE_HOST",
                  std::to_string(coresPerNode));
    }
  }

  // BLOCK: params end like this:
  // --_Part_1_701508.1145579811786--
  //
  body += "--" + boundaryInner + "--\r\n\r\n";

  // BLOCK: end like this:
  body += "--" + boundary + "--\r\n\r\n";

  return body;
}

/**
 * Helper method to encode the body of file upload requests.
 *
 * @param boundary Boundary string between parts of the multi-part body
 * @param destDir Path where to upload the file on the remote compute
 * resource/server
 * @param filename Name (path) of the local file to upload
 *
 * @return A string ready to be used as body of a 'file upload' HTTP request
 */
std::string LSFJobManager::buildUploadBody(const std::string &boundary,
                                           const std::string &destDir,
                                           const std::string &filename) {
  // build file name as given in the request body. Note that this is
  // not building a path on the local OS / filesystem, but for the
  // remote compute resource. Platform LSF implies a Unix filesystem.
  // discard up to last / (path)
  std::string upName = Poco::Path(filename).getFileName();

  // BLOCK: start and encode destination directory like this:
  // --4k89ogja023oh1-gkdfk903jf9wngmujfs95m
  // Content-Disposition: form-data; name="DirName"
  // Content-ID: <DirName>
  //
  // /work/imat/foo_test
  std::string body = "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"DirName\"\r\n"
          "Content-ID: <DirName>\r\n"
          "\r\n" +
          destDir + "\r\n";

  // BLOCK: encode file like this (could be repeated for multi-file uploads):
  // --4k89ogja023oh1-gkdfk903jf9wngmujfs95m
  // Content-Disposition: form-data; name="bar.txt"; filename=bar.txt
  // Content-Type: application/octet-stream
  // Content-ID: <bar.txt>
  //
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"" + upName + "\"\r\n";
  body += "Content-Type: application/octet-stream \r\n";
  body += "Content-Transfer-Encoding: UTF-8\r\n";
  body += "Content-ID: <" + upName + ">\r\n";
  body += "\r\n";

  // BLOCK: the file
  std::ifstream fileStream(filename.c_str(),
                           std::ios_base::binary | std::ios_base::in);
  Poco::StreamCopier::copyToString(fileStream, body);

  // BLOCK: end like this:
  body += "--" + boundary + "--" + "\r\n\r\n";

  return body;
}

/**
 * Helper to check if it's possible to write an output file and give
 * informative messages.
 *
 * @param localPath Destination directory
 * @param fname Name of the file being downloaded
 *
 * @return The full patch checked
 */
const std::string
LSFJobManager::checkDownloadOutputFile(const std::string &localPath,
                                       const std::string &fname) const {
  Poco::Path path = localPath;
  path.append(fname);
  Poco::File f(path);
  if (f.exists()) {
    std::string outName = path.toString();
    if (f.canWrite()) {
      g_log.notice() << "Overwriting output file: " << outName << '\n';
    } else {
      g_log.warning() << "It is not possible to write into the output file: "
                      << outName
                      << ", you may not have the required "
                         "permissions. Please check.\n";
    }
  }
  return f.path();
}

/**
 * Turns the esoteric name used in LSF PAC web service into a normal
 * filename (as a basename + extention, discarding the path to
 * it). For example, this method translates:
 * 'PAC Server* /home/isisg/scarf362/../scarf362/
 * Mantid_tomography_1_1423743450375PtlPj/417666.error*FILE*281*true'
 * into '417666.error'.
 *
 * @param PACName A file name specification as returned by PAC LSF
 * when downloading multiple files from jobs
 *
 * @return A filename ready to be used to save the file locally. Empty
 * string if fails.
 */
const std::string
LSFJobManager::filterPACFilename(const std::string &PACName) const {
  // discard up to last / (path)
  std::string name = PACName.substr(PACName.rfind('/') + 1);
  // remove trailing parameters
  size_t ast = name.find('*');
  if (std::string::npos != ast)
    name.replace(ast, std::string::npos, "");
  return name;
}

/**
 * Download a job file once we have obtained the remote path.
 *
 * @param jobId Identifier of a job as used by the job scheduler (integer
 * number)
 *
 * @param remotePath File name (of a job file on the compute resource)
 *
 * @param localPath Local path where to download the file (already checked)
 *
 * @param t Authentication token/cookie including url+string
 */
void LSFJobManager::getOneJobFile(const std::string &jobId,
                                  const std::string &remotePath,
                                  const std::string &localPath,
                                  const Token &t) {
  // Job download (one) file once we know the remote path, needs these headers:
  // headers = {'Content-Type': 'text/plain', 'Cookie': token, 'Accept':
  // ACCEPT_TYPE}
  // - and as request body the name of the file
  const std::string token = t.m_token_str;

  const Poco::URI fullURL = makeFullURI(t.m_url, g_downloadOneBasePath, jobId);
  const StringToStringMap headers =
      makeHeaders(std::string("application/xml"), token, g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(
        fullURL, ss, headers, Poco::Net::HTTPRequest::HTTP_GET, remotePath);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error(
        "Error while sending HTTP request to download a file: " +
        std::string(ie.what()));
  }
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    // this is what indicates success/failure: response content empty/not empty
    if (ss.rdbuf()->in_avail() > 0) {
      // check file is writeable and inform user
      // get basename from 'PAC' name
      std::string name = filterPACFilename(remotePath);
      if (name.empty()) {
        g_log.notice() << "Could not download remote file " << remotePath
                       << " into " << localPath
                       << ", a problem with its name was found\n";
      }
      std::string outName = checkDownloadOutputFile(localPath, name);
      std::ofstream file(outName.c_str(),
                         std::ios_base::binary | std::ios_base::out);
      Poco::StreamCopier::copyStream(ss, file);
      g_log.notice() << "Downloaded remote file " << outName << " into "
                     << localPath << ".\n";
      // do this only if you want to log the file contents!
      // g_log.debug() << "Response from server: " << ss.str() << '\n';
    } else {
      // log an error but potentially continue with other files
      g_log.error()
          << "Download failed. You may not have the required permissions "
             "or the file may not be available: "
          << remotePath << '\n';
    }
  } else {
    throw std::runtime_error(
        "Failed to download a file for job Id:" + jobId +
        " through the web service at:" + fullURL.toString() +
        ". Please "
        "check your existing jobs, username, and parameters.");
  }
}

/**
 * Download all files for a remote job.
 *
 * @param jobId Identifier of a job as used by the job scheduler (integer
 * number)
 *
 * @param localDir Local directory where to download the file (already
 * checked)
 *
 * @param t Authentication token/cookie including url+string
 */
void LSFJobManager::getAllJobFiles(const std::string &jobId,
                                   const std::string &localDir,
                                   const Token &t) {
  // Job download (multiple) files, needs these headers:
  // headers = {'Content-Type': 'text/plain', 'Cookie': token, 'Accept':
  // ACCEPT_TYPE}
  const std::string token = t.m_token_str;

  const Poco::URI fullURL =
      makeFullURI(t.m_url, g_downloadAllJobFilesBasePath, jobId);
  const StringToStringMap headers =
      makeHeaders(std::string("application/xml"), token, g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error(
        "Error while sending HTTP request to download files: " +
        std::string(ie.what()));
  }
  // what you get in this response is one line with text like this:
  // 'PAC Server*/home/isisg/scarf362/../scarf362/
  // Mantid_tomography_1_1423743450375PtlPj/417666.error*FILE*281*true;PAC
  // Server*/
  // home/isisg/scarf362/../scarf362/
  // Mantid_tomography_1_1423743450375PtlPj/417666.output*FILE*1145*true;'
  //   (the number between *FILE* and *true is the size in bytes)
  std::vector<std::string> filePACNames;
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    std::string resp = ss.str();
    // this is what indicates success/failure: presence of '/' or '\'
    if (std::string::npos != resp.find('/') ||
        std::string::npos != resp.find('\\')) {
      // you can get multiple files, as remote file names listed separated by
      // ';'
      std::string PACname;
      while (std::getline(ss, PACname, ';')) {
        filePACNames.push_back(PACname);
      }
      for (auto &filePACName : filePACNames) {
        getOneJobFile(jobId, filePACName, localDir, t);
      }
    }
  } else {
    throw std::runtime_error("Failed to download job files (Id:" + jobId +
                             " ) through "
                             "the web service at:" +
                             fullURL.toString() +
                             ". Please check your "
                             "existing jobs, username, and parameters.");
  }
}

/**
 * Gets the error message from a more or less xml response body. Sometimes these
 * error responses may read like this:
 * <?xml version="1.0" encoding="UTF-8" standalone="yes"?><Job>
 * <errMsg>Job &lt;417940&gt;: Job has already finished</errMsg><id>0</id></Job>
 *
 * @param response Body of an HHTP response that apparently contains some error
 * message
 *
 * @return Part of the response that seems to contain the specific error message
 */
std::string LSFJobManager::extractPACErrMsg(const std::string &response) const {
  // discard up to last errMsg start tag
  const std::string openTag = "<errMsg>";
  std::string msg = response.substr(response.rfind(openTag) + openTag.size());
  if (msg.empty())
    return response;

  // remove close tags
  size_t tags = msg.rfind("</errMsg>");
  msg.replace(tags, std::string::npos, "");

  // avoid/translate common entities
  boost::replace_all(msg, "&lt;", "<");
  boost::replace_all(msg, "&gt;", ">");

  return msg;
}

/**
 * Construct a url by appending a base url (which usually is obtained
 * when authenticating) and one or two additional path components.
 *
 * @param base base url (note this usually contains schema, authority
 * and a partial path)
 *
 * @param path path to append to the base url to form a
 * command/request specific url (to submit, cancel, etc.)
 *
 * @param additional component to append to the path. Its
 * interpretation is usually an action specific parameger, for
 * example: job ID of the job to cancel/kill.
 *
 * @return full URL ready to be used for a request to Platform LSF
 */
Poco::URI LSFJobManager::makeFullURI(const Poco::URI &base,
                                     const std::string &path,
                                     const std::string &pathParam) const {

  // this could be done with Poco::URI::mergePath which is protected
  if (pathParam.empty()) {
    return Poco::URI(base.toString() + path);
  } else {
    return Poco::URI(base.toString() + path + pathParam);
  }
}

/**
 * Helper to add frequent headers to the headers map before sending
 * HTTP requests
 *
 * @param contentType Type of content string, like "application/xml"
 * or "multipart/mixed; boundary=...", as a string, as it will go in
 * the HTTP header
 *
 * @param token Value for the "Cookie" header (authentication token),
 * as a string, as it will go in the HTTP header
 *
 * @param acceptType Accept-Type value as a string, as it will go in
 * the HTTP header
 *
 * @return headers an HTTP headers object (supposedly being
 * constructed before submitting a request). You might want to add
 * more haders after this.
 */
LSFJobManager::StringToStringMap
LSFJobManager::makeHeaders(const std::string &contentType,
                           const std::string &token,
                           const std::string &acceptType) const {

  StringToStringMap headers;
  if (!contentType.empty()) {
    headers.insert(
        std::pair<std::string, std::string>("Content-Type", contentType));
  }
  if (!token.empty()) {
    headers.emplace("Cookie", token);
  }
  if (!acceptType.empty()) {
    headers.emplace("Accept", acceptType);
  }

  return headers;
}

/**
 * Checks if a transaction is registered and has not been stopped.
 *
 * @param id transaction ID as produced by startRemoteTransaction()
 *
 * @return There is a transaction with the id given and it has not
 * being stopped.
 */
bool LSFJobManager::findTransaction(const std::string &id) const {
  auto it = g_transactions.find(id);

  return (it != g_transactions.end() && !it->second.stopped);
}

/**
 * Adss a job (identified by id) as part of a transaction
 *
 * @param id job ID as produced in submitRemobeJob()
 *
 */
void LSFJobManager::addJobInTransaction(const std::string &jobID) {
  if (g_transactions.empty())
    return;
  auto &jobs = g_transactions.rbegin()->second.jobIDs;
  auto it = std::find(jobs.begin(), jobs.end(), jobID);
  if (jobs.end() == it)
    jobs.push_back(jobID);
}

} // end namespace RemoteJobManagers
} // end namespace Mantid
