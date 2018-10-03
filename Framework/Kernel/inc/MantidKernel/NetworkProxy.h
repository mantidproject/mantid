// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_NETWORKPROXY_H_
#define MANTID_KERNEL_NETWORKPROXY_H_

#include "MantidKernel/Logger.h"
#include "MantidKernel/ProxyInfo.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Kernel {

/** NetworkProxy : Network proxy utility for getting network proxy information.
*/
class DLLExport NetworkProxy {
public:
  /// Constructor
  NetworkProxy();

  /// Get http proxy information.
  ProxyInfo getHttpProxy(const std::string &targetURLString);

  /// Destructor
  virtual ~NetworkProxy() = default;

private:
  /// Logger object
  Mantid::Kernel::Logger m_logger;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_NETWORKPROXY_H_ */
