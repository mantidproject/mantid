#include "MantidQtMantidWidgets/ICatHelper.h"
#include "MantidQtAPI/AlgorithmDialog.h"

namespace MantidQt
{
  namespace MantidWidgets
  {

    /**
     * Obtain the list of instruments from the ICAT Catalog algorithm.
     * @return :: A vector containing the list of all instruments available.
     */
    std::vector<std::string> ICatHelper::getInstrumentList()
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm;
      try
      {
        catalogAlgorithm = Mantid::API::AlgorithmManager::Instance().create("CatalogListInstruments");
      }
      catch(std::runtime_error& exception)
      {
        exception.what();
      }

      catalogAlgorithm->execute();
      // return the vector containing the list of instruments available.
      return (catalogAlgorithm->getProperty("InstrumentList"));
    }

    /**
     * Obtain the list of investigation types from the ICAT Catalog algorithm.
     * @return :: A vector containing the list of all investigation types available.
     */
    std::vector<std::string> ICatHelper::getInvestigationTypeList()
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm;
      try
      {
        catalogAlgorithm = Mantid::API::AlgorithmManager::Instance().create("CatalogListInvestigationTypes");
      }
      catch(std::runtime_error& exception)
      {
        exception.what();
      }

      catalogAlgorithm->execute();
      // return the vector containing the list of investigation types available.
      return (catalogAlgorithm->getProperty("InvestigationTypes"));
    }

    /**
     * Search the archive with the user input terms provided.
     * @param :: A map containing all users' search fields - (key => FieldName, value => FieldValue).
     */
    void ICatHelper::executeSearch(std::map<std::string, std::string> userInputFields)
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm;
      try
      {
        catalogAlgorithm = Mantid::API::AlgorithmManager::Instance().create("CatalogSearch");
      }
      catch(std::runtime_error& exception)
      {
        exception.what();
      }

      // This will be the workspace where the content of the search result is output to.
      catalogAlgorithm->setProperty("OutputWorkspace", "searchResults");

      // Iterate over the provided map of user input fields. For each field that isn't empty (e.g. a value was input by the user)
      // then we will set the algorithm property with the key and value of that specific value.
      for ( std::map<std::string, std::string>::const_iterator it = userInputFields.begin(); it != userInputFields.end(); it++)
      {
        std::string value = it->second;
        // If the user has input any search terms.
        if (!value.empty())
        {
          // Set the property that the search algorithm uses to: (key => FieldName, value => FieldValue) (e.g., (Keywords, bob))
          catalogAlgorithm->setProperty(it->first, value);
        }
      }
      catalogAlgorithm->execute();
    }
  }

}
