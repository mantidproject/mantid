#ifndef MANTIDQTWIDGETS_ICATHELPER_H_
#define MANTIDQTWIDGETS_ICATHELPER_H_

#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
    class ICatHelper
    {

    public:
      /// Obtain the list of instruments that are available.
      std::vector<std::string> getInstrumentList();
      /// Obtain the list of instruments that are available.
      std::vector<std::string> getInvestigationTypeList();
      /// Run the search algorithm with the given user input.
      void executeSearch(std::map<std::string, std::string> userInputs);
      /// Search for all related dataFiles for the specified investigation.
      void executeGetDataFiles(int64_t investigationId);
      /// Download dataFile (via HTTP or copy if access to archive) and return the path to it.
      std::vector<std::string> downloadDataFiles(std::vector<std::pair<int64_t, std::string>> userSelectedFiles, std::string downloadPath);
      /// Validate each input field against the related algorithm property.
      std::map<std::string, std::string> validateProperties(std::map<std::string, std::string> &inputFields);
      /// Using a property (isValid) in the list instruments algorithm verify if the session is valid.
      bool validSession();

    private:
      /// Creates an algorithm with the name provided.
      Mantid::API::IAlgorithm_sptr createCatalogAlgorithm(const std::string& algName);
      /// Obtain the documentation for a given name from the given algorithm properties.
      const std::string propertyDocumentation(const std::vector<Mantid::Kernel::Property*> &properties, const std::string &name);

    };
  } // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTWIDGETS_ICATHELPER_H_
