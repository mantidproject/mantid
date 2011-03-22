//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/OrbiterDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"

//#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/Context.h>
#include <Poco/URI.h>

#include <iostream>

//using Poco::Net::HTTPSClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::URI;

namespace Mantid
{
  namespace DataHandling
  {

    DECLARE_ARCHIVESEARCH(OrbiterDataArchive,OrbiterDataSearch);

    /**
     * Calls a web service to get a full path to a file
     * @param fName :: The file name.
     * @return The path to the file or empty string in case of error.
     */
    std::string OrbiterDataArchive::getPath(const std::string& fName) const
    {

      std::string baseURL(
          "https://orbiter.sns.gov/orbiter/service/webservice/OrbiterFindFileService.php");
      std::string findFileWS("/operation/findFile/format/space/filename/");
      std::string URL;

      std::string out;

      //#ifdef _WIN32
      //	// Return an empty string
      //#else
      //
      //#endif

      Poco::URI uri(baseURL + findFileWS + fName);
      std::string path(uri.getPathAndQuery());

// FIXME: Put this back in.
// ********* START OF COMMENTED OUT SECTION ************
//      Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "",
//          "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
//
//      Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
//      HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
//      session.sendRequest(req);
//
//      HTTPResponse res;
//      std::istream& rs = session.receiveResponse(res);
//
//      char buff[300];
//      std::streamsize n;
//
//      do
//      {
//        rs.read(&buff[0], 300);
//        n = rs.gcount();
//        out.append(&buff[0], n);
//      } while (n == 300);
// ********* END OF COMMENTED OUT SECTION **********


      //TODO: Look for a space, and split on it.  Just take the first value.

      //std::cout << "Returned Filename = " << out << std::endl;

      return out;
    }

  } // namespace DataHandling
} // namespace Mantid
