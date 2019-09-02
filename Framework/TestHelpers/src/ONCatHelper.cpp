// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidTestHelpers/ONCatHelper.h"

#include <exception>

using Mantid::Catalog::OAuth::IOAuthTokenStore;
using Mantid::Catalog::OAuth::IOAuthTokenStore_uptr;
using Mantid::Catalog::OAuth::OAuthToken;
using Mantid::Kernel::Exception::InternetError;
using Mantid::Kernel::Exception::NotImplementedError;

namespace Mantid {
namespace TestHelpers {

MockONCatAPI::MockONCatAPI(const MockResponseMap &responseMap)
    : Mantid::Kernel::InternetHelper(), m_responseMap(responseMap),
      m_responseCallCounts() {
  for (const auto &mapping : responseMap) {
    m_responseCallCounts[mapping.first] = 0;
  }
}

MockONCatAPI::~MockONCatAPI() {}

bool MockONCatAPI::allResponsesCalledOnce() const {
  return std::all_of(m_responseCallCounts.cbegin(), m_responseCallCounts.cend(),
                     [](const MockResponseCallMapping &mapping) {
                       return mapping.second == 1;
                     });
}

bool MockONCatAPI::allResponsesCalled() const {
  return std::all_of(m_responseCallCounts.cbegin(), m_responseCallCounts.cend(),
                     [](const MockResponseCallMapping &mapping) {
                       return mapping.second >= 1;
                     });
}

int MockONCatAPI::sendHTTPRequest(const std::string &url,
                                  std::ostream &responseStream) {
  return sendHTTPSRequest(url, responseStream);
}

int MockONCatAPI::sendHTTPSRequest(const std::string &url,
                                   std::ostream &responseStream) {
  const auto mockResponse = m_responseMap.find(url);

  if (mockResponse == m_responseMap.end()) {
    // If the test that is using this has not set up a corresponding URL
    // then throw an exception rather than segfault.
    throw NotImplementedError(
        url + " has not been assigned a corresponding response.");
  }

  m_responseCallCounts[url] += 1;

  const auto statusCode = mockResponse->second.first;
  const auto responseBody = mockResponse->second.second;

  // Approximate the behaviour of the actual helper class when a non-OK
  // response is observed.
  if (statusCode != HTTPResponse::HTTP_OK) {
    throw InternetError(responseBody, statusCode);
  }

  responseStream << responseBody;
  return statusCode;
}

std::shared_ptr<MockONCatAPI>
make_mock_oncat_api(const MockResponseMap &responseMap) {
  return std::make_shared<MockONCatAPI>(responseMap);
}

std::unique_ptr<ONCat>
make_oncat_with_mock_api(const std::shared_ptr<MockONCatAPI> &mockAPI) {
  auto oncat = ONCat::fromMantidSettings();
  oncat->setInternetHelper(mockAPI);
  return oncat;
}

MockTokenStore::MockTokenStore() : m_token(boost::none) {}

void MockTokenStore::setToken(const boost::optional<OAuthToken> &token) {
  m_token = token;
}

boost::optional<OAuthToken> MockTokenStore::getToken() { return m_token; }

IOAuthTokenStore_uptr make_mock_token_store() {
  return std::make_unique<MockTokenStore>();
}

IOAuthTokenStore_uptr make_mock_token_store_already_logged_in() {
  auto tokenStore = std::make_unique<MockTokenStore>();
  tokenStore->setToken(OAuthToken(
      "Bearer", 3600, "2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ",
      "api:read data:read settings:read",
      boost::make_optional<std::string>("eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb")));
  return std::move(tokenStore);
}

} // namespace TestHelpers
} // namespace Mantid
