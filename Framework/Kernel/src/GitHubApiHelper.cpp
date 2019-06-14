// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/GitHubApiHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include <boost/lexical_cast.hpp>
#include <json/json.h>
#include <map>
#include <ostream>
#include <string>

namespace Mantid {
using namespace Types::Core;
namespace Kernel {

// Forward declare
class ProxyInfo;

using namespace Poco::Net;
using std::string;

namespace {
// anonymous namespace for some utility functions
/// static Logger object
Logger g_log("GitHubApiHelper");

const std::string RATE_LIMIT_URL("https://api.github.com/rate_limit");

std::string formatRateLimit(const int rateLimit, const int remaining,
                            const int expires) {
  DateAndTime expiresDateAndTime;
  expiresDateAndTime.set_from_time_t(expires);

  std::stringstream msg;
  msg << "GitHub API limited to " << remaining << " of " << rateLimit
      << " calls left. Resets at " << expiresDateAndTime.toISO8601String()
      << "Z";
  return msg.str();
}
} // namespace

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
  int rateLimitReset;
  try {
    rateLimitLimit =
        boost::lexical_cast<int>(res.get("X-RateLimit-Limit", "-1"));
    rateLimitRemaining =
        boost::lexical_cast<int>(res.get("X-RateLimit-Remaining", "-1"));
    rateLimitReset =
        boost::lexical_cast<int>(res.get("X-RateLimit-Reset", "0"));
  } catch (boost::bad_lexical_cast const &) {
    rateLimitLimit = -1;
  }
  if (rateLimitLimit > -1) {
    g_log.debug(
        formatRateLimit(rateLimitLimit, rateLimitRemaining, rateLimitReset));
  }
}

std::string GitHubApiHelper::getRateLimitDescription() {
  std::stringstream responseStream;
  this->sendRequest(RATE_LIMIT_URL, responseStream);
  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(responseStream, root)) {
    return "Failed to parse json document from \"" + RATE_LIMIT_URL + "\"";
  }

  const auto &rateInfo = root.get("rate", "");
  if (rateInfo.empty())
    return std::string();

  const int limit = rateInfo.get("limit", -1).asInt();
  const int remaining = rateInfo.get("remaining", -1).asInt();
  const int expires = rateInfo.get("reset", 0).asInt();

  return formatRateLimit(limit, remaining, expires);
}

int GitHubApiHelper::processAnonymousRequest(
    const Poco::Net::HTTPResponse &response, Poco::URI &uri,
    std::ostream &responseStream) {
  if (!isAuthenticated()) {
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
    if (m_response)
      processResponseHeaders(*m_response);
    else
      g_log.warning("Response is null pointer");
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
