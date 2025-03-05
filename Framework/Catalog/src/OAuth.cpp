// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCatalog/OAuth.h"
#include "MantidCatalog/Exception.h"
#include "MantidKernel/ConfigService.h"

#include <sstream>
#include <utility>

#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPResponse.h>

#include <json/json.h>

namespace Mantid::Catalog::OAuth {

using Mantid::Catalog::Exception::TokenParsingError;

//----------------------------------------------------------------------
// OAuthToken
//----------------------------------------------------------------------

OAuthToken::OAuthToken(std::string tokenType, int expiresIn, std::string accessToken, std::string scope,
                       std::optional<std::string> refreshToken)
    : m_expiresAt(DateAndTime::getCurrentTime() + static_cast<double>(expiresIn)), m_tokenType(std::move(tokenType)),
      m_expiresIn(expiresIn), m_accessToken(std::move(accessToken)), m_scope(std::move(scope)),
      m_refreshToken(std::move(refreshToken)) {}

OAuthToken::~OAuthToken() = default;

OAuthToken OAuthToken::fromJSONStream(std::istream &tokenStringStream) {
  try {
    Json::Value full_token;
    tokenStringStream >> full_token;

    const auto tokenTypeStr = full_token["token_type"].asString();
    const auto expiresInUint = static_cast<unsigned int>(full_token["expires_in"].asUInt());
    const auto accessTokenStr = full_token["access_token"].asString();
    const auto scopeStr = full_token["scope"].asString();

    const auto parsedRefreshToken = full_token["refresh_token"].asString();
    const std::optional<std::string> refreshTokenStr =
        parsedRefreshToken == "" ? std::nullopt : std::optional<std::string>(parsedRefreshToken);

    return OAuthToken(tokenTypeStr, expiresInUint, accessTokenStr, scopeStr, refreshTokenStr);
  } catch (...) {
    throw TokenParsingError("Unable to parse authentication token!");
  }
}

bool OAuthToken::isExpired() const { return isExpired(DateAndTime::getCurrentTime()); }

bool OAuthToken::isExpired(const DateAndTime &currentTime) const { return currentTime > m_expiresAt; }

const std::string &OAuthToken::tokenType() const { return m_tokenType; }

int OAuthToken::expiresIn() const { return m_expiresIn; }

const std::string &OAuthToken::accessToken() const { return m_accessToken; }

const std::string &OAuthToken::scope() const { return m_scope; }

std::optional<std::string> OAuthToken::refreshToken() const { return m_refreshToken; }

//----------------------------------------------------------------------
// ConfigServiceTokenStore
//----------------------------------------------------------------------

namespace {
static const std::string CONFIG_PATH_BASE = "catalog.oncat.token.";
}

ConfigServiceTokenStore::~ConfigServiceTokenStore() {
  try {
    // Here we attempt to persist our OAuth token to disk before the
    // up-until-now only-in-memory token store is destroyed.
    //
    // I don't believe this is a great solution.  Some things to
    // consider:
    //
    // * We have to save the *entire* contents of the config.  This may
    //   not be desirable.
    // * Ideally we would persist on every token set.
    auto &config = Mantid::Kernel::ConfigService::Instance();
    config.saveConfig(config.getUserFilename());
  } catch (...) {
    // It's not the end of the world if there was an error persisting
    // the token (the worst that could happen is a user has to login
    // again), but it *is* the end of the world if we seg fault.
  }
}

void ConfigServiceTokenStore::setToken(const std::optional<OAuthToken> &token) {
  auto &config = Mantid::Kernel::ConfigService::Instance();

  if (token) {
    config.setString(CONFIG_PATH_BASE + "tokenType", token->tokenType());
    config.setString(CONFIG_PATH_BASE + "expiresIn", std::to_string(token->expiresIn()));
    config.setString(CONFIG_PATH_BASE + "accessToken", token->accessToken());
    config.setString(CONFIG_PATH_BASE + "scope", token->scope());
    config.setString(CONFIG_PATH_BASE + "refreshToken",
                     token->refreshToken() ? *token->refreshToken() : std::string(""));
  } else {
    config.setString(CONFIG_PATH_BASE + "tokenType", "");
    config.setString(CONFIG_PATH_BASE + "expiresIn", "");
    config.setString(CONFIG_PATH_BASE + "accessToken", "");
    config.setString(CONFIG_PATH_BASE + "scope", "");
    config.setString(CONFIG_PATH_BASE + "refreshToken", "");
  }
}

std::optional<OAuthToken> ConfigServiceTokenStore::getToken() {
  auto &config = Mantid::Kernel::ConfigService::Instance();

  const auto tokenType = config.getString(CONFIG_PATH_BASE + "tokenType");
  const auto expiresIn = config.getString(CONFIG_PATH_BASE + "expiresIn");
  const auto accessToken = config.getString(CONFIG_PATH_BASE + "accessToken");
  const auto scope = config.getString(CONFIG_PATH_BASE + "scope");
  const auto refreshToken = config.getString(CONFIG_PATH_BASE + "refreshToken");

  // A partially written-out token is useless and is therefore
  // effectively the same as a token not having been written out at
  // all.  So, it's all or nothing (excluding the refresh token of
  // course, which is not present for all OAuth flows).
  if (tokenType.empty() || expiresIn.empty() || accessToken.empty() || scope.empty()) {
    return std::nullopt;
  }

  try {
    std::optional<std::string> refreshTokenOption =
        refreshToken.empty() ? std::nullopt : std::make_optional<std::string>(refreshToken);

    return std::make_optional(OAuthToken(tokenType, std::stoi(expiresIn), accessToken, scope, refreshTokenOption));
  } catch (std::invalid_argument &) {
    // Catching any std::stoi failures silently -- a malformed token is
    // useless and may as well not be there.
  }

  return std::nullopt;
}

} // namespace Mantid::Catalog::OAuth
