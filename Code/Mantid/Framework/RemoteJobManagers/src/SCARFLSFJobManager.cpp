#include <sstream>

#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidRemoteJobManagers/SCARFLSFJobManager.h"

#include "boost/algorithm/string/replace.hpp"

namespace Mantid {
namespace RemoteJobManagers {

// Register the manager into the RemoteJobManagerFactory
DECLARE_REMOTEJOBMANAGER(SCARFLSFJobManager)

namespace {
// static logger object
Mantid::Kernel::Logger g_log("SCARFLSFJobManager");
}

std::string LSFJobManager::g_loginBaseURL = "https://portal.scarf.rl.ac.uk/";
std::string LSFJobManager::g_loginPath = "/cgi-bin/token.py";

std::string SCARFLSFJobManager::g_logoutPath = "webservice/pacclient/logout/";
std::string SCARFLSFJobManager::g_pingPath =
    "platform/webservice/pacclient/ping/";
// This could be passed here from facilities or similar
// (like loginBaseURL above) - but note that in principle
// the port number is known only after logging in
std::string SCARFLSFJobManager::g_pingBaseURL =
    "https://portal.scarf.rl.ac.uk:8443/";

/**
 * Log into SCARF. If it goes well, it will produce a token that can
 * be reused for a while in subsequent queries. Internally it relies
 * on the InternetHelper to send an HTTP request and obtain the
 * response.
 *
 * @param username normally an STFC federal ID
 * @param password user password
 */
void SCARFLSFJobManager::authenticate(const std::string &username,
                                      const std::string &password) {
  // base LSFJobManager class only supports a single user presently
  m_tokenStash.clear();
  m_transactions.clear();

  const std::string params = "?username=" + username + "&password=" + password;
  const Poco::URI fullURL =
      makeFullURI(Poco::URI(g_loginBaseURL), g_loginPath, params);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error("Error while sending HTTP request to authenticate "
                             "(log in): " +
                             std::string(ie.what()));
  }
  // We would check (Poco::Net::HTTPResponse::HTTP_OK == code) but the SCARF
  // login script (token.py) seems to return 200 whatever happens, as far as the
  // request is well formed. So this is how to know if authentication succeeded:
  const std::string expectedSubstr = g_loginBaseURL;
  std::string resp = ss.str();
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code &&
      resp.find(expectedSubstr) != std::string::npos) {
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
    g_log.notice() << "Got authentication token. You are now logged in "
                   << std::endl;
  } else {
    throw std::runtime_error("Login failed. Please check your username and "
                             "password. Got status code " +
                             boost::lexical_cast<std::string>(code) +
                             ", with this response: " + resp);
  }
}

/**
 * Ping the server to see if the web service is active/available.
 * Note that this method does not need the user to be logged in.
 *
 * For now this ping method sits here as specific to SCARF.  It is not
 * clear at the moment if it is general to LSF. It could well be
 * possible to pull this into LSFJobManager.
 *
 * @return true if the web service responds.
 */
bool SCARFLSFJobManager::ping() {
  // Job ping, needs these headers:
  // headers = {'Content-Type': 'application/xml', 'Accept': ACCEPT_TYPE}
  const Poco::URI fullURL = makeFullURI(Poco::URI(g_pingBaseURL), g_pingPath);
  const StringToStringMap headers =
      makeHeaders(std::string("text/plain"), "", g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error("Error while sending HTTP request to ping the "
                             "server " +
                             std::string(ie.what()));
  }
  bool ok = false;
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    std::string resp = ss.str();
    if (std::string::npos != resp.find("Web Services are ready")) {
      g_log.notice()
          << "Pinged compute resource with apparently good response: " << resp
          << std::endl;
      ok = true;
    } else {
      g_log.warning() << "Pinged compute resource but got what looks like an "
                         "error message: " << resp << std::endl;
    }
  } else {
    throw std::runtime_error(
        "Failed to ping the web service at:" + fullURL.toString() +
        ". Please check your parameters, software version, "
        "etc.");
  }

  return ok;
}

/**
 * Log out from SCARF. In practice, it trashes the cookie (if we were
 * successfully logged in).
 *
 * As the authentication method is specific to SCARF, this logout
 * method has been placed here as specific to SCARF too. Probably it
 * is general to other LSF systems without any/much changes.
 *
 * @param username Username to use (should have authenticated
 * before). Leave it empty to log out the last (maybe only) user that
 * logged in with authenticate().
 */
void SCARFLSFJobManager::logout(const std::string &username) {
  if (0 == m_tokenStash.size()) {
    throw std::runtime_error("Logout failed. No one is currenlty logged in.");
  }

  std::map<std::string, Token>::iterator it;
  if (!username.empty()) {
    it = m_tokenStash.find(username);
    if (m_tokenStash.end() == it) {
      throw std::invalid_argument(
          "Logout failed. The username given is not logged in: " + username);
    }
  }
  // only support for single-user
  Token tok = m_tokenStash.begin()->second;

  // logout query, needs headers = {'Content-Type': 'text/plain', 'Cookie':
  // token,
  //    'Accept': 'text/plain,application/xml,text/xml'}
  const std::string token = tok.m_token_str;
  const Poco::URI fullURL = makeFullURI(tok.m_url, g_logoutPath);
  const StringToStringMap headers =
      makeHeaders(std::string("text/plain"), token, g_acceptType);
  int code = 0;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(fullURL, ss, headers);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error("Error while sending HTTP request to log out: " +
                             std::string(ie.what()));
  }
  if (Mantid::Kernel::InternetHelper::HTTP_OK == code) {
    g_log.notice() << "Logged out." << std::endl;
    g_log.debug() << "Response from server: " << ss.str() << std::endl;
  } else {
    throw std::runtime_error("Failed to logout from the web service at: " +
                             fullURL.toString() +
                             ". Please check your username.");
  }

  // successfully logged out, forget the token
  if (username.empty()) {
    // delete first one
    m_tokenStash.erase(m_tokenStash.begin());
  } else {
    // delete requested one
    if (m_tokenStash.end() != it)
      m_tokenStash.erase(it);
  }
}

} // end namespace RemoteJobManagers
} // end namespace Mantid
