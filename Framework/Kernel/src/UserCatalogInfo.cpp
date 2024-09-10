// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/UserCatalogInfo.h"

namespace Mantid::Kernel {

UserCatalogInfo::UserCatalogInfo(const ICatalogInfo &catInfo, const CatalogConfigService &catalogConfigService)
    : m_catInfo(catInfo.clone()), m_mountPoint(catalogConfigService.preferredMountPoint()) {}

UserCatalogInfo::UserCatalogInfo(const UserCatalogInfo &other)
    : m_catInfo(other.m_catInfo->clone()), m_mountPoint(other.m_mountPoint) {}

const std::string UserCatalogInfo::catalogName() const { return m_catInfo->catalogName(); }

const std::string UserCatalogInfo::soapEndPoint() const { return m_catInfo->soapEndPoint(); }

const std::string UserCatalogInfo::externalDownloadURL() const { return m_catInfo->externalDownloadURL(); }

const std::string UserCatalogInfo::catalogPrefix() const { return m_catInfo->catalogPrefix(); }

const std::string UserCatalogInfo::windowsPrefix() const {
  if (m_mountPoint) {
    return m_mountPoint.value();
  }
  return m_catInfo->windowsPrefix();
}

const std::string UserCatalogInfo::macPrefix() const {
  if (m_mountPoint) {
    return m_mountPoint.value();
  }
  return m_catInfo->macPrefix();
}

const std::string UserCatalogInfo::linuxPrefix() const {
  if (m_mountPoint) {
    return m_mountPoint.value();
  }
  return m_catInfo->linuxPrefix();
}

UserCatalogInfo *UserCatalogInfo::clone() const { return new UserCatalogInfo(*this); }

} // namespace Mantid::Kernel
