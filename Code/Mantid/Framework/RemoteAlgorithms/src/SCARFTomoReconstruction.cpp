#include "MantidRemoteAlgorithms/SCARFTomoReconstruction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/RemoteJobManager.h"
#include "MantidRemoteAlgorithms/SimpleJSON.h"

#include <fstream>

#include <Poco/File.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/StreamCopier.h>

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SCARFTomoReconstruction)

using namespace Mantid::Kernel;

std::map<std::string, SCARFTomoReconstruction::Token>
    SCARFTomoReconstruction::m_tokenStash;

std::string SCARFTomoReconstruction::m_acceptType =
    "text/plain,application/xml,text/xml";

const std::string SCARFTomoReconstruction::m_SCARFComputeResource = "SCARF@STFC";

SCARFTomoReconstruction::SCARFTomoReconstruction():
  Mantid::API::Algorithm(), m_action()
{ }

void SCARFTomoReconstruction::init() {
  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  // list of all actions
  std::vector<std::string> actions;
  actions.push_back("LogIn");
  actions.push_back("LogOut");
  actions.push_back("SubmitJob");
  actions.push_back("JobStatus");
  actions.push_back("JobStatusByID");
  actions.push_back("Ping");
  actions.push_back("CancelJob");
  actions.push_back("Upload");
  actions.push_back("Download");
  auto listValue = boost::make_shared<StringListValidator>(actions);

  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".*");

  // User
  declareProperty("UserName", "", requireValue,
                  "Name of the user to authenticate as", Direction::Input);

  // Password
  declareProperty(new MaskedProperty<std::string>("Password", "", requireValue,
                                                  Direction::Input),
                  "The password for the user");

  // Operation to perform : Update description as enum changes
  declareProperty("Action", "", listValue, "Choose the operation to perform "
                                              "on SCARF; "
                                              "[CreateJob,JobStatus,JobCancel]",
                  Direction::Input);

  // Runnable file when submitting a job
  declareProperty(new PropertyWithValue<std::string>("RunnablePath",
                      "/work/imat/webservice_test/tomopy/imat_recon_FBP.py",
                      Direction::Input),
                  "The path on SCARF of a file to run (example: shell or python "
                  "script)");

  // Path to parameter file for reconstruction
  declareProperty(new PropertyWithValue<std::string>("JobOptions",
                      "/work/imat/webservice_test/remote_output/test_",
                      Direction::Input),
                  "Options for the job command line, application dependent. It "
                  "can inclue for example the NXTomo input file when using savu "
                  "for tomographic reconstruction.");

  // Path for upload file (on the server/compute resource)
  declareProperty(new PropertyWithValue<std::string>("DestinationDirectory",
                                                     "/work/imat",
                                                     Direction::Input),
                  "Path where to upload the file on the compute resource/server");

  // Local (full path) file name to upload
  declareProperty(new API::FileProperty("FileToUpload", "",
                                        API::FileProperty::Load, "",
                                        Direction::Input),
                  "Name of the file (full path) to upload to the compute "
                  "resource/server ");

  // Job ID on SCARF
  declareProperty(
      new PropertyWithValue<std::string>("JobID", "", Direction::Input),
      "The ID for a currently running job on SCARF");

  // Name of a file from a job running on the compute resource, to download
  declareProperty(new PropertyWithValue<std::string>("RemoteJobFilename", "",
                                                     Direction::Input),
                  "Name of the job file to download");

  // Local path where to download files
  declareProperty(new API::FileProperty("LocalDirectory", "",
                                        API::FileProperty::Directory, "",
                                        Direction::Input),
                  "Local path where to download files from the compute "
                  "resource/server");
}

// gets action code in m_action, if input argument is valid
SCARFTomoReconstruction::Action::Type SCARFTomoReconstruction::getAction() {
  std::string par = getPropertyValue("Action");
  Action::Type act = Action::UNDEF;
  if (par == "LogIn") {
    act = Action::LOGIN;
  } else if (par == "LogOut") {
    act = Action::LOGOUT;
  } else if (par == "SubmitJob") {
    act = Action::SUBMIT;
  } else if (par == "JobStatus") {
    act = Action::QUERYSTATUS;
  } else if (par == "JobStatusByID") {
    act = Action::QUERYSTATUSBYID;
  } else if (par == "Ping") {
    act = Action::PING;
  } else if (par == "CancelJob") {
    act = Action::CANCEL;
  } else if (par == "Upload") {
    act = Action::UPLOAD;
  } else if (par == "Download") {
    act = Action::DOWNLOAD;
  } else {
    g_log.error() << "Unknown action specified: '" <<
      m_action << "', ignoring it.";
  }
  return act;
}

