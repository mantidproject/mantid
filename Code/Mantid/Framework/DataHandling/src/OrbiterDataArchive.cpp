//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "MantidDataHandling/OrbiterDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"

#include <Poco/File.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/NetException.h>
#include <Poco/URI.h>
#include <boost/algorithm/string.hpp>

#include <iostream>

using Poco::Net::HTTPSClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::ConnectionRefusedException;
using Poco::URI;

namespace Mantid
{
  namespace DataHandling
  {

  // Get a reference to the logger
  Mantid::Kernel::Logger & OrbiterDataArchive::g_log = Mantid::Kernel::Logger::get("OrbiterDataArchive");

    DECLARE_ARCHIVESEARCH(OrbiterDataArchive,OrbiterDataSearch);

    /**
     * Calls a web service to get a full path to a file
     * @param fName :: The file name.
     * @return The path to the file or empty string in case of error.
     */
    std::string OrbiterDataArchive::getPath(const std::string& fName) const
    {

      std::string baseURL(
          "https://orbiter.sns.gov/orbiter/current/service/webservice/OrbiterFindFileService.php");
      std::string findFileWS("/operation/findFile/format/space/fileName/");
      std::string URL(baseURL + findFileWS + fName);
      g_log.debug() << "URL = \'" << URL << "\'\n";

      std::string wsResult = "";
      std::string result = "";

      //#ifdef _WIN32
      //	// Return an empty string
      //#else
      //
      //#endif

      Poco::URI uri(URL);
      std::string path(uri.getPathAndQuery());

      Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "",
          "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

      try { // workaround for ubuntu 11.04
        Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context); // this line is broken
        HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        session.sendRequest(req);

        HTTPResponse res;
        std::istream& rs = session.receiveResponse(res);

        char buff[300];
        std::streamsize n;

        do
        {
          rs.read(&buff[0], 300);
          n = rs.gcount();
          wsResult.append(&buff[0], n);
        } while (n == 300);
      } catch (ConnectionRefusedException &) {
        g_log.information() << "Connection refused by orbiter\n";
        throw;
      } catch(Poco::IOException &e) {
        g_log.debug() << e.name() << " thrown.\n";
        g_log.information() << e.message() << "\n";
        throw;
      }

      //Look for spaces, and split on them.
      std::vector<std::string> filenames;
      boost::split(filenames, wsResult, boost::is_any_of(" "));

      // Debug statement to print returned filename(s)
      for (std::size_t i = 0; i < filenames.size(); ++i) {
        g_log.debug() << "Filename[" << i << "] = \'" << filenames[i] << "\' ";
        if (!(filenames[i].empty()))
        {
          Poco::File temp(filenames[i]);
          if (temp.exists())
          {
            if (result.empty())
              result = filenames[i];
            g_log.debug() << "exists";
          }
          else
          {
            g_log.debug() << "does not exist";
          }
        }
        g_log.debug() << "\n";
      }
      g_log.debug() << "Returning Filename = \'" << result << "\'\n";

      return result;
    }

  } // namespace DataHandling
} // namespace Mantid
