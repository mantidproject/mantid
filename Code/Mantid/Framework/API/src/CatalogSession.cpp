#include "MantidAPI/CatalogSession.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {
/**
 * Initialise the related catalog session variables.
 * @param sessionID :: The session ID generated from logging into the catalog.
 * @param facility  :: The facility of the catalog the user logged in to.
 * @param endpoint  :: The endpoint of the catalog the user logged in to.
 */
CatalogSession::CatalogSession(const std::string &sessionID,
                               const std::string &facility,
                               const std::string &endpoint)
    : m_sessionID(sessionID), m_facility(facility), m_endpoint(endpoint) {}

/**
 * Obtain the session ID for the catalog created.
 * @return The sesssion Id of the catalog created.
 */
std::string CatalogSession::getSessionId() const { return m_sessionID; }

/**
 * Used to clear the session ID on logout.
 * @param sessionID :: The value to set the session id.
 */
void CatalogSession::setSessionId(const std::string &sessionID) {
  m_sessionID = sessionID;
}

/**
 * Obtains the soap end-point of the catalog created.
 * @return The soap end-point used to create the catalog.
 */
const std::string &CatalogSession::getSoapEndpoint() const {
  return m_endpoint;
}

/**
 * Obtain the facility of the catalog created.
 * @return The facility used to create the catalog.
 */
const std::string &CatalogSession::getFacility() const { return m_facility; }
}
}
