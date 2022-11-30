// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {

/** ProxyInfo : Container for carrying around network proxy information
 */
class MANTID_KERNEL_DLL ProxyInfo {
private:
  std::string m_host;
  int m_port;
  bool m_isHttpProxy;
  bool m_isEmptyProxy;

public:
  virtual ~ProxyInfo() = default;
  ProxyInfo();
  ProxyInfo(const std::string &host, const int port, const bool isHttpProxy);
  std::string host() const;
  int port() const;
  bool isHttpProxy() const;
  bool emptyProxy() const;
};

} // namespace Kernel
} // namespace Mantid
