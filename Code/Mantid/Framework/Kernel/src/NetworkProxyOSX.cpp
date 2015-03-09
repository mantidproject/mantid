// Compile on OSX only.
#if defined(__APPLE__)

#include "MantidKernel/NetworkProxy.h"
#include <SystemConfiguration/SystemConfiguration.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFProxySupport.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

/**
 * Helper function to convert CFStringRefs to std::string
 * @param stringRef : CRFStringRef variable
 * @return std::string
 */
std::string toString(CFStringRef stringRef) {
  std::string buffer;
  const int max_size = 1000;
  buffer.resize(max_size);
  CFStringGetCString(stringRef, &buffer[0], max_size, kCFStringEncodingUTF8);
  // Strip out whitespace
  std::stringstream trimmer;
  trimmer << buffer;
  buffer.clear();
  trimmer >> buffer;
  return buffer;
}

/**
 * Helper enums.
 */
enum ProxyType {
  DefaultProxy,
  Socks5Proxy,
  NoProxy,
  HttpProxy,
  HttpCachingProxy,
  FtpCachingProxy
};

/// Typedef Collection of proxy information.
typedef std::vector<ProxyInfo> ProxyInfoVec;

/**
 * Extract proxy information from a CFDistionaryRef
 * @param dict : CFDictionary item
 * @return ProxyInfo object.
 */
ProxyInfo proxyFromDictionary(CFDictionaryRef dict) {
  ProxyInfo proxyInfo;
  ProxyType proxyType = NoProxy;

  CFStringRef cfProxyType =
      (CFStringRef)CFDictionaryGetValue(dict, kCFProxyTypeKey);

  if (CFStringCompare(cfProxyType, kCFProxyTypeFTP, 0) == kCFCompareEqualTo) {
    proxyType = FtpCachingProxy;
  } else if (CFStringCompare(cfProxyType, kCFProxyTypeHTTP, 0) ==
             kCFCompareEqualTo) {
    proxyType = HttpProxy;
  } else if (CFStringCompare(cfProxyType, kCFProxyTypeHTTPS, 0) ==
             kCFCompareEqualTo) {
    proxyType = HttpProxy;
  } else if (CFStringCompare(cfProxyType, kCFProxyTypeSOCKS, 0) ==
             kCFCompareEqualTo) {
    proxyType = Socks5Proxy;
  }

  int port = 0;
  std::string hostName =
      toString((CFStringRef)CFDictionaryGetValue(dict, kCFProxyHostNameKey));
  CFNumberRef portNumber =
      (CFNumberRef)CFDictionaryGetValue(dict, kCFProxyPortNumberKey);
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
ProxyInfoVec proxyInformationFromPac(CFDictionaryRef dict,
                                     const std::string &targetURLString,
                                     Logger &logger) {
  ProxyInfoVec proxyInfoVec;

  // is there a PAC enabled? If so, use it first.
  CFNumberRef pacEnabled;
  if ((pacEnabled = (CFNumberRef)CFDictionaryGetValue(
           dict, kSCPropNetProxiesProxyAutoConfigEnable))) {
    int enabled;
    if (CFNumberGetValue(pacEnabled, kCFNumberIntType, &enabled) && enabled) {
      // PAC is enabled
      CFStringRef cfPacLocation = (CFStringRef)CFDictionaryGetValue(
          dict, kSCPropNetProxiesProxyAutoConfigURLString);
      CFDataRef pacData;
      CFURLRef pacURL =
          CFURLCreateWithString(kCFAllocatorDefault, cfPacLocation, NULL);
      SInt32 errorCode;
      if (!CFURLCreateDataAndPropertiesFromResource(
              kCFAllocatorDefault, pacURL, &pacData, NULL, NULL, &errorCode)) {
        logger.debug() << "Unable to get the PAC script at "
                       << toString(cfPacLocation) << "Error code: " << errorCode
                       << std::endl;
        return proxyInfoVec;
      }

      CFStringRef pacScript = CFStringCreateFromExternalRepresentation(
          kCFAllocatorDefault, pacData, kCFStringEncodingISOLatin1);

      CFURLRef targetURL = CFURLCreateWithBytes(
          kCFAllocatorDefault, (UInt8 *)targetURLString.c_str(),
          targetURLString.size(), kCFStringEncodingUTF8, NULL);
      if (!targetURL) {
        logger.debug("Problem with Target URI for proxy script");
        return proxyInfoVec;
      }

      CFErrorRef pacError;
      CFArrayRef proxies = CFNetworkCopyProxiesForAutoConfigurationScript(
          pacScript, targetURL, &pacError);

      if (!proxies) {
        std::string pacLocation = toString(cfPacLocation);
        CFStringRef pacErrorDescription = CFErrorCopyDescription(pacError);
        logger.debug() << "Execution of PAC script at \"%s\" failed: %s"
                       << pacLocation << toString(pacErrorDescription)
                       << std::endl;
      }

      CFIndex size = CFArrayGetCount(proxies);
      for (CFIndex i = 0; i < size; ++i) {
        CFDictionaryRef proxy =
            (CFDictionaryRef)CFArrayGetValueAtIndex(proxies, i);
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
ProxyInfo proxyFromDictionary(CFDictionaryRef dict, CFStringRef enableKey,
                              CFStringRef hostKey, CFStringRef portKey) {
  ProxyInfo proxyInfo;
  CFNumberRef protoEnabled;
  CFNumberRef protoPort;
  CFStringRef protoHost;
  if (enableKey &&
      (protoEnabled = (CFNumberRef)CFDictionaryGetValue(dict, enableKey)) &&
      (protoHost = (CFStringRef)CFDictionaryGetValue(dict, hostKey)) &&
      (protoPort = (CFNumberRef)CFDictionaryGetValue(dict, portKey))) {
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
  ProxyInfo tempProxy = proxyFromDictionary(dict, kSCPropNetProxiesHTTPEnable,
                                            kSCPropNetProxiesHTTPProxy,
                                            kSCPropNetProxiesHTTPPort);

  return tempProxy;
}

/**
 * Find the http proxy.
 * Look through the proxy settings script first.
 * @param targetURLString : Target remote URL string
 * @param logger : ref to log object.
 * @return Proxy object.
 */
ProxyInfo findHttpProxy(const std::string &targetURLString,
                        Mantid::Kernel::Logger &logger) {
  ProxyInfo httpProxy;
  CFDictionaryRef dict = SCDynamicStoreCopyProxies(NULL);
  if (!dict) {
    logger.debug("ScriptRepository SCDynamicStoreCopyProxies returned NULL");
  }

  // Query the proxy pac first.
  ProxyInfoVec info = proxyInformationFromPac(dict, targetURLString, logger);

  bool foundHttpProxy = false;
  for (ProxyInfoVec::iterator it = info.begin(); it != info.end(); ++it) {
    ProxyInfo proxyInfo = *it;
    if (proxyInfo.isHttpProxy()) {
      foundHttpProxy = true;
      httpProxy = *it;
      break;
    }
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
    logger.debug("ScriptRepositry. No system HTTP Proxy set!");
  }

  return httpProxy;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NetworkProxy::NetworkProxy() : m_logger("network_proxy_logger_osx") {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
NetworkProxy::~NetworkProxy() {}

ProxyInfo NetworkProxy::getHttpProxy(const std::string &targetURLString) {
  return findHttpProxy(targetURLString, m_logger);
}

} // namespace Kernel
} // namespace Mantid

#endif
