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

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
