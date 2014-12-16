// WorkspaceFactory include must be first otherwise you get a bizarre
// Poco-related compilation error on Windows
#include "MantidAPI/WorkspaceFactory.h"
#if GCC_VERSION >= 40800 // 4.8.0
GCC_DIAG_OFF(literal - suffix)
#endif
#include "MantidICat/ICat3/ICat3Helper.h"
#include "MantidICat/ICat3/ICat3ErrorHandling.h"
#include "MantidKernel/Logger.h"
#include <iomanip>
#include <time.h>
#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace ICat {
using namespace Kernel;
using namespace API;
using namespace ICat3;

namespace {
/// static logger
Kernel::Logger g_log("CICatHelper");
}

CICatHelper::CICatHelper() : m_session() {}

/* This method calls ICat API searchbydavanced and do the basic run search
 * @param icat :: Proxy object for ICat
 * @param request :: request object
 * @param response :: response object
 */
int CICatHelper::doSearch(ICATPortBindingProxy &icat,
                          boost::shared_ptr<ns1__searchByAdvanced> &request,
                          ns1__searchByAdvancedResponse &response) {
  setICATProxySettings(icat);

  clock_t start = clock();
  int ret_advsearch = icat.searchByAdvanced(request.get(), &response);
  if (ret_advsearch != 0) {

    CErrorHandling::throwErrorMessages(icat);
  }
  clock_t end = clock();
  float diff = float(end - start) / CLOCKS_PER_SEC;
  g_log.information() << " Time taken to do  search is " << diff << "  seconds "
                      << std::endl;
  return ret_advsearch;
}

/** This method saves the search response( investigations )data to a table
 * workspace
 *  @param response :: const reference to response object
 *  @param outputws :: shared pointer to output workspace
 */
void CICatHelper::saveSearchRessults(
    const ns1__searchByAdvancedPaginationResponse &response,
    API::ITableWorkspace_sptr &outputws) {
  if (outputws->getColumnNames().empty()) {
    outputws->addColumn("str", "Investigation id");
    outputws->addColumn("str", "Facility");
    outputws->addColumn("str", "Title");
    outputws->addColumn("str", "Instrument");
    outputws->addColumn("str", "Run range");
    outputws->addColumn("str", "Start date");
    outputws->addColumn("str", "End date");
    outputws->addColumn("str", "SessionID");
  }
  saveInvestigations(response.return_, outputws);
}

/** This method saves investigations  to a table workspace
 *  @param investigations :: a vector containing investigation data
 *  @param outputws :: shared pointer to output workspace
 */
void CICatHelper::saveInvestigations(
    const std::vector<ns1__investigation *> &investigations,
    API::ITableWorkspace_sptr &outputws) {
  try {
    std::vector<ns1__investigation *>::const_iterator citr;
    for (citr = investigations.begin(); citr != investigations.end(); ++citr) {
      API::TableRow t = outputws->appendRow();

      std::string id = boost::lexical_cast<std::string>(*(*citr)->id);

      savetoTableWorkspace(&id, t);
      savetoTableWorkspace((*citr)->facility, t);
      savetoTableWorkspace((*citr)->title, t);
      savetoTableWorkspace((*citr)->instrument, t);
      savetoTableWorkspace((*citr)->invParamValue, t);

      std::string startDate =
          boost::lexical_cast<std::string>(*(*citr)->invStartDate);
      savetoTableWorkspace(&startDate, t);

      std::string endDate =
          boost::lexical_cast<std::string>(*(*citr)->invEndDate);
      savetoTableWorkspace(&endDate, t);

      std::string sessionID = m_session->getSessionId();
      savetoTableWorkspace(&sessionID, t);
    }
  } catch (std::runtime_error &) {
    throw std::runtime_error(
        "Error when saving  the ICat Search Results data to Workspace");
  }
}

/**
 * This method calls ICat API getInvestigationIncludes and returns investigation
 * details for a given investigation Id
 * @param invstId :: investigation id
 * @param include :: enum parameter for selecting the response data from the db.
 * @param responsews_sptr :: table workspace to save the response data
 * @returns zero if success otherwise error code
 */
