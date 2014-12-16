#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidICat/ICat4/GSoapGenerated/ICat4ICATPortBindingProxy.h"
#include "MantidICat/ICat4/ICat4Catalog.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace ICat {
using namespace Kernel;
using namespace ICat4;

namespace {
/// static logger
Logger g_log("ICat4Catalog");
}

DECLARE_CATALOG(ICat4Catalog)

ICat4Catalog::ICat4Catalog() : m_session() {}

/**
 * Authenticate the user against all catalogues in the container.
 * @param username :: The login name of the user.
 * @param password :: The password of the user.
 * @param endpoint :: The endpoint url of the catalog to log in to.
 * @param facility :: The facility of the catalog to log in to.
 */
API::CatalogSession_sptr ICat4Catalog::login(const std::string &username,
                                             const std::string &password,
                                             const std::string &endpoint,
                                             const std::string &facility) {
  // Created the session object here in order to set the endpoint, which is used
  // in setICATProxySettings.
  // We can then manually set the sessionID later if it exists.
  m_session = boost::make_shared<API::CatalogSession>("", facility, endpoint);

  // Securely set, including soap-endpoint.
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  // Used to authenticate the user.
  ns1__login login;
  ns1__loginResponse loginResponse;

  // Used to add entries to the login class.
  _ns1__login_credentials_entry entry;

  // Name of the authentication plugin in use.
  std::string plugin;

  if (endpoint.find("sns") != std::string::npos) {
    plugin = std::string("ldap");
  } else {
    plugin = std::string("uows");
  }
  login.plugin = &plugin;

  // Making string as cannot convert from const.
  std::string userName(username);
  std::string passWord(password);

  std::string usernameKey("username");
  std::string passwordKey("password");

  // Instantiate an instance of an entry to prevent null pointer.
  // This then allows us to push entries as required below.
  std::vector<_ns1__login_credentials_entry> entries;
  login.credentials.entry = &entries;

  // Setting the username and pass credentials to the login class.
  entry.key = &usernameKey;
  entry.value = &userName;
  entries.push_back(entry);

  entry.key = &passwordKey;
  entry.value = &passWord;
  entries.push_back(entry);

  int result = icat.login(&login, &loginResponse);

  if (result == 0) {
    m_session->setSessionId(*(loginResponse.return_));
  } else {
    throwErrorMessage(icat);
  }
  // Will not reach here if user cannot log in (e.g. no session is created).
  return m_session;
}

/**
 * Disconnects the client application from ICat4 based catalog services.
 */
void ICat4Catalog::logout() {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__logout request;
  ns1__logoutResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;

  int result = icat.logout(&request, &response);

  if (result == 0) {
    m_session->setSessionId("");
  } else {
    throwErrorMessage(icat);
  }
}

/**
 * Creates a search query string based on inputs provided by the user.
 * @param inputs :: reference to a class contains search inputs.
 * @return a query string constructed from user input.
 */
