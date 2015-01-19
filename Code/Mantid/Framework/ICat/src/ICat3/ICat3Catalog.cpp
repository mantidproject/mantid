#include "MantidICat/ICat3/ICat3Catalog.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/AnalysisDataService.h"

namespace Mantid {
namespace ICat {

DECLARE_CATALOG(ICat3Catalog)

/// constructor
ICat3Catalog::ICat3Catalog() : m_helper(new CICatHelper()) {}

/// destructor
ICat3Catalog::~ICat3Catalog() { delete m_helper; }

/**
 * Authenticate the user against all catalogues in the container.
 * @param username :: The login name of the user.
 * @param password :: The password of the user.
 * @param endpoint :: The endpoint url of the catalog to log in to.
 * @param facility :: The facility of the catalog to log in to.
 */
API::CatalogSession_sptr ICat3Catalog::login(const std::string &username,
                                             const std::string &password,
                                             const std::string &endpoint,
                                             const std::string &facility) {
  return m_helper->doLogin(username, password, endpoint, facility);
}

/// This method disconnects the client application from ICat3 based catalog
/// services
void ICat3Catalog::logout() { m_helper->doLogout(); }

/*This method returns the logged in user's investigations data .
 *@param mydataws_sptr :: pointer to table workspace which stores the data
 */
void ICat3Catalog::myData(Mantid::API::ITableWorkspace_sptr &mydataws_sptr) {
  m_helper->doMyDataSearch(mydataws_sptr);
}

/*This method returns  the datasets associated to the given investigationid .
 *@param investigationId :: unique identifier of the investigation
 *@param datasetsws_sptr :: shared pointer to datasets
 */
void
ICat3Catalog::getDataSets(const std::string &investigationId,
                          Mantid::API::ITableWorkspace_sptr &datasetsws_sptr) {
  // search datasets for a given investigation id using ICat api.
  m_helper->doDataSetsSearch(
      boost::lexical_cast<int64_t>(investigationId),
      ICat3::
          ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATASET_USCOREPARAMETERS_USCOREONLY,
      datasetsws_sptr);
}

/*This method returns the datafiles associated to the given investigationid .
 *@param investigationId :: unique identifier of the investigation
 *@param datafilesws_sptr :: shared pointer to datasets
 */
void ICat3Catalog::getDataFiles(
    const std::string &investigationId,
    Mantid::API::ITableWorkspace_sptr &datafilesws_sptr) {
  m_helper->getDataFiles(
      boost::lexical_cast<int64_t>(investigationId),
      ICat3::ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,
      datafilesws_sptr);
}

/**This method returns the list of instruments
 *@param instruments :: instruments list
 */
void ICat3Catalog::listInstruments(std::vector<std::string> &instruments) {
  m_helper->listInstruments(instruments);
}

/**This method returns the list of investigationtypes
 *@param invstTypes :: investigation types list
 */
void
ICat3Catalog::listInvestigationTypes(std::vector<std::string> &invstTypes) {
  m_helper->listInvestigationTypes(invstTypes);
}

/**
 * Gets the file location string from the archives.
 * @param fileID :: The id of the file to search for.
 * @return The location of the datafile stored on the archives.
 */
const std::string ICat3Catalog::getFileLocation(const long long &fileID) {
  return m_helper->getlocationString(fileID);
}

/**
 * Downloads a file from the given url if not downloaded from archive.
 * @param fileID :: The id of the file to search for.
 * @return A URL to download the datafile from.
 */
const std::string ICat3Catalog::getDownloadURL(const long long &fileID) {
  return m_helper->getdownloadURL(fileID);
}

/**
 * Get the URL where the datafiles will be uploaded to.
 * @param investigationID :: The investigation used to obtain the related
 * dataset ID.
 * @param createFileName  :: The name to give to the file being saved.
 * @param dataFileDescription :: The description of the data file being saved.
 * @return URL to PUT datafiles to.
 */
const std::string
ICat3Catalog::getUploadURL(const std::string &investigationID,
                           const std::string &createFileName,
                           const std::string &dataFileDescription) {
  UNUSED_ARG(investigationID);
  UNUSED_ARG(createFileName);
  UNUSED_ARG(dataFileDescription);
  throw std::runtime_error("ICat3Catalog does not support publishing.");
}

/**This method method does the search for investigations
 *@param inputs :: reference to a class conatains search inputs
 *@param ws_sptr :: -shared pointer to search results workspace
 *@param offset  :: skip this many rows and start returning rows from this
 *point.
 *@param limit   :: limit the number of rows returned by the query.
 */
void ICat3Catalog::search(const CatalogSearchParam &inputs,
                          Mantid::API::ITableWorkspace_sptr &ws_sptr,
                          const int &offset, const int &limit) {
  m_helper->doAdvancedSearch(inputs, ws_sptr, offset, limit);
}

/**
 * Modifies the search query to obtain the number
 * of investigations to be returned by the catalog.
 * @return The number of investigations returned by the search performed.
 */
int64_t
ICat3Catalog::getNumberOfSearchResults(const CatalogSearchParam &inputs) {
  return m_helper->getNumberOfSearchResults(inputs);
}

/// keep alive
void ICat3Catalog::keepAlive() {}

/**
 * Obtains the investigations that the user can publish
 * to and saves related information to a workspace.
 * @return A workspace containing investigation information the user can publish
 * to.
 */
API::ITableWorkspace_sptr ICat3Catalog::getPublishInvestigations() {
  throw std::runtime_error("Publishing is not supported in ICat3Catalog.");
}
}
}