/**
 * Execute algorithm: check what action/command has to be run and call
 * specific methods.
 *
 * The implementation of the more specific methods is based on:
 * Mantid::Kernel::InternetHelper and Mantid::RemoteAlgorithms::SimpleJSON?
 */
void SCARFTomoReconstruction::exec() {

  m_action = getAction();

  g_log.information("Running SCARFTomoReconstruction");

  // only action that doesn't require any credentials
  if (Action::PING == m_action) {
    doPing();
    return;
  }

  // otherwise, check first username and then action-specific parameters
  std::string username;
  try {
    username = getPropertyValue("UserName");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "To use this algorithm to perform the requested action "
      "you need to give a valid username on the compute resource" +
      m_SCARFComputeResource << std::endl;
    throw;
  }
  // all actions that require at least a username
  if (Action::LOGIN == m_action) {
    std::string password;
    try {
      password = getPropertyValue("Password");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To log in using this algorithm you need to give a "
        "valid username and password on the compute resource " <<
        m_SCARFComputeResource << "." << std::endl;
      throw;
    }
    doLogin(username, password);
  } else if (Action::LOGOUT == m_action) {
    doLogout(username);
  } else if (Action::SUBMIT == m_action) {
    doSubmit(username);
  } else if (Action::QUERYSTATUS == m_action) {
    doQueryStatus(username);
  } else if (Action::QUERYSTATUSBYID == m_action) {
    std::string jobId;
    try {
      jobId = getPropertyValue("JobID");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To query the detailed status of a job by its ID you "
        "need to give the ID of a job running on " <<
        m_SCARFComputeResource << "." << std::endl;
      throw;
    }
    doQueryStatusById(username, jobId);
  } else if (Action::CANCEL == m_action) {
    std::string jobId;
    try {
      jobId = getPropertyValue("JobID");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To cancel a job you need to give the ID of a job "
        "running on " << m_SCARFComputeResource << "." << std::endl;
      throw;
    }
    doCancel(username, jobId);
  } else if (Action::UPLOAD == m_action) {
    std::string filename, destDir;
    try {
      filename = getPropertyValue("FileToUpload");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To upload a file you need to provide an existing "
        "local file." << std::endl;
      throw;
    }
    try {
      destDir = getPropertyValue("DestinationDirectory");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To upload a file you need to provide a destination "
        "directory on " << m_SCARFComputeResource << "." << std::endl;
      throw;
    }
    doUploadFile(username, destDir, filename);
  } else if (Action::DOWNLOAD == m_action) {
    std::string jobId, fname, localDir;
    try {
      jobId = getPropertyValue("JobID");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To download a file you need to give the ID of a job "
        "running on " << m_SCARFComputeResource << "." << std::endl;
      throw;
    }
    try {
      fname = getPropertyValue("RemoteJobFilename");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To download a file you need to provide the name of a "
        "file from the remote job." << std::endl;
      throw;
    }
    try {
      localDir = getPropertyValue("LocalDirectory");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To download a file you need to provide a destination "
        "(local) directory." << std::endl;
      throw;
    }
    doDownload(username, jobId, fname, localDir);
  }
}

/**
 * Log into SCARF. If it goes well, it will produce a token that can
 * be reused for a while in subsequent queries. Internally it relies
 * on the InternetHelper to send an HTTP request and obtain the
 * response.
 *
 * @param username normally an STFC federal ID
 * @param password user password
 */
