// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCatalog/DllConfig.h"
#include "MantidCatalog/OAuth.h"
#include "MantidCatalog/ONCatEntity.h"
#include "MantidKernel/DateAndTime.h"

#include <memory>
#include <vector>

#include <optional>

namespace Mantid {

namespace Kernel {
class InternetHelper;
}

namespace Catalog {
namespace ONCat {

class ONCat;
using ONCat_uptr = std::unique_ptr<ONCat>;

using Mantid::Catalog::OAuth::IOAuthTokenStore_sptr;
using Mantid::Catalog::OAuth::IOAuthTokenStore_uptr;
using Mantid::Catalog::OAuth::OAuthFlow;
using Types::Core::DateAndTime;

// Here we use a vector of pairs rather than a map because we would like
// the ability to set a parameter with a given name more than once --
// this denotes an arrayed parameter.
using QueryParameter = std::pair<std::string, std::string>;
using QueryParameters = std::vector<QueryParameter>;

/**
 * The main class to be used when interacting with ONCat from C++.  It can
 * be used to retrieve "entities" from REST-like "resources".  Please refer
 * to the API documentation at https://oncat.ornl.gov/ for more information
 * about each resource.
 *
 * Rather than use constructors, the helper method ONCat::fromMantidSettings()
 * is strongly recommended.  This will create an ONCat object taking into
 * account the settings configured in the currently-running instance of Mantid.
 *
 * Creation of an ONCat object can be done as follows:
 *
 *     auto oncat = ONCat::fromMantidSettings(true);
 *
 * Or, when *unauthenticated* access is preferred, as follows:
 *
 *     auto oncat = ONCat::fromMantidSettings();
 *
 * Once you have that, logging in either assumes that a client ID and
 * client secret have been added to the Mantid.local.properties file (this
 * essentially allows machine-to-machine authentication for a use case like
 * auto-reduction, and there is no explicit "login" step), or that you are able
 * to pronpt a user for their ORNL XCAMS / UCAMS username and password, or that
 * you will only be accessing resources in an unauthenticated manner.  If an
 * explicit login step is necessary, it should look something like this:
 *
 *     oncat.login("some_user", "a_password");
 *
 * From then on, basic usage is as per the following example:
 *
 *     // Get a list of the experiments for NOMAD, specifying the fields
 *     // we are interested in as a "projection".
 *     const auto nomadExperiments = oncat.list("api", "experiments", {
 *         {"facility", "SNS"},
 *         {"instrument", "NOM"},
 *         {"projection", "name"},
 *         {"projection", "size"}
 *     });
 *
 *     // Print out the IPTS numbers of each one.
 *     for (const auto & experiment : nomadExperiments) {
 *       std::cout
 *         << *experiment.get<std::string>("name") << " has "
 *         << *experiment.get<int>("size") << " ingested datafiles.";
 *     }
 *
 * For logged-in users, no further credential prompting should be required
 * as part of the standard workflow, although you should be prepared for
 * an authenticated user to have their tokens invalidated *eventually*, as
 * refresh tokens will *not* last forever (and may eventually be set to expire
 * every 12 hours or so).  Once tokens are expired, any call to the API will
 * fail, and an error will be written to the log asking the user to login
 # again.
 *
 * @author Peter Parker
 * @date 2018
*/
class MANTID_CATALOG_DLL ONCat {
public:
  static ONCat_uptr fromMantidSettings(bool authenticate = false);

  ONCat() = delete;
  ONCat(const ONCat &other);
  ONCat &operator=(const ONCat &other);
  ~ONCat();

  bool isUserLoggedIn() const;
  const std::string &url() const;

  void login(const std::string &username, const std::string &password);
  void logout();

  ONCatEntity retrieve(const std::string &resourceNamespace, const std::string &resource, const std::string &identifier,
                       const QueryParameters &queryParameters);
  std::vector<ONCatEntity> list(const std::string &resourceNamespace, const std::string &resource,
                                const QueryParameters &queryParameters);

  //////////////////////////////////////////////////////////////////////
  // Exposed publicly for testing purposes only.
  //////////////////////////////////////////////////////////////////////
  ONCat(const std::string &url);
  ONCat(std::string url, IOAuthTokenStore_uptr tokenStore, OAuthFlow flow, std::optional<std::string> clientId,
        std::optional<std::string> clientSecret = std::nullopt);
  void refreshTokenIfNeeded();
  void refreshTokenIfNeeded(const DateAndTime &currentTime);
  void setInternetHelper(const std::shared_ptr<Mantid::Kernel::InternetHelper> &internetHelper);
  //////////////////////////////////////////////////////////////////////

private:
  void sendAPIRequest(const std::string &uri, const QueryParameters &queryParameters, std::ostream &response);

  std::string m_url;
  IOAuthTokenStore_sptr m_tokenStore;
  std::optional<std::string> m_clientId;
  std::optional<std::string> m_clientSecret;

  OAuthFlow m_flow;
  std::shared_ptr<Mantid::Kernel::InternetHelper> m_internetHelper;
};

} // namespace ONCat
} // namespace Catalog
} // namespace Mantid
