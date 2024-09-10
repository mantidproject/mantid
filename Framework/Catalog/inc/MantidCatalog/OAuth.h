// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCatalog/DllConfig.h"
#include "MantidKernel/DateAndTime.h"

#include <optional>

namespace Mantid {
namespace Catalog {
namespace OAuth {

using Types::Core::DateAndTime;

/**
 * Classes providing basic Client Credentials / Resource Owner Credentials
 * OAuth functionality.
 *
 * To be used by other cataloging classes and so it should not be necessary
 * to use this directly anywhere else.
 */

enum class OAuthFlow : uint8_t {
  CLIENT_CREDENTIALS,
  RESOURCE_OWNER_CREDENTIALS,
  NONE,
};

class MANTID_CATALOG_DLL OAuthToken {
public:
  OAuthToken() = delete;
  OAuthToken(std::string tokenType, int expiresIn, std::string accessToken, std::string scope,
             std::optional<std::string> refreshToken);
  ~OAuthToken();

  std::string tokenType() const;
  int expiresIn() const;
  std::string accessToken() const;
  std::string scope() const;
  std::optional<std::string> refreshToken() const;

  bool isExpired() const;
  bool isExpired(const DateAndTime &currentTime) const;

  static OAuthToken fromJSONStream(std::istream &tokenStringStream);

private:
  DateAndTime m_expiresAt;

  std::string m_tokenType;
  int m_expiresIn;
  std::string m_accessToken;
  std::string m_scope;
  std::optional<std::string> m_refreshToken;
};

class MANTID_CATALOG_DLL IOAuthTokenStore {
public:
  virtual void setToken(const std::optional<OAuthToken> &token) = 0;
  virtual std::optional<OAuthToken> getToken() = 0;
  virtual ~IOAuthTokenStore() = default;
};

class MANTID_CATALOG_DLL ConfigServiceTokenStore : public IOAuthTokenStore {
public:
  ConfigServiceTokenStore() = default;
  ConfigServiceTokenStore &operator=(const ConfigServiceTokenStore &other) = default;
  ~ConfigServiceTokenStore() override;

  void setToken(const std::optional<OAuthToken> &token) override;
  std::optional<OAuthToken> getToken() override;
};

using IOAuthTokenStore_uptr = std::unique_ptr<IOAuthTokenStore>;
using IOAuthTokenStore_sptr = std::shared_ptr<IOAuthTokenStore>;
using ConfigServiceTokenStore_uptr = std::unique_ptr<ConfigServiceTokenStore>;

} // namespace OAuth
} // namespace Catalog
} // namespace Mantid
