#ifndef MANTIDQTWIDGETS_CATALOGHELPER_H_
#define MANTIDQTWIDGETS_CATALOGHELPER_H_

#include "MantidAPI/AlgorithmManager.h"
#include <QWidget>

namespace MantidQt
{
  namespace MantidWidgets
  {
    class CatalogHelper
    {

    public:
      /// Constructor needed to initalise member variable.
      CatalogHelper();
      /// Obtain the list of instruments that are available.
      const std::vector<std::string> getInstrumentList();
      /// Obtain the list of instruments that are available.
      const std::vector<std::string> getInvestigationTypeList();
      /// Run the search algorithm with the given user input.
      void executeSearch(const std::map<std::string, std::string> &userInputs, const int &offset = -1, const int &limit = -1);
      /// Obtain the number of search results to be returned by the query of the user.
      long getNumberOfSearchResults();
      /// Search for all related dataFiles for the specified investigation.
      void executeGetDataFiles(const int64_t &investigationId);
      /// Download dataFile (via HTTP or copy if access to archive) and return the path to it.
      const std::vector<std::string> downloadDataFiles(const std::vector<std::pair<int64_t, std::string>> &userSelectedFiles, const std::string &downloadPath);
      /// Validate each input field against the related algorithm property.
      const std::map<std::string, std::string> validateProperties(const std::map<std::string, std::string> &inputFields);
      /// Creates a time_t value from an input date ("23/06/2003") for comparison.
      time_t getTimevalue(const std::string& inputDate);
      /// Holds the number of results from executeSearch to use in getNumberOfSearchResults.
      long m_numberOfResults;

    private:
      /// Creates an algorithm with the name provided.
      Mantid::API::IAlgorithm_sptr createCatalogAlgorithm(const std::string& algName);
      /// Obtain the documentation for a given name from the given algorithm properties.
      const std::string propertyDocumentation(const std::vector<Mantid::Kernel::Property*> &properties, const std::string &name);
      /// Execute the given algorithm asynchronously.
      void executeAsynchronously(const Mantid::API::IAlgorithm_sptr &algorithm);

    };
  } // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTWIDGETS_CATALOGHELPER_H_