void CICatHelper::getDataFiles(long long invstId,
                               ns1__investigationInclude include,
                               API::ITableWorkspace_sptr &responsews_sptr) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__getInvestigationIncludes request;
  ns1__getInvestigationIncludesResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  int64_t investigationID = invstId;
  request.investigationId = &investigationID;
  request.investigationInclude = &include;

  int result = icat.getInvestigationIncludes(&request, &response);
  if (result == 0) {
    saveInvestigationIncludesResponse(response, responsews_sptr);
  } else {
    CErrorHandling::throwErrorMessages(icat);
  }
}

/**
 * This method loops through the response return_vector and saves the datafile
 * details to a table workspace
 * @param response :: const reference to response object
 * @param outputws :: shared pointer to table workspace which stores the data
 */
void CICatHelper::saveInvestigationIncludesResponse(
    const ns1__getInvestigationIncludesResponse &response,
    API::ITableWorkspace_sptr &outputws) {
  if (outputws->getColumnNames().empty()) {
    outputws->addColumn("str", "Name");
    outputws->addColumn("str", "Location");
    outputws->addColumn("str", "Create Time");
    outputws->addColumn("long64", "Id");
    outputws->addColumn("long64", "File size(bytes)");
    outputws->addColumn("str", "File size");
    outputws->addColumn("str", "Description");
  }

  try {
    std::vector<ns1__dataset *> datasetVec;
    datasetVec.assign((response.return_)->datasetCollection.begin(),
                      (response.return_)->datasetCollection.end());
    if (datasetVec.empty()) {
      throw std::runtime_error("No data files exists in the ICAT database for "
                               "the selected investigation");
    }
    std::vector<ns1__dataset *>::const_iterator dataset_citr;
    for (dataset_citr = datasetVec.begin(); dataset_citr != datasetVec.end();
         ++dataset_citr) {
      std::vector<ns1__datafile *> datafileVec;
      datafileVec.assign((*dataset_citr)->datafileCollection.begin(),
                         (*dataset_citr)->datafileCollection.end());
      if (datafileVec.empty()) {
        throw std::runtime_error("No data files exists in the ICAT database "
                                 "for the selected  investigation ");
      }

      std::vector<ns1__datafile *>::const_iterator datafile_citr;
      for (datafile_citr = datafileVec.begin();
           datafile_citr != datafileVec.end(); ++datafile_citr) {

        API::TableRow t = outputws->appendRow();

        // File Name
        savetoTableWorkspace((*datafile_citr)->name, t);
        savetoTableWorkspace((*datafile_citr)->location, t);

        // File creation Time.
        std::string *creationtime = NULL;
        if ((*datafile_citr)->datafileCreateTime != NULL) {
          time_t crtime = *(*datafile_citr)->datafileCreateTime;
          char temp[25];
          strftime(temp, 25, "%Y-%b-%d %H:%M:%S", localtime(&crtime));
          std::string ftime(temp);
          creationtime = new std::string;
          creationtime->assign(ftime);
        }
        savetoTableWorkspace(creationtime, t);
        if (creationtime)
          delete creationtime;

        //
        savetoTableWorkspace((*datafile_citr)->id, t);

        LONG64 fileSize =
            boost::lexical_cast<LONG64>(*(*datafile_citr)->fileSize);
        savetoTableWorkspace(&fileSize, t);

        savetoTableWorkspace((*datafile_citr)->description, t);
      }
    }

  } catch (std::runtime_error &) {
    throw;
  }
}

/**This method calls ICat API getInvestigationIncludes and returns datasets
 * details for a given investigation Id
 * @param invstId :: investigation id
 * @param include :: enum parameter for selecting the response data from iact
 * db.
 * @param responsews_sptr :: table workspace to save the response data
 * @returns an integer zero if success if not an error number.
 */
void CICatHelper::doDataSetsSearch(long long invstId,
                                   ns1__investigationInclude include,
                                   API::ITableWorkspace_sptr &responsews_sptr) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__getInvestigationIncludes request;
  ns1__getInvestigationIncludesResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  request.investigationInclude = &include;
  int64_t investigationID = invstId;
  request.investigationId = &investigationID;

  int result = icat.getInvestigationIncludes(&request, &response);
  if (result == 0) {
    saveDataSets(response, responsews_sptr);
  } else {
    CErrorHandling::throwErrorMessages(icat);
  }
}

/** This method loops through the response return_vector and saves the datasets
 * details to a table workspace
 * @param response :: const reference to response object
 * @param outputws ::  shred pointer to workspace
 * @returns shared pointer to table workspace which stores the data
 */