void SCARFTomoReconstruction::doLogin(const std::string &username,
                                      const std::string &password) {
  // log into "https://portal.scarf.rl.ac.uk/cgi-bin/token.py";

  // this should go away and obtained from 'computeResourceInfo' (like
  // a very simple InstrumentInfo) or similar. What we need here is
  // computeResourceInfo::baseURL()
  const std::string SCARFLoginBaseURL = "https://portal.scarf.rl.ac.uk/";
  const std::string SCARFLoginPath = "/cgi-bin/token.py";

  std::vector<std::string> res = ConfigService::Instance().getFacility().
    computeResources();
  auto it = std::find(res.begin(), res.end(), m_SCARFComputeResource);
  if (res.end() == it)
    throw std::runtime_error(std::string("Failed to find a compute resource "
                               "for " +  m_SCARFComputeResource + " (facility: " +
                               ConfigService::Instance().getFacility().name() +
                               ")."));

  InternetHelper session;
  std::string httpsURL = SCARFLoginBaseURL + SCARFLoginPath + "?username=" +
    username + "&password=" + password;

  std::stringstream ss;
  int respCode = session.sendRequest(httpsURL, ss);
  std::string resp = ss.str();
  // We would check (Poco::Net::HTTPResponse::HTTP_OK == respCode) but the SCARF
  // login script (token.py) seems to return 200 whatever happens, as far as the
  // request is well formed. So this is how to know if authentication succeeded:
  const std::string expectedSubstr = "https://portal.scarf.rl.ac.uk";
  if (resp.find(expectedSubstr) != std::string::npos) {
    // it went fine, stash cookie/token which looks like this (2 lines):
    // https://portal.scarf.rl.ac.uk:8443/platform/
    // scarf362"2015-02-10T18:50:00Z"Mv2ncX8Z0TpH0lZHxMyXNVCb7ucT6jHNOx...
    std::string url, token_str;
    std::getline(ss, url);
    std::getline(ss, token_str);
    // note that the token needs a substring replace and a prefix, like this:
    boost::replace_all(token_str, "\"", "#quote#");
    token_str = "platform_token=" + token_str;
    // insert in the token stash
    UsernameToken tok(username, Token(url, token_str));
    m_tokenStash.insert(tok); // the password is never stored
    g_log.notice() << "Got authentication token. You are now logged into "
                   << m_SCARFComputeResource << std::endl;
  } else {
    throw std::runtime_error("Login failed. Please check your username and "
                             "password.");
  }
}

/**
 * Log out from SCARF. In practice, it trashes the cookie (if we were
 * successfully logged in).
 *
 * @param username Username to use (should have authenticated before)
 */
void SCARFTomoReconstruction::doLogout(const std::string &username) {
  auto it = m_tokenStash.find(username);
  if (m_tokenStash.end() == it) {
    throw std::runtime_error("Logout failed. You do not seem to be logged in. "
                             "I do not remember this username. Please check your "
                             "username.");
  }

  // logout query, needs headers = {'Content-Type': 'text/plain', 'Cookie': token,
  //    'Accept': 'text/plain,application/xml,text/xml'}
  const std::string logoutPath = "webservice/pacclient/logout/";
  const std::string baseURL = it->second.m_url;
  const std::string token = it->second.m_token_str;

  InternetHelper session;
  std::string httpsURL = baseURL + logoutPath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "text/plain"));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  int code = session.sendRequest(httpsURL, ss, headers);
  std::string resp = ss.str();
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    g_log.notice() << "Logged out with response: " << resp << std::endl;
  } else {
    throw std::runtime_error("Failed to logout from the web service at: " +
                             httpsURL + ". Please check your username.");
  }
}

/**
 * Submits a job to SCARF. The different ways jobs could be submitted
 * (supported toolkits, LSF PAC submission forms, launcher scripts,
 * supported options, etc.) are not well defined at the moment.
 *
 * @param username Username to use (should have authenticated before)
 */
