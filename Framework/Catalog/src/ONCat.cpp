// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCatalog/ONCat.h"
#include "MantidCatalog/Exception.h"
#include "MantidCatalog/OAuth.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"

#include <algorithm>
#include <sstream>

#include <boost/algorithm/string/join.hpp>
#include <utility>

#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPResponse.h>

namespace Mantid::Catalog::ONCat {

using Poco::Net::HTTPResponse;

using Mantid::Catalog::Exception::CatalogError;
using Mantid::Catalog::Exception::InvalidCredentialsError;
using Mantid::Catalog::Exception::InvalidRefreshTokenError;
using Mantid::Catalog::Exception::TokenRejectedError;
using Mantid::Catalog::OAuth::ConfigServiceTokenStore;
using Mantid::Catalog::OAuth::OAuthToken;
using Mantid::Catalog::ONCat::ONCatEntity;
using Mantid::Kernel::Exception::InternetError;

namespace {
Mantid::Kernel::Logger g_log("ONCat");

static const std::string CONFIG_PATH_BASE = "catalog.oncat.";
// It could be argued that this should be read in from Facilities.xml or
// similar, but I will put this off for now as it is unclear how to
// reconcile ONCat's functionality with the current <soapendpoint> /
// <externaldownload> tags in the XML.
static const std::string DEFAULT_ONCAT_URL = "https://oncat.ornl.gov";
static const std::string DEFAULT_CLIENT_ID = "d16ea847-41ce-4b30-9167-40298588e755";
} // namespace

/**
 * Constructs an ONCat object based on various settings gathered from
 * the ConfigService.
 *
 * The resulting object will work with resources that require no
 * authentication at all, or assuming authentication is to be done in one of
 * two possible modes:
 *
 * 1 - User Login Mode (Default)
 * -----------------------------
 *
 * Users must log in with their UCAMS / XCAMS credentials before calls
 * to the ONCat API can be made.  This mode should work "out of the box"
 * (requires no changes to config files), and is the default mode of
 * operation when authenticating.  User access to API information is governed
 * by the same LDAP instance that controls file system access, so users should
 * only see the experiment data they are allowed to see.
 *
 * This mode uses the "Resource Owner Credentials" OAuth flow.
 *
 * 2 - Machine-to-Machine Mode
 * ---------------------------
 *
 * No user login is necessary in this case, but for this mode to be enabled a
 * client ID and secret must exist in the ConfigService.  Recommended practice
 * would be to add the following two entries to the Mantid.local.properties
 * file on the machine to be given access, using the credentials issued by the
 * ONCat administrator:
 *
 *     catalog.oncat.client_id = "[CLIENT ID]"
 *     catalog.oncat.client_secret = "[CLIENT SECRET]"
 *
 * API read access is completely unrestricted in this mode, and so it is
 * intended for autoreduction use cases or similar.
 *
 * This mode uses the "Client Credentials" OAuth flow.
 *
 * @return The constructed ONCat object.
 */
ONCat_uptr ONCat::fromMantidSettings(bool authenticate) {
  if (!authenticate) {
    return std::make_unique<ONCat>(DEFAULT_ONCAT_URL, nullptr, OAuthFlow::NONE, std::nullopt, std::nullopt);
  }

  auto &config = Mantid::Kernel::ConfigService::Instance();
  const auto client_id = config.getString(CONFIG_PATH_BASE + "client_id");
  const auto client_secret = config.getString(CONFIG_PATH_BASE + "client_secret");
  const bool hasClientCredentials = client_id != "" && client_secret != "";

  if (hasClientCredentials) {
    g_log.debug() << "Found client credentials in Mantid.local.properties.  "
                  << "No user login required." << std::endl;
  } else {
    g_log.debug() << "Could not find client credentials in Mantid.local.properties.  "
                  << "Falling back to default -- user login required." << std::endl;
  }

  std::optional<std::string> client_secret_option =
      hasClientCredentials ? std::make_optional(client_secret) : std::nullopt;

  return std::make_unique<ONCat>(DEFAULT_ONCAT_URL, std::make_unique<ConfigServiceTokenStore>(),
                                 hasClientCredentials ? OAuthFlow::CLIENT_CREDENTIALS
                                                      : OAuthFlow::RESOURCE_OWNER_CREDENTIALS,
                                 hasClientCredentials ? client_id : DEFAULT_CLIENT_ID, client_secret_option);
}

ONCat::ONCat(std::string url, IOAuthTokenStore_uptr tokenStore, OAuthFlow flow, std::optional<std::string> clientId,
             std::optional<std::string> clientSecret)
    : m_url(std::move(url)), m_tokenStore(std::move(tokenStore)), m_clientId(std::move(clientId)),
      m_clientSecret(std::move(clientSecret)), m_flow(flow), m_internetHelper(new Mantid::Kernel::InternetHelper()) {}

ONCat::ONCat(const ONCat &other) = default;

ONCat &ONCat::operator=(const ONCat &other) = default;

ONCat::~ONCat() = default;

/**
 * Whether or not a user is currently logged in.  (Not relevant when
 * using machine-to-machine authentication as part of the Client
 * Credentials flow, and not required when accessing unauthenticated
 * parts of the API.)
 *
 * Something to bear in mind is that the term "logged in" is used quite
 * loosely here.  In an OAuth context it roughly equates to, "there is a
 * token stored locally", which is not quite the same thing. This may
 * sound strange, but consider the following:
 *
 * - Tokens expire after a given amount of time, at which point they can
 *   be "refreshed".  A successful token refresh happens behind the
 *   scenes without the user even knowing it took place.
 *
 * - While it is possible to tell when a token needs to be refreshed,
 *   token refreshes are not always successful.  If they fail then the
 *   client must prompt the user to enter their credentials again.
 *
 * - There is no way for the client to know whether or not the refresh
 *   will be successful ahead of time (i.e., whether a token has been
 *   revoked server-side), since the OAuth spec provides no mechanism to
 *   check the validity of a refresh token.
 *
 * - Tokens can be revoked at any time with absolutely no notice as part
 *   of standard OAuth practice.  Also, only a limited number of tokens
 *   can exist for each unique client / user combination at any one
 *   time.
 *
 * Hopefully it is clear that working with OAuth client-side requires
 * you to use an almost-Pythonic "ask for forgiveness rather than for
 * permission" strategy -- i.e., code as if locally-stored tokens can be
 * refreshed, but be ready to prompt the user for their credentials if
 * the refresh fails.
 *
 * Some useful links with related information:
 *
 * - http://qr.ae/TUTke2 (quora.com)
 * - https://stackoverflow.com/a/30826806/778572
 *
 * @param true if a user is "logged in", else false.
 */
bool ONCat::isUserLoggedIn() const {
  if (m_flow == OAuthFlow::NONE || m_flow == OAuthFlow::CLIENT_CREDENTIALS) {
    return false;
  }

  return m_tokenStore->getToken().has_value();
}

const std::string &ONCat::url() const { return m_url; }

void ONCat::logout() {
  // Currently, ONCat OAuth does *not* allow clients to revoke tokens
  // that are no longer needed (though this is defined in the OAtuh
  // spec).  A "logout", then, is simply throwing away whatever token we
  // previously stored client-side.
  if (m_tokenStore) {
    m_tokenStore->setToken(std::nullopt);
    g_log.debug() << "Logging out." << std::endl;
  }
}

/**
 * Log in as part of the Resource Ownder Credentials flow so that
 * authenticated resources may be accessed on behalf of a user.
 *
 * @param username : The XCAMS / UCAMS ID of the user.
 * @param password : The XCAMS / UCAMS password of the user.
 *
 * @exception Mantid::Catalog::Exception::InvalidCredentialsError :
 *   Thrown when the given credentials are not valid.
 */
void ONCat::login(const std::string &username, const std::string &password) {
  if (m_flow != OAuthFlow::RESOURCE_OWNER_CREDENTIALS) {
    g_log.warning() << "Unexpected usage detected!  "
                    << "Logging in with user credentials in not required (and is not "
                    << "supported) unless resource owner credentials are being used." << std::endl;
    return;
  }

  Poco::Net::HTMLForm form(Poco::Net::HTMLForm::ENCODING_MULTIPART);
  form.set("username", username);
  form.set("password", password);
  form.set("client_id", m_clientId.value());
  if (m_clientSecret) {
    form.set("client_secret", m_clientSecret.value());
  }
  form.set("grant_type", "password");

  m_internetHelper->setBody(form);

  try {
    std::stringstream ss;

    const auto statusCode = m_internetHelper->sendRequest(m_url + "/oauth/token", ss);

    if (statusCode == Kernel::InternetHelper::HTTPStatus::OK) {
      m_tokenStore->setToken(OAuthToken::fromJSONStream(ss));
    }

    g_log.debug() << "Login was successful!" << std::endl;
    ;
  } catch (InternetError &ie) {
    if (ie.errorCode() == HTTPResponse::HTTP_UNAUTHORIZED) {
      throw InvalidCredentialsError("Invalid UCAMS / XCAMS credentials used for ONCat login.");
    }
    throw CatalogError(ie.what());
  }
}

/**
 * Retrieve a single entity from the given resource (in the given namespace) of
 * ONCat's API.
 *
 * Please see https://oncat.ornl.gov/#/build for more information about the
 * currently-available resources, and what query parameters they allow.
 *
 * @param identifier :
 *   The ID or name that uniquely identifies the entity.
 * @param resourceNamespace :
 *   The "namespace" of the resource.  The most common, "core" resources all
 *   belong to the "api" namespace.
 * @param resource :
 *   The name of the resource to retrieve the entity from.  I.e., "Datafile"
 *   entities can be retrieved from the "datafiles" resource.
 * @param queryParameters :
 *   The name-value-pair query-string parameters.
 *
 * @return The response from the API in the form on an ONCatEntity object.
 *
 * @exception Mantid::Catalog::Exception::CatalogError
 */
ONCatEntity ONCat::retrieve(const std::string &resourceNamespace, const std::string &resource,
                            const std::string &identifier, const QueryParameters &queryParameters) {
  const auto uri = m_url + "/" + resourceNamespace + "/" + resource + "/" + identifier;
  std::stringstream ss;

  sendAPIRequest(uri, queryParameters, ss);

  return ONCatEntity::fromJSONStream(ss);
}

/**
 * Retrieve a collection of entities from the given resource (in the given
 * namespace) of ONCat's API.
 *
 * Please see retrieve documentation for more info.
 */
std::vector<ONCatEntity> ONCat::list(const std::string &resourceNamespace, const std::string &resource,
                                     const QueryParameters &queryParameters) {
  const auto uri = m_url + "/" + resourceNamespace + "/" + resource;
  std::stringstream ss;

  sendAPIRequest(uri, queryParameters, ss);

  return ONCatEntity::vectorFromJSONStream(ss);
}

/**
 * Refresh the current token if it has expired (and if it actually exists).
 *
 * To be called behind-the-scenes before each API query, so that we know
 * our tokens are up-to-date before being used.
 *
 * @exception Mantid::Catalog::Exception::InvalidCredentialsError :
 *   Thrown when the provider decides the current token cannot be refreshed.
 */
void ONCat::refreshTokenIfNeeded() { refreshTokenIfNeeded(DateAndTime::getCurrentTime()); }

/**
 * See overloaded method.
 *
 * @param currentTime : Used in testing to specify a different time.
 *
 * @exception Mantid::Catalog::Exception::InvalidRefreshTokenError :
 *   Thrown when the provider decides the current token cannot be refreshed.
 */
void ONCat::refreshTokenIfNeeded(const DateAndTime &currentTime) {
  if (m_flow == OAuthFlow::NONE) {
    return;
  }

  const auto currentToken = m_tokenStore->getToken();

  if (m_flow == OAuthFlow::CLIENT_CREDENTIALS) {
    if (currentToken && !currentToken->isExpired(currentTime)) {
      return;
    }

    Poco::Net::HTMLForm form(Poco::Net::HTMLForm::ENCODING_MULTIPART);
    form.set("client_id", m_clientId.value());
    if (m_clientSecret) {
      form.set("client_secret", m_clientSecret.value());
    }
    form.set("grant_type", "client_credentials");

    m_internetHelper->reset();
    m_internetHelper->setBody(form);

    try {
      std::stringstream ss;

      const auto statusCode = m_internetHelper->sendRequest(m_url + "/oauth/token", ss);

      if (statusCode == Kernel::InternetHelper::HTTPStatus::OK) {
        m_tokenStore->setToken(OAuthToken::fromJSONStream(ss));
      }
      g_log.debug() << "Token successfully refreshed." << std::endl;
    } catch (InternetError &ie) {
      throw CatalogError(ie.what());
    }
  } else if (m_flow == OAuthFlow::RESOURCE_OWNER_CREDENTIALS) {
    if (!currentToken.has_value()) {
      return;
    }
    if (!currentToken->isExpired(currentTime)) {
      return;
    }
    const auto currentRefreshToken = currentToken->refreshToken();
    if (!currentRefreshToken) {
      return;
    }

    Poco::Net::HTMLForm form(Poco::Net::HTMLForm::ENCODING_MULTIPART);
    form.set("client_id", m_clientId.value());
    if (m_clientSecret) {
      form.set("client_secret", m_clientSecret.value());
    }
    form.set("grant_type", "refresh_token");
    form.set("refresh_token", currentRefreshToken.value());

    m_internetHelper->reset();
    m_internetHelper->setBody(form);

    try {
      std::stringstream ss;

      const auto statusCode = m_internetHelper->sendRequest(m_url + "/oauth/token", ss);

      if (statusCode == Kernel::InternetHelper::HTTPStatus::OK) {
        m_tokenStore->setToken(OAuthToken::fromJSONStream(ss));
      }
      g_log.debug() << "Token successfully refreshed." << std::endl;
    } catch (InternetError &ie) {
      if (ie.errorCode() == HTTPResponse::HTTP_UNAUTHORIZED) {
        // As per OAuth spec, when a refresh token is no longer valid, we
        // can consider ourselves logged out.
        logout();
        throw InvalidRefreshTokenError("You have been logged out.  Please login again.");
      }
      throw CatalogError(ie.what());
    }
  }
}

void ONCat::setInternetHelper(const std::shared_ptr<Mantid::Kernel::InternetHelper> &internetHelper) {
  m_internetHelper = internetHelper;
}

void ONCat::sendAPIRequest(const std::string &uri, const QueryParameters &queryParameters, std::ostream &response) {
  refreshTokenIfNeeded();

  m_internetHelper->clearHeaders();
  m_internetHelper->setMethod("GET");

  if (m_flow != OAuthFlow::NONE) {
    const auto tokenType = m_tokenStore->getToken()->tokenType();
    const auto accessToken = m_tokenStore->getToken()->accessToken();

    m_internetHelper->addHeader("Authorization", tokenType + " " + accessToken);
  }

  std::vector<std::string> queryStringParts(queryParameters.size());
  std::transform(queryParameters.begin(), queryParameters.end(), queryStringParts.begin(),
                 [](const QueryParameter &queryParameter) -> std::string {
                   return queryParameter.first + "=" + queryParameter.second;
                 });
  const auto queryString = boost::algorithm::join(queryStringParts, "&");
  const auto requestUrl = queryString.size() == 0 ? uri : uri + "?" + queryString;

  g_log.debug() << "About to make a call to the following ONCat URL: " << requestUrl;

  try {
    m_internetHelper->sendRequest(requestUrl, response);
  } catch (InternetError &ie) {
    if (ie.errorCode() == HTTPResponse::HTTP_UNAUTHORIZED) {
      std::string errorMessage;
      switch (m_flow) {
      case OAuthFlow::RESOURCE_OWNER_CREDENTIALS:
        errorMessage = "You have been logged out.  Please login again.";
        break;
      case OAuthFlow::CLIENT_CREDENTIALS:
        errorMessage = "The stored OAuth token appears to be invalid.  "
                       "There are a few cases where this might be expected, but in "
                       "principle this should rarely happen.  "
                       "Please try again and if the problem persists contact the "
                       "ONCat administrator at oncat-support@ornl.gov.";
        break;
      case OAuthFlow::NONE:
        assert(false);
        break;
      }
      // The ONCat API does *not* leak information in the case where a
      // resource exists but a user is not allowed access -- a 404 would
      // always be returned instead.  So, if we ever get a 401, it is
      // because our locally-stored token is no longer valid and we
      // should log out.
      logout();
      throw TokenRejectedError(errorMessage);
    }
    throw CatalogError(ie.what());
  }
}

} // namespace Mantid::Catalog::ONCat
