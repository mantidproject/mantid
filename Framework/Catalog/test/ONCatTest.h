// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCatalog/Exception.h"
#include "MantidCatalog/ONCat.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"

#include "MantidFrameworkTestHelpers/ONCatHelper.h"

#include <map>
#include <memory>

using Mantid::Catalog::Exception::InvalidCredentialsError;
using Mantid::Catalog::Exception::InvalidRefreshTokenError;
using Mantid::Catalog::Exception::TokenRejectedError;
using Mantid::Catalog::OAuth::ConfigServiceTokenStore;
using Mantid::Catalog::OAuth::IOAuthTokenStore;
using Mantid::Catalog::OAuth::IOAuthTokenStore_uptr;
using Mantid::Catalog::OAuth::OAuthFlow;
using Mantid::Catalog::OAuth::OAuthToken;
using Mantid::Catalog::ONCat::ONCat;
using Mantid::Catalog::ONCat::QueryParameter;
using Mantid::FrameworkTestHelpers::make_mock_oncat_api;
using Mantid::FrameworkTestHelpers::make_mock_token_store;
using Mantid::FrameworkTestHelpers::make_mock_token_store_already_logged_in;
using Mantid::Kernel::InternetHelper;
using Mantid::Kernel::Exception::InternetError;
using Mantid::Types::Core::DateAndTime;

//----------------------------------------------------------------------
// Helpers, Mocks and Variables
//----------------------------------------------------------------------

namespace {

const static std::string DUMMY_URL = "https://not.a.real.url";
const static std::string DUMMY_CLIENT_ID = "0e527a36-297d-4cb4-8a35-84f6b11248d7";
} // namespace

//----------------------------------------------------------------------
// Tests
//----------------------------------------------------------------------

class ONCatTest : public CxxTest::TestSuite {
public:
  // CxxTest boilerplate.
  static ONCatTest *createSuite() { return new ONCatTest(); }
  static void destroySuite(ONCatTest *suite) { delete suite; }

