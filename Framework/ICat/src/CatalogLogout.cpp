#include "MantidICat/CatalogLogout.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"

namespace Mantid {
namespace ICat {
DECLARE_ALGORITHM(CatalogLogout)

/// Init method to declare algorithm properties
void CatalogLogout::init() {
  declareProperty("Session", std::string(""),
                  "The session information of the catalog to log out. If none "
                  "provided then all catalogs are logged out.",
                  Kernel::Direction::Input);
}

/// execute the algorithm
void CatalogLogout::exec() {
  std::string logoutSession = getPropertyValue("Session");

  // Destroy all sessions if no session provided.
  if (logoutSession.empty())
    API::CatalogManager::Instance().destroyCatalog("");

  auto keepAliveInstances =
      API::AlgorithmManager::Instance().runningInstancesOf("CatalogKeepAlive");

  for (unsigned i = 0; i < keepAliveInstances.size(); ++i) {
    auto keepAliveInstance = API::AlgorithmManager::Instance().getAlgorithm(
        keepAliveInstances.at(i)->getAlgorithmID());

    if (logoutSession ==
        keepAliveInstances.at(i)->getPropertyValue("Session")) {
      keepAliveInstance->cancel();
      API::CatalogManager::Instance().destroyCatalog(logoutSession);
      break;
    } else if (logoutSession.empty()) {
      keepAliveInstance->cancel();
    }
  }
}
}
}
