// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCatalog/ONCat.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"

#include <map>
#include <memory>

using Mantid::Catalog::OAuth::IOAuthTokenStore;
using Mantid::Catalog::OAuth::IOAuthTokenStore_uptr;
using Mantid::Catalog::OAuth::OAuthToken;
using Mantid::Catalog::ONCat::ONCat;
using Mantid::Kernel::InternetHelper;
using Mantid::Kernel::Exception::InternetError;

namespace Mantid {
namespace FrameworkTestHelpers {

using MockResponseMap = std::map<std::string, std::pair<InternetHelper::HTTPStatus, std::string>>;
using MockResponseCallCounts = std::map<std::string, unsigned int>;
using MockResponseCallMapping = std::pair<const std::basic_string<char>, unsigned int>;

class MockONCatAPI : public Mantid::Kernel::InternetHelper {
public:
  MockONCatAPI() = delete;
  MockONCatAPI(const MockResponseMap &responseMap);
  ~MockONCatAPI();

  bool allResponsesCalledOnce() const;
  bool allResponsesCalled() const;

protected:
  InternetHelper::HTTPStatus sendHTTPRequest(const std::string &url, std::ostream &responseStream) override;
  InternetHelper::HTTPStatus sendHTTPSRequest(const std::string &url, std::ostream &responseStream) override;

private:
  MockResponseMap m_responseMap;
  MockResponseCallCounts m_responseCallCounts;
};

std::shared_ptr<MockONCatAPI> make_mock_oncat_api(const MockResponseMap &responseMap);

std::unique_ptr<ONCat> make_oncat_with_mock_api(const std::shared_ptr<MockONCatAPI> &mockAPI);

class MockTokenStore : public IOAuthTokenStore {
public:
  MockTokenStore();

  void setToken(const std::optional<OAuthToken> &token) override;
  std::optional<OAuthToken> getToken() override;

private:
  std::optional<OAuthToken> m_token;
};

IOAuthTokenStore_uptr make_mock_token_store();
IOAuthTokenStore_uptr make_mock_token_store_already_logged_in();

} // namespace FrameworkTestHelpers
} // namespace Mantid
