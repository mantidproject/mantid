#include "MantidAPI/CompositeCatalog.h"

namespace Mantid {
namespace API {
CompositeCatalog::CompositeCatalog() : m_catalogs() {}

/**
 * Add a catalog to the catalog container.
 * @param catalog :: The catalog to add to the container.
 */
void CompositeCatalog::add(const ICatalog_sptr catalog) {
  m_catalogs.push_back(catalog);
}

/**
 * Authenticate the user against all catalogues in the container.
 * @param username :: The login name of the user.
 * @param password :: The password of the user.
 * @param endpoint :: The endpoint url of the catalog to log in to.
 * @param facility :: The facility of the catalog to log in to.
 */
CatalogSession_sptr CompositeCatalog::login(const std::string &username,
                                            const std::string &password,
                                            const std::string &endpoint,
                                            const std::string &facility) {
  UNUSED_ARG(username);
  UNUSED_ARG(password);
  UNUSED_ARG(endpoint);
  UNUSED_ARG(facility);
  throw std::runtime_error(
      "You cannot log into multiple catalogs at the same time.");
}

/**
 * Log the user out of all catalogues in the container.
 */
void CompositeCatalog::logout() {
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    (*catalog)->logout();
  }
}

/**
 * Search through all catalogues that are in the container.
 * @param inputs   :: A reference to a class containing the user's inputs.
 * @param outputws :: A shared pointer to workspace were the search results are
 * stored.
 * @param offset   :: Skip this many rows and start returning rows from this
 * point.
 * @param limit    :: The limit of the number of rows returned by the query.
 */
void CompositeCatalog::search(const ICat::CatalogSearchParam &inputs,
                              ITableWorkspace_sptr &outputws, const int &offset,
                              const int &limit) {
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    (*catalog)->search(inputs, outputws, offset, limit);
  }
}

/**
 * Obtain the number of investigations to be returned by the catalog.
 * @return The number of investigations from the search performed.
 */
int64_t CompositeCatalog::getNumberOfSearchResults(
    const ICat::CatalogSearchParam &inputs) {
  int64_t numberOfSearchResults = 0;
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    numberOfSearchResults += (*catalog)->getNumberOfSearchResults(inputs);
  }
  return numberOfSearchResults;
}

/**
 * Obtain and save the investigations that the user is an investigator of within
 * each catalog.
 * @param outputws :: The workspace to store the results.
 */
void CompositeCatalog::myData(ITableWorkspace_sptr &outputws) {
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    (*catalog)->myData(outputws);
  }
}

/**
 * Obtain and save the datasets for a given investigation for each catalog in
 * the container.
 * @param investigationId :: A unique identifier of the investigation.
 * @param outputws        :: The workspace to store the results.
 */
void CompositeCatalog::getDataSets(const std::string &investigationId,
                                   ITableWorkspace_sptr &outputws) {
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    (*catalog)->getDataSets(investigationId, outputws);
  }
}

/**
 * Obtain and save the datafiles for a given investigation for each catalog in
 * the container.
 * @param investigationId :: A unique identifier of the investigation.
 * @param outputws        :: The workspace to store the results.
 */
void CompositeCatalog::getDataFiles(const std::string &investigationId,
                                    ITableWorkspace_sptr &outputws) {
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    (*catalog)->getDataFiles(investigationId, outputws);
  }
}

/**
 * Obtain a list of instruments from each catalog in the container.
 * @param instruments :: A reference to the vector to store the results.
 */
void CompositeCatalog::listInstruments(std::vector<std::string> &instruments) {
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    (*catalog)->listInstruments(instruments);
  }
}

/**
 * Obtain a list of investigations from each catalog in the container.
 * @param invstTypes :: A reference to the vector to store the results.
 */
void
CompositeCatalog::listInvestigationTypes(std::vector<std::string> &invstTypes) {
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    (*catalog)->listInvestigationTypes(invstTypes);
  }
}

/**
 * Keep each catalog session alive in the container.
 */
void CompositeCatalog::keepAlive() {
  for (auto catalog = m_catalogs.begin(); catalog != m_catalogs.end();
       ++catalog) {
    (*catalog)->keepAlive();
  }
}
}
}