std::string ICat4Catalog::buildSearchQuery(const CatalogSearchParam &inputs) {
  // Contain the related where and join clauses for the search query based on
  // user-input.
  std::vector<std::string> whereClause, joinClause;

  // Format the timestamps in order to compare them.
  std::string startDate =
      formatDateTime(inputs.getStartDate(), "%Y-%m-%d %H:%M:%S");
  std::string endDate =
      formatDateTime(inputs.getEndDate() + ((23 * 60 * 60) + (59 * 60) + 59),
                     "%Y-%m-%d %H:%M:%S");

  // Investigation startDate if endDate is not selected
  if (inputs.getStartDate() != 0 && inputs.getEndDate() == 0) {
    whereClause.push_back("inves.startDate >= '" + startDate + "'");
  }

  // Investigation endDate if startdate is not selected
  if (inputs.getEndDate() != 0 && inputs.getStartDate() == 0) {
    whereClause.push_back("inves.endDate <= '" + endDate + "'");
  }

  // Investigation Start and end date if both selected
  if (inputs.getStartDate() != 0 && inputs.getEndDate() != 0) {
    whereClause.push_back("inves.startDate BETWEEN '" + startDate + "' AND '" +
                          endDate + "'");
  }

  // Investigation name (title)
  if (!inputs.getInvestigationName().empty()) {
    whereClause.push_back("inves.title LIKE '%" +
                          inputs.getInvestigationName() + "%'");
  }

  // Investigation id
  if (!inputs.getInvestigationId().empty()) {
    whereClause.push_back("inves.name = '" + inputs.getInvestigationId() + "'");
  }

  // Investigation type
  if (!inputs.getInvestigationType().empty()) {
    joinClause.push_back("JOIN inves.type itype");
    whereClause.push_back("itype.name = '" + inputs.getInvestigationType() +
                          "'");
  }

  // Instrument name
  if (!inputs.getInstrument().empty()) {
    joinClause.push_back("JOIN inves.investigationInstruments invInst");
    joinClause.push_back("JOIN invInst.instrument inst");
    whereClause.push_back("inst.fullName = '" + inputs.getInstrument() + "'");
  }

  // Keywords
  if (!inputs.getKeywords().empty()) {
    joinClause.push_back("JOIN inves.keywords keywords");
    whereClause.push_back("keywords.name IN ('" + inputs.getKeywords() + "')");
  }

  // Sample name
  if (!inputs.getSampleName().empty()) {
    joinClause.push_back("JOIN inves.samples sample");
    whereClause.push_back("sample.name LIKE '%" + inputs.getSampleName() +
                          "%'");
  }

  // If the user has selected the "My data only" button.
  // (E.g. they want to display or search through all the data they have access
  // to.
  if (inputs.getMyData()) {
    joinClause.push_back("JOIN inves.investigationUsers users");
    joinClause.push_back("JOIN users.user user");
    whereClause.push_back("user.name = :user");
  }

  // Investigators complete name.
  if (!inputs.getInvestigatorSurName().empty()) {
    // We join another investigationUsers & user tables as we need two aliases.
    joinClause.push_back("JOIN inves.investigationUsers usrs");
    joinClause.push_back("JOIN usrs.user usr");
    whereClause.push_back("usr.fullName LIKE '%" +
                          inputs.getInvestigatorSurName() + "%'");
  }

  // Similar to above. We check if either has been input,
  // join the related table and add the specific WHERE clause.
  if (!inputs.getDatafileName().empty() ||
      (inputs.getRunStart() > 0 && inputs.getRunEnd() > 0)) {
    joinClause.push_back("JOIN inves.datasets dataset");
    joinClause.push_back("JOIN dataset.datafiles datafile");

    if (!inputs.getDatafileName().empty()) {
      whereClause.push_back("datafile.name LIKE '%" + inputs.getDatafileName() +
                            "%'");
    }

    if (inputs.getRunStart() > 0 && inputs.getRunEnd() > 0) {
      joinClause.push_back("JOIN datafile.parameters datafileparameters");
      joinClause.push_back("JOIN datafileparameters.type dtype");
      whereClause.push_back("dtype.name='run_number' AND "
                            "datafileparameters.numericValue BETWEEN " +
                            Strings::toString(inputs.getRunStart()) + " AND " +
                            Strings::toString(inputs.getRunEnd()) + "");
    }
  }

  std::string query;

  // This prevents the user searching the entire archive (E.g. there is no
  // "default" query).
  if (!whereClause.empty() || !joinClause.empty()) {
    std::string from, join, where, orderBy, includes;

    from = " FROM Investigation inves ";
    join = Strings::join(joinClause.begin(), joinClause.end(), " ");
    where = Strings::join(whereClause.begin(), whereClause.end(), " AND ");
    orderBy = " ORDER BY inves.id DESC";
    includes = " INCLUDE inves.facility, "
               "inves.investigationInstruments.instrument, inves.parameters";

    // As we joined all WHERE clause with AND we need to include the WHERE at
    // the start of the where segment.
    where.insert(0, " WHERE ");
    // Build the query from the result.
    query = from + join + where + orderBy + includes;
  }

  return (query);
}

/**
 * Searches for the relevant data based on user input.
 * @param inputs   :: reference to a class contains search inputs
 * @param outputws :: shared pointer to search results workspace
 * @param offset   :: skip this many rows and start returning rows from this
 * point.
 * @param limit    :: limit the number of rows returned by the query.
 */
void ICat4Catalog::search(const CatalogSearchParam &inputs,
                          Mantid::API::ITableWorkspace_sptr &outputws,
                          const int &offset, const int &limit) {
  std::string query = buildSearchQuery(inputs);

  // Check if the query built was valid.
  if (query.empty())
    throw std::runtime_error("You have not input any terms to search for.");

  // Modify the query to include correct SELECT and LIMIT clauses.
  query.insert(0, "SELECT DISTINCT inves");
  query.append(" LIMIT " + boost::lexical_cast<std::string>(offset) + "," +
               boost::lexical_cast<std::string>(limit));

  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  auto searchResults = performSearch(icat, query);
  saveInvestigations(searchResults, outputws);
}

