//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/ISISDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"

#include <iostream>

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::URI;

namespace Mantid
{
namespace DataHandling
{

DECLARE_ARCHIVESEARCH(ISISDataArchive,ISISDataSearch);

/**
  * Calls a web service to get a full path to a file
  * @param fName The file name.
  * @return The path to the file or empty string in case of error.
  */
std::string ISISDataArchive::getPath(const std::string& fName)const
{
#ifdef _WIN32
  std::string URL("http://data.isis.rl.ac.uk/where.py/windir?name=");
#else
  std::string URL("http://data.isis.rl.ac.uk/where.py/unixdir?name=");
#endif
  URL += fName;

  URI uri(URL);
  std::string path(uri.getPathAndQuery());

  HTTPClientSession session(uri.getHost(), uri.getPort());
  HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
  session.sendRequest(req);

  HTTPResponse res;
  std::istream& rs = session.receiveResponse(res);
  std::string out;
  char buff[300];
  std::streamsize n;

  do
  {
    rs.read(&buff[0],300);
    n = rs.gcount();
    out.append(&buff[0],n);
  }
  while(n == 300);

  if ( !out.empty() )
  {
    if (out[0] == '<' || out.find("ERROR") != std::string::npos)
    {// if error return empty string
      out.clear();
    }
    else
    {// append the file name
      out += out[0] + fName;
    }
  }

  return out;
}

} // namespace DataHandling
} // namespace Mantid
