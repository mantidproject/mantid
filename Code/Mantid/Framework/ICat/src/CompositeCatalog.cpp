#include "MantidICat/CompositeCatalog.h"

namespace Mantid
{
  namespace ICat
  {

    CompositeCatalog::CompositeCatalog() : m_catalogs() {}

    CompositeCatalog::~CompositeCatalog() {}

    void CompositeCatalog::add(API::ICatalog_sptr catalog)
    {

    }

    void CompositeCatalog::login(const std::string& username,const std::string& password,const std::string& url)
    {

    }

    void CompositeCatalog::logout()
    {

    }

    void CompositeCatalog::search(const CatalogSearchParam& inputs, Mantid::API::ITableWorkspace_sptr& outputws,
        const int &offset, const int &limit)
    {

    }

    int64_t CompositeCatalog::getNumberOfSearchResults(const CatalogSearchParam& inputs)
    {

    }

    void CompositeCatalog::myData(Mantid::API::ITableWorkspace_sptr& outputws)
    {

    }

    void CompositeCatalog::getDataSets(const std::string&investigationId,Mantid::API::ITableWorkspace_sptr& outputws)
    {

    }

    void CompositeCatalog::getDataFiles(const std::string&investigationId,Mantid::API::ITableWorkspace_sptr& outputws)
    {

    }

    void CompositeCatalog::listInstruments(std::vector<std::string>& instruments)
    {

    }

    void CompositeCatalog::listInvestigationTypes(std::vector<std::string>& invstTypes)
    {

    }

    void CompositeCatalog::getFileLocation(const long long&fileID,std::string& fileLocation)
    {

    }

    void CompositeCatalog::getDownloadURL(const long long& fileID,std::string& url)
    {

    }

    const std::string CompositeCatalog::getUploadURL(
        const std::string &investigationID, const std::string &createFileName, const std::string &dataFileDescription)
    {

    }

    void CompositeCatalog::keepAlive()
    {

    }

    int CompositeCatalog::keepAliveinminutes()
    {

    }

  }
}
