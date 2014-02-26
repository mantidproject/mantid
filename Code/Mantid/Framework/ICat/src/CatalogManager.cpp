#include "MantidICat/CatalogManager.h"
#include "MantidAPI/CatalogFactory.h"

namespace Mantid
{
  namespace ICat
  {
    CatalogManagerImpl::CatalogManagerImpl() : m_activeCatalogs(), m_compositeCatalog(new CompositeCatalog()) {}

    CatalogManagerImpl::~CatalogManagerImpl(){}

    /**
     * Creates a new catalog and adds it to the compositeCatalog and activeCatalog list.
     * @param facilityName :: The name of the facility to create a catalog for.
     * @return A catalog for the facility specified.
     */
    API::ICatalog_sptr CatalogManagerImpl::create(const std::string facilityName)
    {
       auto catalog = API::CatalogFactory::Instance().create(facilityName);
       m_compositeCatalog->add(catalog);
       m_activeCatalogs.insert(std::make_pair("",catalog));
       return catalog;
    }

    /**
     * Obtain a specific catalog using the sessionID.
     * @param sessionID :: The session to search for in the active catalogs list.
     * @return A specific catalog using the sessionID.
     */
    API::ICatalog_sptr CatalogManagerImpl::getCatalog(const std::string sessionID)
    {
      auto pos = m_activeCatalogs.find(sessionID);
      // If the key element exists in the map we want the related catalog.
      if (pos != m_activeCatalogs.end()) return pos->second;
    }

    /**
     * Obtain a list of all active catalogs.
     * @return A composite catalog object as it holds and performs operations on all catalogs.
     */
    boost::shared_ptr<CompositeCatalog> CatalogManagerImpl::getCatalogs()
    {
      return m_compositeCatalog;
    }

    /**
     * Destroy and remove a specific catalog from the active catalogs list and the composite catalog.
     * @param sessionID :: The session to search for in the active catalogs list.
     */
    void CatalogManagerImpl::destroyCatalog(const std::string sessionID)
    {

    }

    /**
     * Destroy all active catalogs.
     */
    void CatalogManagerImpl::destroyCatalogs()
    {

    }

  }
}
