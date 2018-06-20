#include "MantidCatalog/OAuth.h"
#include "MantidCatalog/Exception.h"
#include "MantidKernel/ConfigService.h"

#include <sstream>

#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPResponse.h>

#include <json/json.h>

namespace Mantid {
namespace Catalog {
namespace OAuth {

using Mantid::Catalog::Exception::TokenParsingError;

//----------------------------------------------------------------------
// OAuthToken
//----------------------------------------------------------------------

OAuthToken::OAuthToken(
  const std::string & tokenType,
  int expiresIn,
  const std::string & accessToken,
  const std::string & scope,
  const boost::optional<std::string> & refreshToken
) :
  m_expiresAt(DateAndTime::getCurrentTime() + static_cast<double>(expiresIn)),
  m_tokenType(tokenType),
  m_expiresIn(expiresIn),
  m_accessToken(accessToken),
  m_scope(scope),
  m_refreshToken(refreshToken) {}

OAuthToken::~OAuthToken() {}

OAuthToken OAuthToken::fromJSONStream(
  std::istream & tokenStringStream
) {
  try {
    Json::Value full_token;
    tokenStringStream >> full_token;
    
    const auto tokenType = full_token["token_type"].asString();
    const auto expiresIn =
      static_cast<unsigned int>(full_token["expires_in"].asUInt());
    const auto accessToken = full_token["access_token"].asString();
    const auto scope = full_token["scope"].asString();

    const auto parsedRefreshToken = full_token["refresh_token"].asString();
    const boost::optional<std::string> refreshToken =
      parsedRefreshToken == "" ?
      boost::none :
      boost::optional<std::string>(parsedRefreshToken);

    return OAuthToken(
      tokenType,
      expiresIn,
      accessToken,
      scope,
      refreshToken
    );
  } catch (...) {
    throw TokenParsingError(
      "Unable to parse authentication token!"
    );
  }
}

bool OAuthToken::isExpired() const {
  return isExpired(DateAndTime::getCurrentTime());
}

bool OAuthToken::isExpired(const DateAndTime & currentTime) const {
  return currentTime > m_expiresAt;
}

std::string OAuthToken::tokenType() const {
  return m_tokenType;
}

int OAuthToken::expiresIn() const {
  return m_expiresIn;
}

std::string OAuthToken::accessToken() const {
  return m_accessToken;
}

std::string OAuthToken::scope() const {
  return m_scope;
}

boost::optional<std::string> OAuthToken::refreshToken() const {
  return m_refreshToken;
}

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
    auto & config = Mantid::Kernel::ConfigService::Instance();
    config.saveConfig(config.getUserFilename());
  } catch (...) {
    // It's not the end of the world if there was an error persisting
    // the token (the worst that could happen is a user has to login
    // again), but it *is* the end of the world if we seg fault.
  }
}

void ConfigServiceTokenStore::setToken(
  const boost::optional<OAuthToken> & token
) {
  auto & config = Mantid::Kernel::ConfigService::Instance();

  if (token) {
    config.setString(CONFIG_PATH_BASE + "tokenType", token->tokenType());
    config.setString(
      CONFIG_PATH_BASE + "expiresIn",
      std::to_string(token->expiresIn())
    );
    config.setString(CONFIG_PATH_BASE + "accessToken", token->accessToken());
    config.setString(CONFIG_PATH_BASE + "scope", token->scope());
    config.setString(
      CONFIG_PATH_BASE + "refreshToken",
      token->refreshToken() ? *token->refreshToken() : std::string("")
    );
  } else {
    config.setString(CONFIG_PATH_BASE + "tokenType", "");
    config.setString(CONFIG_PATH_BASE + "expiresIn", "");
    config.setString(CONFIG_PATH_BASE + "accessToken", "");
    config.setString(CONFIG_PATH_BASE + "scope", "");
    config.setString(CONFIG_PATH_BASE + "refreshToken", "");
  }
}

boost::optional<OAuthToken> ConfigServiceTokenStore::getToken() {
  auto & config = Mantid::Kernel::ConfigService::Instance();

  const auto tokenType = config.getString(CONFIG_PATH_BASE + "tokenType");
  const auto expiresIn = config.getString(CONFIG_PATH_BASE + "expiresIn");
  const auto accessToken = config.getString(
    CONFIG_PATH_BASE + "accessToken"
  );
  const auto scope = config.getString(CONFIG_PATH_BASE + "scope");
  const auto refreshToken = config.getString(
    CONFIG_PATH_BASE + "refreshToken"
  );

  // A partially written-out token is useless and is therefore
  // effectively the same as a token not having been written out at
  // all.  So, it's all or nothing (excluding the refresh token of
  // course, which is not present for all OAuth flows).
  if (tokenType == "" || expiresIn == "" || accessToken == "" || scope == "") {
    return boost::none;
  }

  try {
    return boost::make_optional(OAuthToken(
      tokenType,
      std::stoi(expiresIn),
      accessToken,
      scope,
      boost::make_optional(refreshToken != "", refreshToken)
    ));
  } catch (std::invalid_argument &) {
    // Catching any std::stoi failures silently -- a malformed token is
    // useless and may as well not be there.
  }

  return boost::none;
}

} // namespace OAuth
} // namespace Catalog
} // namespace Mantid
