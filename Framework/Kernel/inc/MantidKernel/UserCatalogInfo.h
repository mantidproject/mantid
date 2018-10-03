// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_USERCATALOGINFO_H_
#define MANTID_KERNEL_USERCATALOGINFO_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ICatalogInfo.h"
#include <boost/optional.hpp>
#include <memory>

namespace Mantid {
namespace Kernel {

using OptionalPath = boost::optional<std::string>;

class CatalogConfigService {
public:
  virtual OptionalPath preferredMountPoint() const = 0;
  virtual ~CatalogConfigService() = default;
};

template <typename T>
CatalogConfigService *makeCatalogConfigServiceAdapter(
    const T &adaptee, const std::string key = "icatDownload.mountPoint") {
  class Adapter : public CatalogConfigService {
  private:
    const T &m_adaptee;
    std::string m_key;

  public:
    Adapter(const T &adaptee, const std::string key)
        : m_adaptee(adaptee), m_key(key) {}
    OptionalPath preferredMountPoint() const override {
      return m_adaptee.getString(m_key);
    }
  };
  return new Adapter(adaptee, key);
}

/** UserCatalogInfo : Takes catalog info from the facility (via CatalogInfo),
  but provides
  the ability to override the facility defaults based on user preferences.
*/

class MANTID_KERNEL_DLL UserCatalogInfo : public ICatalogInfo {
public:
  UserCatalogInfo(const ICatalogInfo &catInfo,
                  const CatalogConfigService &catalogConfigService);

  UserCatalogInfo(const UserCatalogInfo &other);

  // ICatalogInfo interface
  const std::string catalogName() const override;
  const std::string soapEndPoint() const override;
  const std::string externalDownloadURL() const override;
  const std::string catalogPrefix() const override;
  const std::string windowsPrefix() const override;
  const std::string macPrefix() const override;
  const std::string linuxPrefix() const override;
  UserCatalogInfo *clone() const override;

private:
  /// Facility catalog info. Aggregation only solution here.
  const std::unique_ptr<ICatalogInfo> m_catInfo;
  /// Archive mount point
  const OptionalPath m_mountPoint;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_USERCATALOGINFO_H_ */
