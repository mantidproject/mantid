// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// Compile on OSX only.
#if defined(__APPLE__)

#include "MantidKernel/NetworkProxy.h"
#include <CFNetwork/CFProxySupport.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Authorization.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <sstream>
#include <vector>

namespace Mantid {
namespace Kernel {

/**
 * Helper function to convert CFStringRefs to std::string
 * @param str : CRFStringRef variable
 * @return std::string containing the contents of the input string
 */
std::string toString(CFStringRef str) {
  if (!str)
    return std::string();
  CFIndex length = CFStringGetLength(str);
  const UniChar *chars = CFStringGetCharactersPtr(str);
  if (chars)
    return std::string(reinterpret_cast<const char *>(chars), length);

  std::vector<UniChar> buffer(length);
  CFStringGetCharacters(str, CFRangeMake(0, length), buffer.data());
  return std::string(reinterpret_cast<const char *>(buffer.data()), length);
}

/**
 * Helper enums.
 */
enum ProxyType { DefaultProxy, Socks5Proxy, NoProxy, HttpProxy, HttpCachingProxy, FtpCachingProxy };

/// Typedef Collection of proxy information.
using ProxyInfoVec = std::vector<ProxyInfo>;

/**
 * Extract proxy information from a CFDistionaryRef
 * @param dict : CFDictionary item
 * @return ProxyInfo object.
 */
ProxyInfo proxyFromDictionary(CFDictionaryRef dict) {
  ProxyInfo proxyInfo;
  ProxyType proxyType = NoProxy;

  CFStringRef cfProxyType = reinterpret_cast<CFStringRef>(CFDictionaryGetValue(dict, kCFProxyTypeKey));

  if (CFStringCompare(cfProxyType, kCFProxyTypeFTP, 0) == kCFCompareEqualTo) {
    proxyType = FtpCachingProxy;
  } else if (CFStringCompare(cfProxyType, kCFProxyTypeHTTP, 0) == kCFCompareEqualTo) {
    proxyType = HttpProxy;
  } else if (CFStringCompare(cfProxyType, kCFProxyTypeHTTPS, 0) == kCFCompareEqualTo) {
    proxyType = HttpProxy;
  } else if (CFStringCompare(cfProxyType, kCFProxyTypeSOCKS, 0) == kCFCompareEqualTo) {
    proxyType = Socks5Proxy;
  }

  int port = 0;
  std::string hostName = toString(reinterpret_cast<CFStringRef>(CFDictionaryGetValue(dict, kCFProxyHostNameKey)));
  CFNumberRef portNumber = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(dict, kCFProxyPortNumberKey));
  if (portNumber) {
    CFNumberGetValue(portNumber, kCFNumberSInt16Type, &port);
  }
  if (proxyType != NoProxy) {
    proxyInfo = ProxyInfo(hostName, port, proxyType == HttpProxy);
  }

  return proxyInfo;
}

/**
 * Get proxy information from a proxy script.
 * @param dict : Dictionary to search through.
 * @param targetURLString : Target remote URL
 * @param logger : Log object
 * @return Collection of proxy information.
 */
ProxyInfoVec proxyInformationFromPac(CFDictionaryRef dict, const std::string &targetURLString, Logger &logger) {
  ProxyInfoVec proxyInfoVec;

  // is there a PAC enabled? If so, use it first.
  CFNumberRef pacEnabled;
  if ((pacEnabled =
           reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(dict, kSCPropNetProxiesProxyAutoConfigEnable)))) {
    int enabled;
    if (CFNumberGetValue(pacEnabled, kCFNumberIntType, &enabled) && enabled) {
      // PAC is enabled
      CFStringRef cfPacLocation =
          reinterpret_cast<CFStringRef>(CFDictionaryGetValue(dict, kSCPropNetProxiesProxyAutoConfigURLString));
      CFDataRef pacData;
      CFURLRef pacURL = CFURLCreateWithString(kCFAllocatorDefault, cfPacLocation, nullptr);
      SInt32 errorCode;
      if (!CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, pacURL, &pacData, nullptr, nullptr,
                                                    &errorCode)) {
        logger.debug() << "Unable to get the PAC script at " << toString(cfPacLocation) << "Error code: " << errorCode
                       << '\n';
        return proxyInfoVec;
      }

      CFStringRef pacScript =
          CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, pacData, kCFStringEncodingISOLatin1);