/**
 * Obtain the number of investigations to be returned by the catalog.
 * @return The number of investigations returned by the search performed.
 */
int64_t
ICat4Catalog::getNumberOfSearchResults(const CatalogSearchParam &inputs) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  std::string query = buildSearchQuery(inputs);
  if (query.empty())
    throw std::runtime_error("You have not input any terms to search for.");
  query.insert(0, "SELECT COUNT(DISTINCT inves)");

  auto searchResults = performSearch(icat, query);
  auto numRes = dynamic_cast<xsd__long *>(searchResults.at(0));

  if (numRes) {
    g_log.debug() << "The number of paging results returned in "
                     "ICat4Catalog::getNumberOfSearchResults is: "
                  << numRes->__item << "\n";
    return numRes->__item;
  } else
    return -1;
}

/**
 * Returns the logged in user's investigations data.
 * @param outputws :: Pointer to table workspace that stores the data.
 */
void ICat4Catalog::myData(Mantid::API::ITableWorkspace_sptr &outputws) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  std::string query =
      "SELECT DISTINCT inves "
      "FROM Investigation inves "
      "JOIN inves.investigationUsers users "
      "JOIN users.user user "
      "WHERE user.name = :user "
      "ORDER BY inves.id DESC "
      "INCLUDE inves.facility, inves.investigationInstruments.instrument, "
      "inves.parameters";

  auto searchResults = performSearch(icat, query);
  saveInvestigations(searchResults, outputws);
}

/**
 * Saves investigations to a table workspace.
 * @param response :: A vector containing the results of the search query.
 * @param outputws :: Shared pointer to output workspace.
 */
void ICat4Catalog::saveInvestigations(std::vector<xsd__anyType *> response,
                                      API::ITableWorkspace_sptr &outputws) {
  if (outputws->getColumnNames().empty()) {
    // Add rows headers to the output workspace.
    outputws->addColumn("long64", "DatabaseID");
    outputws->addColumn("str", "InvestigationID");
    outputws->addColumn("str", "Facility");
    outputws->addColumn("str", "Title");
    outputws->addColumn("str", "Instrument");
    outputws->addColumn("str", "Run range");
    outputws->addColumn("str", "Start date");
    outputws->addColumn("str", "End date");
    outputws->addColumn("str", "SessionID");
  }

  // Add data to each row in the output workspace.
  std::vector<xsd__anyType *>::const_iterator iter;
  for (iter = response.begin(); iter != response.end(); ++iter) {
    // Cast from xsd__anyType to subclass (xsd__string).
    ns1__investigation *investigation =
        dynamic_cast<ns1__investigation *>(*iter);
    if (investigation) {
      API::TableRow table = outputws->appendRow();
      // Used to insert an empty string into the cell if value does not exist.
      std::string emptyCell("");

      // Now add the relevant investigation data to the table (They always
      // exist).
      savetoTableWorkspace(investigation->id, table);
      savetoTableWorkspace(investigation->name, table);
      savetoTableWorkspace(investigation->facility->name, table);
      savetoTableWorkspace(investigation->title, table);
      savetoTableWorkspace(
          investigation->investigationInstruments.at(0)->instrument->name,
          table);

      // Verify that the run parameters vector exist prior to doing anything.
      // Since some investigations may not have run parameters.
      if (!investigation->parameters.empty()) {
        savetoTableWorkspace(investigation->parameters[0]->stringValue, table);
      } else {
        savetoTableWorkspace(&emptyCell, table);
      }

      // Again, we need to check first if start and end date exist prior to
      // insertion.
      if (investigation->startDate) {
        std::string startDate =
            formatDateTime(*investigation->startDate, "%Y-%m-%d");
        savetoTableWorkspace(&startDate, table);
      } else {
        savetoTableWorkspace(&emptyCell, table);
      }

      if (investigation->endDate) {
        std::string endDate =
            formatDateTime(*investigation->endDate, "%Y-%m-%d");
        savetoTableWorkspace(&endDate, table);
      } else {
        savetoTableWorkspace(&emptyCell, table);
      }
      std::string sessionID = m_session->getSessionId();
      savetoTableWorkspace(&sessionID, table);
    } else {
      throw std::runtime_error("ICat4Catalog::saveInvestigations expected an "
                               "investigation. Please contact the Mantid "
                               "development team.");
    }
  }
}