  void test_login_with_invalid_credentials_throws() {
    ONCat oncat(DUMMY_URL, make_mock_token_store(), OAuthFlow::RESOURCE_OWNER_CREDENTIALS, DUMMY_CLIENT_ID);

    TS_ASSERT(!oncat.isUserLoggedIn());

    auto mock_oncat_api = make_mock_oncat_api(
        {{DUMMY_URL + "/oauth/token", std::make_pair(InternetHelper::HTTPStatus::UNAUTHORIZED,
                                                     "{\"error\": \"invalid_grant\", "
                                                     "\"error_description\": \"Invalid credentials given.\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    TS_ASSERT_THROWS(oncat.login("user", "does_not_exist"), const InvalidCredentialsError &);
    TS_ASSERT(!oncat.isUserLoggedIn());

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_login_with_valid_credentials_is_successful() {
    ONCat oncat(DUMMY_URL, make_mock_token_store(), OAuthFlow::RESOURCE_OWNER_CREDENTIALS, DUMMY_CLIENT_ID);

    TS_ASSERT(!oncat.isUserLoggedIn());

    auto mock_oncat_api = make_mock_oncat_api(
        {{DUMMY_URL + "/oauth/token",
          std::make_pair(InternetHelper::HTTPStatus::OK, "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
                                                         "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
                                                         "\"scope\": \"api:read data:read settings:read\", "
                                                         "\"refresh_token\": \"eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    oncat.login("user", "does_exist");

    TS_ASSERT(oncat.isUserLoggedIn());

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_refreshing_token_when_needed() {
    ONCat oncat(DUMMY_URL, make_mock_token_store(), OAuthFlow::RESOURCE_OWNER_CREDENTIALS, DUMMY_CLIENT_ID);

    auto mock_oncat_api = make_mock_oncat_api(
        {{DUMMY_URL + "/oauth/token",
          std::make_pair(InternetHelper::HTTPStatus::OK, "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
                                                         "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
                                                         "\"scope\": \"api:read data:read settings:read\", "
                                                         "\"refresh_token\": \"eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    oncat.login("user", "does_exist");

    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.refreshTokenIfNeeded();
    TS_ASSERT(oncat.isUserLoggedIn());

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());

    mock_oncat_api = make_mock_oncat_api(
        {{DUMMY_URL + "/oauth/token",
          std::make_pair(InternetHelper::HTTPStatus::OK, "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
                                                         "\"access_token\": \"7dS7flfhsf7ShndHJSFknfskfeu789\", "
                                                         "\"scope\": \"api:read data:read settings:read\", "
                                                         "\"refresh_token\": \"sdagSDGF87dsgljerg6gdfgddfgfdg\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    oncat.refreshTokenIfNeeded(DateAndTime::getCurrentTime() + 3601.0);
    TS_ASSERT(oncat.isUserLoggedIn());

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_logged_out_when_refreshing_fails() {
    ONCat oncat(DUMMY_URL, make_mock_token_store(), OAuthFlow::RESOURCE_OWNER_CREDENTIALS, DUMMY_CLIENT_ID);

    auto mock_oncat_api = make_mock_oncat_api(
        {{DUMMY_URL + "/oauth/token",
          std::make_pair(InternetHelper::HTTPStatus::OK, "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
                                                         "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
                                                         "\"scope\": \"api:read data:read settings:read\", "
                                                         "\"refresh_token\": \"eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    oncat.login("user", "does_exist");

    TS_ASSERT(oncat.isUserLoggedIn());

    oncat.refreshTokenIfNeeded();
    TS_ASSERT(oncat.isUserLoggedIn());

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());

    mock_oncat_api = make_mock_oncat_api(
        {{DUMMY_URL + "/oauth/token", std::make_pair(InternetHelper::HTTPStatus::UNAUTHORIZED,
                                                     "{\"error\": \"invalid_grant\", "
                                                     "\"error_description\": \"Bearer token not found.\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    TS_ASSERT_THROWS(oncat.refreshTokenIfNeeded(DateAndTime::getCurrentTime() + 3601.0),
                     const InvalidRefreshTokenError &);

    TS_ASSERT(!oncat.isUserLoggedIn());

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_retrieve_entity_unauthenticated() {
    ONCat oncat(DUMMY_URL, nullptr, OAuthFlow::NONE, std::nullopt, std::nullopt);

    TS_ASSERT(!oncat.isUserLoggedIn());

    auto mock_oncat_api =
        make_mock_oncat_api({{DUMMY_URL + "/api/instruments/HB2C?facility=HFIR",
                              std::make_pair(InternetHelper::HTTPStatus::OK, "{\"facility\": \"HFIR\","
                                                                             "\"name\": \"HB2C\","
                                                                             "\"id\": \"HB2C\","
                                                                             "\"type\": \"instrument\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    const auto entity = oncat.retrieve("api", "instruments", "HB2C", {{"facility", "HFIR"}});

    TS_ASSERT_EQUALS(entity.id(), std::string("HB2C"));
    TS_ASSERT_EQUALS(entity.get<std::string>("name"), std::string("HB2C"));

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_retrieve_entity() {
    ONCat oncat(DUMMY_URL, make_mock_token_store_already_logged_in(), OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
                DUMMY_CLIENT_ID);

    TS_ASSERT(oncat.isUserLoggedIn());

    auto mock_oncat_api =
        make_mock_oncat_api({{DUMMY_URL + "/api/instruments/HB2C?facility=HFIR",
                              std::make_pair(InternetHelper::HTTPStatus::OK, "{\"facility\": \"HFIR\","
                                                                             "\"name\": \"HB2C\","
                                                                             "\"id\": \"HB2C\","
                                                                             "\"type\": \"instrument\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    const auto entity = oncat.retrieve("api", "instruments", "HB2C", {{"facility", "HFIR"}});

    TS_ASSERT_EQUALS(entity.id(), std::string("HB2C"));
    TS_ASSERT_EQUALS(entity.get<std::string>("name"), std::string("HB2C"));

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_list_entities() {
    ONCat oncat(DUMMY_URL, make_mock_token_store_already_logged_in(), OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
                DUMMY_CLIENT_ID);

    TS_ASSERT(oncat.isUserLoggedIn());

    auto mock_oncat_api =
        make_mock_oncat_api({{DUMMY_URL + "/api/instruments?facility=HFIR",
                              std::make_pair(InternetHelper::HTTPStatus::OK, "["
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
                                                                             "]")}});

    oncat.setInternetHelper(mock_oncat_api);

    const auto entities = oncat.list("api", "instruments", {{"facility", "HFIR"}});

    TS_ASSERT_EQUALS(entities.size(), 2);
    TS_ASSERT_EQUALS(entities[0].id(), std::string("HB2C"));
    TS_ASSERT_EQUALS(entities[0].get<std::string>("name"), std::string("HB2C"));
    TS_ASSERT_EQUALS(entities[1].id(), std::string("CG1D"));
    TS_ASSERT_EQUALS(entities[1].get<std::string>("name"), std::string("CG1D"));

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_send_api_request_logs_out_with_invalid_grant() {
    ONCat oncat(DUMMY_URL, make_mock_token_store_already_logged_in(), OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
                DUMMY_CLIENT_ID);

    TS_ASSERT(oncat.isUserLoggedIn());

    auto mock_oncat_api = make_mock_oncat_api({{DUMMY_URL + "/api/instruments?facility=HFIR",
                                                std::make_pair(InternetHelper::HTTPStatus::UNAUTHORIZED, "{}")}});

    oncat.setInternetHelper(mock_oncat_api);

    TS_ASSERT_THROWS(oncat.list("api", "instruments", {{"facility", "HFIR"}}), const TokenRejectedError &);
    TS_ASSERT(!oncat.isUserLoggedIn());

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_client_credentials_flow_with_refresh() {
    ONCat oncat(DUMMY_URL, make_mock_token_store(), OAuthFlow::CLIENT_CREDENTIALS, DUMMY_CLIENT_ID,
                std::make_optional<std::string>("9a2ad07a-a139-438b-8116-08c5452f96ad"));

    auto mock_oncat_api = make_mock_oncat_api(
        {{DUMMY_URL + "/oauth/token",
          std::make_pair(InternetHelper::HTTPStatus::OK, "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
                                                         "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
                                                         "\"scope\": \"api:read data:read settings:read\"}")},
         {DUMMY_URL + "/api/instruments/HB2C?facility=HFIR",
          std::make_pair(InternetHelper::HTTPStatus::OK, "{\"facility\": \"HFIR\","
                                                         "\"name\": \"HB2C\","
                                                         "\"id\": \"HB2C\","
                                                         "\"type\": \"instrument\"}")}});

    oncat.setInternetHelper(mock_oncat_api);

    oncat.retrieve("api", "instruments", "HB2C", {{"facility", "HFIR"}});

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());

    mock_oncat_api = make_mock_oncat_api({{
        DUMMY_URL + "/oauth/token",
        std::make_pair(InternetHelper::HTTPStatus::OK, "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
                                                       "\"access_token\": \"987JHGFiusdvs72fAkjhsKJH32tkjk\", "
                                                       "\"scope\": \"api:read data:read settings:read\"}"),
    }});

    oncat.setInternetHelper(mock_oncat_api);

    oncat.refreshTokenIfNeeded(DateAndTime::getCurrentTime() + 3601.0);

    TS_ASSERT(mock_oncat_api->allResponsesCalledOnce());
  }

  void test_config_service_token_store_roundtrip() {
    ConfigServiceTokenStore tokenStore;

    const auto testToken = std::make_optional(
        OAuthToken("Bearer", 3600, "2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ", "api:read data:read settings:read",
                   std::make_optional<std::string>("eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb")));

    tokenStore.setToken(testToken);

    const auto result = tokenStore.getToken();

    TS_ASSERT_EQUALS(testToken->tokenType(), result->tokenType());
    TS_ASSERT_EQUALS(testToken->expiresIn(), result->expiresIn());
    TS_ASSERT_EQUALS(testToken->accessToken(), result->accessToken());
    TS_ASSERT_EQUALS(testToken->scope(), result->scope());
    TS_ASSERT_EQUALS(*testToken->refreshToken(), *result->refreshToken());
  }
};
