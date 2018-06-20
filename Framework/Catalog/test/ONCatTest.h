#ifndef MANTID_CATALOG_ONCATTEST_H_
#define MANTID_CATALOG_ONCATTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCatalog/Exception.h"
#include "MantidCatalog/ONCat.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/make_unique.h"

#include <map>

#include <Poco/Net/HTTPResponse.h>

using Poco::Net::HTTPResponse;

using Mantid::Catalog::Exception::InvalidCredentialsError;
using Mantid::Catalog::Exception::InvalidRefreshTokenError;
using Mantid::Catalog::Exception::TokenRejectedError;
using Mantid::Catalog::OAuth::OAuthFlow;
using Mantid::Catalog::OAuth::OAuthToken;
using Mantid::Catalog::OAuth::ConfigServiceTokenStore;
using Mantid::Catalog::OAuth::IOAuthTokenStore;
using Mantid::Catalog::OAuth::IOAuthTokenStore_uptr;
using Mantid::Catalog::ONCat::ONCat;
using Mantid::Catalog::ONCat::QueryParameter;
using Mantid::Types::Core::DateAndTime;
using Mantid::Kernel::Exception::InternetError;

//----------------------------------------------------------------------
// Helpers, Mocks and Variables
//----------------------------------------------------------------------

namespace {

using MockResponseMap = std::map<std::string, std::pair<int, std::string>>;
using MockResponseCallCounts = std::map<std::string, unsigned int>;
using MockResponseCallMapping = std::pair<const std::basic_string<char>, unsigned int>;

class MockONCatAPI : public Mantid::Kernel::InternetHelper {
public:
  MockONCatAPI() = delete;
  MockONCatAPI(
    const MockResponseMap & responseMap
  ) :
    Mantid::Kernel::InternetHelper(),
    m_responseMap(responseMap),
    m_responseCallCounts()
  {
    for (const auto & mapping : responseMap) {
      m_responseCallCounts[mapping.first] = 0;
    }
  }
  ~MockONCatAPI() {
    TS_ASSERT(allResponsesCalledOnce());
  }

  bool allResponsesCalledOnce() const {
    return std::all_of(
      m_responseCallCounts.cbegin(), m_responseCallCounts.cend(),
      [](const MockResponseCallMapping & mapping){
        return mapping.second == 1;
      }
    );
  }

protected:
  int sendHTTPRequest(
    const std::string &url, std::ostream & responseStream
  ) override { return sendHTTPSRequest(url, responseStream); }

  int sendHTTPSRequest(
    const std::string &url, std::ostream & responseStream
  ) override {
    auto mockResponse = m_responseMap.find(url);

    assert(mockResponse != m_responseMap.end());
    m_responseCallCounts[url] += 1;

    const auto statusCode = mockResponse->second.first;
    const auto responseBody = mockResponse->second.second;

    // Approximate the behaviour of the actual helper class when a
    // non-OK response is observed.
    if (statusCode != HTTPResponse::HTTP_OK) {
      throw InternetError(responseBody, statusCode);
    }

    responseStream << responseBody;
    return statusCode;
  }

private:
  MockResponseMap m_responseMap;
  MockResponseCallCounts m_responseCallCounts;
};

std::unique_ptr<MockONCatAPI> make_mock_oncat_api(
  const MockResponseMap & responseMap
) {
  return Mantid::Kernel::make_unique<MockONCatAPI>(responseMap);
}

class MockTokenStore : public IOAuthTokenStore {
public:
  MockTokenStore() : m_token(boost::none) {}

  void setToken(const boost::optional<OAuthToken> & token) override {
    m_token = token;
  }
  boost::optional<OAuthToken> getToken() override { return m_token; }
private:
  boost::optional<OAuthToken> m_token;
};

IOAuthTokenStore_uptr make_mock_token_store() {
  return Mantid::Kernel::make_unique<MockTokenStore>();
}

IOAuthTokenStore_uptr make_mock_token_store_already_logged_in() {
  auto tokenStore = Mantid::Kernel::make_unique<MockTokenStore>();
  tokenStore->setToken(OAuthToken(
    "Bearer",
    3600,
    "2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ",
    "api:read data:read settings:read",
    boost::make_optional<std::string>("eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb")
  ));
  return std::move(tokenStore);
}

const static std::string DUMMY_URL = "https://not.a.real.url";
const static std::string DUMMY_CLIENT_ID =
  "0e527a36-297d-4cb4-8a35-84f6b11248d7";

}

