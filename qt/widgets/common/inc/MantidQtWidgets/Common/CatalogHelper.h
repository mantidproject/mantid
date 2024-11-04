// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt {
namespace MantidWidgets {
class EXPORT_OPT_MANTIDQT_COMMON CatalogHelper {

public:
  /// Obtain the list of instruments that are available for the given session
  /// information.
  const std::vector<std::string> getInstrumentList(const std::vector<std::string> &sessionIDs);
  /// Obtain the list of instruments that are available.
  const std::vector<std::string> getInvestigationTypeList(const std::vector<std::string> &sessionIDs);
  /// Run the search algorithm with the given user input.
  void executeSearch(const std::map<std::string, std::string> &userInputs, const int &offset, const int &limit,
                     const std::vector<std::string> &sessionIDs);
  /// Obtain the number of search results to be returned by the query of the
  /// user.
  int64_t getNumberOfSearchResults(const std::map<std::string, std::string> &userInputFields,
                                   const std::vector<std::string> &sessionIDs);
  /// Search for all related dataFiles for the specified investigation.
  void executeGetDataFiles(const std::string &investigationId, const std::string &sessionID);
  /// Download dataFile (via HTTP or copy if access to archive) and return the
  /// path to it.
  const std::vector<std::string>
  downloadDataFiles(const std::vector<std::pair<int64_t, std::string>> &userSelectedFiles,
                    const std::string &downloadPath, const std::string &sessionID);
  /// Validate each input field against the related algorithm property.
  const std::map<std::string, std::string> validateProperties(const std::map<std::string, std::string> &inputFields);
  /// Creates a time_t value from an input date ("23/06/2003") for comparison.
  time_t getTimevalue(const std::string &inputDate);
  /// Executes the catalog login algorithm dialog, returning immediately
  void showLoginDialog();
  /// Create a publishing dialog.
  void showPublishDialog();

private:
  /// Creates an algorithm with the name provided.
  Mantid::API::IAlgorithm_sptr createCatalogAlgorithm(const std::string &algName);
  /// Obtain the documentation for a given name from the given algorithm
  /// properties.
  const std::string propertyDocumentation(const std::vector<Mantid::Kernel::Property *> &properties,
                                          const std::string &name);
  /// Execute the given algorithm asynchronously.
  void executeAsynchronously(const Mantid::API::IAlgorithm_sptr &algorithm);
  /// Set the "search" properties to their related input fields.
  void setSearchProperties(const Mantid::API::IAlgorithm_sptr &catalogAlgorithm,
                           const std::map<std::string, std::string> &userInputFields);
};
} // namespace MantidWidgets
} // namespace MantidQt
