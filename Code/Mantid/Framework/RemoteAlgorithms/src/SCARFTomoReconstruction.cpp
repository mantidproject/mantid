#include "MantidRemoteAlgorithms/SCARFTomoReconstruction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/RemoteJobManager.h"
#include "MantidRemoteAlgorithms/SimpleJSON.h"

#include <Poco/Net/HTTPRequest.h>

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
  Mantid::API::Algorithm(),
  m_action(), m_jobID(), m_nxTomoPath(), m_parameterPath(), m_outputPath()
{ }

void SCARFTomoReconstruction::init() {
  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  std::vector<std::string> reconstOps;
  reconstOps.push_back("LogIn");
  reconstOps.push_back("LogOut");
  reconstOps.push_back("SubmitJob");
  reconstOps.push_back("JobStatus");
  reconstOps.push_back("CancelJob");
  auto listValue = boost::make_shared<StringListValidator>(reconstOps);

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

  // NXTomo File path on SCARF
  declareProperty(new PropertyWithValue<std::string>("RemoteNXTomoPath", "",
                                                     Direction::Input),
                  "The path on SCARF to the NXTomo file to reconstruct");

  // Job ID on SCARF
  declareProperty(
      new PropertyWithValue<std::string>("JobID", "", Direction::Input),
      "The ID for a currently running job on SCARF");

  // Path to parameter file for reconstruction
  declareProperty(new API::FileProperty("ParameterFilePath", "",
                                        API::FileProperty::OptionalLoad, exts,
                                        Direction::Input),
                  "Parameter file for the reconstruction job");
}

// gets action code in m_action, if input argument is valid
SCARFTomoReconstruction::Action::Type SCARFTomoReconstruction::getAction()
{
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
  } else if (par == "CancelJob") {
    act = Action::CANCEL;
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

  std::string username;
  try {
    username = getPropertyValue("UserName");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "To use this algorithm you need to give a valid username "
      "on the compute resource" + m_SCARFComputeResource << std::endl;
    throw;
  }

  if (Action::LOGIN == m_action) {
    std::string password;
    try {
      password = getPropertyValue("Password");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To log in using this algorithm you need to give a "
        "valid username and password on the compute resource " <<
        m_SCARFComputeResource << std::endl;
      throw;
    }
    doLogin(username, password);
  } else if (Action::LOGOUT == m_action) {
    doLogout(username);
  } else if (Action::SUBMIT == m_action) {
    doSubmit(username);
  } else if (Action::QUERYSTATUS == m_action) {
    doQueryStatus(username);
  } else if (Action::CANCEL == m_action) {
    doCancel(username);
  }
}

/**
 * Log into SCARF. If it goes well, it will produce a token that can
 * be reused for a while in subsequent queries. Internally it relies
 * on the InternetHelper to send an HTTP request and obtain the
 * response.
 *
 * @param username normally an STFC federal ID
 * @param passwork user password
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

  g_log.debug() << "Sending HTTP GET request to: " << SCARFLoginBaseURL +
    SCARFLoginPath << std::endl;
  InternetHelper session;
  std::string httpsURL = SCARFLoginBaseURL + SCARFLoginPath + "?username=" +
    username + "&password=" + password;

  std::stringstream ss;
  int respCode = session.sendRequest(httpsURL, ss);
  std::string resp = ss.str();
  g_log.debug() << "Got HTTP code " << respCode << ", response: " <<
    resp << std::endl;
  // We would check (Poco::Net::HTTPResponse::HTTP_OK == respCode) but the SCARF
  // login script (token.py) seems to return 200 whatever happens. So this is
  // the way to know if authentication succeeded:
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
  g_log.debug() << "Sending HTTP GET request to: " << httpsURL << std::endl;
  std::stringstream ss;
  InternetHelper::StringToStringMap headers;
  headers.insert(std::pair<std::string, std::string>("Content-Type",
                                                     "text/plain"));
  headers.insert(std::pair<std::string, std::string>("Cookie", token));
  headers.insert(std::pair<std::string, std::string>("Accept", m_acceptType));
  int code = session.sendRequest(httpsURL, ss, headers);
  std::string resp = ss.str();
  g_log.debug() << "Got HTTP code " << code << ", response: " <<  resp << std::endl;
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

  try {
    m_nxTomoPath = getPropertyValue("RemoteNXTomoPath");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify the remote path to the NXTomo file "
      "which is required to create a new reconstruction job. Please provide "
      "a valid path on " << m_SCARFComputeResource << std::endl;
    throw;
  }

  try {
    m_parameterPath = getPropertyValue("ParameterFilePath");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify a the path to the parameter file "
      "which is required to create a new reconstruction job. Please provide "
      "a valid tomography reconstruction parameter file" << std::endl;
    throw;
  }

  progress(0, "Starting tomographic reconstruction job...");

  // Job submit query, requires specific parameters for LSF submit
  // Example params passed to python submit utility:
  // $ pacclient.py submit --app TOMOPY_0_0_3 --param "INPUT_FILE=
  // /work/imat/webservice_test/tomopy/imat_recon_FBP.py;INPUT_ARGS=
  // /work/imat/scripts/test_;JOB_NAME=01_test_job;OUTPUT_FILE=%J.output;ERROR_FILE=
  // %J.error"
  const std::string appName = "TOMOPY_0_0_3";
  // this gets executed (for example via 'exec' or 'python', depending on the appName
  const std::string inputFiles = "/work/imat/webservice_test/tomopy/imat_recon_FBP.py";
  const std::string inputArgs = "/work/imat/webservice_test/remote_output/test_";
  const std::string boundary = "bqJky99mlBWa-ZuqjC53mG6EzbmlxB";
  const std::string &body = buildSubmitBody(appName, boundary, inputFiles, inputArgs);

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
  g_log.debug() << "Sending HTTP POST request to: " << httpsURL << std::endl;
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
  g_log.debug() << "Got HTTP code " << code << ", response: " <<  resp << std::endl;
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
  try {
    m_jobID = getPropertyValue("JobID");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify a JobID which is required "
      "to query its status." << std::endl;
    throw;
  }

  progress(0, "Starting tomographic reconstruction job...");

  // TODO: query about jobID and report

  progress(1.0, "Job created.");
}

/**
 * Cancel a submitted job, identified by its ID in the job queue.
 *
 * @param username Username to use (should have authenticated before)
 */
void SCARFTomoReconstruction::doCancel(const std::string &username) {
  try {
    m_jobID = getPropertyValue("JobID");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify a JobID which is required "
      "to cancel a job." << std::endl;
    throw;
  }

  progress(0, "Cancelling tomographic reconstruction job...");

  // TODO: query+cancel jobID, and report result
  // TODO: handle failure

  progress(1.0, "Job cancelled.");
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
    +appName + "\r\n";

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

} // end namespace RemoteAlgorithms
} // end namespace Mantid
