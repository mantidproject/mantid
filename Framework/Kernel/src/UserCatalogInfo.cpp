#include "MantidKernel/UserCatalogInfo.h"

namespace Mantid {
namespace Kernel {

UserCatalogInfo::UserCatalogInfo(
    const ICatalogInfo &catInfo,
    const CatalogConfigService &catalogConfigService)
    : m_catInfo(catInfo), m_catalogConfigService(catalogConfigService) {}

UserCatalogInfo::~UserCatalogInfo() {}

const std::string UserCatalogInfo::catalogName() const {
  return m_catInfo.catalogName();
}

const std::string UserCatalogInfo::soapEndPoint() const {
  return m_catInfo.soapEndPoint();
}

const std::string UserCatalogInfo::externalDownloadURL() const {
  return m_catInfo.externalDownloadURL();
}

const std::string UserCatalogInfo::catalogPrefix() const {
  return m_catInfo.catalogPrefix();
}

const std::string UserCatalogInfo::windowsPrefix() const {
  auto userPrefix = m_catalogConfigService.preferredMountPoint();
  if (userPrefix) {
    return userPrefix.get();
  }
  return m_catInfo.windowsPrefix();
}

const std::string UserCatalogInfo::macPrefix() const {
  auto userPrefix = m_catalogConfigService.preferredMountPoint();
  if (userPrefix) {
    return userPrefix.get();
  }
  return m_catInfo.macPrefix();
}

const std::string UserCatalogInfo::linuxPrefix() const {
  auto userPrefix = m_catalogConfigService.preferredMountPoint();
  if (userPrefix) {
    return userPrefix.get();
  }
  return m_catInfo.linuxPrefix();
}

} // namespace Kernel
} // namespace Mantid