void SCARFTomoReconstruction::doSubmit(const std::string &username) {
  auto it = m_tokenStash.find(username);
  if (m_tokenStash.end() == it) {
    throw std::runtime_error("Job submission failed. You do not seem to be logged "
                             "in. I do not remember this username. Please check "
                             "your username.");
  }

  // Not sure at this point if there could be commands without options
  // For the time being it's possible.
  std::string jobOptions = "";
  try {
    jobOptions = getPropertyValue("JobOptions");
  } catch(std::runtime_error& /*e*/) {
    g_log.warning() << "You did not specify any options for the job. Maybe you "
      "forgot to pass the options?" << std::endl;
    m_jobOptions = "";
  }

  std::string runnablePath = "";
  try {
    getPropertyValue("RunnablePath");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify a the path to the parameter file "
      "which is required to create a new reconstruction job. Please provide "
      "a valid tomography reconstruction parameter file" << std::endl;
    throw;
  }

  progress(0, "Starting job...");

  // Job submit query, requires specific parameters for LSF submit
  // Example params passed to python submit utility:
  // $ pacclient.py submit --app TOMOPY_0_0_3 --param "INPUT_FILE=
  // /work/imat/webservice_test/tomopy/imat_recon_FBP.py;INPUT_ARGS=
  // /work/imat/scripts/test_;JOB_NAME=01_test_job;OUTPUT_FILE=%J.output;ERROR_FILE=
  // %J.error"
  const std::string appName = "TOMOPY_0_0_3";
  // this gets executed (for example via 'exec' or 'python', depending on the appName
  const std::string boundary = "bqJky99mlBWa-ZuqjC53mG6EzbmlxB";
  const std::string &body = buildSubmitBody(appName, boundary,
                                            runnablePath, jobOptions);

  // Job submit, needs these headers:
  // headers = {'Content-Type': 'multipart/mixed; boundary='+boundary,
  //                 'Accept': 'text/xml,application/xml;', 'Cookie': token,
  //                 'Content-Length': str(len(body))}
  // Content-Length is added by InternetHelper/Poco HTTP request
  const std::string submitPath = "webservice/pacclient/submitapp";
  const std::string baseURL = it->second.m_url;
  const std::string token = it->second.m_token_str;

  InternetHelper session;
  std::string httpsURL = baseURL + submitPath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "multipart/mixed; boundary=" +
                                                     boundary));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));
  int code = session.sendRequest(httpsURL, ss, headers,
                                 Poco::Net::HTTPRequest::HTTP_POST, body);
  std::string resp = ss.str();
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    // TODO: still need to parse response string contents, look for either 'ok' or
    //      '<errMsg>'
    g_log.notice() << "Submitted job with response: " << resp << std::endl;
  } else {
    throw std::runtime_error("Failed to submit a job through the web service at:" +
                             httpsURL + ". Please check your username, credentials, "
                             "and parameters.");
  }

  progress(1.0, "Job started on " + m_SCARFComputeResource);
}

/**
 * Query the status of jobs running (if successful will return info on
 * jobs running for our user)
 *
 * @param username Username to use (should have authenticated before)
 */
void SCARFTomoReconstruction::doQueryStatus(const std::string &username) {
  auto it = m_tokenStash.find(username);
  if (m_tokenStash.end() == it) {
    throw std::runtime_error("Job status query failed. You do not seem to be logged "
                             "in. I do not remember this username. Please check "
                             "your username.");
  }

  progress(0, "Checking the status of jobs...");

  // Job query status, needs these headers:
  // headers = {'Content-Type': 'application/xml', 'Cookie': token,
  //            'Accept': ACCEPT_TYPE}
  const std::string jobStatusPath = "webservice/pacclient/jobs?";
  const std::string baseURL = it->second.m_url;
  const std::string token = it->second.m_token_str;

  InternetHelper session;
  std::string httpsURL = baseURL + jobStatusPath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "application/xml"));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));
  int code = session.sendRequest(httpsURL, ss, headers);
  std::string resp = ss.str();
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    // TODO: still need to parse response string contents, look for a certain pattern
    // in the response body. Maybe put it into an output TableWorkspace
    g_log.notice() << "Queried job status with response: " << resp << std::endl;
  } else {
    throw std::runtime_error("Failed to obtain job status information through the "
                             "web service at:" + httpsURL + ". Please check your "
                             "username, credentials, and parameters.");
  }

  progress(1.0, "Status of jobs retrived.");
}

/**
 * Query the status of jobs running (if successful will return info on
 * jobs running for our user)
 *
 * @param username Username to use (should have authenticated before)
 * @param jobId Identifier of a job as used by the job scheduler (integer number)
 */
