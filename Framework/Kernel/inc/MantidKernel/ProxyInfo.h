// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PROXYINFO_H_
#define MANTID_KERNEL_PROXYINFO_H_

#include "MantidKernel/System.h"
#include <string>

namespace Mantid {
namespace Kernel {

/** ProxyInfo : Container for carrying around network proxy information
 */
class DLLExport ProxyInfo {
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

#endif /* MANTID_KERNEL_PROXYINFO_H_ */