      CFURLRef targetURL = CFURLCreateWithBytes(kCFAllocatorDefault,
                                                reinterpret_cast<UInt8 *>(const_cast<char *>(targetURLString.c_str())),
                                                targetURLString.size(), kCFStringEncodingUTF8, nullptr);
      if (!targetURL) {
        logger.debug("Problem with Target URI for proxy script");
        return proxyInfoVec;
      }

      CFErrorRef pacError;
      CFArrayRef proxies = CFNetworkCopyProxiesForAutoConfigurationScript(pacScript, targetURL, &pacError);

      if (!proxies) {
        std::string pacLocation = toString(cfPacLocation);
        CFStringRef pacErrorDescription = CFErrorCopyDescription(pacError);
        logger.debug() << "Execution of PAC script at \"%s\" failed: %s" << pacLocation << toString(pacErrorDescription)
                       << '\n';
      }

      CFIndex size = CFArrayGetCount(proxies);
      for (CFIndex i = 0; i < size; ++i) {
        CFDictionaryRef proxy = reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(proxies, i));
        proxyInfoVec.push_back(proxyFromDictionary(proxy));
      }
    }
  }
  return proxyInfoVec;
}

/**
 * Proxy from dictionary.
 * @param dict
 * @param enableKey
 * @param hostKey
 * @param portKey
 * @return return Proxy object.
 */
ProxyInfo proxyFromDictionary(CFDictionaryRef dict, CFStringRef enableKey, CFStringRef hostKey, CFStringRef portKey) {
  ProxyInfo proxyInfo;
  CFNumberRef protoEnabled;
  CFNumberRef protoPort;
  CFStringRef protoHost;
  if (enableKey && (protoEnabled = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(dict, enableKey))) &&
      (protoHost = reinterpret_cast<CFStringRef>(CFDictionaryGetValue(dict, hostKey))) &&
      (protoPort = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(dict, portKey)))) {
    int enabled;
    if (CFNumberGetValue(protoEnabled, kCFNumberIntType, &enabled) && enabled) {
      std::string host = toString(protoHost);

      int port;
      CFNumberGetValue(protoPort, kCFNumberIntType, &port);
      proxyInfo = ProxyInfo(host, port, HttpProxy);
    }
  }

  // proxy not enabled
  return proxyInfo;
}

/**
 * Specifially look for http proxy settings.
 * @param dict :
 * @return Return the proxy info object.
 */
ProxyInfo httpProxyFromSystem(CFDictionaryRef dict) {
  ProxyInfo tempProxy =
      proxyFromDictionary(dict, kSCPropNetProxiesHTTPEnable, kSCPropNetProxiesHTTPProxy, kSCPropNetProxiesHTTPPort);

  return tempProxy;
}

/**
 * Find the http proxy.
 * Look through the proxy settings script first.
 * @param targetURLString : Target remote URL string
 * @param logger : ref to log object.
 * @return Proxy object.
 */
ProxyInfo findHttpProxy(const std::string &targetURLString, Mantid::Kernel::Logger &logger) {
  ProxyInfo httpProxy;
  CFDictionaryRef dict = SCDynamicStoreCopyProxies(nullptr);
  if (!dict) {
    logger.debug("NetworkProxyOSX SCDynamicStoreCopyProxies returned NULL. No "
                 "proxy information retrieved");
    return httpProxy;
  }

  // Query the proxy pac first.
  ProxyInfoVec info = proxyInformationFromPac(dict, targetURLString, logger);

  bool foundHttpProxy = false;
  auto proxyIt = std::find_if(info.cbegin(), info.cend(), [](const auto &proxy) { return proxy.isHttpProxy(); });
  if (proxyIt != info.cend()) {
    foundHttpProxy = true;
    httpProxy = *proxyIt;
  }

  // Query the http proxy settings second.
  if (!foundHttpProxy) {
    ProxyInfo tempProxy = httpProxyFromSystem(dict);
    if (tempProxy.isHttpProxy()) {
      httpProxy = tempProxy;
      foundHttpProxy = true;
    }
  }

  if (!foundHttpProxy) {
    logger.debug("NetworkProxyOSX. No system HTTP Proxy set!");
  }

  return httpProxy;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NetworkProxy::NetworkProxy() : m_logger("network_proxy_logger_osx") {}

ProxyInfo NetworkProxy::getHttpProxy(const std::string &targetURLString) {
  return findHttpProxy(targetURLString, m_logger);
}

} // namespace Kernel
} // namespace Mantid

#endif