void
CICatHelper::saveDataSets(const ns1__getInvestigationIncludesResponse &response,
                          API::ITableWorkspace_sptr &outputws) {
  // create table workspace
  if (outputws->getColumnNames().empty()) {
    outputws->addColumn("str", "Name"); // File name
    outputws->addColumn("str", "Status");
    outputws->addColumn("str", "Type");
    outputws->addColumn("str", "Description");
    outputws->addColumn("long64", "Sample Id");
  }
  try {

    std::vector<ns1__dataset *> datasetVec;
    datasetVec.assign((response.return_)->datasetCollection.begin(),
                      (response.return_)->datasetCollection.end());

    std::vector<ns1__dataset *>::const_iterator dataset_citr;
    for (dataset_citr = datasetVec.begin(); dataset_citr != datasetVec.end();
         ++dataset_citr) {
      API::TableRow t = outputws->appendRow();

      // DataSet Name
      savetoTableWorkspace((*dataset_citr)->name, t);
      // DataSet Status
      savetoTableWorkspace((*dataset_citr)->datasetStatus, t);
      // DataSet Type
      savetoTableWorkspace((*dataset_citr)->datasetType, t);
      // DataSet Type
      savetoTableWorkspace((*dataset_citr)->description, t);
      // DataSet Type
      savetoTableWorkspace((*dataset_citr)->sampleId, t);
    }
  }

  catch (std::runtime_error &) {
    throw;
  }

  // return outputws;
}

/**
 * Updates the list of instruments.
 * @param instruments :: instruments list
 */
void CICatHelper::listInstruments(std::vector<std::string> &instruments) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__listInstruments request;
  ns1__listInstrumentsResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;

  int result = icat.listInstruments(&request, &response);

  if (result == 0) {
    for (unsigned i = 0; i < response.return_.size(); ++i) {
      instruments.push_back(response.return_[i]);
    }
  } else {
    CErrorHandling::throwErrorMessages(icat);
  }
}

/**
 * Updates the list of investigation types.
 * @param investTypes :: The list of investigation types.
 */
void
CICatHelper::listInvestigationTypes(std::vector<std::string> &investTypes) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__listInvestigationTypes request;
  ns1__listInvestigationTypesResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;

  int result = icat.listInvestigationTypes(&request, &response);

  if (result == 0) {
    for (unsigned i = 0; i < response.return_.size(); ++i) {
      investTypes.push_back(response.return_[i]);
    }
  } else {
    CErrorHandling::throwErrorMessages(icat);
  }
}

/**
 *This method calls ICat api logout and disconnects from ICat DB
 * @returns zero if successful otherwise error code
 */
int CICatHelper::doLogout() {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__logout request;
  ns1__logoutResponse response;
  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  int ret = icat.logout(&request, &response);
  if (ret != 0) {
    throw std::runtime_error(
        "You are not currently logged into the cataloging system.");
  }
  m_session->setSessionId("");
  return ret;
}

/**
 * This method calls ICat api getmyinvestigations and do returns the
 * investigations of the logged in user
 * @param ws_sptr :: shared pointer to table workspace which stores the
 * investigations search result
 */
void CICatHelper::doMyDataSearch(API::ITableWorkspace_sptr &ws_sptr) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__getMyInvestigationsIncludes request;
  ns1__getMyInvestigationsIncludesResponse response;
  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  // investigation include
  boost::shared_ptr<ns1__investigationInclude> invstInculde_sptr(
      new ns1__investigationInclude);
  request.investigationInclude = invstInculde_sptr.get();
  *request.investigationInclude =
      ns1__investigationInclude__INVESTIGATORS_USCORESHIFTS_USCOREAND_USCORESAMPLES;

  int ret = icat.getMyInvestigationsIncludes(&request, &response);
  if (ret != 0) {
    CErrorHandling::throwErrorMessages(icat);
  }
  if (response.return_.empty()) {
    g_log.information()
        << "ICat Mydata search is complete.There are no results to display"
        << std::endl;
    return;
  }
  // save response to a table workspace
  saveMyInvestigations(response, ws_sptr);
}

/**This method calls ICat api getmyinvestigations and do returns the
 * investigations of the logged in user
 * @param response :: reference to response  object
 * @param outputws :: shared pointer to table workspace which stores the
 * investigations search result
 */
