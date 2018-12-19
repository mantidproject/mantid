// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIHELPER_H
#define MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIHELPER_H

#include "MantidKernel/DllConfig.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <Poco/Net/HTTPResponse.h>

namespace Poco {
namespace XML {
class Element;
}

namespace Net {
class HTTPCookie;
class NameValueCollection;
class HTTPClientSession;
class HTTPRequest;
} // namespace Net
} // namespace Poco

namespace Mantid {
namespace RemoteJobManagers {
/**
MantidWebServiceAPIHelper handles HTTP requests and has been crated
starting from chunks of the class RemoteJobManager. This should/could
(ideally) be replaced by the newer InternetHelper class.

implements a remote job manager that
knows how to talk to the Mantid web service / job submission API
(http://www.mantidproject.org/Remote_Job_Submission_API). This is
being used for example for the Fermi cluster at SNS.
*/

class DLLExport MantidWebServiceAPIHelper {
public:
  MantidWebServiceAPIHelper();

  virtual ~MantidWebServiceAPIHelper();

  // Name/Value pairs for POST data.  Note that the second string might be
  // binary, and might be
  // fairly large.  (If it were a JPG image for example...)
  using PostDataMap = std::map<std::string, std::string>;

  // Low level HTTP functions - GET, POST, etc...
  // It's up to the various algorithms to know what to do with these functions

  // Perform an HTTP GET request (with optional HTTP Basic Auth)
  std::istream &httpGet(const std::string &path,
                        const std::string &query_str = "",
                        const std::string &username = "",
                        const std::string &password = "") const;

  // Perform an HTTP POST request
  std::istream &httpPost(const std::string &path, const PostDataMap &postData,
                         const PostDataMap &fileData = PostDataMap(),
                         const std::string &username = "",
                         const std::string &password = "") const;

  // Return the status code (200, 404, etc..) from the most recent request
  Poco::Net::HTTPResponse::HTTPStatus lastStatus() const {
    return m_response.getStatus();
  }
  const std::string &lastStatusReason() {
    return m_response.getReasonForStatus(m_response.getStatus());
  }

  const std::string &getDisplayName() const { return m_displayName; }

  // forget authentication token / cookie
  void clearSessionCookies();

private:
  // Wraps up some of the boilerplate code needed to execute HTTP GET and POST
  // requests
  void initGetRequest(Poco::Net::HTTPRequest &req, std::string extraPath,
                      std::string queryString) const;
  void initPostRequest(Poco::Net::HTTPRequest &req,
                       std::string extraPath) const;
  void initHTTPRequest(Poco::Net::HTTPRequest &req, const std::string &method,
                       std::string extraPath,
                       std::string queryString = "") const;

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
  static std::vector<Poco::Net::HTTPCookie> g_cookies;
  Poco::Net::NameValueCollection getCookies() const;

  mutable std::unique_ptr<Poco::Net::HTTPClientSession>
      m_session; // Pointer to session object for all our HTTP requests
                 // (Has to be a pointer because we allocate and delete
                 // it multiple times)
  Poco::Net::HTTPResponse
      m_response; // Response object for all of our HTTP requests

  // No default copy constructor or assignment operator (mainly because
  // HTTPResponse doesn't have them
  MantidWebServiceAPIHelper(const MantidWebServiceAPIHelper &rjm);
  MantidWebServiceAPIHelper &operator=(const MantidWebServiceAPIHelper &rjm);
};

} // end namespace RemoteJobManagers
} // end namespace Mantid

#endif //  MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIHELPER_H
