#include "MantidKernel/GitHubApiHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPClientSession.h>

#include <boost/lexical_cast.hpp>

#include <map>
#include <ostream>
#include <string>

namespace Mantid {
namespace Kernel {

// Forward declare
class ProxyInfo;

using namespace Poco::Net;
using std::map;
using std::string;

namespace {
// anonymous namespace for some utility functions
/// static Logger object
Logger g_log("InternetHelper");
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
GitHubApiHelper::GitHubApiHelper() : InternetHelper() {
  addAuthenticationToken();
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
GitHubApiHelper::GitHubApiHelper(const Kernel::ProxyInfo &proxy)
    : InternetHelper(proxy) {
  addAuthenticationToken();
}

void GitHubApiHelper::reset() {
  InternetHelper::reset();
  addAuthenticationToken();
}

bool GitHubApiHelper::isAuthenticated() {
  return (m_headers.find("Authorization") != m_headers.end());
}

void GitHubApiHelper::processResponseHeaders(
    const Poco::Net::HTTPResponse &res) {
  // get github api rate limit information if available;
  int rateLimitRemaining = 0;
  int rateLimitLimit;
  DateAndTime rateLimitReset;
  try {
    rateLimitLimit =
        boost::lexical_cast<int>(res.get("X-RateLimit-Limit", "-1"));
    rateLimitRemaining =
        boost::lexical_cast<int>(res.get("X-RateLimit-Remaining", "-1"));
    rateLimitReset.set_from_time_t(
        boost::lexical_cast<int>(res.get("X-RateLimit-Reset", "0")));
  } catch (boost::bad_lexical_cast const &) {
    rateLimitLimit = -1;
  }
  if (rateLimitLimit > -1) {
    g_log.debug() << "GitHub API " << rateLimitRemaining << " of "
                  << rateLimitLimit << " calls left. Resets at "
                  << rateLimitReset.toSimpleString() << " GMT\n";
  }
}

int GitHubApiHelper::processAnonymousRequest(
    const Poco::Net::HTTPResponse &response, Poco::URI &uri,
    std::ostream &responseStream) {
  if (isAuthenticated()) {
    g_log.debug("Repeating API call anonymously\n");
    removeHeader("Authorization");
    return this->sendRequest(uri.toString(), responseStream);
  } else {
    g_log.warning("Authentication failed and anonymous access refused\n");
    return response.getStatus();
  }
}

int GitHubApiHelper::sendRequestAndProcess(HTTPClientSession &session,
                                           Poco::URI &uri,
                                           std::ostream &responseStream) {
  // create a request
  this->createRequest(uri);
  session.sendRequest(*m_request) << m_body;

  std::istream &rs = session.receiveResponse(*m_response);
  int retStatus = m_response->getStatus();
  g_log.debug() << "Answer from web: " << retStatus << " "
                << m_response->getReason() << "\n";

  if (retStatus == HTTP_OK ||
      (retStatus == HTTP_CREATED && m_method == HTTPRequest::HTTP_POST)) {
    Poco::StreamCopier::copyStream(rs, responseStream);
    processResponseHeaders(*m_response);
    return retStatus;
  } else if ((retStatus == HTTP_FORBIDDEN && isAuthenticated()) ||
             (retStatus == HTTP_UNAUTHORIZED) ||
             (retStatus == HTTP_NOT_FOUND)) {
    // If authentication fails you can get HTTP_UNAUTHORIZED or HTTP_NOT_FOUND
    // If the limit runs out you can get HTTP_FORBIDDEN
    return this->processAnonymousRequest(*m_response, uri, responseStream);
  } else if (isRelocated(retStatus)) {
    return this->processRelocation(*m_response, responseStream);
  } else {
    Poco::StreamCopier::copyStream(rs, responseStream);
    return processErrorStates(*m_response, rs, uri.toString());
  }
}

} // namespace Kernel
} // namespace Mantid