void SCARFTomoReconstruction::doQueryStatusById(const std::string& username,
                                                const std::string& jobId) {
  auto it = m_tokenStash.find(username);
  if (m_tokenStash.end() == it) {
    throw std::runtime_error("Job status query failed. You do not seem to be logged "
                             "in. I do not remember this username. Please check "
                             "your username.");
  }

  progress(0, "Checking the status of job with Id " + jobId);

  // Job query status, needs these headers:
  // headers = {'Content-Type': 'application/xml', 'Cookie': token,
  //            'Accept': ACCEPT_TYPE}
  const std::string jobIdStatusPath = "webservice/pacclient/jobs/";
  const std::string baseURL = it->second.m_url;
  const std::string token = it->second.m_token_str;

  InternetHelper session;
  std::string httpsURL = baseURL + jobIdStatusPath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "application/xml"));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));
  int code = session.sendRequest(httpsURL, ss, headers);
  std::string resp = ss.str();
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    // TODO: still need to parse response string contents, look for a certain pattern
    // in the response body. Maybe put it into an output TableWorkspace
    g_log.notice() << "Queried job status (Id" << jobId << " ) with response: " <<
      resp << std::endl;
  } else {
    throw std::runtime_error("Failed to obtain job (Id:" + jobId +" ) status "
                             "information through the web service at:" + httpsURL +
                             ". Please check your username, credentials, and "
                             "parameters.");
  }

  progress(1.0, "Status of job " + jobId + "retrived.");
}

/**
 * Ping the server to see if the web service is active/available.
 *
 * @return true if the web service responds.
 */
bool SCARFTomoReconstruction::doPing() {
  progress(0, "Pinging compute resource " + m_SCARFComputeResource);

  // Job ping, needs these headers:
  // headers = {'Content-Type': 'application/xml', 'Accept': ACCEPT_TYPE}
  const std::string pingPath = "platform/webservice/pacclient/ping/";
  // TODO: this should be retrieved from facilities or similar
  // (like SCARFLoginBaseURL above)
  // the port number is known only after logging in
  const std::string baseURL = "https://portal.scarf.rl.ac.uk:8443/";

  InternetHelper session;
  std::string httpsURL = baseURL + pingPath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "application/xml"));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  int code = session.sendRequest(httpsURL, ss, headers);
  std::string resp = ss.str();
  bool ok = false;
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    // TODO: still need to parse response string contents, look for a certain pattern
    // in the response body. Maybe put it into an output TableWorkspace
    g_log.notice() << "Pinged compute resource with response: " <<
      resp << std::endl;
    ok = true;
  } else {
    throw std::runtime_error("Failed to ping the web service at:" + httpsURL +
                             ". Please check your parameters, software version, "
                             "etc.");
  }

  progress(1.0, "Ping compute resource " + m_SCARFComputeResource +  " done.");

  return ok;
}

/**
 * Cancel a submitted job, identified by its ID in the job queue.
 *
 * @param username Username to use (should have authenticated before)
 * @param jobId Identifier of a job as used by the job scheduler (integer number)
 */
void SCARFTomoReconstruction::doCancel(const std::string &username,
                                       const std::string &jobId) {
  auto it = m_tokenStash.find(username);
  if (m_tokenStash.end() == it) {
    throw std::runtime_error("Job status query failed. You do not seem to be logged "
                             "in. I do not remember this username. Please check "
                             "your username.");
  }

  progress(0, "Cancelling/killing job " + jobId);

  // Job kill, needs these headers:
  // headers = {'Content-Type': 'text/plain', 'Cookie': token, 'Accept': ACCEPT_TYPE}
  const std::string killPath = "webservice/pacclient/jobOperation/kill" + jobId;
  const std::string baseURL = it->second.m_url;
  const std::string token = it->second.m_token_str;

  InternetHelper session;
  std::string httpsURL = baseURL + killPath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "application/xml"));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  int code = session.sendRequest(httpsURL, ss, headers);
  std::string resp = ss.str();
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    // TODO: still need to parse response string contents, look for a certain pattern
    // in the response body. Maybe put it into an output TableWorkspace
    g_log.notice() << "Killed job with Id" << jobId << " with response: " <<
      resp << std::endl;
  } else {
    throw std::runtime_error("Failed to kill job (Id:" + jobId +" ) through the web "
                             "service at:" + httpsURL + ". Please check your "
                             "existing jobs, username, and parameters.");
  }

  progress(1.0, "Killed job with Id " + jobId + ".");
}

/**
 * Upload a file to a directory on the server.
 *
 * @param username Username to use (should have authenticated before)
 * @param destDir Destination directory on the server
 * @param filename File name of the local file to upload
 */
