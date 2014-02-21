#include "MantidICat/CompositeCatalog.h"

namespace Mantid
{
  namespace ICat
  {
    CompositeCatalog::CompositeCatalog() : m_catalogs() {}

    /**
     * When the object is deleted we want to clear the catalog container.
     */
    CompositeCatalog::~CompositeCatalog()
    {
      m_catalogs.clear();
    }

    /**
     * Add a catalog to the catalog container.
     * @param catalog :: The catalog to add to the container.
     */
    void CompositeCatalog::add(API::ICatalog_sptr catalog)
    {
      m_catalogs.push_back(catalog);
    }

    /**
     * Authenticate the user against all catalogues in the container.
     * @param username :: The login name of the user.
     * @param password :: The password of the user.
     * @param endpoint :: The endpoint url of the catalog to log in to.
     */
    void CompositeCatalog::login(const std::string& username,const std::string& password,const std::string& endpoint)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->login(username, password, endpoint);
      }
    }

    /**
     * Log the user out of all catalogues in the container.
     */
    void CompositeCatalog::logout()
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->logout();
      }
    }

    /**
     * Search through all catalogues that are in the container.
     * @param inputs   :: A reference to a class containing the user's inputs.
     * @param outputws :: A shared pointer to workspace were the search results are stored.
     * @param offset   :: Skip this many rows and start returning rows from this point.
     * @param limit    :: The limit of the number of rows returned by the query.
     */
    void CompositeCatalog::search(const CatalogSearchParam& inputs,API::ITableWorkspace_sptr& outputws,
        const int &offset, const int &limit)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->search(inputs, outputws, offset, limit);
      }
    }

    /**
     * Obtain the number of investigations to be returned by the catalog.
     * @return The number of investigations from the search performed.
     */
    int64_t CompositeCatalog::getNumberOfSearchResults(const CatalogSearchParam& inputs)
    {
      int64_t numberOfSearchResults = 0;
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        numberOfSearchResults += (*catalog)->getNumberOfSearchResults(inputs);
      }
      return numberOfSearchResults;
    }

    /**
     * Obtain and save the investigations that the user is an investigator of within each catalog.
     * @param outputws :: The workspace to store the results.
     */
    void CompositeCatalog::myData(API::ITableWorkspace_sptr& outputws)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->myData(outputws);
      }
    }

    /**
     * Obtain and save the datasets for a given investigation for each catalog in the container.
     * @param investigationId :: A unique identifier of the investigation.
     * @param outputws        :: The workspace to store the results.
     */
    void CompositeCatalog::getDataSets(const std::string&investigationId,API::ITableWorkspace_sptr& outputws)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->getDataSets(investigationId, outputws);
      }
    }

    /**
     * Obtain and save the datafiles for a given investigation for each catalog in the container.
     * @param investigationId :: A unique identifier of the investigation.
     * @param outputws        :: The workspace to store the results.
     */
    void CompositeCatalog::getDataFiles(const std::string&investigationId,API::ITableWorkspace_sptr& outputws)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->getDataFiles(investigationId, outputws);
      }
    }

    /**
     * Obtain a list of instruments from each catalog in the container.
     * @param instruments :: A reference to the vector to store the results.
     */
    void CompositeCatalog::listInstruments(std::vector<std::string>& instruments)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->listInstruments(instruments);
      }
    }

    /**
     * Obtain a list of investigations from each catalog in the container.
     * @param invstTypes :: A reference to the vector to store the results.
     */
    void CompositeCatalog::listInvestigationTypes(std::vector<std::string>& invstTypes)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->listInvestigationTypes(invstTypes);
      }
    }

    /**
     * Gets the datafile location string from the archives from each catalog in the container.
     * @param fileID       :: The id of the file to obtain.
     * @param fileLocation :: A reference to store the location of the datafile in the archives.
     */
    void CompositeCatalog::getFileLocation(const long long&fileID,std::string& fileLocation)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->getFileLocation(fileID, fileLocation);
      }
    }

    /**
     * Obtain a download url for a file from each catalog in the container.
     * @param fileID :: The id of the file to download.
     * @param url    :: The url to download the file from.
     */
    void CompositeCatalog::getDownloadURL(const long long& fileID,std::string& url)
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->getDownloadURL(fileID, url);
      }
    }

    /**
     * Obtain an upload url for a file from each catalog in the container.
     * @param investigationID :: The investigation used to obtain the related dataset ID.
     * @param createFileName  :: The name to give to the file being saved.
     * @param dataFileDescription :: The description of the data file being saved.
     * @return The url to PUT datafiles to.
     */
    const std::string CompositeCatalog::getUploadURL(const std::string &investigationID,
        const std::string &createFileName, const std::string &dataFileDescription)
    {
      std::string uploadURL;
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        uploadURL = (*catalog)->getUploadURL(investigationID, createFileName, dataFileDescription);
      }
      return uploadURL;
    }

    /**
     * Keep each catalog session alive in the container.
     */
    void CompositeCatalog::keepAlive()
    {
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        (*catalog)->keepAlive();
      }
    }

    /**
     * Keep each catalog alive in the container in minutes.
     */
    int CompositeCatalog::keepAliveinminutes()
    {
      int numberOfMinutes = 0;
      for(auto catalog = m_catalogs.begin(); catalog != m_catalogs.end(); ++catalog)
      {
        numberOfMinutes = (*catalog)->keepAliveinminutes();
      }
      return numberOfMinutes;
    }

  }
}