void CICatHelper::saveMyInvestigations(
    const ns1__getMyInvestigationsIncludesResponse &response,
    API::ITableWorkspace_sptr &outputws) {
  if (outputws->getColumnNames().empty()) {
    outputws->addColumn("str", "Investigation id");
    outputws->addColumn("str", "Facility");
    outputws->addColumn("str", "Title");
    outputws->addColumn("str", "Instrument");
    outputws->addColumn("str", "Run range");
    outputws->addColumn("str", "Start date");
    outputws->addColumn("str", "End date");
    outputws->addColumn("str", "SessionID");
  }
  saveInvestigations(response.return_, outputws);
}

/* This method does advanced search and returns investigation data
 * @param inputs :: reference to class containing search inputs
 * @param outputws :: shared pointer to output workspace
 * @param offset   :: skip this many rows and start returning rows from this
 * point.
 * @param limit    :: limit the number of rows returned by the query.
 */
void CICatHelper::doAdvancedSearch(const CatalogSearchParam &inputs,
                                   API::ITableWorkspace_sptr &outputws,
                                   const int &offset, const int &limit) {
  // Show "my data" (without paging).
  if (inputs.getMyData()) {
    doMyDataSearch(outputws);
    return;
  }

  ns1__searchByAdvancedPagination request;
  ns1__searchByAdvancedPaginationResponse response;

  // If offset or limit is default value then we want to return as
  // we just wanted to perform the getSearchQuery to build our COUNT query.
  if (offset == -1 || limit == -1)
    return;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  // Setup paging information to search with paging enabled.
  request.numberOfResults = limit;
  request.startIndex = offset;
  request.advancedSearchDetails = buildSearchQuery(inputs);

  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  int result = icat.searchByAdvancedPagination(&request, &response);

  if (result != 0) {
    // replace with mantid error routine
    CErrorHandling::throwErrorMessages(icat);
  }
  if (response.return_.empty()) {
    g_log.information() << "ICat investigations search is complete.There are "
                           "no results to display" << std::endl;
    return;
  }
  // save response to a table workspace
  saveSearchRessults(response, outputws);
}

/**
 * Creates a search query based on search inputs provided by the user.
 * @param inputs :: Reference to a class containing search inputs.
 * Return A populated searchDetails class used for performing a query against
 * ICAT.
 */
ICat3::ns1__advancedSearchDetails *
CICatHelper::buildSearchQuery(const CatalogSearchParam &inputs) {
  // As this is a member variable we need to reset the search terms once
  // a new search is performed.
  ICat3::ns1__advancedSearchDetails *advancedSearchDetails =
      new ICat3::ns1__advancedSearchDetails;

  ns1__investigationInclude invesInclude =
      ns1__investigationInclude__INVESTIGATORS_USCOREAND_USCOREKEYWORDS;
  advancedSearchDetails->investigationInclude = &invesInclude;

  double runStart, runEnd;
  // run start
  if (inputs.getRunStart() > 0) {
    runStart = inputs.getRunStart();
    advancedSearchDetails->runStart = &runStart;
  }

  // run end
  if (inputs.getRunEnd() > 0) {
    runEnd = inputs.getRunEnd();
    advancedSearchDetails->runEnd = &runEnd;
  }

  time_t startDate, endDate;
  // start date
  if (inputs.getStartDate() != 0) {
    startDate = inputs.getStartDate();
    advancedSearchDetails->dateRangeStart = &startDate;
  }

  // end date
  if (inputs.getEndDate() != 0) {
    endDate = inputs.getEndDate();
    advancedSearchDetails->dateRangeEnd = &endDate;
  }

  // instrument name
  if (!inputs.getInstrument().empty()) {
    advancedSearchDetails->instruments.push_back(inputs.getInstrument());
  }

  // keywords
  if (!inputs.getKeywords().empty()) {
    advancedSearchDetails->keywords.push_back(inputs.getKeywords());
  }

  std::string investigationName, investigationType, datafileName, sampleName;

  // Investigation name
  if (!inputs.getInvestigationName().empty()) {
    investigationName = inputs.getInvestigationName();
    advancedSearchDetails->investigationName = &investigationName;
  }

  // Investigation type
  if (!inputs.getInvestigationType().empty()) {
    investigationType = inputs.getInvestigationType();
    advancedSearchDetails->investigationType = &investigationType;
  }

  // datafile name
  if (!inputs.getDatafileName().empty()) {
    datafileName = inputs.getDatafileName();
    advancedSearchDetails->datafileName = &datafileName;
  }

  // sample name
  if (!inputs.getSampleName().empty()) {
    sampleName = inputs.getSampleName();
    advancedSearchDetails->sampleName = &sampleName;
  }

  // investigator's surname
  if (!inputs.getInvestigatorSurName().empty()) {
    advancedSearchDetails->investigators.push_back(
        inputs.getInvestigatorSurName());
  }

  return advancedSearchDetails;
}

