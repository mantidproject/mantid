#ifndef MANTID_ICAT_COMPOSITECATALOG_H_
#define MANTID_ICAT_COMPOSITECATALOG_H_

#include "MantidAPI/ICatalog.h"

namespace Mantid
{
  namespace ICat
  {
    class DLLExport CompositeCatalog : public API::ICatalog
    {
      public:
        /// Constructor
        CompositeCatalog();
        /// Destructor
        ~CompositeCatalog();
        /// Adds a catalog to the list of catalogs (m_catalogs)
        void add(API::ICatalog_sptr catalog);

        /// Log the user into the catalog system.
        virtual void login(const std::string& username,const std::string& password,const std::string& endpoint);
        /// Log the user out of the catalog system.
        virtual void logout();
        /// Search the catalog for data.
        virtual void search(const CatalogSearchParam& inputs,API::ITableWorkspace_sptr& outputws,
            const int &offset, const int &limit);
        /// Obtain the number of results returned by the search method.
        virtual int64_t getNumberOfSearchResults(const CatalogSearchParam& inputs);
        /// Show the logged in user's investigations search results.
        virtual void myData(API::ITableWorkspace_sptr& outputws);
        /// Get datasets.
        virtual void getDataSets(const std::string&investigationId,API::ITableWorkspace_sptr& outputws);
        /// Get datafiles
        virtual void getDataFiles(const std::string&investigationId,API::ITableWorkspace_sptr& outputws);
        /// Get instruments list
        virtual void listInstruments(std::vector<std::string>& instruments);
        /// Get investigationtypes list
        virtual void listInvestigationTypes(std::vector<std::string>& invstTypes);
        /// Get the file location string(s) from archive.
        virtual void getFileLocation(const long long&fileID,std::string& fileLocation);
        /// Get the url based on the fileID.
        virtual void getDownloadURL(const long long& fileID,std::string& url);
        /// get URL of where to PUT (publish) files.
        virtual const std::string getUploadURL(
            const std::string &investigationID, const std::string &createFileName, const std::string &dataFileDescription);
        /// Keep current session alive
        virtual void keepAlive();
        /// Keep alive in minutes
        virtual int keepAliveinminutes();

      private:
        std::list<API::ICatalog_sptr> m_catalogs;
    };
  }
}
#endif
