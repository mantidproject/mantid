#include "MantidQtMantidWidgets/ICatHelper.h"
#include "MantidQtAPI/AlgorithmDialog.h"

#include <QCoreApplication>

namespace MantidQt
{
  namespace MantidWidgets
  {

    /**
     * Obtain the list of instruments from the ICAT Catalog algorithm.
     * @return A vector containing the list of all instruments available.
     */
    std::vector<std::string> ICatHelper::getInstrumentList()
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm = createCatalogAlgorithm("CatalogListInstruments");
      catalogAlgorithm->execute();
      // return the vector containing the list of instruments available.
      return (catalogAlgorithm->getProperty("InstrumentList"));
    }

    /**
     * Obtain the list of investigation types from the ICAT Catalog algorithm.
     * @return A vector containing the list of all investigation types available.
     */
    std::vector<std::string> ICatHelper::getInvestigationTypeList()
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm = createCatalogAlgorithm("CatalogListInvestigationTypes");
      catalogAlgorithm->execute();
      // return the vector containing the list of investigation types available.
      return (catalogAlgorithm->getProperty("InvestigationTypes"));
    }

    /**
     * Search the archive with the user input terms provided and save them to a workspace ("searchResults").
     * @param userInputFields :: A map containing all users' search fields - (key => FieldName, value => FieldValue).
     */
    void ICatHelper::executeSearch(std::map<std::string, std::string> userInputFields)
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm = createCatalogAlgorithm("CatalogSearch");

      // This will be the workspace where the content of the search result is output to.
      catalogAlgorithm->setProperty("OutputWorkspace", "__searchResults");

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
      // Allow asynchronous execution to update label while search is being carried out.
      Poco::ActiveResult<bool> result(catalogAlgorithm->executeAsync());
      while( !result.available() )
      {
        QCoreApplication::processEvents();
      }
    }

    /**
     * Search the archives for all dataFiles related to an "investigation id" then save results to workspace ("dataFileResults").
     * @param investigationId :: The investigation id to use for the search.
     */
    void ICatHelper::executeGetDataFiles(int64_t investigationId)
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm = createCatalogAlgorithm("CatalogGetDataFiles");

      // Search for all related dataFiles to this investigation id.
      catalogAlgorithm->setProperty("InvestigationId", investigationId);
      // This will be the workspace where the content of the search result is saved to.
      catalogAlgorithm->setPropertyValue("OutputWorkspace","__dataFileResults");

      // Allow asynchronous execution to update label(s) while search is being carried out.
      Poco::ActiveResult<bool> result(catalogAlgorithm->executeAsync());
      while( !result.available() )
      {
        QCoreApplication::processEvents();
      }
    }

    /**
     * Retrieve the path(s) to the file that was downloaded (via HTTP) or is stored in the archive.
     * @param userSelectedFiles :: The file(s) the user has selected and wants to download.
     * @param downloadPath      :: The location to save the datafile(s).
     * @return A vector containing the paths to the file(s) the user wants.
     */
    std::vector<std::string> ICatHelper::downloadDataFiles(std::vector<std::pair<int64_t, std::string>> userSelectedFiles, std::string downloadPath)
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm = createCatalogAlgorithm("CatalogDownloadDataFiles");

      // Prepare for the ugly!

      // These two vectors are required by the "CatalogDownloadDataFiles" algorithm.
      std::vector<int64_t> fileIDs;
      std::vector<std::string> fileNames;

      // For each pair in userSelectedFiles we want to add them to their related vector to pass to the algorithm.
      for (std::vector<std::pair<int64_t,std::string>>::iterator it = userSelectedFiles.begin(); it != userSelectedFiles.end(); ++it)
      {
        fileIDs.push_back(it->first);
        fileNames.push_back(it->second);
      }

      // End of the ugly!

      // The file IDs and file names of the data file(s) the user wants to download.
      catalogAlgorithm->setProperty("FileIds",fileIDs);
      catalogAlgorithm->setProperty("FileNames",fileNames);
      catalogAlgorithm->setProperty("downloadPath",downloadPath);

      Poco::ActiveResult<bool> result(catalogAlgorithm->executeAsync());
      while( !result.available() )
      {
        //TODO: Inform the user where the file was saved to depending on result, e.g:
        // (You do not have access to the archives. Downloading requested file over Internet...)
        QCoreApplication::processEvents();
      }
      // Return a vector containing the file paths to the files to download.
      return (catalogAlgorithm->getProperty("FileLocations"));
    }

    /**
     * Creates an algorithm with the provided name.
     * @param algName :: The name of the algorithm to create.
     * @return A shared pointer to the algorithm created.
     */
    Mantid::API::IAlgorithm_sptr ICatHelper::createCatalogAlgorithm(const std::string& algName)
    {
      Mantid::API::IAlgorithm_sptr catalogAlgorithm;
      try
      {
        catalogAlgorithm = Mantid::API::AlgorithmManager::Instance().create(algName);
      }
      catch(std::runtime_error& exception)
      {
        exception.what();
      }
      // Since no exceptions have occurred we return the algorithm.
      return catalogAlgorithm;
    }


  } // namespace MantidWidgets
} // namespace MantidQt
