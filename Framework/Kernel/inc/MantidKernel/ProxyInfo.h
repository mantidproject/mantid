#ifndef MANTID_KERNEL_PROXYINFO_H_
#define MANTID_KERNEL_PROXYINFO_H_

#include "MantidKernel/System.h"
#include <string>

namespace Mantid {
namespace Kernel {

/** ProxyInfo : Container for carrying around network proxy information

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ProxyInfo {
private:
  std::string m_host;
  int m_port;
  bool m_isHttpProxy;
  bool m_isEmptyProxy;

public:
  virtual ~ProxyInfo();
  ProxyInfo();
  ProxyInfo(const ProxyInfo &other);
  ProxyInfo &operator=(const ProxyInfo &other);
  ProxyInfo(const std::string &host, const int port, const bool isHttpProxy);
  std::string host() const;
  int port() const;
  bool isHttpProxy() const;
  bool emptyProxy() const;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_PROXYINFO_H_ */
