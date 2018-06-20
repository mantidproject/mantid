#ifndef MANTID_CATALOG_ONCAT_H_
#define MANTID_CATALOG_ONCAT_H_

#include "MantidCatalog/DllConfig.h"
#include "MantidCatalog/OAuth.h"
#include "MantidCatalog/ONCatEntity.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/make_unique.h"

#include <vector>

#include <boost/optional.hpp>

namespace Mantid {

namespace Kernel {
class InternetHelper;
}

namespace Catalog {
namespace ONCat {

using Mantid::Catalog::OAuth::OAuthFlow;
using Mantid::Catalog::OAuth::IOAuthTokenStore_uptr;
using Mantid::Catalog::OAuth::IOAuthTokenStore_sptr;
using Types::Core::DateAndTime;

// Here we use a vector of pairs rather than a map because we would like
// the ability to set a parameter with a given name more than once --
// this denotes an arrayed parameter.
using QueryParameter = std::pair<std::string, std::string>;
using QueryParameters = std::vector<QueryParameter>;

/**
  The main class to be used when interacting with ONCat from C++.  It can
  be used to retrieve "entities" from REST-like "resources".  Please refer
  to the API documentation at https://oncat.ornl.gov/#/build for more
  information about each resource.

  Rather than use constructors, the helper method ONCat::fromMantidSettings()
  is strongly recommended.  This will create an ONCat object taking into
  account the authentication settings configured in a given instance of
  Mantid.

  Creation of an ONCat object can be done as follows:

      auto oncat = ONCat::fromMantidSettings();

  Once you have that, logging in either assumes that a client ID and
  client secret have been added to the Mantid.local.properties file (this
  essentially allows machine-to-machine authentication for a use case like
  auto-reduction, and there is no explicit "login" step), or that a user
  is somehow prompted for their ORNL XCAMS / UCAMS username and password,
  or that you will only be accessing unauthenticated resources.  If an
  explicit login step is necessary it should look something like this:

      oncat.login("some_user", "a_password");

  From then on, basic usage is as follows:

      // Get a list of the experiments for NOMAD, specifying the fields
      // we are interested in as a "projection".
      const auto nomadExperiments = oncat.list("api", "experiments", {
        QueryParameter("facility", "SNS"),
        QueryParameter("instrument", "NOM"),
        QueryParameter("projection", "name"),
        QueryParameter("projection", "size")
      });

      // Print out the IPTS numbers of each one.
      for (const auto & experiment : nomadExperiments) {
        std::cout
          << *experiment.asString("name") << " has "
          << *experiment.asInt("size") << " ingested datafiles.";
      }

  For logged in users, no further credential prompting should be required
  as part of the standard workflow, although you should be prepared for
  an authenticated user to have their tokens invalidated  *eventually*.
  At that point, any call to the API will fail, and an error will be written
  to the log asking them to login again.

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
class MANTID_CATALOG_DLL ONCat {
public:
  static ONCat fromMantidSettings();

  ONCat() = delete;
  ONCat(const ONCat &other);
  ~ONCat();

  bool isUserLoggedIn() const;

  void login(const std::string &username, const std::string &password);
  void logout();

  ONCatEntity retrieve(const std::string &resourceNamespace,
                       const std::string &resource,
                       const std::string &identifier,
                       const QueryParameters &queryParameters);
  std::vector<ONCatEntity> list(const std::string &resourceNamespace,
                                const std::string &resource,
                                const QueryParameters &queryParameters);

  //////////////////////////////////////////////////////////////////////
  // Exposed publicly for testing purposes only.
  //////////////////////////////////////////////////////////////////////
  ONCat(const std::string &url, IOAuthTokenStore_uptr tokenStore,
        OAuthFlow flow, const std::string &clientId,
        const boost::optional<std::string> &clientSecret = boost::none);
  void refreshTokenIfNeeded();
  void refreshTokenIfNeeded(const DateAndTime &currentTime);
  void setInternetHelper(
      std::unique_ptr<Mantid::Kernel::InternetHelper> internetHelper);
  //////////////////////////////////////////////////////////////////////

private:
  void sendAPIRequest(const std::string &uri,
                      const QueryParameters &queryParameters,
                      std::ostream &response);

  std::string m_url;
  IOAuthTokenStore_sptr m_tokenStore;
  std::string m_clientId;
  boost::optional<std::string> m_clientSecret;

  OAuthFlow m_flow;
  std::unique_ptr<Mantid::Kernel::InternetHelper> m_internetHelper;
};

} // namespace ONCat
} // namespace Catalog
} // namespace Mantid

#endif /* MANTID_CATALOG_ONCAT_H_ */