/**
 * Uses user input fields to perform a search & obtain the COUNT of results for
 * paging.
 * @return The number of investigations returned by the search performed.
 */
int64_t
CICatHelper::getNumberOfSearchResults(const CatalogSearchParam &inputs) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__searchByAdvanced request;
  ns1__searchByAdvancedResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  request.advancedSearchDetails = buildSearchQuery(inputs);

  int result = icat.searchByAdvanced(&request, &response);

  int64_t numOfResults = 0;

  if (result == 0) {
    numOfResults = response.return_.size();
  } else {
    CErrorHandling::throwErrorMessages(icat);
  }

  g_log.debug() << "CICatHelper::getNumberOfSearchResults -> Number of results "
                   "returned is: { " << numOfResults << " }" << std::endl;

  return numOfResults;
}

/**
 * Authenticate the user against all catalogues in the container.
 * @param username :: The login name of the user.
 * @param password :: The password of the user.
 * @param endpoint :: The endpoint url of the catalog to log in to.
 * @param facility :: The facility of the catalog to log in to.
 */
API::CatalogSession_sptr CICatHelper::doLogin(const std::string &username,
                                              const std::string &password,
                                              const std::string &endpoint,
                                              const std::string &facility) {
  m_session = boost::make_shared<API::CatalogSession>("", facility, endpoint);

  // Obtain the ICAT proxy that has been securely set, including soap-endpoint.
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  // Output the soap end-point in use for debugging purposes.
  g_log.debug() << "The ICAT soap end-point is: " << icat.soap_endpoint << "\n";

  // CatalogLogin to icat
  ns1__login login;
  ns1__loginResponse loginResponse;

  std::string userName(username);
  std::string passWord(password);

  login.username = &userName;
  login.password = &passWord;

  int query_id = icat.login(&login, &loginResponse);
  if (query_id == 0) {
    m_session->setSessionId(*(loginResponse.return_));
  } else {
    throw std::runtime_error("Username or password supplied is invalid.");
  }
  return m_session;
}

const std::string CICatHelper::getdownloadURL(const long long &fileId) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__downloadDatafile request;
  ns1__downloadDatafileResponse response;

  std::string downloadURL;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  int64_t fileID = fileId;
  request.datafileId = &fileID;

  int ret = icat.downloadDatafile(&request, &response);

  if (ret == 0) {
    downloadURL = *response.URL;
  } else {
    CErrorHandling::throwErrorMessages(icat);
  }
  return downloadURL;
}

const std::string CICatHelper::getlocationString(const long long &fileid) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__getDatafile request;
  ns1__getDatafileResponse response;

  std::string filelocation;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  int64_t fileID = fileid;
  request.datafileId = &fileID;

  int ret = icat.getDatafile(&request, &response);

  if (ret == 0) {
    if (response.return_->location) {
      filelocation = *response.return_->location;
    }
  }
  return filelocation;
}

/**
 * Sets the soap-endpoint & SSL context for the given ICAT proxy.
 */
void CICatHelper::setICATProxySettings(ICat3::ICATPortBindingProxy &icat) {
  // Set the soap-endpoint of the catalog we want to use.
  icat.soap_endpoint = m_session->getSoapEndpoint().c_str();
  // Sets SSL authentication scheme
  setSSLContext(icat);
}

/**
 * Defines the SSL authentication scheme.
 * @param icat :: ICATPortBindingProxy object.
 */
void CICatHelper::setSSLContext(ICat3::ICATPortBindingProxy &icat) {
  if (soap_ssl_client_context(
          &icat, SOAP_SSL_CLIENT, /* use SOAP_SSL_DEFAULT in production code */
          NULL, /* keyfile: required only when client must authenticate to
                server (see SSL docs on how to obtain this file) */
          NULL, /* password to read the keyfile */
          NULL, /* optional cacert file to store trusted certificates */
          NULL, /* optional capath to directory with trusted certificates */
          NULL  /* if randfile!=NULL: use a file with random data to seed
                   randomness */
          )) {
    CErrorHandling::throwErrorMessages(icat);
  }
}
}
}