void SCARFTomoReconstruction::doUploadFile(const std::string &username,
                                           const std::string &destDir,
                                           const std::string &filename) {
  auto it = m_tokenStash.find(username);
  if (m_tokenStash.end() == it) {
    throw std::runtime_error("File upload failed. You do not seem to be logged "
                             "in. I do not remember this username. Please check "
                             "your username.");
  }

  progress(0, "Uploading file: " + filename);

  // File upload, needs these headers:
  // headers = {'Content-Type': 'multipart/mixed; boundary='+boundary,
  //                 'Accept': 'text/plain;', 'Cookie': token,
  //                 'Content-Length': str(len(body))}
  // Content-Length is added by InternetHelper/Poco HTTP request
  //  The 0 at the end of the upload path is 'jobId' 0, if a jobId is given the
  //  upload goes to a path relative to the job path.
  const std::string uploadPath = "webservice/pacclient/upfile/0";
  const std::string boundary = "4k89ogja023oh1-gkdfk903jf9wngmujfs95m";
  const std::string baseURL = it->second.m_url;
  const std::string token = it->second.m_token_str;

  InternetHelper session;
  std::string httpsURL = baseURL + uploadPath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "multipart/mixed; boundary=" +
                                                     boundary));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));

  const std::string &body = buildUploadBody(boundary, destDir, filename);
  int code = session.sendRequest(httpsURL, ss, headers,
                                 Poco::Net::HTTPRequest::HTTP_POST, body);
  std::string resp = ss.str();
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    g_log.notice() << "Uploaded file with response: " << resp << std::endl;
  } else {
    throw std::runtime_error("Failed to upload file through the web service at:" +
                             httpsURL + ". Please check your username, credentials, "
                             "and parameters.");
  }

  progress(1.0, "File uploaded to " + m_SCARFComputeResource);
}

/**
 * Helper method to encode the body of file upload requests.
 *
 * @param boundary Boundary string between parts of the multi-part body
 * @param destDir Path where to upload the file on the remote compute resource/server
 * @param filename Name (path) of the local file to upload
 *
 * @return A string ready to be used as body of a 'file upload' HTTP request
 */
std::string SCARFTomoReconstruction::buildUploadBody(const std::string &boundary,
                                                     const std::string &destDir,
                                                     const std::string &filename) {
  // build file name as given in the request body
  std::string upName = filename;
  std::replace(upName.begin(), upName.end(), '\\', '/');
  // discard up to last / (path)
  upName = upName.substr(upName.rfind("/") + 1);

  // BLOCK: start and encode destination directory like this:
  // --4k89ogja023oh1-gkdfk903jf9wngmujfs95m
  // Content-Disposition: form-data; name="DirName"
  // Content-ID: <DirName>
  //
  // /work/imat/foo_test
  std::string body = "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"DirName\"\r\n"
    "Content-ID: <DirName>\r\n"
    "\r\n"
    + destDir + "\r\n";

  // BLOCK: encode file like this (could be repeated for multi-file uploads):
  // --4k89ogja023oh1-gkdfk903jf9wngmujfs95m
  // Content-Disposition: form-data; name="bar.txt"; filename=bar.txt
  // Content-Type: application/octet-stream
  // Content-ID: <bar.txt>
  //
  body += "--" + boundary + "\r\n";
  const std::string boundaryInner = "_Part_1_701508.1145579811786";
  body += "Content-Disposition: form-data; name=\"" + upName  +"\"\r\n";
  body += "Content-Type: application/octet-stream \r\n";
  body += "Content-Transfer-Encoding: UTF-8\r\n";
  body += "Content-ID: <" + upName + ">\r\n";
  body += "\r\n";

  // BLOCK: the file
  std::ifstream fileStream(filename, std::ios_base::binary);
  Poco::StreamCopier::copyToString(fileStream, body);

  // BLOCK: end like this:
  body += "--" + boundary + "--" + "\r\n\r\n";

  return body;
}

