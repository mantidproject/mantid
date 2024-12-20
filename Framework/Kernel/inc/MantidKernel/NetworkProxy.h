// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ProxyInfo.h"

namespace Mantid {
namespace Kernel {

/** NetworkProxy : Network proxy utility for getting network proxy information.
 */
class MANTID_KERNEL_DLL NetworkProxy {
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
