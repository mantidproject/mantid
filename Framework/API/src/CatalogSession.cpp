// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/CatalogSession.h"
#include "MantidKernel/Logger.h"

namespace Mantid::API {
/**
 * Initialise the related catalog session variables.
 * @param sessionID :: The session ID generated from logging into the catalog.
 * @param facility  :: The facility of the catalog the user logged in to.
 * @param endpoint  :: The endpoint of the catalog the user logged in to.
 */
CatalogSession::CatalogSession(std::string sessionID, std::string facility, std::string endpoint)
    : m_sessionID(std::move(sessionID)), m_facility(std::move(facility)), m_endpoint(std::move(endpoint)) {}

/**
 * Obtain the session ID for the catalog created.
 * @return The sesssion Id of the catalog created.
 */
const std::string &CatalogSession::getSessionId() const { return m_sessionID; }

/**
 * Used to clear the session ID on logout.
 * @param sessionID :: The value to set the session id.
 */
void CatalogSession::setSessionId(const std::string &sessionID) { m_sessionID = sessionID; }

/**
 * Obtains the soap end-point of the catalog created.
 * @return The soap end-point used to create the catalog.
 */
const std::string &CatalogSession::getSoapEndpoint() const { return m_endpoint; }

/**
 * Obtain the facility of the catalog created.
 * @return The facility used to create the catalog.
 */
const std::string &CatalogSession::getFacility() const { return m_facility; }
} // namespace Mantid::API
