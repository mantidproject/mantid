#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

// Poco
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/StreamCopier.h>
#include <Poco/TemporaryFile.h>
#include <Poco/URI.h>
// Visual Studio complains with the inclusion of Poco/FileStream
// disabling this warning.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable : 4250)
#include <Poco/FileStream.h>
#include <Poco/NullStream.h>
#include <Winhttp.h>
#pragma warning(pop)
#else
#include <Poco/FileStream.h>
#include <Poco/NullStream.h>
#include <stdlib.h>
#endif

// std
#include <fstream>

namespace Mantid {
namespace Kernel {

using namespace Poco::Net;
using std::map;
using std::string;

namespace {
// anonymous namespace for some utility functions

/// static Logger object
Logger g_log("InternetHelper");

/// Convert the method into the proper get/post string
/// @returns Poco::Net::HTTPRequest::HTTP_GET if it is empty
std::string getMethod(const std::string &method) {
  if (method.empty()) {
    return HTTPRequest::HTTP_GET;
  } else {
    return method;
  }
}
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
InternetHelper::InternetHelper()
    : m_proxyInfo(), m_isProxySet(false), m_timeout(30),
      m_contentType("application/json"), m_request(NULL) {}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
InternetHelper::InternetHelper(const Kernel::ProxyInfo &proxy)
    : m_proxyInfo(proxy), m_isProxySet(true), m_timeout(30),
      m_contentType("application/json"), m_request(NULL) {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
InternetHelper::~InternetHelper() {
  if (m_request != NULL) {
    delete m_request;
  }
}

void InternetHelper::createRequest(Poco::URI &uri, const std::string method,
                                   const std::map<string, string> &headers) {
  string reqMethod = getMethod(method);

  m_request = new HTTPRequest(getMethod(method), uri.getPathAndQuery(),
                              HTTPMessage::HTTP_1_1);
  if (reqMethod == HTTPRequest::HTTP_POST) {
    m_request->setContentType(m_contentType);
  }
  m_request->set("User-Agent", "MANTID");
  for (auto itHeaders = headers.begin(); itHeaders != headers.end();
       ++itHeaders) {
    m_request->set(itHeaders->first, itHeaders->second);
  }
}

/** Performs a request using http or https depending on the url
* @param url the address to the network resource
* @param responseStream The stream to fill with the reply on success
* @param headers [optional] : A key value pair map of any additional headers to
*include in the request.
**/
int InternetHelper::sendRequest(const std::string &url,
                                std::ostream &responseStream,
                                const StringToStringMap &headers,
                                const std::string &method,
                                const std::string &body) {
  Poco::URI uri(url);
  if ((uri.getScheme() == "https") || (uri.getPort() == 443)) {
    return sendHTTPSRequest(url, responseStream, headers, method, body);
  } else {
    return sendHTTPRequest(url, responseStream, headers, method, body);
  }
}

/** Performs a request using http
* @param url the address to the network resource
* @param responseStream The stream to fill with the reply on success
* @param headers [optional] : A key value pair map of any additional headers to
*include in the request.
**/
int InternetHelper::sendHTTPRequest(const std::string &url,
                                    std::ostream &responseStream,
                                    const StringToStringMap &headers,
                                    const std::string &method,
                                    const std::string &body) {
  int retStatus = 0;
  g_log.debug() << "Sending request to: " << url << "\n";

  Poco::URI uri(url);
  // Configure Poco HTTP Client Session
  try {
    Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
    session.setTimeout(Poco::Timespan(m_timeout, 0)); // m_timeout seconds

    // configure proxy
    if (!getProxy(url).emptyProxy()) {
      session.setProxyHost(getProxy(url).host());
      session.setProxyPort(static_cast<Poco::UInt16>(getProxy(url).port()));
    }

    this->createRequest(uri, method, headers);
    m_request->setContentLength(body.length());
    session.sendRequest(*m_request) << body;

    HTTPResponse res;
    std::istream &rs = session.receiveResponse(res);
    retStatus = res.getStatus();
    g_log.debug() << "Answer from web: " << retStatus << " "
                  << res.getReason() << std::endl;

    if (retStatus == HTTPResponse::HTTP_OK || retStatus == HTTPResponse::HTTP_CREATED) {
      Poco::StreamCopier::copyStream(rs, responseStream);
      return retStatus;
    } else if ((retStatus == HTTPResponse::HTTP_FOUND) ||
               (retStatus == HTTPResponse::HTTP_MOVED_PERMANENTLY) ||
               (retStatus == HTTPResponse::HTTP_TEMPORARY_REDIRECT) ||
               (retStatus == HTTPResponse::HTTP_SEE_OTHER)) {
      // extract the new location
      std::string newLocation = res.get("location", "");
      if (newLocation != "") {
        return sendRequest(newLocation, responseStream, headers, method, body);
      }
    } else {
      retStatus = processErrorStates(res, rs, url);
    }
  }
  catch (HostNotFoundException &ex) {
    // this exception occurs when the pc is not connected to the internet
    std::stringstream info;
    info << "Failed to download " << url
         << " because there is no connection to the host " << ex.message()
         << ".\nHint: Check your connection following this link: <a href=\""
         << url << "\">" << url << "</a> ";
    throw Exception::InternetError(info.str() + ex.displayText());
  }
  catch (Poco::Exception &ex) {
    throw Exception::InternetError("Connection and request failed " +
                                   ex.displayText());
  }
  return retStatus;
}

/** Performs a request using https
* @param url the address to the network resource
* @param responseStream The stream to fill with the reply on success
* @param headers [optional] : A key value pair map of any additional headers to
*include in the request.
**/
int InternetHelper::sendHTTPSRequest(const std::string &url,
                                     std::ostream &responseStream,
                                     const StringToStringMap &headers,
                                     const std::string &method,
                                     const std::string &body) {
  int retStatus = 0;
  std::string reqMethod = getMethod(method);
  g_log.debug() << "Sending " << reqMethod << " request : " << url << "\n";

  Poco::URI uri(url);
  try {
    // initialize ssl
    Poco::SharedPtr<InvalidCertificateHandler> certificateHandler =
        new AcceptCertificateHandler(true);
    // Currently do not use any means of authentication. This should be updated
    // IDS has signed certificate.
    const Context::Ptr context =
        new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE);
    // Create a singleton for holding the default context.
    // e.g. any future requests to publish are made to this certificate and
    // context.
    SSLManager::instance().initializeClient(NULL, certificateHandler, context);
    // Create the session
    HTTPSClientSession session(uri.getHost(),
                               static_cast<Poco::UInt16>(uri.getPort()));
    session.setTimeout(Poco::Timespan(m_timeout, 0)); // m_timeout seconds

    // HACK:: Currently the automatic proxy detection only supports http proxy
    // detection
    // most locations use the same proxy for http and https, so force it to use
    // the http proxy
    std::string urlforProxy =
        ConfigService::Instance().getString("proxy.httpsTargetUrl");
    if (urlforProxy.empty()) {
      urlforProxy = "http://" + uri.getHost();
    }

    auto proxy = getProxy(urlforProxy);
    if (!proxy.emptyProxy()) {
      session.setProxyHost(proxy.host());
      session.setProxyPort(static_cast<Poco::UInt16>(proxy.port()));
    }

    // create a request
    this->createRequest(uri, method, headers);
    m_request->setContentLength(body.length());
    session.sendRequest(*m_request) << body;

    HTTPResponse res;
    std::istream &rs = session.receiveResponse(res);
    retStatus = res.getStatus();
    g_log.debug() << "Answer from web: " << retStatus << " "
                  << res.getReason() << std::endl;

    if (retStatus == HTTPResponse::HTTP_OK || retStatus == HTTPResponse::HTTP_CREATED) {
      Poco::StreamCopier::copyStream(rs, responseStream);
      return retStatus;
    } else if ((retStatus == HTTPResponse::HTTP_FOUND) ||
               (retStatus == HTTPResponse::HTTP_MOVED_PERMANENTLY) ||
               (retStatus == HTTPResponse::HTTP_TEMPORARY_REDIRECT) ||
               (retStatus == HTTPResponse::HTTP_SEE_OTHER)) {
      // extract the new location
      std::string newLocation = res.get("location", "");
      if (newLocation != "") {
        return sendRequest(newLocation, responseStream, headers, method, body);
      }
    } else {
      retStatus = processErrorStates(res, rs, url);
    }
  }
  catch (HostNotFoundException &ex) {
    // this exception occurs when the pc is not connected to the internet
    std::stringstream info;
    info << "Failed to download " << url
         << " because there is no connection to the host " << ex.message()
         << ".\nHint: Check your connection following this link: <a href=\""
         << url << "\">" << url << "</a> ";
    throw Exception::InternetError(info.str() + ex.displayText());
  }
  catch (Poco::Exception &ex) {
    throw Exception::InternetError("Connection and request failed " +
                                   ex.displayText());
  }
  return retStatus;
}

/** Gets proxy details for a system and a url.
@param url : The url to be called
*/
Kernel::ProxyInfo &InternetHelper::getProxy(const std::string &url) {
  // set the proxy
  if (!m_isProxySet) {
    setProxy(ConfigService::Instance().getProxy(url));
  }
  return m_proxyInfo;
}

/** Clears cached proxy details.
*/
void InternetHelper::clearProxy() { m_isProxySet = false; }

/** sets the proxy details.
@param proxy the proxy information to use
*/
void InternetHelper::setProxy(const Kernel::ProxyInfo &proxy) {
  m_proxyInfo = proxy;
  m_isProxySet = true;
}

/** Process any HTTP errors states.

@param res : The http response
@param rs : The iutput stream from the response
@param url : The url originally called

@exception Mantid::Kernel::Exception::InternetError : Coded for the failure
state.
*/
int InternetHelper::processErrorStates(const Poco::Net::HTTPResponse &res,
                                       std::istream &rs,
                                       const std::string &url) {
  int retStatus = res.getStatus();
  g_log.debug() << "Answer from web: " << res.getStatus() << " "
                << res.getReason() << std::endl;

  // get github api rate limit information if available;
  int rateLimitRemaining;
  DateAndTime rateLimitReset;
  try {
    rateLimitRemaining =
        boost::lexical_cast<int>(res.get("X-RateLimit-Remaining", "-1"));
    rateLimitReset.set_from_time_t(
        boost::lexical_cast<int>(res.get("X-RateLimit-Reset", "0")));
  }
  catch (boost::bad_lexical_cast const &) {
    rateLimitRemaining = -1;
  }

  if (retStatus == HTTPResponse::HTTP_OK) {
    throw Exception::InternetError("Response was ok, processing should never "
                                   "have entered processErrorStates",
                                   retStatus);
  } else if (retStatus == HTTPResponse::HTTP_FOUND) {
    throw Exception::InternetError("Response was HTTP_FOUND, processing should "
                                   "never have entered processErrorStates",
                                   retStatus);
  } else if (retStatus == HTTPResponse::HTTP_MOVED_PERMANENTLY) {
    throw Exception::InternetError("Response was HTTP_MOVED_PERMANENTLY, "
                                   "processing should never have entered "
                                   "processErrorStates",
                                   retStatus);
  } else if (retStatus == HTTPResponse::HTTP_NOT_MODIFIED) {
    throw Exception::InternetError("Not modified since provided date" +
                                       rateLimitReset.toSimpleString(),
                                   retStatus);
  } else if ((retStatus == HTTPResponse::HTTP_FORBIDDEN) &&
             (rateLimitRemaining == 0)) {
    throw Exception::InternetError(
        "The Github API rate limit has been reached, try again after " +
            rateLimitReset.toSimpleString(),
        retStatus);
  } else {
    std::stringstream info;
    std::stringstream ss;
    Poco::StreamCopier::copyStream(rs, ss);
    if (retStatus == HTTPResponse::HTTP_NOT_FOUND)
      info << "Failed to download " << url << " with the link "
           << "<a href=\"" << url << "\">.\n"
           << "Hint. Check that link is correct</a>";
    else {
      // show the error
      info << res.getReason();
      info << ss.str();
    }
    throw Exception::InternetError(info.str() + ss.str(), retStatus);
  }
}

/** Download a url and fetch it inside the local path given.

@param urlFile: Define a valid URL for the file to be downloaded. Eventually, it
may give
any valid https path. For example:

url_file = "http://www.google.com"

url_file = "https://mantidweb/repository/README.md"

The result is to connect to the http server, and request the path given.

The answer, will be inserted at the local_file_path.

@param localFilePath : Provide the destination of the file downloaded at the
url_file.

@param headers [optional] : A key value pair map of any additional headers to
include in the request.

@exception Mantid::Kernel::Exception::InternetError : For any unexpected
behaviour.
*/
int InternetHelper::downloadFile(const std::string &urlFile,
                                 const std::string &localFilePath,
                                 const StringToStringMap &headers) {
  int retStatus = 0;
  g_log.debug() << "DownloadFile : " << urlFile << " to file: " << localFilePath
                << std::endl;

  Poco::TemporaryFile tempFile;
  Poco::FileStream tempFileStream(tempFile.path());
  retStatus = sendRequest(urlFile, tempFileStream, headers);
  tempFileStream.close();

  // if there have been no errors move it to the final location, and turn off
  // automatic deletion.
  tempFile.moveTo(localFilePath);
  tempFile.keep();

  return retStatus;
}

/** Sets the timeout in seconds
* @param seconds The value in seconds for the timeout
**/
void InternetHelper::setTimeout(int seconds) { m_timeout = seconds; }

} // namespace Kernel
} // namespace Mantid
