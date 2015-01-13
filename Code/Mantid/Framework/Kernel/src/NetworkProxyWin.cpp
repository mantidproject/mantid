// Only compile on windows.
#if defined(_WIN32) || defined(_WIN64)

#include "MantidKernel/NetworkProxy.h"
// std
#include <sstream>
// windows
#include <windows.h>
#include <Winhttp.h>

namespace Mantid {
namespace Kernel {

bool get_proxy_configuration_win(const std::string &target_url,
                                 std::string &proxy_str, std::string &err_msg) {
  HINTERNET hSession = NULL;
  std::wstring proxy;
  std::wstring wtarget_url;
  if (target_url.find("http://") == std::string::npos) {
    wtarget_url = L"http://";
  }
  wtarget_url += std::wstring(target_url.begin(), target_url.end());
  bool fail = false;
  std::stringstream info;
  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_proxy;
  WINHTTP_AUTOPROXY_OPTIONS proxy_options;
  WINHTTP_PROXY_INFO proxy_info;
  ZeroMemory(&proxy_options, sizeof(proxy_options));
  ZeroMemory(&ie_proxy, sizeof(ie_proxy));
  ZeroMemory(&proxy_info, sizeof(proxy_info));

  // the loop is just to allow us to go out of this session whenever we want
  while (true) {
    // Use WinHttpOpen to obtain a session handle.
    hSession = WinHttpOpen(L"ScriptRepository FindingProxy/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
      fail = true;
      info << "Failed to create the session (Error Code: " << GetLastError()
           << ").";
      break;
    }
    // get the configuration of the web browser
    if (!WinHttpGetIEProxyConfigForCurrentUser(&ie_proxy)) {
      fail = true;
      info << "Could not find the proxy settings (Error code :"
           << GetLastError();
      break;
    }

    if (ie_proxy.lpszProxy) {
      // the proxy was already given,
      // it is not necessary to query the system for the auto proxy
      proxy = ie_proxy.lpszProxy;
      break;
    }

    if (ie_proxy.fAutoDetect) {
      // if auto detect, than setup the proxy to auto detect
      proxy_options.dwFlags |= WINHTTP_AUTOPROXY_AUTO_DETECT;
      proxy_options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DNS_A;
    }

    if (ie_proxy.lpszAutoConfigUrl) {
      // configure to auto proxy
      proxy_options.dwFlags |= WINHTTP_AUTOPROXY_CONFIG_URL;
      proxy_options.lpszAutoConfigUrl = ie_proxy.lpszAutoConfigUrl;
    }

    if (!WinHttpGetProxyForUrl(hSession, wtarget_url.c_str(), &proxy_options,
                               &proxy_info)) {
      info << "Could not find the proxy for this url (Error code :"
           << GetLastError() << ").";
      fail = true;
      break;
    }

    // std::cout << "get proxy for url passed" << std::endl;
    if (proxy_info.dwAccessType == WINHTTP_ACCESS_TYPE_NO_PROXY) {
      // no proxy (return an empty proxy)
      break;
    }

    if (proxy_info.lpszProxy) {
      // proxy found. Get it.
      proxy = proxy_info.lpszProxy;
      break;
    }
    break; // loop finished
  }

  // free memory of all possibly allocated objects
  // ie_proxy
  if (ie_proxy.lpszAutoConfigUrl)
    GlobalFree(ie_proxy.lpszAutoConfigUrl);
  if (ie_proxy.lpszProxy)
    GlobalFree(ie_proxy.lpszProxy);
  if (ie_proxy.lpszProxyBypass)
    GlobalFree(ie_proxy.lpszProxyBypass);
  // proxy_info
  if (proxy_info.lpszProxyBypass)
    GlobalFree(proxy_info.lpszProxyBypass);
  if (proxy_info.lpszProxy)
    GlobalFree(proxy_info.lpszProxy);

  // hSession
  if (hSession)
    WinHttpCloseHandle(hSession);

  if (fail) {
    err_msg = info.str();
  }
  proxy_str = std::string(proxy.begin(), proxy.end());
  return !fail;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NetworkProxy::NetworkProxy() : m_logger("network_proxy_logger_win") {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
NetworkProxy::~NetworkProxy() {}

/**
 * Get the http proxy information
 * @param targetURLString
 * @return Proxy information
 */
ProxyInfo NetworkProxy::getHttpProxy(const std::string &targetURLString) {

  ProxyInfo proxyInfo; // No proxy
  std::string errmsg, proxy_option;
  m_logger.debug()
      << "Attempt to get the windows proxy configuration for this connection"
      << std::endl;
  if (get_proxy_configuration_win(targetURLString, proxy_option, errmsg)) {
    std::string proxyServer;
    int proxyPort = 0;
    if (!proxy_option.empty()) {
      size_t pos = proxy_option.rfind(':');
      if (pos != std::string::npos) {
        if (pos == 4 || pos == 5) // means it found http(s):
        {
          proxyServer = proxy_option;
          proxyPort = 8080; // default port for proxy
        } else {
          proxyServer =
              std::string(proxy_option.begin(), proxy_option.begin() + pos);
          std::stringstream port_str;
          port_str << std::string(proxy_option.begin() + pos + 1,
                                  proxy_option.end());
          port_str >> proxyPort;
        }
      } else {
        proxyServer = proxy_option;
        proxyPort = 8080;
      }
    }
    proxyInfo = ProxyInfo(proxyServer, proxyPort, true);
  } else {
    m_logger.information() << "ScriptRepository failed to find the proxy "
                              "information. It will attempt without proxy. "
                           << errmsg << std::endl;
  }

  return proxyInfo;
}

} // namespace Kernel
} // namespace Mantid

#endif
