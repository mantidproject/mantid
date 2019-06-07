// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/CatalogHelper.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"

#include <Poco/ActiveResult.h>
#include <QCoreApplication>
#include <QTime>
#include <boost/algorithm/string/regex.hpp>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Obtain a list of instruments for specified catalogs based on session
 * information.
 * @param sessionIDs :: The sessions information of each active catalog.
 * @return A vector containing the list of all instruments available.
 */
const std::vector<std::string>
CatalogHelper::getInstrumentList(const std::vector<std::string> &sessionIDs) {
  auto catalogAlgorithm = createCatalogAlgorithm("CatalogListInstruments");
  auto session = Mantid::API::CatalogManager::Instance().getActiveSessions();

  // If ONE or ALL catalogs are selected to search through use no session
  // (empty).
  // This will invoke the compositeCatalog instead of specific catalogs for each
  // session.
  if (session.size() == sessionIDs.size()) {
    executeAsynchronously(catalogAlgorithm);
    return catalogAlgorithm->getProperty("InstrumentList");
  } else {
    // Use catalogs for the specified sessions.
    for (const auto &sessionID : sessionIDs) {
      catalogAlgorithm->setProperty("Session", sessionID);
      executeAsynchronously(catalogAlgorithm);
    }
    // Return the vector containing the list of instruments available.
    return catalogAlgorithm->getProperty("InstrumentList");
  }
}

/**
 * Obtain the list of investigation types for specified catalogs based on
 * session information.
 * @param sessionIDs :: The sessions information of each active catalog.
 * @return A vector containing the list of all investigation types available.
 */
const std::vector<std::string> CatalogHelper::getInvestigationTypeList(
    const std::vector<std::string> &sessionIDs) {
  auto catalogAlgorithm =
      createCatalogAlgorithm("CatalogListInvestigationTypes");
  auto session = Mantid::API::CatalogManager::Instance().getActiveSessions();

  if (session.size() == sessionIDs.size()) {
    executeAsynchronously(catalogAlgorithm);
    return catalogAlgorithm->getProperty("InvestigationTypes");
  } else {
    for (const auto &sessionID : sessionIDs) {
      catalogAlgorithm->setProperty("Session", sessionID);
      executeAsynchronously(catalogAlgorithm);
    }
    return catalogAlgorithm->getProperty("InvestigationTypes");
  }
}

/**
 * Search the archive with the user input terms provided and save them to a
 * workspace ("searchResults").
 * @param userInputFields :: A map containing all users' search fields - (key =>
 * FieldName, value => FieldValue).
 * @param offset :: skip this many rows and start returning rows from this
 * point.
 * @param limit  :: limit the number of rows returned by the query.
 * @param sessionIDs :: The sessions information of each active catalog.
 */
void CatalogHelper::executeSearch(
    const std::map<std::string, std::string> &userInputFields,
    const int &offset, const int &limit,
    const std::vector<std::string> &sessionIDs) {
  auto catalogAlgorithm = createCatalogAlgorithm("CatalogSearch");
  // Set the properties to limit the number of results returned for paging
  // purposes.
  catalogAlgorithm->setProperty("Limit", limit);
  catalogAlgorithm->setProperty("Offset", offset);
  // Set the "search" properties to their related input fields.
  setSearchProperties(catalogAlgorithm, userInputFields);

  auto session = Mantid::API::CatalogManager::Instance().getActiveSessions();
  if (session.size() == sessionIDs.size()) {
    executeAsynchronously(catalogAlgorithm);
  } else {
    for (const auto &sessionID : sessionIDs) {
      catalogAlgorithm->setProperty("Session", sessionID);
      executeAsynchronously(catalogAlgorithm);
    }
  }
}

/**
 * The number of results returned by the search query (based on values of input
 * fields).
 * @param userInputFields :: A map containing the users' search input (key =>
 * FieldName, value => FieldValue).
 * @param sessionIDs :: The sessions information of each active catalog.
 * @return Number of results returned by the search query.
 */