/**
 * Returns the datasets associated to the given investigation id.
 * @param investigationId :: unique identifier of the investigation
 * @param outputws        :: shared pointer to datasets
 */
void ICat4Catalog::getDataSets(const std::string &investigationId,
                               Mantid::API::ITableWorkspace_sptr &outputws) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  auto searchResults =
      performSearch(icat, "Dataset INCLUDE DatasetType, Datafile, "
                          "Investigation <-> Investigation[name = '" +
                              investigationId + "']");
  saveDataSets(searchResults, outputws);
}

/**
 * Loops through the response vector and saves the datasets details to a table
 * workspace.
 * @param response :: A vector containing the results of the search query.
 * @param outputws :: Shared pointer to output workspace.
 */
void ICat4Catalog::saveDataSets(std::vector<xsd__anyType *> response,
                                API::ITableWorkspace_sptr &outputws) {
  if (outputws->getColumnNames().empty()) {
    // Add rows headers to the output workspace.
    outputws->addColumn("long64", "ID");
    outputws->addColumn("str", "Name");
    outputws->addColumn("str", "Description");
    outputws->addColumn("str", "Type");
    outputws->addColumn("str", "Related investigation ID");
    outputws->addColumn("size_t", "Number of datafiles");
  }

  std::string emptyCell = "";
  for (auto iter = response.begin(); iter != response.end(); ++iter) {
    ns1__dataset *dataset = dynamic_cast<ns1__dataset *>(*iter);
    if (dataset) {
      API::TableRow table = outputws->appendRow();

      savetoTableWorkspace(dataset->id, table);
      savetoTableWorkspace(dataset->name, table);

      if (dataset->description)
        savetoTableWorkspace(dataset->description, table);
      else
        savetoTableWorkspace(&emptyCell, table);

      if (dataset->type)
        savetoTableWorkspace(dataset->type->name, table);
      else
        savetoTableWorkspace(&emptyCell, table);

      if (dataset->investigation)
        savetoTableWorkspace(dataset->investigation->name, table);
      else
        savetoTableWorkspace(&emptyCell, table);

      size_t datafileCount = dataset->datafiles.size();
      savetoTableWorkspace(&datafileCount, table);
    } else {
      throw std::runtime_error("ICat4Catalog::saveDataSets expected a dataset. "
                               "Please contact the Mantid development team.");
    }
  }
}

/**
 * Returns the datafiles associated to the given investigation id.
 * @param investigationId  :: unique identifier of the investigation
 * @param outputws         :: shared pointer to datasets
 */
void ICat4Catalog::getDataFiles(const std::string &investigationId,
                                Mantid::API::ITableWorkspace_sptr &outputws) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  auto searchResults =
      performSearch(icat, "Datafile <-> Dataset <-> Investigation[name = '" +
                              investigationId + "']");
  saveDataFiles(searchResults, outputws);
}

/**
 * Saves result from "getDataFiles" to workspace.
 * @param response :: result response from the catalog.
 * @param outputws :: shared pointer to datasets
 */
void ICat4Catalog::saveDataFiles(std::vector<xsd__anyType *> response,
                                 API::ITableWorkspace_sptr &outputws) {
  if (outputws->getColumnNames().empty()) {
    // Add rows headers to the output workspace.
    outputws->addColumn("str", "Name");
    outputws->addColumn("str", "Location");
    outputws->addColumn("str", "Create Time");
    outputws->addColumn("long64", "Id");
    outputws->addColumn("long64", "File size(bytes)");
    outputws->addColumn("str", "File size");
    outputws->addColumn("str", "Description");
  }

  std::vector<xsd__anyType *>::const_iterator iter;
  for (iter = response.begin(); iter != response.end(); ++iter) {
    ns1__datafile *datafile = dynamic_cast<ns1__datafile *>(*iter);
    if (datafile) {
      API::TableRow table = outputws->appendRow();
      // Now add the relevant investigation data to the table.
      savetoTableWorkspace(datafile->name, table);
      savetoTableWorkspace(datafile->location, table);

      std::string createDate =
          formatDateTime(*datafile->createTime, "%Y-%m-%d %H:%M:%S");
      savetoTableWorkspace(&createDate, table);

      savetoTableWorkspace(datafile->id, table);
      savetoTableWorkspace(datafile->fileSize, table);

      std::string fileSize = bytesToString(*datafile->fileSize);
      savetoTableWorkspace(&fileSize, table);

      if (datafile->description)
        savetoTableWorkspace(datafile->description, table);
    } else {
      throw std::runtime_error("ICat4Catalog::saveDataFiles expected a "
                               "datafile. Please contact the Mantid "
                               "development team.");
    }
  }
}

