// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ProxyInfo.h"

#include <ios>
#include <map>
#include <memory>
#include <string>

namespace Poco {
// forward declaration
class URI;

namespace Net {
// forward declarations
class HTTPClientSession;
class HTTPResponse;
class HTTPRequest;
class HostNotFoundException;
class HTMLForm;
} // namespace Net
} // namespace Poco

namespace Mantid {
namespace Kernel {
/** InternetHelper : A helper class for supporting access to resources through
  HTTP and HTTPS
*/
class MANTID_KERNEL_DLL InternetHelper {
public:
  enum class HTTPStatus {
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NONAUTHORITATIVE = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USEPROXY = 305,
    // UNUSED: 306
    TEMPORARY_REDIRECT = 307,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    REQUESTENTITYTOOLARGE = 413,
    REQUESTURITOOLONG = 414,
    UNSUPPORTEDMEDIATYPE = 415,
    REQUESTED_RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    I_AM_A_TEAPOT = 418,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    VERSION_NOT_SUPPORTED = 505
  };

  InternetHelper();
  InternetHelper(const Kernel::ProxyInfo &proxy);
  virtual ~InternetHelper();

  // Convenience typedef
  using StringToStringMap = std::map<std::string, std::string>;

  // getters and setters
  void setTimeout(int seconds);
  int getTimeout();

  void setMethod(const std::string &method);
  const std::string &getMethod();

  void setContentType(const std::string &contentType);
  const std::string &getContentType();

  void setContentLength(std::streamsize length);
  std::streamsize getContentLength();

  void setBody(const std::string &body);
  void setBody(const std::ostringstream &body);
  void setBody(Poco::Net::HTMLForm &form);
  const std::string &getBody();

  HTTPStatus getResponseStatus();
  const std::string &getResponseReason();

  void addHeader(const std::string &key, const std::string &value);
  void removeHeader(const std::string &key);
  const std::string &getHeader(const std::string &key);
  void clearHeaders();
  StringToStringMap &headers();
  virtual void reset();

  // Proxy methods
  Kernel::ProxyInfo &getProxy(const std::string &url);
  void clearProxy();
  void setProxy(const Kernel::ProxyInfo &proxy);

  // Execute call methods
  virtual HTTPStatus downloadFile(const std::string &urlFile, const std::string &localFilePath = "");
  virtual HTTPStatus sendRequest(const std::string &url, std::ostream &responseStream);

protected:
  virtual HTTPStatus sendHTTPSRequest(const std::string &url, std::ostream &responseStream);
  virtual HTTPStatus sendHTTPRequest(const std::string &url, std::ostream &responseStream);
  virtual void processResponseHeaders(const Poco::Net::HTTPResponse &res);
  virtual HTTPStatus processErrorStates(const Poco::Net::HTTPResponse &res, std::istream &rs, const std::string &url);
  virtual HTTPStatus sendRequestAndProcess(Poco::Net::HTTPClientSession &session, Poco::URI &uri,
                                           std::ostream &responseStream);

  void setupProxyOnSession(Poco::Net::HTTPClientSession &session, const std::string &proxyUrl);
  void createRequest(Poco::URI &uri);
  InternetHelper::HTTPStatus processRelocation(const Poco::Net::HTTPResponse &response, std::ostream &responseStream);
  bool isRelocated(const HTTPStatus &response);
  void throwNotConnected(const std::string &url, const Poco::Net::HostNotFoundException &ex);

  void logDebugRequestSending(const std::string &schemeName, const std::string &url) const;

  Kernel::ProxyInfo m_proxyInfo;
  bool m_isProxySet;
  int m_timeout;
  bool m_isTimeoutSet;
  std::streamsize m_contentLength;
  std::string m_method;
  std::string m_contentType;
  std::string m_body;
  StringToStringMap m_headers;
  std::unique_ptr<Poco::Net::HTTPRequest> m_request;
  std::unique_ptr<Poco::Net::HTTPResponse> m_response;
};

} // namespace Kernel
} // namespace Mantid
