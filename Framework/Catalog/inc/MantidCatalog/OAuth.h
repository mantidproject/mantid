#ifndef MANTID_CATALOG_OAUTH_H_
#define MANTID_CATALOG_OAUTH_H_

#include "MantidCatalog/DllConfig.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/DateAndTime.h"

#include <boost/optional.hpp>

namespace Mantid {
namespace Catalog {
namespace OAuth {

using Types::Core::DateAndTime;

/**
  Classes providing basic Client Credentials / Resource Owner Credentials
  OAuth functionality.

  To be used by other cataloging classes and so it should not be necessary
  to use this directly anywhere else.

  @author Peter Parker
  @date 2018

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

enum class OAuthFlow : uint8_t {
  CLIENT_CREDENTIALS,
  RESOURCE_OWNER_CREDENTIALS
};

class MANTID_CATALOG_DLL OAuthToken {
public:
  OAuthToken() = delete;
  OAuthToken(
    const std::string & tokenType,
    int expiresIn,
    const std::string & accessToken,
    const std::string & scope,
    const boost::optional<std::string> & refreshToken
  );
  ~OAuthToken();

  std::string tokenType() const;
  int expiresIn() const;
  std::string accessToken() const;
  std::string scope() const;
  boost::optional<std::string> refreshToken() const;

  bool isExpired() const;
  bool isExpired(const DateAndTime & currentTime) const;

  static OAuthToken fromJSONStream(std::istream & tokenStringStream);

private:
  DateAndTime m_expiresAt;

  std::string m_tokenType;
  int m_expiresIn;
  std::string m_accessToken;
  std::string m_scope;
  boost::optional<std::string> m_refreshToken;
};

class MANTID_CATALOG_DLL IOAuthTokenStore {
public:
  virtual void setToken(const boost::optional<OAuthToken> & token) = 0;
  virtual boost::optional<OAuthToken> getToken() = 0;
};

class MANTID_CATALOG_DLL ConfigServiceTokenStore : public IOAuthTokenStore {
public:
  ConfigServiceTokenStore() = default;
  ConfigServiceTokenStore & operator=(
    const ConfigServiceTokenStore& other
  ) = default;
  ~ConfigServiceTokenStore();

  void setToken(const boost::optional<OAuthToken> & token) override;
  boost::optional<OAuthToken> getToken() override;
};

using IOAuthTokenStore_uptr = std::unique_ptr<IOAuthTokenStore>;
using IOAuthTokenStore_sptr = std::shared_ptr<IOAuthTokenStore>;
using ConfigServiceTokenStore_uptr = std::unique_ptr<ConfigServiceTokenStore>;

} // namespace OAuth
} // namespace Catalog
} // namespace Mantid

#endif /* MANTID_CATALOG_OAUTH_H_ */