int64_t CatalogHelper::getNumberOfSearchResults(
    const std::map<std::string, std::string> &userInputFields,
    const std::vector<std::string> &sessionIDs) {
  auto catalogAlgorithm = createCatalogAlgorithm("CatalogSearch");
  // Set the property to only perform a count search.
  catalogAlgorithm->setProperty("CountOnly", true);
  // Set the "search" properties to their related input fields.
  setSearchProperties(catalogAlgorithm, userInputFields);

  auto session = Mantid::API::CatalogManager::Instance().getActiveSessions();
  if (session.size() == sessionIDs.size()) {
    executeAsynchronously(catalogAlgorithm);
    return catalogAlgorithm->getProperty("NumberOfSearchResults");
  } else {
    for (const auto &sessionID : sessionIDs) {
      catalogAlgorithm->setProperty("Session", sessionID);
      executeAsynchronously(catalogAlgorithm);
    }
    return catalogAlgorithm->getProperty("NumberOfSearchResults");
  }
}

/**
 * Search the archives for all dataFiles related to an "investigation id" then
 * save results to workspace ("dataFileResults").
 * @param sessionID :: The sessions ID of the selected investigation.
 * @param investigationId :: The investigation id to use for the search.
 */
void CatalogHelper::executeGetDataFiles(const std::string &investigationId,
                                        const std::string &sessionID) {
  auto catalogAlgorithm = createCatalogAlgorithm("CatalogGetDataFiles");

  catalogAlgorithm->setProperty("InvestigationId", investigationId);
  catalogAlgorithm->setPropertyValue("OutputWorkspace", "__dataFileResults");
  catalogAlgorithm->setProperty("Session", sessionID);

  executeAsynchronously(catalogAlgorithm);
}

/**
 * Retrieve the path(s) to the file that was downloaded (via HTTP) or is stored
 * in the archive.
 * @param userSelectedFiles :: The file(s) the user has selected and wants to
 * download.
 * @param downloadPath      :: The location to save the datafile(s).
 * @param sessionID :: The sessions ID of the selected investigation.
 * @return A vector containing the paths to the file(s) the user wants.
 */
const std::vector<std::string> CatalogHelper::downloadDataFiles(
    const std::vector<std::pair<int64_t, std::string>> &userSelectedFiles,
    const std::string &downloadPath, const std::string &sessionID) {
  auto catalogAlgorithm = createCatalogAlgorithm("CatalogDownloadDataFiles");

  // Prepare for the ugly!

  // These two vectors are required by the "CatalogDownloadDataFiles" algorithm.
  std::vector<int64_t> fileIDs;
  fileIDs.reserve(userSelectedFiles.size());
  std::vector<std::string> fileNames;
  fileNames.reserve(userSelectedFiles.size());

  // For each pair in userSelectedFiles we want to add them to their related
  // vector to pass to the algorithm.
  for (const auto &userSelectedFile : userSelectedFiles) {
    fileIDs.push_back(userSelectedFile.first);
    fileNames.push_back(userSelectedFile.second);
  }

  // End of the ugly!

  // The file IDs and file names of the data file(s) the user wants to download.
  catalogAlgorithm->setProperty("FileIds", fileIDs);
  catalogAlgorithm->setProperty("FileNames", fileNames);
  catalogAlgorithm->setProperty("DownloadPath", downloadPath);
  catalogAlgorithm->setProperty("Session", sessionID);

  executeAsynchronously(catalogAlgorithm);
  // Return a vector containing the file paths to the files to download.
  return (catalogAlgorithm->getProperty("FileLocations"));
}

/**
 * Validate each input field against the related algorithm property.
 * @param inputFields :: The name of the input field and value of the field (key
 * => "StartDate", value => "00/00/0000").
 * @return The name of the input field(s) marker to update and related error to
 * throw.
 */
const std::map<std::string, std::string> CatalogHelper::validateProperties(
    const std::map<std::string, std::string> &inputFields) {
  auto catalogAlgorithm = createCatalogAlgorithm("CatalogSearch");

  // Holds the name of the marker to update if an error is found, and the
  // related error message to use.
  // E.g. key => "StartDate_err", value => "The start date for..."
  std::map<std::string, std::string> errors;

  // Validate all input elements in the map.
  for (const auto &inputField : inputFields) {
    try {
      catalogAlgorithm->setProperty(inputField.first, inputField.second);
    } catch (std::invalid_argument &) {
      std::string documentation = propertyDocumentation(
          catalogAlgorithm->getProperties(), inputField.first);

      // Add the input name + "_err" (to indicate the error marker in the GUI,
      // rather than the input field) as the key, and the related error as the
      // value.
      errors.emplace(inputField.first + "_err", documentation);
    }
  }
  // catch invalid date formats
  std::string dateField = "StartDate";
  try {

    getTimevalue(catalogAlgorithm->getProperty(dateField));
    dateField = "EndDate";

    getTimevalue(catalogAlgorithm->getProperty(dateField));
  } catch (std::invalid_argument &) {
    std::string documentation =
        propertyDocumentation(catalogAlgorithm->getProperties(), dateField);
    errors.emplace(dateField + "_err", documentation);
  }
  return errors;
}

