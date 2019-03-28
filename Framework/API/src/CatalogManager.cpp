// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/CompositeCatalog.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

#include <boost/make_shared.hpp>
#include <map>

namespace Mantid {
namespace API {
/**
 * Logs the user into the catalog if session details are valid.
 * This is used here as we need to obtain the session for a specific catalog
 * (e.g. the one created on login).
 * @param username :: The login name of the user.
 * @param password :: The password of the user.
 * @param endpoint :: The endpoint url of the catalog to log in to.
 * @param facility :: The facility of the catalog to log in to.
 * @return The session created if login was successful.
 */
CatalogSession_sptr CatalogManagerImpl::login(const std::string &username,
                                              const std::string &password,
                                              const std::string &endpoint,
                                              const std::string &facility) {
  std::string className = Kernel::ConfigService::Instance()
                              .getFacility(facility)
                              .catalogInfo()
                              .catalogName();
  auto catalog = CatalogFactory::Instance().create(className);
  CatalogSession_sptr session =
      catalog->login(username, password, endpoint, facility);
  // Creates a new catalog and adds it to the compositeCatalog and activeCatalog
  // list.
  m_activeCatalogs.emplace(session, catalog);
  return session;
}

/**
 * Obtain a specific catalog using the sessionID, otherwise return all active
 * catalogs.
 * @param sessionID :: The session to search for in the active catalogs list.
 * @return A specific catalog using the sessionID, otherwise returns all active
 * catalogs
 */
ICatalog_sptr CatalogManagerImpl::getCatalog(const std::string &sessionID) {
  // Checks if a user is logged into the catalog. Inform the user if they are
  // not.
  if (m_activeCatalogs.empty())
    throw std::runtime_error("You are not currently logged into a catalog.");

  if (sessionID.empty()) {
    auto composite = boost::make_shared<CompositeCatalog>();
    for (auto &activeCatalog : m_activeCatalogs) {
      composite->add(activeCatalog.second);
    }
    return composite;
  }

  const auto found =
      std::find_if(m_activeCatalogs.cbegin(), m_activeCatalogs.cend(),
                   [&sessionID](const auto &catalog) {
                     return catalog.first->getSessionId() == sessionID;
                   });
  if (found == m_activeCatalogs.cend()) {
    throw std::runtime_error("The session ID you have provided is invalid.");
  }
  return found->second;
}

/**
 * Destroy and remove a specific catalog from the active catalogs list and the
 * composite catalog.
 * If sessionID is empty then all catalogs are removed from the active catalogs
 * list.
 * @param sessionID :: The session to search for in the active catalogs list.
 */
void CatalogManagerImpl::destroyCatalog(const std::string &sessionID) {
  if (sessionID.empty()) {
    for (auto &activeCatalog : m_activeCatalogs) {
      activeCatalog.second->logout();
    }
    m_activeCatalogs.clear();
  }

  for (auto iter = m_activeCatalogs.begin(); iter != m_activeCatalogs.end();) {
    if (sessionID == iter->first->getSessionId()) {
      iter->second->logout();
      m_activeCatalogs.erase(iter);
      return;
    }
  }
}

/**
 * Obtains a list of the current active catalog sessions.
 * @return A list of active catalog sessions.
 */
std::vector<CatalogSession_sptr> CatalogManagerImpl::getActiveSessions() {
  std::vector<CatalogSession_sptr> sessions;
  sessions.reserve(m_activeCatalogs.size());
  for (const auto &activeCatalog : m_activeCatalogs) {
    sessions.emplace_back(activeCatalog.first);
  }
  return sessions;
}

/// @returns An unsigned value indicating the number of active sessions
size_t CatalogManagerImpl::numberActiveSessions() const {
  return m_activeCatalogs.size();
}
} // namespace API
} // namespace Mantid
