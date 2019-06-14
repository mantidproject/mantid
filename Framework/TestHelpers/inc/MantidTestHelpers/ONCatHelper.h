// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TESTHELPERS_ONCATHELPER_H_
#define MANTID_TESTHELPERS_ONCATHELPER_H_

#include "MantidCatalog/ONCat.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"

#include <map>
#include <memory>

#include <Poco/Net/HTTPResponse.h>

using Poco::Net::HTTPResponse;

using Mantid::Catalog::OAuth::IOAuthTokenStore;
using Mantid::Catalog::OAuth::IOAuthTokenStore_uptr;
using Mantid::Catalog::OAuth::OAuthToken;
using Mantid::Catalog::ONCat::ONCat;
using Mantid::Kernel::Exception::InternetError;

namespace Mantid {
namespace TestHelpers {

using MockResponseMap = std::map<std::string, std::pair<int, std::string>>;
using MockResponseCallCounts = std::map<std::string, unsigned int>;
using MockResponseCallMapping =
    std::pair<const std::basic_string<char>, unsigned int>;

class MockONCatAPI : public Mantid::Kernel::InternetHelper {
public:
  MockONCatAPI() = delete;
  MockONCatAPI(const MockResponseMap &responseMap);
  ~MockONCatAPI();

  bool allResponsesCalledOnce() const;
  bool allResponsesCalled() const;

protected:
  int sendHTTPRequest(const std::string &url,
                      std::ostream &responseStream) override;
  int sendHTTPSRequest(const std::string &url,
                       std::ostream &responseStream) override;

private:
  MockResponseMap m_responseMap;
  MockResponseCallCounts m_responseCallCounts;
};

std::shared_ptr<MockONCatAPI>
make_mock_oncat_api(const MockResponseMap &responseMap);
std::unique_ptr<ONCat>
make_oncat_with_mock_api(const std::shared_ptr<MockONCatAPI> &mockAPI);

class MockTokenStore : public IOAuthTokenStore {
public:
  MockTokenStore();

  void setToken(const boost::optional<OAuthToken> &token) override;
  boost::optional<OAuthToken> getToken() override;

private:
  boost::optional<OAuthToken> m_token;
};

IOAuthTokenStore_uptr make_mock_token_store();
IOAuthTokenStore_uptr make_mock_token_store_already_logged_in();

} // namespace TestHelpers
} // namespace Mantid

#endif // MANTID_TESTHELPERS_ONCATHELPER_H_