/**
 * Returns the list of instruments.
 * @param instruments :: instruments list
 */
void ICat4Catalog::listInstruments(std::vector<std::string> &instruments) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  auto searchResults =
      performSearch(icat, "Instrument.fullName ORDER BY fullName");

  for (unsigned i = 0; i < searchResults.size(); ++i) {
    auto instrument = dynamic_cast<xsd__string *>(searchResults.at(i));
    if (instrument)
      instruments.push_back(instrument->__item);
  }
}

/**
 * Returns the list of investigation types.
 * @param invstTypes :: investigation types list
 */
void
ICat4Catalog::listInvestigationTypes(std::vector<std::string> &invstTypes) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  auto searchResults =
      performSearch(icat, "InvestigationType.name ORDER BY name");

  for (size_t i = 0; i < searchResults.size(); ++i) {
    auto investigationType = dynamic_cast<xsd__string *>(searchResults.at(i));
    if (investigationType)
      invstTypes.push_back(investigationType->__item);
  }
}

/**
 * Gets the file location string from the archives.
 * @param fileID :: The id of the file to search for.
 * @return The location of the datafile stored on the archives.
 */
const std::string ICat4Catalog::getFileLocation(const long long &fileID) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  auto searchResults =
      performSearch(icat, "Datafile[id = '" +
                              boost::lexical_cast<std::string>(fileID) + "']");
  auto datafile = dynamic_cast<ns1__datafile *>(searchResults.at(0));

  if (datafile && datafile->location)
    return *(datafile->location);
  else
    return "";
}

/**
 * Downloads a file from the given url if not downloaded from archive.
 * @param fileID :: The id of the file to search for.
 * @return A URL to download the datafile from.
 */
const std::string ICat4Catalog::getDownloadURL(const long long &fileID) {
  // Obtain the URL from the Facilities.xml file.
  std::string url = ConfigService::Instance()
                        .getFacility(m_session->getFacility())
                        .catalogInfo()
                        .externalDownloadURL();

  // Set the REST features of the URL.
  std::string session = "sessionId=" + m_session->getSessionId();
  std::string datafile =
      "&datafileIds=" + boost::lexical_cast<std::string>(fileID);
  std::string outname = "&outname=" + boost::lexical_cast<std::string>(fileID);

  // Add all the REST pieces to the URL.
  url += ("getData?" + session + datafile + outname + "&zip=false");
  g_log.debug() << "The download URL in ICat4Catalog::getDownloadURL is: "
                << url << std::endl;
  return url;
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
ICat4Catalog::getUploadURL(const std::string &investigationID,
                           const std::string &createFileName,
                           const std::string &dataFileDescription) {
  // Obtain the URL from the Facilities.xml file.
  std::string url = ConfigService::Instance()
                        .getFacility(m_session->getFacility())
                        .catalogInfo()
                        .externalDownloadURL();

  // Set the elements of the URL.
  std::string session = "sessionId=" + m_session->getSessionId();
  std::string name = "&name=" + createFileName;
  std::string datasetId =
      "&datasetId=" +
      boost::lexical_cast<std::string>(getMantidDatasetId(investigationID));
  std::string description = "&description=" + dataFileDescription;

  // Add pieces of URL together.
  url += ("put?" + session + name + datasetId + description +
          "&datafileFormatId=1");
  g_log.debug() << "The upload URL in ICat4Catalog::getUploadURL is: " << url
                << std::endl;
  return url;
}

/**
 * Obtains the investigations that the user can publish
 * to and saves related information to a workspace.
 * @return A workspace containing investigation information the user can publish
 * to.
 */
API::ITableWorkspace_sptr ICat4Catalog::getPublishInvestigations() {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  auto ws = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  // Populate the workspace with all the investigations that
  // the user is an investigator off and has READ access to.
  myData(ws);

  // Remove each investigation returned from `myData`
  // were the user does not have create/write access.
  for (int row = static_cast<int>(ws->rowCount()) - 1; row >= 0; --row) {
    ns1__dataset dataset;
    ns1__datafile datafile;

    // Verify if the user can CREATE datafiles in the "mantid" specific dataset.
    int64_t datasetID =
        getMantidDatasetId(ws->getRef<std::string>("InvestigationID", row));
    std::string datafileName = "tempName.nxs";

    dataset.id = &datasetID;
    datafile.name = &datafileName;
    datafile.dataset = &dataset;

    if (!isAccessAllowed(ns1__accessType__CREATE, datafile))
      ws->removeRow(row);
  }

  return ws;
}

/**
 * Keep the current session alive.
 */
void ICat4Catalog::keepAlive() {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__refresh request;
  ns1__refreshResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;

  int result = icat.refresh(&request, &response);
  // An error occurred!
  if (result != 0)
    throwErrorMessage(icat);
}

/**
 * Defines the SSL authentication scheme.
 * @param icat :: ICATPortBindingProxy object.
 */
void ICat4Catalog::setSSLContext(ICATPortBindingProxy &icat) {
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
    throwErrorMessage(icat);
  }
}

