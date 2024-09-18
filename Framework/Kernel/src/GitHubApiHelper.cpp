// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/GitHubApiHelper.h"
#include "MantidJson/Json.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include <boost/algorithm/string/predicate.hpp>
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

// key to retreive api token from ConfigService
const std::string CONFIG_KEY_GITHUB_TOKEN("network.github.api_token");

std::string formatRateLimit(const int rateLimit, const int remaining, const int expires) {
  DateAndTime expiresDateAndTime;
  expiresDateAndTime.set_from_time_t(expires);

  std::stringstream msg;
  msg << "GitHub API limited to " << remaining << " of " << rateLimit << " calls left. Resets at "
      << expiresDateAndTime.toISO8601String() << "Z";
  return msg.str();
}

/*
 * Small function to encapsulate getting the token from everything else
 */
std::string getApiToken() {
  // default token is empty string meaning do unauthenticated calls
  std::string token(DEFAULT_GITHUB_TOKEN);
  // get the token from configservice if it has been set
  if (ConfigService::Instance().hasProperty(CONFIG_KEY_GITHUB_TOKEN)) {
    token = ConfigService::Instance().getString(CONFIG_KEY_GITHUB_TOKEN);
  }

  // unset is the user's way of intentionally turning of authentication
  if (token.empty() || boost::istarts_with(token, "unset")) {
    token = "";
  }

  // log what the token is and create final string to set in header
  if (token.empty()) {
    // only unauthenticated calls
    g_log.information("Making unauthenticated calls to GitHub");
    return "";
  } else {
    g_log.information("Attempting authenticated calls to GitHub");

    // create full header using token
    std::stringstream token_header;
    token_header << "token " << token;
    return token_header.str();
  }

  return token;
}
} // namespace

//----------------------------------------------------------------------------------------------
/** Constructor
 */
GitHubApiHelper::GitHubApiHelper() : InternetHelper(), m_api_token(getApiToken()) { addAuthenticationToken(); }

//----------------------------------------------------------------------------------------------
/** Constructor
 */
GitHubApiHelper::GitHubApiHelper(const Kernel::ProxyInfo &proxy) : InternetHelper(proxy), m_api_token(getApiToken()) {
  addAuthenticationToken();
}

void GitHubApiHelper::reset() {
  InternetHelper::reset();
  addAuthenticationToken();
}

void GitHubApiHelper::addAuthenticationToken() {
  // only add the token if it has been set
  if (!m_api_token.empty()) {
    addHeader("Authorization", m_api_token);
  }
}

bool GitHubApiHelper::isAuthenticated() { return (m_headers.find("Authorization") != m_headers.end()); }

void GitHubApiHelper::processResponseHeaders(const HTTPResponse &res) {
  // get github api rate limit information if available;
  int rateLimitRemaining = 0;
  int rateLimitLimit;
  int rateLimitReset;
  try {
    rateLimitLimit = boost::lexical_cast<int>(res.get("X-RateLimit-Limit", "-1"));
    rateLimitRemaining = boost::lexical_cast<int>(res.get("X-RateLimit-Remaining", "-1"));
    rateLimitReset = boost::lexical_cast<int>(res.get("X-RateLimit-Reset", "0"));
  } catch (boost::bad_lexical_cast const &) {
    rateLimitLimit = -1;
  }
  if (rateLimitLimit > -1) {
    g_log.debug(formatRateLimit(rateLimitLimit, rateLimitRemaining, rateLimitReset));
  }
}

std::string GitHubApiHelper::getRateLimitDescription() {
  std::stringstream responseStream;
  this->sendRequest(RATE_LIMIT_URL, responseStream);
  auto responseString = responseStream.str();

  Json::Value root;
  if (!Mantid::JsonHelpers::parse(responseString, &root, NULL)) {
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

Kernel::InternetHelper::HTTPStatus GitHubApiHelper::processAnonymousRequest(Poco::URI &uri,
                                                                            std::ostream &responseStream) {
  g_log.debug("Repeating API call anonymously\n");
  removeHeader("Authorization");
  m_api_token = ""; // all future calls are anonymous
  return this->sendRequest(uri.toString(), responseStream);
}

InternetHelper::HTTPStatus GitHubApiHelper::sendRequestAndProcess(HTTPClientSession &session, Poco::URI &uri,
                                                                  std::ostream &responseStream) {
  // create a request
  this->createRequest(uri);
  session.sendRequest(*m_request) << m_body;

  std::istream &rs = session.receiveResponse(*m_response);
  const auto retStatus = static_cast<HTTPStatus>(m_response->getStatus());
  g_log.debug() << "Answer from web: " << static_cast<int>(retStatus) << " " << m_response->getReason() << "\n";

  if (retStatus == HTTPStatus::OK || (retStatus == HTTPStatus::CREATED && m_method == HTTPRequest::HTTP_POST)) {
    Poco::StreamCopier::copyStream(rs, responseStream);
    if (m_response)
      processResponseHeaders(*m_response);
    else
      g_log.warning("Response is null pointer");
    return retStatus;
  } else if ((retStatus == HTTPStatus::FORBIDDEN && isAuthenticated()) || (retStatus == HTTPStatus::UNAUTHORIZED) ||
             (retStatus == HTTPStatus::NOT_FOUND)) {
    // If authentication fails you can get HTTP_UNAUTHORIZED or HTTP_NOT_FOUND
    // If the limit runs out you can get HTTP_FORBIDDEN
    return this->processAnonymousRequest(uri, responseStream);
  } else if (isRelocated(retStatus)) {
    return static_cast<InternetHelper::HTTPStatus>(this->processRelocation(*m_response, responseStream));
  } else {
    Poco::StreamCopier::copyStream(rs, responseStream);
    return processErrorStates(*m_response, rs, uri.toString());
  }
}

} // namespace Kernel
} // namespace Mantid