//----------------------------------------------------------------------
// Tests
//----------------------------------------------------------------------

class ONCatTest : public CxxTest::TestSuite {
public:
  // CxxTest boilerplate.
  static ONCatTest *createSuite() { return new ONCatTest(); }
  static void destroySuite(ONCatTest *suite) { delete suite; }

  void test_login_with_invalid_credentials_throws() {
    ONCat oncat(
      DUMMY_URL,
      make_mock_token_store(),
      OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
      DUMMY_CLIENT_ID
    );

    TS_ASSERT(!oncat.isUserLoggedIn());

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/oauth/token", std::make_pair(
          HTTPResponse::HTTP_UNAUTHORIZED,
          "{\"error\": \"invalid_grant\", "
          "\"error_description\": \"Invalid credentials given.\"}"
        )
      }
    }));

    TS_ASSERT_THROWS(
      oncat.login("user", "does_not_exist"),
      InvalidCredentialsError
    );
    TS_ASSERT(!oncat.isUserLoggedIn());
  }

  void test_login_with_valid_credentials_is_successful() {
    ONCat oncat(
      DUMMY_URL,
      make_mock_token_store(),
      OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
      DUMMY_CLIENT_ID
    );

    TS_ASSERT(!oncat.isUserLoggedIn());

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/oauth/token", std::make_pair(
          HTTPResponse::HTTP_OK,
          "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
          "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
          "\"scope\": \"api:read data:read settings:read\", "
          "\"refresh_token\": \"eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb\"}"
        )
      }
    }));

    oncat.login("user", "does_exist");

    TS_ASSERT(oncat.isUserLoggedIn());
  }

  void test_refreshing_token_when_needed() {
    ONCat oncat(
      DUMMY_URL,
      make_mock_token_store(),
      OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
      DUMMY_CLIENT_ID
    );

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/oauth/token", std::make_pair(
          HTTPResponse::HTTP_OK,
          "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
          "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
          "\"scope\": \"api:read data:read settings:read\", "
          "\"refresh_token\": \"eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb\"}"
        )
      }
    }));

    oncat.login("user", "does_exist");

    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.refreshTokenIfNeeded();
    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/oauth/token", std::make_pair(
          HTTPResponse::HTTP_OK,
          "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
          "\"access_token\": \"7dS7flfhsf7ShndHJSFknfskfeu789\", "
          "\"scope\": \"api:read data:read settings:read\", "
          "\"refresh_token\": \"sdagSDGF87dsgljerg6gdfgddfgfdg\"}"
        )
      }
    }));

    oncat.refreshTokenIfNeeded(DateAndTime::getCurrentTime() + 3601.0);
    TS_ASSERT(oncat.isUserLoggedIn());
  }

  void test_logged_out_when_refreshing_fails() {
    ONCat oncat(
      DUMMY_URL,
      make_mock_token_store(),
      OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
      DUMMY_CLIENT_ID
    );

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/oauth/token", std::make_pair(
          HTTPResponse::HTTP_OK,
          "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
          "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
          "\"scope\": \"api:read data:read settings:read\", "
          "\"refresh_token\": \"eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb\"}"
        )
      }
    }));

    oncat.login("user", "does_exist");

    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.refreshTokenIfNeeded();
    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/oauth/token", std::make_pair(
          HTTPResponse::HTTP_UNAUTHORIZED,
          "{\"error\": \"invalid_grant\", "
          "\"error_description\": \"Bearer token not found.\"}"
        )
      }
    }));

    TS_ASSERT_THROWS(
      oncat.refreshTokenIfNeeded(DateAndTime::getCurrentTime() + 3601.0),
      InvalidRefreshTokenError
    );

    TS_ASSERT(!oncat.isUserLoggedIn());
  }

  void test_retrieve_entity() {
    ONCat oncat(
      DUMMY_URL,
      make_mock_token_store_already_logged_in(),
      OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
      DUMMY_CLIENT_ID
    );

    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/api/instruments/HB2C?facility=HFIR", std::make_pair(
          HTTPResponse::HTTP_OK,
          "{\"facility\": \"HFIR\","
          "\"name\": \"HB2C\","
          "\"id\": \"HB2C\","
          "\"type\": \"instrument\"}"
        )
      }
    }));

    const auto entity = oncat.retrieve("api", "instruments", "HB2C", {
      QueryParameter("facility", "HFIR")
    });

    TS_ASSERT_EQUALS(entity.id(), std::string("HB2C"));
    TS_ASSERT_EQUALS(entity.asString("name"), std::string("HB2C"));
  }

  void test_list_entities() {
    ONCat oncat(
      DUMMY_URL,
      make_mock_token_store_already_logged_in(),
      OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
      DUMMY_CLIENT_ID
    );

    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/api/instruments?facility=HFIR", std::make_pair(
          HTTPResponse::HTTP_OK,
          "["
          "  {"
          "    \"facility\": \"HFIR\","
          "    \"name\": \"HB2C\","
          "    \"id\": \"HB2C\","
          "    \"type\": \"instrument\""
          "  },"
          "  {"
          "    \"facility\": \"HFIR\","
          "    \"name\": \"CG1D\","
          "    \"id\": \"CG1D\","
          "    \"type\": \"instrument\""
          "  }"
          "]"
        )
      }
    }));

    const auto entities = oncat.list("api", "instruments", {
      QueryParameter("facility", "HFIR")
    });

    TS_ASSERT_EQUALS(entities.size(), 2);
    TS_ASSERT_EQUALS(entities[0].id(), std::string("HB2C"));
    TS_ASSERT_EQUALS(entities[0].asString("name"), std::string("HB2C"));
    TS_ASSERT_EQUALS(entities[1].id(), std::string("CG1D"));
    TS_ASSERT_EQUALS(entities[1].asString("name"), std::string("CG1D"));
  }

  void test_send_api_request_logs_out_with_invalid_grant() {
    ONCat oncat(
      DUMMY_URL,
      make_mock_token_store_already_logged_in(),
      OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
      DUMMY_CLIENT_ID
    );

    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/api/instruments?facility=HFIR", std::make_pair(
          HTTPResponse::HTTP_UNAUTHORIZED, "{}"
        )
      }
    }));

    TS_ASSERT_THROWS(
      oncat.list("api", "instruments", {QueryParameter("facility", "HFIR")}),
      TokenRejectedError
    );
    TS_ASSERT(!oncat.isUserLoggedIn());
  }

  void test_client_credentials_flow_with_refresh() {
    ONCat oncat(
      DUMMY_URL,
      make_mock_token_store(),
      OAuthFlow::CLIENT_CREDENTIALS,
      DUMMY_CLIENT_ID,
      boost::make_optional<std::string>(
        "9a2ad07a-a139-438b-8116-08c5452f96ad"
      )
    );

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/oauth/token", std::make_pair(
          HTTPResponse::HTTP_OK,
          "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
          "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
          "\"scope\": \"api:read data:read settings:read\"}"
        )
      }, {
        DUMMY_URL + "/api/instruments/HB2C?facility=HFIR", std::make_pair(
          HTTPResponse::HTTP_OK,
          "{\"facility\": \"HFIR\","
          "\"name\": \"HB2C\","
          "\"id\": \"HB2C\","
          "\"type\": \"instrument\"}"
        )
      }
    }));

    oncat.retrieve("api", "instruments", "HB2C", {
      QueryParameter("facility", "HFIR")
    });

    oncat.setInternetHelper(make_mock_oncat_api({
      {
        DUMMY_URL + "/oauth/token", std::make_pair(
          HTTPResponse::HTTP_OK,
          "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
          "\"access_token\": \"987JHGFiusdvs72fAkjhsKJH32tkjk\", "
          "\"scope\": \"api:read data:read settings:read\"}"
        ),
      }
    }));

    oncat.refreshTokenIfNeeded(DateAndTime::getCurrentTime() + 3601.0);
  }

  void test_config_service_token_store_roundtrip() {
    ConfigServiceTokenStore tokenStore;

    const auto testToken = boost::make_optional(OAuthToken(
      "Bearer",
      3600,
      "2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ",
      "api:read data:read settings:read",
      boost::make_optional<std::string>("eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb")
    ));

    tokenStore.setToken(testToken);

    const auto result = tokenStore.getToken();

    TS_ASSERT_EQUALS(testToken->tokenType(), result->tokenType());
    TS_ASSERT_EQUALS(testToken->expiresIn(), result->expiresIn());
    TS_ASSERT_EQUALS(testToken->accessToken(), result->accessToken());
    TS_ASSERT_EQUALS(testToken->scope(), result->scope());
    TS_ASSERT_EQUALS(*testToken->refreshToken(), *result->refreshToken());
  }
};

#endif /* MANTID_CATALOG_ONCATTEST_H_ */