/**
 * Throws an error message (returned by gsoap) to Mantid upper layer.
 * @param icat :: ICATPortBindingProxy object.
 */
void ICat4Catalog::throwErrorMessage(ICATPortBindingProxy &icat) {
  char buf[600];
  const int len = 600;
  icat.soap_sprint_fault(buf, len);
  std::string error(buf);
  std::string begmsg("<message>");
  std::string endmsg("</message>");

  std::basic_string<char>::size_type start = error.find(begmsg);
  std::basic_string<char>::size_type end = error.find(endmsg);
  std::string exception;

  if (start != std::string::npos && end != std::string::npos) {
    exception =
        error.substr(start + begmsg.length(), end - (start + begmsg.length()));
  }
  // If no error is returned by ICAT then there is a connection problem.
  if (exception.empty())
    exception = "ICAT appears to be offline. Please check your connection or "
                "report this issue.";

  throw std::runtime_error(exception);
}

/**
 * Convert a file size to human readable file format.
 * @param fileSize :: The size in bytes of the file.
 */
std::string ICat4Catalog::bytesToString(int64_t &fileSize) {
  const char *args[] = {"B", "KB", "MB", "GB"};
  std::vector<std::string> units(args, args + 4);

  unsigned order = 0;

  while (fileSize >= 1024 && order + 1 < units.size()) {
    order++;
    fileSize = fileSize / 1024;
  }

  return boost::lexical_cast<std::string>(fileSize) + units.at(order);
}

/**
 * Formats a given timestamp to human readable datetime.
 * @param timestamp :: Unix timestamp.
 * @param format    :: The desired format to output.
 * @return string   :: Formatted Unix timestamp.
 */
std::string ICat4Catalog::formatDateTime(const time_t &timestamp,
                                         const std::string &format) {
  auto dateTime = DateAndTime(boost::posix_time::from_time_t(timestamp));
  return (dateTime.toFormattedString(format));
}

/**
 * Search the archive & obtain the "mantid" dataset ID for a specific
 * investigation if it exists.
 * If it does not exist, we will attempt to create it.
 * @param investigationID :: Used to obtain the related dataset ID.
 * @return Dataset ID of the provided investigation.
 */
int64_t ICat4Catalog::getMantidDatasetId(const std::string &investigationID) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  auto searchResults = performSearch(
      icat, "Dataset <-> Investigation[name = '" + investigationID + "']");

  int64_t datasetID = -1;
  for (size_t i = 0; i < searchResults.size(); ++i) {
    auto dataset = dynamic_cast<ns1__dataset *>(searchResults.at(i));
    if (dataset && *(dataset->name) == "mantid")
      datasetID = *(dataset->id);
  }

  if (datasetID == -1)
    datasetID = createMantidDataset(investigationID);
  g_log.debug() << "The dataset ID of the mantid dataset was: " << datasetID
                << "\n";

  return datasetID;
}

/**
 * Creates a dataset for an investigation (based on ID) named 'mantid' if it
 * does not already exist.
 * @param investigationID :: The investigation to create a dataset for.
 * @return The ID of the mantid dataset.
 */
