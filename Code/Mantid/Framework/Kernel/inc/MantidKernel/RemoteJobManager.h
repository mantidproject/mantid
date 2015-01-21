#ifndef REMOTEJOBMANAGER_H
#define REMOTEJOBMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <iosfwd>

#include <Poco/Net/HTTPResponse.h>

// Forward declarations
namespace Poco {
namespace XML {
class Element;
}
namespace Net {
class HTTPCookie;
class NameValueCollection;
class HTTPClientSession;
class HTTPRequest;
}
}

namespace Mantid {
namespace Kernel {

class MANTID_KERNEL_DLL RemoteJobManager {
public:
  RemoteJobManager(const Poco::XML::Element *elem);
  virtual ~RemoteJobManager();

  // Name/Value pairs for POST data.  Note that the second string might be
  // binary, and might be
  // fairly large.  (If it were a JPG image for example...)
  typedef std::map<std::string, std::string> PostDataMap;

  // Low level HTTP functions - GET, POST, etc...
  // It's up to the various algorithms to know what to do with these functions

  // Perform an HTTP GET request (with optional HTTP Basic Auth)
  std::istream &httpGet(const std::string &path,
                        const std::string &query_str = "",
                        const std::string &username = "",
                        const std::string &password = "");

  // Perform an HTTP POST request
  std::istream &httpPost(const std::string &path, const PostDataMap &postData,
                         const PostDataMap &fileData = PostDataMap(),
                         const std::string &username = "",
                         const std::string &password = "");

  // Return the status code (200, 404, etc..) from the most recent request
  Poco::Net::HTTPResponse::HTTPStatus lastStatus() {
    return m_response.getStatus();
  }
  const std::string &lastStatusReason() {
    return m_response.getReasonForStatus(m_response.getStatus());
  }

  const std::string &getDisplayName() const { return m_displayName; }

private:
  // Wraps up some of the boilerplate code needed to execute HTTP GET and POST
  // requests
  void initGetRequest(Poco::Net::HTTPRequest &req, std::string extraPath,
                      std::string queryString);
  void initPostRequest(Poco::Net::HTTPRequest &req, std::string extraPath);
  void initHTTPRequest(Poco::Net::HTTPRequest &req, const std::string &method,
                       std::string extraPath, std::string queryString = "");

  std::string m_displayName;
  std::string
      m_serviceBaseUrl; // What we're going to connect to.  The full URL will be
  // built by appending a path (and possibly a query string)
  // to this string.

  // Store any cookies that the HTTP server sends us so we can send them back
  // on future requests.  (In particular, the ORNL servers use session cookies
  // so we don't have to authenticate to the LDAP server on every single
  // request.)
  //
  // NOTE: For reasons that are unclear, Poco's HTTPResponse class returns
  // cookies
  // in a vector of HTTPCookie objects, but its HTTPRequest::setCookies()
  // function
  // takes a NameValueCollection object, so we have to convert.  (WTF Poco
  // devs?!?)
  std::vector<Poco::Net::HTTPCookie> m_cookies;
  Poco::Net::NameValueCollection getCookies();

  Poco::Net::HTTPClientSession *
      m_session; // Pointer to session object for all our HTTP requests
                 // (Has to be a pointer because we allocate and delete
                 // it multiple times)
  Poco::Net::HTTPResponse
      m_response; // Response object for all of our HTTP requests

  // No default copy constructor or assignment operator (mainly because
  // HTTPResponse doesn't have them
  RemoteJobManager(const RemoteJobManager &rjm);
  RemoteJobManager &operator=(const RemoteJobManager &rjm);
};

} // end namespace Kernel
} // end namespace Mantid

#endif // REMOTEJOBMANAGER_H