/**
 * Creates a time_t value from an input date ("23/06/2003") for comparison.
 * @param inputDate :: string containing the date.
 * @return time_t value of date
 */
time_t CatalogHelper::getTimevalue(const std::string &inputDate) {
  // Prevent any possible errors.
  if (inputDate.empty())
    return 0;
  // A container to hold the segments of the date.
  std::vector<std::string> dateSegments;
  // Split input by "/" prior to rearranging the date
  boost::algorithm::split_regex(dateSegments, inputDate, boost::regex("/"));
  // Reorganise the date to be ISO format.
  std::string isoDate = dateSegments.at(2) + "-" + dateSegments.at(1) + "-" +
                        dateSegments.at(0) + " 00:00:00.000";
  // Return the date as time_t value.

  return Mantid::Types::Core::DateAndTime(isoDate).to_time_t();
}

/**
 * Opens auto-generated dialog, and executes the catalog login algorithm.
 * Returns true if login was a success.
 */
void CatalogHelper::showLoginDialog() {
  API::InterfaceManager interfaceMgr;
  auto dlg = interfaceMgr.createDialogFromName("CatalogLogin");
  dlg->setModal(false);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

/**
 * Creates a publishing dialog GUI and runs the publishing algorithm when "Run"
 * is pressed.
 */
void CatalogHelper::showPublishDialog() {
  API::InterfaceManager interfaceMgr;
  auto dlg = interfaceMgr.createDialogFromName("CatalogPublish");
  dlg->setModal(false);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

/**
 * Obtain the algorithm documentation for the given property.
 * @param properties :: A list of properties for a provided algorithm.
 * @param name       :: The name of the property to search for.
 * @return The documentation for a given property name.
 */
const std::string CatalogHelper::propertyDocumentation(
    const std::vector<Mantid::Kernel::Property *> &properties,
    const std::string &name) {
  for (auto property : properties) {
    if (property->name() == name) {
      return property->documentation();
    }
  }
  return "";
}

/**
 * Creates an algorithm with the provided name.
 * @param algName :: The name of the algorithm to create.
 * @return A shared pointer to the algorithm created.
 */
Mantid::API::IAlgorithm_sptr
CatalogHelper::createCatalogAlgorithm(const std::string &algName) {
  // If there is an exception we want it to be thrown.
  return Mantid::API::AlgorithmManager::Instance().create(algName);
}

/**
 * Execute the given algorithm asynchronously.
 * @param algorithm :: The algorithm to execute.
 */
void CatalogHelper::executeAsynchronously(
    const Mantid::API::IAlgorithm_sptr &algorithm) {
  Poco::ActiveResult<bool> result(algorithm->executeAsync());
  while (!result.available()) {
    QCoreApplication::processEvents();
  }
}

/**
 * Set the "search" properties to their related input fields.
 * @param catalogAlgorithm :: Algorithm to set the search properties for.
 * @param userInputFields  :: The search properties to set against the
 * algorithm.
 */
void CatalogHelper::setSearchProperties(
    const Mantid::API::IAlgorithm_sptr &catalogAlgorithm,
    const std::map<std::string, std::string> &userInputFields) {
  // This will be the workspace where the content of the search result is output
  // to.
  catalogAlgorithm->setProperty("OutputWorkspace", "__searchResults");

  // Iterate over the provided map of user input fields. For each field that
  // isn't empty (e.g. a value was input by the user)
  // then we will set the algorithm property with the key and value of that
  // specific value.
  for (const auto &userInputField : userInputFields) {
    std::string value = userInputField.second;
    // If the user has input any search terms.
    if (!value.empty()) {
      // Set the property that the search algorithm uses to: (key => FieldName,
      // value => FieldValue) (e.g., (Keywords, bob))
      catalogAlgorithm->setProperty(userInputField.first, value);
    }
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
