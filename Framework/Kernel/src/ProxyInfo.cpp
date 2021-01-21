// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ProxyInfo.h"
#include <stdexcept>

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ProxyInfo::ProxyInfo() : m_host(""), m_port(0), m_isHttpProxy(false), m_isEmptyProxy(true) {}

/**
 * Proxy information constructor
 * @param host : host url
 * @param port : port number
 * @param isHttpProxy : is this a http proxy
 */
ProxyInfo::ProxyInfo(const std::string &host, const int port, const bool isHttpProxy)
    : m_host(host), m_port(port), m_isHttpProxy(isHttpProxy), m_isEmptyProxy(false) {
  if (host.empty() || port == 0) {
    m_isEmptyProxy = true;
  }
}

/**
 * Host url
 * @return host url or throws if an unset proxy.
 */
std::string ProxyInfo::host() const {
  if (m_isEmptyProxy) {
    throw std::logic_error("Calling host on an undefined proxy");
  }
  return m_host;
}

/**
 * Port Number
 * @return Port number or throws if an unset proxy.
 */
int ProxyInfo::port() const {
  if (m_isEmptyProxy) {
    throw std::logic_error("Calling port on an undefined proxy");
  }
  return m_port;
}

/**
 * Is this a http proxy
 * @return True if a http proxy or throws if an unset proxy.
 */
bool ProxyInfo::isHttpProxy() const { return m_isHttpProxy; }

/**
 *
 * @return True if an empty proxy.
 */
bool ProxyInfo::emptyProxy() const { return m_isEmptyProxy; }

} // namespace Kernel
} // namespace Mantid
