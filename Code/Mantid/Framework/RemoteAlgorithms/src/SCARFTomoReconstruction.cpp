#include "MantidRemoteAlgorithms/SCARFTomoReconstruction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/RemoteJobManager.h"
#include "MantidRemoteAlgorithms/SimpleJSON.h"

#include "MantidKernel/InternetHelper.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SCARFTomoReconstruction)

using namespace Mantid::Kernel;

std::map<std::string, SCARFTomoReconstruction::Token>
    SCARFTomoReconstruction::m_tokenStash;

std::string SCARFTomoReconstruction::m_acceptType =
    "text/plain,application/xml,text/xml";

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

  if (Action::LOGIN == m_action) {
    std::string username, password;
    try {
      username = getPropertyValue("UserName");
      password = getPropertyValue("Password");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To log in using this algorithm you need to give a "
        "valid SCARF username and password." << std::endl;
      throw;
    }
    doLogin(username, password);
  } else if (Action::LOGOUT == m_action) {
    std::string username;
    try {
      username = getPropertyValue("UserName");
    } catch(std::runtime_error& /*e*/) {
      g_log.error() << "To log out using this algorithm you need to give a "
        "valid SCARF username." << std::endl;
      throw;
    }
    doLogout(username);
  } else if (Action::SUBMIT == m_action) {
    doSubmit();
  } else if (Action::QUERYSTATUS == m_action) {
    doQueryStatus();
  } else if (Action::CANCEL == m_action) {
    doCancel();
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
void SCARFTomoReconstruction::doLogin(std::string &username,
                                      std::string &password) {
  // log into "https://portal.scarf.rl.ac.uk/cgi-bin/token.py";

  const std::string SCARFComputeResource = "SCARF@STFC";
  // this should go away and obtained from 'computeResourceInfo' (like
  // a very simple InstrumentInfo) or similar. What we need here is
  // computeResourceInfo::baseURL()
  const std::string SCARFLoginBaseURL = "https://portal.scarf.rl.ac.uk/";
  const std::string SCARFLoginPath = "/cgi-bin/token.py";

  std::vector<std::string> res = ConfigService::Instance().getFacility().
    computeResources();
  auto it = std::find(res.begin(), res.end(), SCARFComputeResource);
  if (res.end() == it)
    throw std::runtime_error(std::string("Failed to find a compute resource "
                               "for " +  SCARFComputeResource + " (facility: " +
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
    // insert in the token stash
    UsernameToken tok(username, Token(url, token_str));
    m_tokenStash.insert(tok); // the password is never stored
    g_log.notice() << "Got authentication token. You are now logged into "
                   << SCARFComputeResource << std::endl;
  } else {
    throw std::runtime_error("Login failed. Please check your username and "
                             "password.");
  }
}

/**
 * Log out from SCARF. In practice, it trashes the cookie (if we were
 * successfully logged in).
 */
void SCARFTomoReconstruction::doLogout(std::string &username) {

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
  //headers.insert(std::pair<"Cookie", token>);
  //headers.insert(std::pair<"Accept", m_acceptType>);
  int code = session.sendRequest(httpsURL, ss, headers);
  std::string resp = ss.str();
  g_log.debug() << "Got HTTP code " << code << ", response: " <<  resp << std::endl;
  if (Poco::Net::HTTPResponse::HTTP_OK == code) {
    g_log.notice() << "Logged out with response: " << resp;
  } else {
    throw std::runtime_error("Failed to logout from the web service at: " +
                             httpsURL + ". Please check your username.");
  }
}

/**
 * Submits a job to SCARF. The different ways jobs could be submitted
 * (supported toolkits, LSF PAC submission forms, launcher scripts,
 * supported options, etc.) are not well defined at the moment.
 */
void SCARFTomoReconstruction::doSubmit() {
  progress(0, "Starting tomographic reconstruction job...");

  try {
    m_nxTomoPath = getPropertyValue("RemoteNXTomoPath");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify the remote path to the NXTomo file "
      "which is required to create a new reconstruction job. Please provide "
      "a valid path on the SCARF cluster" << std::endl;
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

  // TODO: create
  // TODO: handle failure

  progress(1.0, "Job created.");
}

/**
 * Query the status of jobs running (if successful will return info on
 * jobs running for our user)
 */
void SCARFTomoReconstruction::doQueryStatus() {
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
 */
void SCARFTomoReconstruction::doCancel() {
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

} // end namespace RemoteAlgorithms
} // end namespace Mantid
