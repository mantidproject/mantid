//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "MantidDataHandling/SNSDataArchiveICAT2.h"
#include "MantidAPI/ArchiveSearchFactory.h"

#include <Poco/File.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/NetException.h>
#include <Poco/URI.h>
#include <boost/algorithm/string.hpp>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include "Poco/SAX/InputSource.h"
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>

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
  namespace
  {
    // Get a reference to the logger
    Kernel::Logger g_log("SNSDataArchiveICAT2");
  }

DECLARE_ARCHIVESEARCH(SNSDataArchiveICAT2,SNSDataSearchICAT2);

/**
 * Calls a web service to get a full path to a file
 * @param fName :: The file name.
 * @return The path to the file or empty string in case of error.
 */
std::string SNSDataArchiveICAT2::getPath(const std::string& fName) const
{
  std::string baseURL(
        "https://prod.sns.gov/sns-icat-ws/icat-location/fileName/");
  std::string URL(baseURL + fName);
  g_log.debug() << "SNSDataArchiveICAT2 URL = \'" << URL << "\'\n";

  std::string wsResult = "";

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
    g_log.information() << "Connection refused by prod.sns.gov\n";
    throw;
  } catch(Poco::IOException &e) {
    g_log.debug() << e.name() << " thrown.\n";
    g_log.information() << e.message() << "\n";
    throw;
  }

  g_log.debug() << "SNSDataArchiveICAT2 Returning Filename = \'" << wsResult << "\'\n";

  return wsResult;
}

/**
 * @param filenames : List of files to search
 * @param exts : List of extensions to check against
 * @return list of archive locations
 */
std::string SNSDataArchiveICAT2::getArchivePath(const std::set<std::string>& filenames, const std::vector<std::string>& exts) const
{
  std::vector<std::string>::const_iterator ext = exts.begin();
  for (; ext != exts.end(); ++ext)
  {
    std::set<std::string>::const_iterator it = filenames.begin();
    for(; it!=filenames.end(); ++it)
    {
      std::string path = getPath(*it + *ext);
      try
      {
        if (!path.empty() && Poco::File(path).exists())
        {
          return path;
        }
      }
      catch(std::exception& e)
      {
        g_log.error() << "Cannot open file " << path << ": " << e.what() << '\n';
        return "";
      }
    } // it
  } // ext
  return "";
} // end of getArchivePath

} // namespace DataHandling
} // namespace Mantid