/**
 * Download a file or a set of files from a remote job into a local
 * directory. Note that this download as supported by LSF at SCARF is
 * job-specific: you download a file from a job and not a file in the
 * file system in general. When downloading multiple files this action
 * requires two steps: one first HTTP request to get the remote
 * path(s) for all the job file(s), and a second request or series of
 * requests to actually download the file(s).
 *
 * @param username Username to use (should have authenticated before)
 * @param jobId Identifier of a job as used by the job scheduler (integer number)
 * @param fname File name (of a job file on the compute resource). If no name is
 * given then all the job files are downloaded into localDir
 * @param localDir Local directory where to download the file(s)
 */
void SCARFTomoReconstruction::doDownload(const std::string &username,
                                         const std::string &jobId,
                                         const std::string &fname,
                                         const std::string &localDir) {
  auto it = m_tokenStash.find(username);
  if (m_tokenStash.end() == it) {
    throw std::runtime_error("File upload failed. You do not seem to be logged "
                             "in. I do not remember this username. Please check "
                             "your username.");
  }

  progress(0, "Downloading file: " + fname + " in " + localDir);

  if (fname.empty()) {
    // no name implies we want all the files of a remote job
    getAllJobFiles(jobId, localDir, it->second);
  } else {
    // name given, so we directly download this single file
    getOneJobFile(jobId, fname, localDir, it->second);
  }
}

/**
 * Adds one param to a submit request body (first argument). This is
 * part of a multipart body content.
 *
 * @param body Body string being built for an HTTP request
 * @param boundary Boundary string between parameters
 * @param paramName Name of a parameter, for example INPUT_FILE
 * @param paramVal Value of the parameter
 */
void SCARFTomoReconstruction::encodeParam(std::string &body,
                                          const std::string &boundary,
                                          const std::string &paramName,
                                          const std::string &paramVal) {
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"" + paramName + "\"\r\n";
  body += "Content-Type: application/xml; charset=US-ASCII\r\n";
  body += "Content-Transfer-Encoding: 8bit\r\n";
  body += "\r\n";
  body += "<AppParam><id>" +paramName+ "</id><value>" +
    paramVal + "</value><type></type></AppParam>\r\n";
}

/**
 * Tiny helper to generate an integer sequence number
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
std::string SCARFTomoReconstruction::buildSubmitBody(const std::string &appName,
                                                     const std::string &boundary,
                                                     const std::string &inputFile,
                                                     const std::string &inputArgs) {
  // BLOCK: start and encode app name like this:
  // --bqJky99mlBWa-ZuqjC53mG6EzbmlxB
  // Content-Disposition: form-data; name="AppName"
  // Content-ID: <AppName>
  //
  // TOMOPY_0_0_3
  std::string body = "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"AppName\"\r\n"
    "Content-ID: <AppName>\r\n"
    "\r\n"
    + appName + "\r\n";

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
    encodeParam(body, boundaryInner, "JOB_NAME", "Mantid_tomography_" +
                boost::lexical_cast<std::string>(seqNo()));
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
  // BLOCK: params end like this:
  // --_Part_1_701508.1145579811786--
  //
  body += "--" + boundaryInner + "--\r\n\r\n";

  // BLOCK: end like this:
  body += "--" + boundary + "--\r\n\r\n";

  return body;
}

/**
 * Helper to check if it's possible to write an output file and give
 * informative messages.
 *
 * @param localPath Destination directory
 * @param filename Name of the file being downloaded
 */
