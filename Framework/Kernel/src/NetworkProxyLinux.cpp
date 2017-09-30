#include "MantidKernel/NetworkProxy.h"
#include <Poco/URI.h>

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NetworkProxy::NetworkProxy() : m_logger("network_proxy_logger_generic") {}

ProxyInfo NetworkProxy::getHttpProxy(const std::string &) {
  ProxyInfo proxyInfo; // NoProxy.
  char *proxy_var = getenv("http_proxy");
  if (proxy_var == nullptr)
    proxy_var = getenv("HTTP_PROXY");

  if (proxy_var != nullptr) {
    Poco::URI uri_p(proxy_var);
    proxyInfo =
        ProxyInfo(uri_p.getHost(), uri_p.getPort(), true /*http proxy*/);
  }
  return proxyInfo;
}

} // namespace Kernel
} // namespace Mantid
