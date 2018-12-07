// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_InternetHelper_H_
#define MANTID_KERNEL_InternetHelper_H_

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
  enum HTTPStatus {
    HTTP_CONTINUE = 100,
    HTTP_SWITCHING_PROTOCOLS = 101,
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NONAUTHORITATIVE = 203,
    HTTP_NO_CONTENT = 204,
    HTTP_RESET_CONTENT = 205,
    HTTP_PARTIAL_CONTENT = 206,
    HTTP_MULTIPLE_CHOICES = 300,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_FOUND = 302,
    HTTP_SEE_OTHER = 303,
    HTTP_NOT_MODIFIED = 304,
    HTTP_USEPROXY = 305,
    // UNUSED: 306
    HTTP_TEMPORARY_REDIRECT = 307,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_PAYMENT_REQUIRED = 402,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_NOT_ACCEPTABLE = 406,
    HTTP_PROXY_AUTHENTICATION_REQUIRED = 407,
    HTTP_REQUEST_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    HTTP_LENGTH_REQUIRED = 411,
    HTTP_PRECONDITION_FAILED = 412,
    HTTP_REQUESTENTITYTOOLARGE = 413,
    HTTP_REQUESTURITOOLONG = 414,
    HTTP_UNSUPPORTEDMEDIATYPE = 415,
    HTTP_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
    HTTP_EXPECTATION_FAILED = 417,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505
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

  int getResponseStatus();
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
  virtual int downloadFile(const std::string &urlFile,
                           const std::string &localFilePath = "");
  virtual int sendRequest(const std::string &url, std::ostream &responseStream);

protected:
  virtual int sendHTTPSRequest(const std::string &url,
                               std::ostream &responseStream);
  virtual int sendHTTPRequest(const std::string &url,
                              std::ostream &responseStream);
  virtual void processResponseHeaders(const Poco::Net::HTTPResponse &res);
  virtual int processErrorStates(const Poco::Net::HTTPResponse &res,
                                 std::istream &rs, const std::string &url);
  virtual int sendRequestAndProcess(Poco::Net::HTTPClientSession &session,
                                    Poco::URI &uri,
                                    std::ostream &responseStream);

  void setupProxyOnSession(Poco::Net::HTTPClientSession &session,
                           const std::string &proxyUrl);
  void createRequest(Poco::URI &uri);
  int processRelocation(const Poco::Net::HTTPResponse &response,
                        std::ostream &responseStream);
  bool isRelocated(const int response);
  void throwNotConnected(const std::string &url,
                         const Poco::Net::HostNotFoundException &ex);

  void logDebugRequestSending(const std::string &schemeName,
                              const std::string &url) const;

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

#endif /* MANTID_KERNEL_InternetHelper_H_ */
