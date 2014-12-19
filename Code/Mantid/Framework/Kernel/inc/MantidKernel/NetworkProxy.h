#ifndef MANTID_KERNEL_NETWORKPROXY_H_
#define MANTID_KERNEL_NETWORKPROXY_H_

#include "MantidKernel/System.h"
#include "MantidKernel/ProxyInfo.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Kernel {

/** NetworkProxy : Network proxy utility for getting network proxy information.

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
class DLLExport NetworkProxy {
public:
  /// Constructor
  NetworkProxy();

  /// Get http proxy information.
  ProxyInfo getHttpProxy(const std::string &targetURLString);

  /// Destructor
  virtual ~NetworkProxy();

private:
  /// Logger object
  Mantid::Kernel::Logger m_logger;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_NETWORKPROXY_H_ */
