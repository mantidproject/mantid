//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/ISISDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Exception.h>

#include <sstream>
#include <iostream>

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::URI;

namespace Mantid {
namespace DataHandling {
namespace {
/// static logger
Kernel::Logger g_log("ISISDataArchive");
}

DECLARE_ARCHIVESEARCH(ISISDataArchive, ISISDataSearch);

namespace {
#ifdef _WIN32
const char *URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/windir?name=";
#else
const char *URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/unixdir?name=";
#endif
}

/**
 * Query the ISIS archive for a set of filenames & extensions. The method goes
 * through each extension
 * and checks whether it can find a match with each filename in the filenames
 * list. The first match is returned.
 * @param filenames :: A set of filenames without extensions
 * @param exts :: A list of extensions to try in order with each filename
 * @returns The full path to the first found
 */
std::string
ISISDataArchive::getArchivePath(const std::set<std::string> &filenames,
                                const std::vector<std::string> &exts) const {
  std::set<std::string>::const_iterator iter = filenames.begin();
  for (; iter != filenames.end(); ++iter) {
    g_log.debug() << *iter << ")\n";
  }
  std::vector<std::string>::const_iterator iter2 = exts.begin();
  for (; iter2 != exts.end(); ++iter2) {
    g_log.debug() << *iter2 << ")\n";
  }

  std::vector<std::string>::const_iterator ext = exts.begin();
  for (; ext != exts.end(); ++ext) {
    std::set<std::string>::const_iterator it = filenames.begin();
    for (; it != filenames.end(); ++it) {
      const std::string fullPath = getPath(*it + *ext);
      if (!fullPath.empty())
        return fullPath;
    } // it
  }   // ext
  return "";
}

/**
* Calls a web service to get a full path to a file.
* Only returns a full path string if the file exists
* @param fName :: The file name.
* @return The path to the file or an empty string in case of error/non-existing
* file.
*/
std::string ISISDataArchive::getPath(const std::string &fName) const {
  g_log.debug() << "ISISDataArchive::getPath() - fName=" << fName << "\n";
  if (fName.empty())
    return ""; // Avoid pointless call to service

  URI uri(URL_PREFIX + fName);
  std::string path(uri.getPathAndQuery());

  HTTPClientSession session(uri.getHost(), uri.getPort());
  HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
  session.sendRequest(req);

  HTTPResponse res;
  std::istream &rs = session.receiveResponse(res);
  const HTTPResponse::HTTPStatus status = res.getStatus();
  g_log.debug() << "HTTP response=" << res.getStatus() << "\n";
  if (status == HTTPResponse::HTTP_OK) {
    std::ostringstream os;
    Poco::StreamCopier::copyStream(rs, os);
    os << Poco::Path::separator() << fName;
    try {
      const std::string expectedPath = os.str();
      if (Poco::File(expectedPath).exists())
        return expectedPath;
    } catch (Poco::Exception &) {
    }
  }
  return "";
}

} // namespace DataHandling
} // namespace Mantid
