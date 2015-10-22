#ifndef MANTID_KERNEL_USERCATALOGINFO_H_
#define MANTID_KERNEL_USERCATALOGINFO_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ICatalogInfo.h"
#include "Poco/Path.h"
#include <boost/optional.hpp>
#include <memory>

namespace Mantid {
namespace Kernel {

typedef boost::optional<std::string> OptionalPath;

class CatalogConfigService {
public:
  virtual OptionalPath preferredMountPoint() const = 0;
  virtual ~CatalogConfigService() {}
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
    virtual OptionalPath preferredMountPoint() const {
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
  virtual ~UserCatalogInfo();

  // ICatalogInfo interface
  const std::string catalogName() const;
  const std::string soapEndPoint() const;
  const std::string externalDownloadURL() const;
  const std::string catalogPrefix() const;
  const std::string windowsPrefix() const;
  const std::string macPrefix() const;
  const std::string linuxPrefix() const;
  UserCatalogInfo *clone() const;

private:
  /// Facility catalog info. Aggregation only solution here.
  const std::unique_ptr<ICatalogInfo> m_catInfo;
  /// Archive mount point
  const OptionalPath m_mountPoint;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_USERCATALOGINFO_H_ */