const std::string
SCARFTomoReconstruction::checkDownloadOutputFile(const std::string &localPath,
                                                 const std::string &fname) {
  std::string outName = localPath + "/" + fname;
  Poco::File f(outName);
  if (f.exists()) {
    if (f.canWrite()) {
      g_log.notice() << "Overwriting output file: " << outName << std::endl;
    } else {
      g_log.warning() << "It is not possible to write into the output file: "
                      << outName << ", you may not have the required "
        "permissions. Please check." << std::endl;
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
SCARFTomoReconstruction::filterPACFilename(const std::string PACName) {
  // discard up to last / (path)
  std::string name = PACName.substr(PACName.rfind("/") + 1);
  // remove trailing parameters
  size_t ast = name.find("*");
  name.replace(ast, std::string::npos, "");
  return name;
}

/**
 * Download a job file once we have obtained the remote path.
 *
 * @param jobId Identifier of a job as used by the job scheduler (integer number)
 * @param remotePath File name (of a job file on the compute resource)
 * @param localPath Local path where to download the file (already checked)
 * @param t Authentication token/cookie including url+string
 */
void SCARFTomoReconstruction::getOneJobFile(const std::string &jobId,
                                            const std::string &remotePath,
                                            const std::string &localPath,
                                            const Token &t)
{
  // Job download (one) file once we know the remote path, needs these headers:
  // headers = {'Content-Type': 'text/plain', 'Cookie': token, 'Accept': ACCEPT_TYPE}
  // - and as request body the name of the file
  const std::string downloadOnePath = "webservice/pacclient/file/" + jobId;
  const std::string baseURL = t.m_url;
  const std::string token = t.m_token_str;

  InternetHelper session;
  std::string httpsURL = baseURL + downloadOnePath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "application/xml"));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  std::string body = remotePath;
  int code = session.sendRequest(httpsURL, ss, headers,
                                 Poco::Net::HTTPRequest::HTTP_GET, body);
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    // this is what indicates success/failure: response content empty/not empty
    if (ss.rdbuf()->in_avail() > 0) {
      // check file is writeable and inform user
      // get basename from 'PAC' name
      std::string name = filterPACFilename(remotePath);
      if (name.empty()) {
        g_log.notice() << "Could not download remote file " << remotePath <<
          " into " << localPath << ", a problem with its name was found" <<
          std::endl;
      }
      std::string outName = checkDownloadOutputFile(localPath, name);
      std::ofstream file(outName, std::ios_base::binary);
      Poco::StreamCopier::copyStream(ss, file);
      g_log.notice() << "Downloaded remote file " << outName << " into " <<
        localPath << std::endl;
    } else {
      // log an error but potentially continue with other files
      g_log.error() << "Download failed. You may not have the required permissions "
        "or the file may not be available on " << m_SCARFComputeResource << ": " <<
        remotePath << std::endl;
    }
  } else {
    throw std::runtime_error("Failed to download a file for job Id:" + jobId +
                             " through the web service at:" + httpsURL + ". Please "
                             "check your existing jobs, username, and parameters.");
  }
}

/**
 * Download all files for a remote job.
 *
 * @param jobId Identifier of a job as used by the job scheduler (integer number)
 * @param localDir Local directory where to download the file (already checked)
 * @param t Authentication token/cookie including url+string
 */
void SCARFTomoReconstruction::getAllJobFiles(const std::string &jobId,
                                             const std::string &localDir,
                                             const Token &t)
{
  // Job download (multiple) files, needs these headers:
  // headers = {'Content-Type': 'text/plain', 'Cookie': token, 'Accept': ACCEPT_TYPE}
  const std::string downloadPath = "webservice/pacclient/jobfiles/" + jobId;
  const std::string baseURL = t.m_url;
  const std::string token = t.m_token_str;

  InternetHelper session;
  std::string httpsURL = baseURL + downloadPath;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "application/xml"));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  int code = session.sendRequest(httpsURL, ss, headers);
  std::string resp = ss.str();
  // what you get in this response is one line with text like this:
  // 'PAC Server*/home/isisg/scarf362/../scarf362/
  // Mantid_tomography_1_1423743450375PtlPj/417666.error*FILE*281*true;PAC Server*/
  // home/isisg/scarf362/../scarf362/
  // Mantid_tomography_1_1423743450375PtlPj/417666.output*FILE*1145*true;'
  //   (the number between *FILE* and *true is the size in bytes)
  std::vector<std::string> filePACNames;
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    // this is what indicates success/failure: presence of '/' or '\'
    if (std::string::npos != resp.find('/') ||
        std::string::npos != resp.find('\\')) {
      // you can get multiple files, as remote file names listed separated by ';'
      std::string PACname;
      while (std::getline(ss, PACname, ';')) {
        filePACNames.push_back(PACname);
      }
      for (size_t i=0; i<filePACNames.size(); i++) {
        getOneJobFile(jobId, filePACNames[i], localDir, t);
      }
    }
  } else {
    throw std::runtime_error("Failed to download job files (Id:" + jobId +" ) through "
                             "the web service at:" + httpsURL + ". Please check your "
                             "existing jobs, username, and parameters.");
  }

  progress(1.0, "Download  of " + boost::lexical_cast<std::string>(filePACNames.size())
           + " file(s) completed in " + localDir);
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
