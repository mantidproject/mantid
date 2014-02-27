#include "MantidICat/CatalogManager.h"
#include "MantidICat/CompositeCatalog.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

namespace Mantid
{
  namespace ICat
  {
    CatalogManagerImpl::CatalogManagerImpl() : m_activeCatalogs() {}

    CatalogManagerImpl::~CatalogManagerImpl(){}

    /**
     * Creates a new catalog and adds it to the compositeCatalog and activeCatalog list.
     * @param facilityName :: The name of the facility to obtain the catalog name from.
     * @return A catalog for the facility specified.
     */
    API::ICatalog_sptr CatalogManagerImpl::create(const std::string facilityName)
    {
      std::string className = Kernel::ConfigService::Instance().getFacility(facilityName).catalogInfo().catalogName();
      auto catalog = API::CatalogFactory::Instance().create(className);
      m_activeCatalogs.insert(std::make_pair("",catalog));
      return catalog;
    }

    /**
     * Obtain a specific catalog using the sessionID, otherwise return all active catalogs.
     * @param sessionID :: The session to search for in the active catalogs list.
     * @return A specific catalog using the sessionID, otherwise returns all active catalogs
     */
    API::ICatalog_sptr CatalogManagerImpl::getCatalog(const std::string sessionID)
    {
      if(sessionID.empty())
      {
        auto composite = boost::make_shared<CompositeCatalog>();
        for (auto item = m_activeCatalogs.begin(); item != m_activeCatalogs.end(); ++item)
        {
          composite->add(item->second);
        }
        return composite;
      }

      auto pos = m_activeCatalogs.find(sessionID);
      // If the key element exists in the map we want the related catalog.
      if (pos != m_activeCatalogs.end()) return pos->second;
      else throw std::runtime_error("The session ID you have provided is invalid");
    }

    /**
     * Destroy and remove a specific catalog from the active catalogs list and the composite catalog.
     * @param sessionID :: The session to search for in the active catalogs list.
     */
    void CatalogManagerImpl::destroyCatalog(const std::string sessionID)
    {
      auto pos = m_activeCatalogs.find(sessionID);

      if (pos != m_activeCatalogs.end())
      {
        pos->second->logout();
        m_activeCatalogs.erase(pos);
      }
    }

    /**
     * Destroy all active catalogs.
     */
    void CatalogManagerImpl::destroyCatalogs()
    {
      for (auto item = m_activeCatalogs.begin(); item != m_activeCatalogs.end(); ++item)
      {
        item->second->logout();
      }

      m_activeCatalogs.clear();
    }

  }
}