int64_t ICat4Catalog::createMantidDataset(const std::string &investigationID) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  // We need to obtain an already existing datasetType as it's not recommended
  // to create a new one.
  auto datasetTypeSearch = performSearch(icat, "DatasetType[name ='analyzed']");
  auto datasetType = dynamic_cast<ns1__datasetType *>(datasetTypeSearch.at(0));

  auto investigationSearch =
      performSearch(icat, "Investigation[name = '" + investigationID + "']");
  auto investigation =
      dynamic_cast<ns1__investigation *>(investigationSearch.at(0));

  ns1__dataset dataset;
  std::string datasetName = "mantidTempNotDuplicate";

  dataset.name = &datasetName;
  dataset.complete = false;
  dataset.type = datasetType;
  dataset.investigation = investigation;

  int64_t datasetID = -1;

  if (isAccessAllowed(ns1__accessType__CREATE, dataset)) {
    ns1__create createRequest;
    ns1__createResponse createResponse;

    // We have to re-set the dataset name as when performing isAccessAllowed
    // an error will be thrown if the dataset already exists.
    std::string mantidName = "mantid";
    dataset.name = &mantidName;

    std::string sessionID = m_session->getSessionId();
    createRequest.sessionId = &sessionID;
    createRequest.bean = &dataset;

    if (icat.create(&createRequest, &createResponse) == SOAP_OK) {
      g_log.debug() << "Creating a new dataset named: " << *(dataset.name)
                    << " with investigationID " << investigationID << "\n";
      datasetID = createResponse.return_;
    }
    // Do not throw error from ICAT as we want to continue on GUI. Instead,
    // return -1 below.
  }

  g_log.debug()
      << "The dataset ID returned from ICat4Catalog::createMantidDataset was: "
      << datasetID << "\n";
  return datasetID; // Since we did not have access or could not create the file
                    // the default value (-1).
}

/**
 * Sets the soap-endpoint & SSL context for the given ICAT proxy.
 */
void ICat4Catalog::setICATProxySettings(ICATPortBindingProxy &icat) {
  // The soapEndPoint is only set when the user logs into the catalog.
  // If it's not set the correct error is returned (invalid sessionID) from the
  // ICAT server.
  if (m_session->getSoapEndpoint().empty())
    return;
  // Stop receiving packets from ICAT server after period of time.
  icat.recv_timeout = boost::lexical_cast<int>(
      ConfigService::Instance().getString("catalog.timeout.value"));
  // Set the soap-endpoint of the catalog we want to use.
  icat.soap_endpoint = m_session->getSoapEndpoint().c_str();
  // Sets SSL authentication scheme
  setSSLContext(icat);
}

/**
 * Returns the results of a search against ICAT for a given query.
 * Note: The ICatProxy object takes care of the deletion of the response object.
 * @param icat  :: The proxy object used to interact with gSOAP.
 * @param query :: The query to send to ICAT.
 */
std::vector<xsd__anyType *>
ICat4Catalog::performSearch(ICATPortBindingProxy &icat, std::string query) {
  ns1__search request;
  ns1__searchResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;
  request.query = &query;

  g_log.debug() << "The search query sent to ICAT was: \n" << query
                << std::endl;

  std::vector<xsd__anyType *> searchResults;

  if (icat.search(&request, &response) == SOAP_OK) {
    searchResults = response.return_;
  } else {
    throwErrorMessage(icat);
  }

  return searchResults;
}

/**
 * Is the specified access type allowed for a specific bean?
 * @param accessType :: The access type to check against the bean.
 * @param bean       :: The bean to check access type against. E.g.
 *CREATE,READ,UPDATE,DELETE.
 * @return True if access is allowed, otherwise false.
 **/
template <class T>
bool ICat4Catalog::isAccessAllowed(ns1__accessType accessType, T &bean) {
  ICATPortBindingProxy icat;
  setICATProxySettings(icat);

  ns1__isAccessAllowed request;
  ns1__isAccessAllowedResponse response;

  std::string sessionID = m_session->getSessionId();
  request.sessionId = &sessionID;

  ns1__accessType_ type;
  type.__item = accessType;

  request.accessType = &type.__item;
  request.bean = &bean;

  if (icat.isAccessAllowed(&request, &response) == SOAP_OK)
    return response.return_;
  else
    throwErrorMessage(icat);
  return false;
}
}
}
