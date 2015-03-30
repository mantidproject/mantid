#include <sstream>

#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidRemoteJobManagers/SCARFLSFJobManager.h"

#include "boost/algorithm/string/replace.hpp"

namespace Mantid {
namespace RemoteJobManagers {

// Register the manager into the RemoteJobManagerFactory
// TODO Factory TODO
// DECLARE_REMOTEJOBMANAGER(SCARFLSFJobManager);

namespace {
// static logger object
Mantid::Kernel::Logger g_log("SCARFLSFJobManager");
}

std::string LSFJobManager::m_loginBaseURL = "https://portal.scarf.rl.ac.uk/";
std::string LSFJobManager::m_loginPath = "/cgi-bin/token.py";

std::map<std::string, Token> SCARFLSFJobManager::m_tokenStash;

/**
 * Log into SCARF. If it goes well, it will produce a token that can
 * be reused for a while in subsequent queries. Internally it relies
 * on the InternetHelper to send an HTTP request and obtain the
 * response.
 *
 * @param username normally an STFC federal ID
 * @param password user password
 */
void SCARFLSFJobManager::authenticate(std::string &username,
                                      std::string &password) {
  // base LSFJobManager class only supports a single user presently
  m_tokenStash.clear();
  m_transactions.clear();

  std::string httpsURL = m_loginBaseURL + m_loginPath + "?username=" +
                         username + "&password=" + password;
  int code;
  std::stringstream ss;
  try {
    code = doSendRequestGetResponse(httpsURL, ss);
  } catch (Kernel::Exception::InternetError &ie) {
    throw std::runtime_error("Error while sending HTTP request to authenticate "
                             "(log in): " +
                             std::string(ie.what()));
  }
  // We would check (Poco::Net::HTTPResponse::HTTP_OK == code) but the SCARF
  // login script (token.py) seems to return 200 whatever happens, as far as the
  // request is well formed. So this is how to know if authentication succeeded:
  const std::string expectedSubstr = m_loginBaseURL;
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
                             "password. Got this response: " +
                             resp);
  }
}

} // end namespace RemoteJobManagers
} // end namespace Mantid
