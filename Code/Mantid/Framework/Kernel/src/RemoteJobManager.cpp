#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/RemoteJobManager.h"

#include <Poco/Base64Encoder.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPCookie.h>
#include <Poco/Net/NameValueCollection.h>
#include <Poco/URI.h>

#include <Poco/AutoPtr.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>

#include <ostream>
#include <sstream>

namespace Mantid {
namespace Kernel {

namespace {
// static logger object
Logger g_log("RemoteJobManager");
}

RemoteJobManager::RemoteJobManager(const Poco::XML::Element *elem)
    : m_displayName(elem->getAttribute("name")),
      m_session(
          NULL) // Make sure this is always either NULL or a valid pointer.
{
  // Sanity check m_displayName
  if (m_displayName.length() == 0) {
    g_log.error("Compute Resources must have a name attribute");
    throw std::runtime_error("Compute Resources must have a name attribute");
  }

  Poco::AutoPtr<Poco::XML::NodeList> nl = elem->getElementsByTagName("baseURL");
  if (nl->length() != 1) {
    g_log.error("HTTP Compute Resources must have exactly one baseURL tag");
    throw std::runtime_error(
        "HTTP Compute Resources must have exactly one baseURL tag");
  } else {
    nl = nl->item(0)->childNodes();
    if (nl->length() > 0) {
      Poco::XML::Text *txt = dynamic_cast<Poco::XML::Text *>(nl->item(0));
      if (txt) {
        m_serviceBaseUrl = txt->getData();
      }
    }
  }
}

RemoteJobManager::~RemoteJobManager() { delete m_session; }

std::istream &RemoteJobManager::httpGet(const std::string &path,
                                        const std::string &query_str,
                                        const std::string &username,
                                        const std::string &password) {
  Poco::Net::HTTPRequest req;
  initGetRequest(req, path, query_str);

  if (username.length() > 0) {
    // Set the Authorization header (base64 encoded)
    std::ostringstream encodedAuth;
    Poco::Base64Encoder encoder(encodedAuth);
    encoder << username << ":" << password;
    encoder.close();
    req.setCredentials("Basic", encodedAuth.str());
  }

  m_session->sendRequest(req);

  std::istream &respStream = m_session->receiveResponse(m_response);

  // For as yet unknown reasons, we don't always get a session cookie back from
  // the
  // server. In that case, we don't want to overwrite the cookie we're currently
  // using...
  // Note: This won't work properly if we ever use cookies other than a
  // session cookie.
  std::vector<Poco::Net::HTTPCookie> newCookies;
  m_response.getCookies(newCookies);
  if (newCookies.size() > 0) {
    m_cookies = newCookies;
  }

  return respStream;
}

std::istream &RemoteJobManager::httpPost(const std::string &path,
                                         const PostDataMap &postData,
                                         const PostDataMap &fileData,
                                         const std::string &username,
                                         const std::string &password) {
  Poco::Net::HTTPRequest req;
  initPostRequest(req, path);

  if (username.length() > 0) {
    // Set the Authorization header (base64 encoded)
    std::ostringstream encodedAuth;
    Poco::Base64Encoder encoder(encodedAuth);
    encoder << username << ":" << password;
    encoder.close();
    req.setCredentials("Basic", encodedAuth.str());
  }

  // We have to do a POST with multipart MIME encoding. MIME is rather picky
  // about
  // how the parts are delimited. See RFC 2045 & 2046 for details.

  char httpLineEnd[3] = {0x0d, 0x0a,
                         0x00}; // HTTP uses CRLF for its line endings

  // boundary can be almost anything (again, see RFC 2046). The important part
  // is that it
  // cannot appear anywhere in the actual data
  std::string boundary = "112233MantidHTTPBoundary44556677";
  std::string boundaryLine = "--" + boundary + httpLineEnd;
  std::string finalBoundaryLine = "--" + boundary + "--" + httpLineEnd;

  req.setContentType("multipart/form-data; boundary=" + boundary);

  // Need to be able to specify the content length, so build up the post body
  // here.
  std::ostringstream postBody;
  PostDataMap::const_iterator it = postData.begin();
  while (it != postData.end()) {
    postBody << boundaryLine;
    postBody << "Content-Disposition: form-data; name=\"" << (*it).first
             << "\"";
    postBody << httpLineEnd << httpLineEnd;
    postBody << (*it).second;
    postBody << httpLineEnd;
    ++it;
  }

  // file data is treated the same as post data, except that we set the filename
  // field
  // in the Content-Disposition header and add the Content-Type header
  it = fileData.begin();
  while (it != fileData.end()) {
    postBody << boundaryLine;
    postBody << "Content-Disposition: form-data; name=\"" << (*it).first
             << "\"; filename=\"" << (*it).first << "\"";
    postBody << httpLineEnd;
    postBody << "Content-Type: application/octet-stream";
    postBody << httpLineEnd << httpLineEnd;
    postBody << (*it).second;
    postBody << httpLineEnd;
    ++it;
  }

  postBody << finalBoundaryLine;

  req.setContentLength(static_cast<int>(postBody.str().size()));

  std::ostream &postStream = m_session->sendRequest(req);

  // upload the actual HTTP body
  postStream << postBody.str() << std::flush;

  std::istream &respStream = m_session->receiveResponse(m_response);

  // For as yet unknown reasons, we don't always get a session cookie back from
  // the
  // server. In that case, we don't want to overwrite the cookie we're currently
  // using...
  // Note: This won't work properly if we ever use cookies other than a
  // session cookie.
  std::vector<Poco::Net::HTTPCookie> newCookies;
  m_response.getCookies(newCookies);
  if (newCookies.size() > 0) {
    m_cookies = newCookies;
  }

  return respStream;
}

// Wrappers for a lot of the boilerplate code needed to perform an HTTPS GET or
// POST
void RemoteJobManager::initGetRequest(Poco::Net::HTTPRequest &req,
                                      std::string extraPath,
                                      std::string queryString) {
  return initHTTPRequest(req, Poco::Net::HTTPRequest::HTTP_GET, extraPath,
                         queryString);
}

void RemoteJobManager::initPostRequest(Poco::Net::HTTPRequest &req,
                                       std::string extraPath) {
  return initHTTPRequest(req, Poco::Net::HTTPRequest::HTTP_POST, extraPath);
}

void RemoteJobManager::initHTTPRequest(Poco::Net::HTTPRequest &req,
                                       const std::string &method,
                                       std::string extraPath,
                                       std::string queryString) {
  // Set up the session object
  if (m_session) {
    delete m_session;
    m_session = NULL;
  }

  if (Poco::URI(m_serviceBaseUrl).getScheme() == "https") {
    // Create an HTTPS session
    // TODO: Notice that we've set the context to VERIFY_NONE. I think that
    // means we're not checking the SSL certificate that the server
    // sends to us. That's BAD!!
    Poco::Net::Context::Ptr context =
        new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "",
                               Poco::Net::Context::VERIFY_NONE, 9, false,
                               "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
    m_session = new Poco::Net::HTTPSClientSession(
        Poco::URI(m_serviceBaseUrl).getHost(),
        Poco::URI(m_serviceBaseUrl).getPort(), context);
  } else {
    // Create a regular HTTP client session.  (NOTE: Using unencrypted HTTP is a
    // really bad idea! We'll be sending passwords in the clear!)
    m_session =
        new Poco::Net::HTTPClientSession(Poco::URI(m_serviceBaseUrl).getHost(),
                                         Poco::URI(m_serviceBaseUrl).getPort());
  }

  Poco::URI uri(m_serviceBaseUrl);
  std::string path = uri.getPath();
  // Path should be something like "/mws/rest", append extraPath to it.
  path += extraPath;

  uri.setPath(path);
  if (method == Poco::Net::HTTPRequest::HTTP_GET && queryString.size() > 0) {
    uri.setQuery(queryString);
  }

  req.setVersion(Poco::Net::HTTPRequest::HTTP_1_1);
  req.setMethod(method);
  req.setURI(uri.toString());

  // Attach any cookies we've got from previous responses
  req.setCookies(getCookies());

  return;
}

// Converts the vector of HTTPCookie objects into a NameValueCollection
Poco::Net::NameValueCollection RemoteJobManager::getCookies() {
  Poco::Net::NameValueCollection nvc;
  std::vector<Poco::Net::HTTPCookie>::const_iterator it = m_cookies.begin();
  while (it != m_cookies.end()) {
    nvc.add((*it).getName(), (*it).getValue());
    ++it;
  }
  return nvc;
}

} // end of namespace Kernel
} // end of namespace Mantid
