#include "MantidRemote/RemoteJobManager.h"
#include "MantidRemote/RemoteTask.h"
#include "MantidKernel/ConfigService.h"
#include <MantidKernel/RemoteJobManagerFactory.h>
#include "MantidRemote/SimpleJSON.h"

#include <Poco/Base64Encoder.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPCookie.h>
#include <Poco/Net/NameValueCollection.h>
#include <Poco/URI.h>

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>

#include <ostream>
#include <sstream>
#include <fstream>


// Register with the job manager factory class
DECLARE_RJM( MwsRemoteJobManager, "MWS")

// Get a reference to the logger
Mantid::Kernel::Logger& RemoteJobManager::g_log = Mantid::Kernel::Logger::get("RemoteJobManager");


RemoteJobManager::RemoteJobManager( const Poco::XML::Element* elem)
{
  m_displayName = elem->getAttribute("name");
  if (m_displayName.length() == 0)
  {
    g_log.error("Compute Resources must have a name attribute");
    throw std::runtime_error("Compute Resources must have a name attribute");
  }

  Poco::XML::NodeList* nl;
  Poco::XML::Text* txt;

  nl = elem->getElementsByTagName("configFileURL");
  if (nl->length() != 1)
  {
    g_log.error("Compute Resources must have exactly one configFileURL tag");
    throw std::runtime_error("Compute Resources must have exactly one configFileURL tag");
  }
  else
  {
    nl = nl->item(0)->childNodes();
    if (nl->length() > 0)
    {
      txt = dynamic_cast<Poco::XML::Text*>(nl->item(0));
      if (txt)
      {
        m_configFileUrl = txt->getData();
      }
    }
  }
}

/* ************* HTTPRemoteJobManagaer member functions ****************** */

HttpRemoteJobManager::HttpRemoteJobManager( const Poco::XML::Element* elem)
  : RemoteJobManager( elem)
{
  Poco::XML::NodeList* nl = elem->getElementsByTagName("baseURL");
  if (nl->length() != 1)
  {
    g_log.error("HTTP Compute Resources must have exactly one baseURL tag");
    throw std::runtime_error("HTTP Compute Resources must have exactly one baseURL tag");
  }
  else
  {
    nl = nl->item(0)->childNodes();
    if (nl->length() > 0)
    {
      Poco::XML::Text* txt = dynamic_cast<Poco::XML::Text*>(nl->item(0));
      if (txt)
      {
        m_serviceBaseUrl = txt->getData();
      }
    }
  }
}



// Notify the cluster that we want to start a new transaction
// On success, transId and directory will contain the transaction ID and the name
// of the directory that's been created for this transaction
// If we get an error message back from the server, it will be returned in the
// serverErr string.
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::startTransaction( std::string &transId, std::string &directory, std::string &serverErr)
{
  JobManagerErrorCode reqErr = JM_OK;

  // Create an HTTPS session
  // TODO: Notice that we've set the context to VERIFY_NONE.  I think that means we're not checking the SSL certificate that the server
  // sends to us.  That's BAD!!
  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( Poco::URI(m_serviceBaseUrl).getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);
  // We need to send a GET request to the server with a query string of "action=start"
  Poco::Net::HTTPRequest req;
  reqErr = initGetRequest( req, "/transaction", "action=start");
  if ( reqErr != JM_OK)
  {
    return reqErr;
  }

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;

  // Note: If we wanted to, we could call response.getStatus() at this point
  // and verify that we got a 200 code back from the server
  // anyway.

  std::istream &responseStream = session.receiveResponse( response);
  std::vector<Poco::Net::HTTPCookie> newCookies;
  // For as yet unknown reasons, we don't always get a session cookie back from the
  // server.  In that case, we don't want to overwrite the cookie we're currently
  // using...
  response.getCookies( newCookies);
  if (newCookies.size() > 0)
  {
    m_cookies = newCookies;
  }

  if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
  {
    // D'oh!  The server didn't like our request.
    std::ostringstream respStatus;
    respStatus << "Status: " << response.getStatus() << "\nReason: " << response.getReasonForStatus( response.getStatus());
    respStatus << "\n\nReply text:\n";
    respStatus << responseStream.rdbuf();
    serverErr = respStatus.str();

    if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
      // Probably some kind of username/password mismatch.  Clear the password so that
      // the user can enter it again
      m_password.clear();
    }

    reqErr = JM_HTTP_SERVER_ERR;
  }
  else
  {
    // Success!
    reqErr = JM_OK;  // This should already be set, but just in case....

    // Parse the response body for the transaction ID and directory name.  The response should
    // be a single object with 2 string values.  Something like
    // JSON element that looks something like: {"transId":"21", "dirName":"/apachefiles/xmr-1234567"}
    JSONObject results;

    initFromStream( results, responseStream);
    JSONObject::const_iterator itResults = results.find( "transId");
    (*itResults).second.getValue(transId);
    itResults = results.find( "dirName");
    (*itResults).second.getValue(directory);
  }

  return reqErr;
}


// Notify the cluster that we want to stop the specified transaction
// If we get an error message back from the server, it will be returned in the
// serverErr string.
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::stopTransaction( std::string &transId, std::string &serverErr)
{
  JobManagerErrorCode reqErr = JM_OK;

  // Create an HTTPS session
  // TODO: Notice that we've set the context to VERIFY_NONE.  I think that means we're not checking the SSL certificate that the server
  // sends to us.  That's BAD!!
  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( Poco::URI(m_serviceBaseUrl).getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);
  // We need to send a GET request to the server with a query string of "action=start"
  Poco::Net::HTTPRequest req;
  std::string queryString = "action=stop&transid=" + transId;
  reqErr = initGetRequest( req, "/transaction", queryString);
  if ( reqErr != JM_OK)
  {
    return reqErr;
  }

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;

  // Note: If we wanted to, we could call response.getStatus() at this point
  // and verify that we got a 200 code back from the server
  // anyway.

  std::istream &responseStream = session.receiveResponse( response);
  std::vector<Poco::Net::HTTPCookie> newCookies;
  // For as yet unknown reasons, we don't always get a session cookie back from the
  // server.  In that case, we don't want to overwrite the cookie we're currently
  // using...
  response.getCookies( newCookies);
  if (newCookies.size() > 0)
  {
    m_cookies = newCookies;
  }


  // All we should get back is an HTTP_OK code...
  if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
  {
    // D'oh!  The server didn't like our request.
    std::ostringstream respStatus;
    respStatus << "Status: " << response.getStatus() << "\nReason: " << response.getReasonForStatus( response.getStatus());
    respStatus << "\n\nReply text:\n";
    respStatus << responseStream.rdbuf();
    serverErr = respStatus.str();

    if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
      // Probably some kind of username/password mismatch.  Clear the password so that
      // the user can enter it again
      m_password.clear();
    }

    reqErr = JM_HTTP_SERVER_ERR;
  }
  else
  {
    // Success!
    reqErr = JM_OK;  // This should already be set, but just in case....
  }

  return reqErr;
}



// Returns a list of all the files on the remote machine associated with the specified transaction
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::listFiles( const std::string &transId,
                                                                       std::vector<std::string> &listing,
                                                                       std::string &serverErr)
{
  JobManagerErrorCode reqErr = JM_OK;

  // Create an HTTPS session
  // TODO: Notice that we've set the context to VERIFY_NONE.  I think that means we're not checking the SSL certificate that the server
  // sends to us.  That's BAD!!
  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( Poco::URI(m_serviceBaseUrl).getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);

  // We need to send a GET request to the server with a query string of "TransID=xxxxx&Action=query"
  Poco::Net::HTTPRequest req;
  std::string queryString = "Action=query&TransID=" + transId;
  reqErr = initGetRequest( req, "/file_transfer", queryString);
  if ( reqErr != JM_OK)
  {
    return reqErr;
  }

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;
  std::istream &responseStream = session.receiveResponse( response);
  std::vector<Poco::Net::HTTPCookie> newCookies;
  // For as yet unknown reasons, we don't always get a session cookie back from the
  // server.  In that case, we don't want to overwrite the cookie we're currently
  // using...
  response.getCookies( newCookies);
  if (newCookies.size() > 0)
  {
    m_cookies = newCookies;
  }

  // We should get back an HTTP_OK code and a JSON array with all the file names
  if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
  {
    // D'oh!  The server didn't like our request.
    std::ostringstream respStatus;
    respStatus << "Status: " << response.getStatus() << "\nReason: " << response.getReasonForStatus( response.getStatus());
    respStatus << "\n\nReply text:\n";
    respStatus << responseStream.rdbuf();
    serverErr = respStatus.str();

    if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
      // Probably some kind of username/password mismatch.  Clear the password so that
      // the user can enter it again
      m_password.clear();
    }

    reqErr = JM_HTTP_SERVER_ERR;
  }
  else
  {
    // Success!
    // Parse the response body for the list of files.  The response should be a
    // single JSON array that looks something like: ["file1.out", "file2.out"]
    JSONObject results;

    initFromStream( results, responseStream);
    JSONObject::const_iterator itResults = results.find( "filenames");
    JSONArray names;
    (*itResults).second.getValue(names);
    JSONArray::const_iterator itNames = names.begin();
    while (itNames != names.end())
    {
      std::string oneName;
      (*itNames).getValue( oneName);
      listing.push_back( oneName);
      itNames++;
    }

    reqErr = JM_OK;  // This should already be set, but just in case....
  }

  return reqErr;

}

// upload the specified file.
// Note: remoteFileName is just the file name (no path), but localFileName should include
// the complete path
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::uploadFile( const std::string &transId,
                                                                        const std::string &remoteFileName,
                                                                        const std::string &localFileName,
                                                                        std::string &serverErr)
{
  JobManagerErrorCode reqErr = JM_OK;

  // Verify that the file we want to upload actually exists
  std::ifstream infile( localFileName.c_str(), std::ios_base::binary);
  if (! infile.good())
  {
    serverErr = "Could not open local file: " + localFileName;
    return JM_LOCAL_FILE_ERROR;
  }

  // Create an HTTPS session
  // TODO: Notice that we've set the context to VERIFY_NONE.  I think that means we're not checking the SSL certificate that the server
  // sends to us.  That's BAD!!
  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( Poco::URI(m_serviceBaseUrl).getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);

  Poco::Net::HTTPRequest req;
  reqErr = initPostRequest( req, "/file_transfer");
  if ( reqErr != JM_OK)
  {
    return reqErr;
  }

  // We have to do a POST with multipart MIME encoding.  MIME is rather picky about
  // how the parts are delimited.  See RFC 2045 & 2046 for details.

  char httpLineEnd[3] = { 0x0d, 0x0a, 0x00 };  // HTTP uses CRLF for its line endings

  // boundary can be almost anything (again, see RFC 2046).  The important part is that it
  // cannot appear anywhere in the actual data
  std::string boundary = "112233MantidHTTPBoundary44556677";
  std::string boundaryLine = "--" + boundary + httpLineEnd;
  std::string finalBoundaryLine = "--" + boundary + "--" + httpLineEnd;

  req.setContentType( "multipart/form-data; boundary=" + boundary);


  // Need to be able to specify the content length, so build up (most
  // of) the post body here.
  std::stringstream postData;

  // Set the POST variable to attach to the PHP debugger
  postData << boundaryLine;
  postData <<"Content-Disposition: form-data; name=\"XDEBUG_SESSION_START\"" << httpLineEnd;
  postData << httpLineEnd;
  postData << "MWS" << httpLineEnd;

  // These are the same variables that we put in the query string when performing an HTTP GET
  postData << boundaryLine;
  postData <<"Content-Disposition: form-data; name=\"Action\"" << httpLineEnd;
  postData << httpLineEnd;
  postData << "upload" << httpLineEnd;

  postData << boundaryLine;
  postData <<"Content-Disposition: form-data; name=\"TransID\"" << httpLineEnd;
  postData << httpLineEnd;
  postData << transId << httpLineEnd;

  postData << boundaryLine;
  postData << "Content-Disposition: form-data; name=\"File\"; filename=\"" << remoteFileName << "\"" << httpLineEnd;
  postData << "Content-Type: application/octet-stream" << httpLineEnd;
  postData << httpLineEnd;

  infile.seekg (0, std::ios_base::end);
  auto fileLen = infile.tellg();
  infile.seekg (0, std::ios_base::beg);

  req.setContentLength( postData.str().size() + fileLen + strlen(httpLineEnd) + finalBoundaryLine.size());
  //req.setContentLength( postData.str().size());

  std::ostream &postStream = session.sendRequest( req);

  // upload the actual HTTP body
  postStream << postData.rdbuf();

  postStream << infile.rdbuf();

  postStream << httpLineEnd;
  postStream << finalBoundaryLine << std::flush;

  infile.close();  // done with the file

  Poco::Net::HTTPResponse response;
  std::istream &responseStream = session.receiveResponse( response);
  std::vector<Poco::Net::HTTPCookie> newCookies;
  // For as yet unknown reasons, we don't always get a session cookie back from the
  // server.  In that case, we don't want to overwrite the cookie we're currently
  // using...
  response.getCookies( newCookies);
  if (newCookies.size() > 0)
  {
    m_cookies = newCookies;
  }

  // We should get back an HTTP_OK code
  if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
  {
    // D'oh!  The server didn't like our request.
    std::ostringstream respStatus;
    respStatus << "Status: " << response.getStatus() << "\nReason: " << response.getReasonForStatus( response.getStatus());
    respStatus << "\n\nReply text:\n";
    respStatus << responseStream.rdbuf();
    serverErr = respStatus.str();

    if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
      // Probably some kind of username/password mismatch.  Clear the password so that
      // the user can enter it again
      m_password.clear();
    }

    reqErr = JM_HTTP_SERVER_ERR;
  }

  return reqErr;
}



// download the specified file.
// Note: remoteFileName is just the file name (no path), but localFileName should include
// the complete path
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::downloadFile( const std::string &transId,
                                                                          const std::string &remoteFileName,
                                                                          const std::string &localFileName,
                                                                          std::string &serverErr)
{
  JobManagerErrorCode reqErr = JM_OK;

  // Create an HTTPS session
  // TODO: Notice that we've set the context to VERIFY_NONE.  I think that means we're not checking the SSL certificate that the server
  // sends to us.  That's BAD!!
  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( Poco::URI(m_serviceBaseUrl).getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);

  // We need to send a GET request to the server with a query string of "TransID=xxxx&Action=download&File=zzzzz"
  Poco::Net::HTTPRequest req;
  std::string queryString = "Action=download&TransID=" + transId + "&File=" + remoteFileName;
  queryString += "&XDEBUG_SESSION_START=MWS";  // enable debugging of the remote PHP
  reqErr = initGetRequest( req, "/file_transfer", queryString);
  if ( reqErr != JM_OK)
  {
    return reqErr;
  }

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;
  std::istream &responseStream = session.receiveResponse( response);
  std::vector<Poco::Net::HTTPCookie> newCookies;
  // For as yet unknown reasons, we don't always get a session cookie back from the
  // server.  In that case, we don't want to overwrite the cookie we're currently
  // using...
  response.getCookies( newCookies);
  if (newCookies.size() > 0)
  {
    m_cookies = newCookies;
  }

  // We should get back an HTTP_OK code and a JSON array with all the file names
  if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
  {
    // D'oh!  The server didn't like our request.
    std::ostringstream respStatus;
    respStatus << "Status: " << response.getStatus() << "\nReason: " << response.getReasonForStatus( response.getStatus());
    respStatus << "\n\nReply text:\n";
    respStatus << responseStream.rdbuf();
    serverErr = respStatus.str();

    if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
      // Probably some kind of username/password mismatch.  Clear the password so that
      // the user can enter it again
      m_password.clear();
    }

    reqErr = JM_HTTP_SERVER_ERR;
  }
  else
  {
    // Successfully downloaded the file.  Now try to save it.
    std::ofstream outfile( localFileName.c_str(), std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    if (outfile.good())
    {
      outfile << responseStream.rdbuf();
      outfile.close();
    }
    else
    {
      reqErr = JM_LOCAL_FILE_ERROR;
      serverErr = "Failed to open local file (" + localFileName + ") for writing.";
    }
  }

  return reqErr;
}



// Wrappers for a lot of the boilerplate code needed to perform an HTTPS GET or POST
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::initHTTPRequest( Poco::Net::HTTPRequest &req,
                                                                             const std::string &method,
                                                                             std::string extraPath,
                                                                             std::string queryString)
{
  // Open an HTTP connection to the cluster
  Poco::URI uri(m_serviceBaseUrl);

  //if (uri.getScheme() != "https")
  //{
//    // Disallow unencrypted channels (because we're sending the password in the
//    // HTTP auth header)
//    return JM_CLEARTEXT_DISALLOWED;
//  }

  std::string path = uri.getPath();
  // Path should be something like "/mws/rest", append extraPath to it.
  path += extraPath;

  uri.setPath( path);
  if (method == Poco::Net::HTTPRequest::HTTP_GET &&
      queryString.size() > 0)
  {
    uri.setQuery(queryString);
  }

  req.setVersion(Poco::Net::HTTPRequest::HTTP_1_1);
  req.setMethod( method);
  req.setURI(uri.toString());

  // Set the Authorization header (base64 encoded)
  std::ostringstream encodedAuth;
  Poco::Base64Encoder encoder( encodedAuth);

  encoder << m_userName << ":" << m_password;
  encoder.close();

  req.setCredentials( "Basic", encodedAuth.str());

  // Attach any cookies we've got from previous responses
  req.setCookies( getCookies());

  return JM_OK;
}

// Converts the vector of HTTPCookie objects into a NameValueCollection
Poco::Net::NameValueCollection HttpRemoteJobManager::getCookies()
{
  Poco::Net::NameValueCollection nvc;
  std::vector<Poco::Net::HTTPCookie>::const_iterator it = m_cookies.begin();
  while (it != m_cookies.end())
  {
    nvc.add( (*it).getName(), (*it).getValue());
    it++;
  }
  return nvc;
}




/* ************* MWSRemoteJobManagaer member functions ****************** */

MwsRemoteJobManager::MwsRemoteJobManager(const Poco::XML::Element *elem)
  : HttpRemoteJobManager( elem)
{
  // MWS rather annoyingly uses its own format for date/time strings.  One of the main
  // differences between MWS strings and ISO 8601 is the use of a timzezone abbreviation
  // instead of an offset from UTC.
  // It turns out there there doesn't seem to be a standardized, cross-platform way
  // to map these abbreviations to their offsets, so I'm just going to build an STL
  // map right here.  Feel free to add more abbreviations as necessary.
  // This map gets used down in convertToISO8601().
  m_tzOffset["EDT"] = "-4";
  m_tzOffset["EST"] = "-5";
  m_tzOffset["CDT"] = "-5";
  m_tzOffset["CST"] = "-6";
  m_tzOffset["MDT"] = "-6";
  m_tzOffset["MST"] = "-7";
  m_tzOffset["PDT"] = "-7";
  m_tzOffset["PST"] = "-8";
  m_tzOffset["PDT"] = "-9";
  m_tzOffset["UTC"] = "+0";

  // Parse the XML for the mpirun and python executables
  Poco::XML::NodeList* nl;
  Poco::XML::Text* txt;

  nl = elem->getElementsByTagName("mpirunExecutable");
  if (nl->length() != 1)
  {
    g_log.error("Compute Resources must have exactly one mpirunExecutable tag");
    throw std::runtime_error("Compute Resources must have exactly one mpirunExecutable tag");
  }
  else
  {
    nl = nl->item(0)->childNodes();
    if (nl->length() > 0)
    {
      txt = dynamic_cast<Poco::XML::Text*>(nl->item(0));
      if (txt)
      {
        m_mpirunExecutable = txt->getData();
      }
    }
  }

  nl = elem->getElementsByTagName("pythonExecutable");
  if (nl->length() != 1)
  {
    g_log.error("Compute Resources must have exactly one pythonExecutable tag");
    throw std::runtime_error("Compute Resources must have exactly one pythonExecutable tag");
  }
  else
  {
    nl = nl->item(0)->childNodes();
    if (nl->length() > 0)
    {
      txt = dynamic_cast<Poco::XML::Text*>(nl->item(0));
      if (txt)
      {
        m_pythonExecutable = txt->getData();
      }
    }
  }
}



// Returns true if the job was successfully submitted, false if there was a problem
// retString will contain the job ID on success or an explanation of the problem on
// failure.
bool MwsRemoteJobManager::submitJob( const RemoteTask &remoteTask, std::string &retString)
{
    /**************************************************************************
     * The minimal JSON text needed to submit a job looks something like this:
     *
     *{
     *   "commandFile": "/tmp/myscript.sh",
     *   "commandLineArguments": "-x",
     *   "user": "jacob",
     *   "group": "wheel",
     *   "name": "job name",
     *   "requirements": [ {
     *   "requiredProcessorCountMinimum": 4,
     *   }],
     *   "standardErrorFilePath": "/home/jacob/err",
     *   "standardOutputFilePath": "/home/jacob/out",
     *}
     ***********************************************************************/
    bool retVal = false;  // assume failure

    // Build up the JSON struct for submitting a job to MWS
    std::ostringstream json;
    // Note:  This is MWS API v1.0.  It looks like the version 2.0 API cleans things up a bit
    // and should be used instead.
    json << "{\n ";
    json << "\"commandFile\": \"" << remoteTask.getResourceValue("executable") << "\",\n";
    json << "\"commandLineArguments\": \"" << escapeQuoteChars( remoteTask.getCmdLineParams() )<< "\",\n";
    json << "\"user\": \"" << m_userName << "\",\n";
    json << "\"group\": \"" << remoteTask.getResourceValue( "group") << "\",\n";
    json << "\"name\": \"" << remoteTask.getName() << "\",\n";
    json << "\"variables\": {\"SUBMITTING_APP\": \"MantidPlot\"},\n";
    json << "\"requirements\": [{\n";
    json << "\t\"requiredProcessorCountMinimum\": \"" << remoteTask.getResourceValue("num_nodes") << "\"}],\n";


    // Note: setting the environment variables is something of a kludge, but it allows me to pass info
    // down to the process that will actually run.  In this case, parameters for the mpirun command line.
    json << "\"environmentVariables\" : {\n";
    json << "\t\"MANTIDPLOT_NUM_NODES\" : \"" << remoteTask.getResourceValue("num_nodes") << "\",\n";
    json << "\t\"MANTIDPLOT_CORES_PER_NODE\" : \"" << remoteTask.getResourceValue("cores_per_node") << "\"}\n"; // don't forget the , before the \n if this is no longer the last line in the json

    //json << "\"standardErrorFilePath\": \"/home/" + user + "\",\n";
    //json << "\"standardOutputFilePath\": \"/home/" + user + "\"\n";
    json << "}";

    // Note: I'm currently not specifying the standardErrorFilePath or standardOutputFilePath
    // parameters.  I don't think I'll need them.

    // Open an HTTP connection to the cluster
    Poco::URI uri(m_serviceBaseUrl);

    if (uri.getScheme() != "https")
    {
      // Disallow unencrypted channels (because we're sending the password in the
      // HTTP auth header)
      retString = "Refusing to initiate unencrypted channel.  Only HTTPS URL's are allowed.";
      return false;
    }

    Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
    Poco::Net::HTTPSClientSession session( uri.getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);

    std::string path = uri.getPathAndQuery();
    // Path should be something like "/mws/rest", append "/jobs" to it.
    path += "/jobs";

    // TODO: Probably don't need this parameter any more
    // append the outfile variable to the URL (the PHP remembers this so we can download the file later)
    //path += "?outfile=" + remoteTask.getSubstitutionParamValue( "outfile");

    Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_POST, path, Poco::Net::HTTPRequest::HTTP_1_1);
    req.setContentType( "application/json");

    // Set the Authorization header (base64 encoded)
    std::ostringstream encodedAuth;
    Poco::Base64Encoder encoder( encodedAuth);

    encoder << m_userName << ":" << m_password;
    encoder.close();

    req.setCredentials( "Basic", encodedAuth.str());

    // Apparently, the Content Length header is required.  Without it, MWS never receives the request
    // body.  (Apperently, either MWS or maybe Tomcat assumes the length to be 0 if it's not specified?!?)
    req.setContentLength( (int)(json.str().length()) );

    // Attach any cookies we've got from previous responses
    req.setCookies( getCookies());

    //std::ostream &reqBody = session.sendRequest( req);
    //reqBody << json.str();
    session.sendRequest( req) << json.str();

    Poco::Net::HTTPResponse response;

    // Note: If we wanted to, we could call response.getStatus() at this point
    // and verify that we got a 200 code back from the server (which would mean that
    // we had sent a valid HTTP_POST message).  I'm not bothering with that
    // because we check for a response code after calling receiveResponse()
    // anyway.

    std::istream &responseStream = session.receiveResponse( response);
    std::vector<Poco::Net::HTTPCookie> newCookies;
    // For as yet unknown reasons, we don't always get a session cookie back from the
    // server.  In that case, we don't want to overwrite the cookie we're currently
    // using...
    response.getCookies( newCookies);
    if (newCookies.size() > 0)
    {
      m_cookies = newCookies;
    }

    if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_CREATED)
    {
        // D'oh!  The MWS server didn't like our request.
        std::ostringstream respStatus;
        respStatus << "Status: " << response.getStatus() << "\nReason: " << response.getReasonForStatus( response.getStatus());
        respStatus << "\n\nReply text:\n";
        respStatus << responseStream.rdbuf();
        retString = respStatus.str();

        if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
        {
          // Probably some kind of username/password mismatch.  Clear the password so that
          // the user can enter it again
          m_password.clear();
        }

        retVal = false;
    }
    else
    {
        // Success!
        retVal = true;

        // Parse the response body for the job ID.  The response should be a single
        // JSON element that looks something like: {"id":"12345"}
        retString = "UNKNOWN JOB ID";

        std::ostringstream respStream;
        respStream << responseStream.rdbuf();
        std::string respBody = respStream.str();
        size_t pos = respBody.find ("\"id\":");
        if (pos != std::string::npos)
        {
            retString = respBody.substr( pos+6, respBody.find_last_of('"') - (pos+6));
        }
    }

    return retVal;
}


// Queries MWS for the status of the specified job.
// If the query is successfully submitted, it writes the status value to retStatus and
// returns true.  If there was a problem submitting the query (network is down, for example)
// it writes an error message to errMsg and returns false;
bool MwsRemoteJobManager::jobStatus( const std::string &jobId,
                                     RemoteJob::JobStatus &retStatus,
                                     std::string &errMsg)
{

    /*****************
      NOTE: There's a lot of redundancy between this function and submitJob().
      Ought to try to clean it up!
      ********************************/


  bool retVal = false;  // assume failure

  // Open an HTTP connection to the cluster
  Poco::URI uri(m_serviceBaseUrl);

  if (uri.getScheme() != "https")
  {
    // Disallow unencrypted channels (because we're sending the password in the
    // HTTP auth header)
    errMsg = "Refusing to initiate unencrypted channel.  Only HTTPS URL's are allowed.";
    return false;
  }

  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( uri.getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);

  std::ostringstream path;
  path << uri.getPathAndQuery();
  // Path should be something like "/mws/rest", append "/jobs/<job_id>" to it.
  path << "/jobs/" << jobId;

  Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path.str(), Poco::Net::HTTPRequest::HTTP_1_1);
  req.setContentType( "application/json");

  // Set the Authorization header (base64 encoded)
  std::ostringstream encodedAuth;
  Poco::Base64Encoder encoder( encodedAuth);

  encoder << m_userName << ":" << m_password;
  encoder.close();

  req.setCredentials( "Basic", encodedAuth.str());

  // Attach any cookies we've got from previous responses
  req.setCookies( getCookies());

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;
  std::istream &responseStream = session.receiveResponse( response);
  std::vector<Poco::Net::HTTPCookie> newCookies;
  // For as yet unknown reasons, we don't always get a session cookie back from the
  // server.  In that case, we don't want to overwrite the cookie we're currently
  // using...
  response.getCookies( newCookies);
  if (newCookies.size() > 0)
  {
    m_cookies = newCookies;
  }

  if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
  {
      // D'oh!  The MWS server didn't like our request.
      std::ostringstream respStatus;
      respStatus << "Status: " << response.getStatus() << "\nReason: " << response.getReasonForStatus( response.getStatus());
      respStatus << "\n\nReply text:\n";
      respStatus << responseStream.rdbuf();
      errMsg = respStatus.str();

      if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
      {
        // Probably some kind of username/password mismatch.  Clear the password so that
        // the user can enter it again
        m_password.clear();
      }
  }
  else
  {
      // Parse the response body for the state
      std::string statusString = "UNKNOWN";

      std::ostringstream respStream;
      respStream << responseStream.rdbuf();
      std::string respBody = respStream.str();
      size_t pos = respBody.find ("\"state\":");
      if (pos != std::string::npos)
      {
          statusString = respBody.substr( pos+9, respBody.find('"', pos+9) - (pos+9));
      }

      // Convert the string into a JobStatus
      if (statusString == "RUNNING")
      {
          retStatus = RemoteJob::JOB_RUNNING;
          retVal = true;
      }
      else if (statusString == "QUEUED")
      {
          retStatus = RemoteJob::JOB_QUEUED;
          retVal = true;
      }
      else if (statusString == "COMPLETED")
      {
          retStatus = RemoteJob::JOB_COMPLETE;
          retVal = true;
      }
      else if (statusString == "REMOVED")
      {
          retStatus = RemoteJob::JOB_REMOVED;
          retVal = true;
      }
      else if (statusString == "DEFERRED")
      {
          retStatus = RemoteJob::JOB_DEFERRED;
          retVal = true;
      }
      else if (statusString == "IDLE")
      {
          retStatus = RemoteJob::JOB_IDLE;
          retVal = true;
      }
      /* else if what else??? */
      else
      {
          retStatus = RemoteJob::JOB_STATUS_UNKNOWN;
          errMsg = "Unknown job state: " + statusString;
      }
  }

  return retVal;
}


// Queries MWS for the details of every job the user has submitted.
// If the query is successfully submitted, it writes the status value to retStatus and
// returns true.  If there was a problem submitting the query (network is down, for example)
// it writes an error message to errMsg and returns false;
bool MwsRemoteJobManager::jobStatusAll( std::vector<RemoteJob> &jobList,
                                        std::string &errMsg)
{

  /*****************
    NOTE: There's a lot of redundancy between this function, jobStatus() &
    submitJob(). Ought to try to clean it up!
   ********************************/


  bool retVal = true;  // assume success

  // Open an HTTP connection to the cluster
  Poco::URI uri(m_serviceBaseUrl);

  if (uri.getScheme() != "https")
  {
    // Disallow unencrypted channels (because we're sending the password in the
    // HTTP auth header
    errMsg = "Refusing to initiate unencrypted channel.  Only HTTPS URL's are allowed.";
    return false;
  }
  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( uri.getHost(), uri.getPort(), context);

  std::ostringstream path;
  path << uri.getPathAndQuery();
  // Path should be something like "/mws/rest", append "/jobs" to it.
  path << "/jobs";

  Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path.str(), Poco::Net::HTTPRequest::HTTP_1_1);
  req.setContentType( "application/json");

  // Set the Authorization header (base64 encoded)
  std::ostringstream encodedAuth;
  Poco::Base64Encoder encoder( encodedAuth);

  encoder << m_userName << ":" << m_password;
  encoder.close();

  // Attach any cookies we've got from previous responses
  req.setCookies( getCookies());

  req.setCredentials( "Basic", encodedAuth.str());

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;
  std::istream &responseStream = session.receiveResponse( response);
  std::vector<Poco::Net::HTTPCookie> newCookies;
  // For as yet unknown reasons, we don't always get a session cookie back from the
  // server.  In that case, we don't want to overwrite the cookie we're currently
  // using...
  response.getCookies( newCookies);
  if (newCookies.size() > 0)
  {
    m_cookies = newCookies;
  }

  if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
  {
    // D'oh!  The MWS server didn't like our request.
    retVal = false;
    std::ostringstream respStatus;
    respStatus << "Status: " << response.getStatus() << "\nReason: " << response.getReasonForStatus( response.getStatus());
    respStatus << "\n\nReply text:\n";
    respStatus << responseStream.rdbuf();
    errMsg = respStatus.str();

    if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
      // Probably some kind of username/password mismatch.  Clear the password so that
      // the user can enter it again
      m_password.clear();
    }
  }
  else
  {
    // Parse the response body for the state
    // (Info for each job is stored in an array of JSON objects - 1 object per job)
    JSONObject jobs;
    initFromStream( jobs, responseStream);
    JSONObject::const_iterator itResults = jobs.find( "results");
    JSONArray resultsArray;
    (*itResults).second.getValue(resultsArray);
    JSONArray::const_iterator it = resultsArray.begin();
    while (it != resultsArray.end())
    {
      JSONObject oneJob;
      it->getValue( oneJob);

      // Verify that this is a job that was originally submitted by MantidPlot
      JSONObject varObj;
      JSONObject::const_iterator itVar = oneJob.find("variables");
      if (itVar != oneJob.end())
      {
        (*itVar).second.getValue( varObj);
        itVar = varObj.find( "SUBMITTING_APP");  // This string *must* match the one used in ::submitJob()!S
// Commenting out the actual 'if' test until Adaptive Computing fix #15864
// As it is right now, MWS 'forgets' about the variables tags after about 45 minutes which means
// we wind up with an empty job status dialog.
//        if (itVar != varObj.end())
        {
          // This is a job submitted by MantidPlot.  Construct a RemoteJob instance and add it to jobList
          // Fields passed to the constructor for RemoteJob
          std::string jobId;
          RemoteJob::JobStatus status;
          std::string algName;

          (*oneJob.find("id")).second.getValue( jobId);
          (*oneJob.find( "name")).second.getValue( algName);
          std::string submitTimeString;
          (*oneJob.find( "submitDate")).second.getValue( submitTimeString);
          // Unfortunately, the string that MWS returns is not quite in ISO 8601 format
          convertToISO8601( submitTimeString);

          // Get start time
          std::string startTimeString;
          (*oneJob.find( "startDate")).second.getValue( startTimeString);
          Mantid::Kernel::DateAndTime startTime;
          if (convertToISO8601( startTimeString))
          {
            startTime = Mantid::Kernel::DateAndTime( startTimeString);
          }
          else
          {
            startTime = Mantid::Kernel::DateAndTime().minimum();
          }

          // Get completion time
          std::string completionTimeString;
          (*oneJob.find( "completionDate")).second.getValue( completionTimeString);
          Mantid::Kernel::DateAndTime completionTime;
          if (convertToISO8601( completionTimeString))
          {
            completionTime = Mantid::Kernel::DateAndTime( completionTimeString);
          }
          else
          {
            completionTime = Mantid::Kernel::DateAndTime().minimum();
          }

          std::string statusString;
          (*oneJob.find( "expectedState")).second.getValue( statusString);

          // Convert the string into a JobStatus
          if (statusString == "RUNNING")
          {
            status = RemoteJob::JOB_RUNNING;
          }
          else if (statusString == "QUEUED")
          {
            status = RemoteJob::JOB_QUEUED;
          }
          else if (statusString == "COMPLETED")
          {
            status = RemoteJob::JOB_COMPLETE;
          }
          else if (statusString == "REMOVED")
          {
            status = RemoteJob::JOB_REMOVED;
          }
          else if (statusString == "DEFERRED")
          {
            status = RemoteJob::JOB_DEFERRED;
          }
          else if (statusString == "IDLE")
          {
            status = RemoteJob::JOB_IDLE;
          }

          /* else if what else??? */
          else
          {
            status = RemoteJob::JOB_STATUS_UNKNOWN;
            errMsg = "Unknown job state: " + statusString;
            retVal = false;
          }

          RemoteJob job = RemoteJob( jobId, this, status, algName, Mantid::Kernel::DateAndTime( submitTimeString));
          job.setStartTime( startTime);
          job.setCompletionTime( completionTime);
          jobList.push_back( job);
        }

      }
      it++;
    }
  }

  return retVal;
}





// Helper function used by jobStatusAll.  Converts a time string
// returned by MWS into a properly formatted ISO 8601 string.
// Note:  Only reason it's a member of MwsRemoteJobManager is so that it
// can access the logger.
bool MwsRemoteJobManager::convertToISO8601( std::string &time)
{
  // First the easy bit: insert a 'T' between the date and time fields
  size_t pos = time.find( ' ');
  if (pos == std::string::npos)
    return false;  // Give up.  String wasn't formatted the way we expected it to be.

  time[pos]='T';

  // Now the hard part: extract the time zone abbreviation and replace it with
  // the appropriate offset value.  Amazingly, there does not seem to be an easy
  // way to convert a timezone abbreviation into an offset, so I've had to make
  // my own map.
  std::string zone = time.substr(time.rfind(' ')+1);
  time.resize(time.rfind(' '));
  std::map<std::string, std::string>::const_iterator it = m_tzOffset.find(zone);
  if (it == m_tzOffset.end())
  {
    // Didn't recognize the timezone abbreviation.  Log a warning, but otherwise
    // ignore it and continue on...
    g_log.warning() << "Unrecognized timezone abbreviation \"" << zone
                    << "\".  Ignoring it and treating the time as UTC." << std::endl;
  }
  else
  {
    time.append((*it).second);
  }

  return true;
}


// puts a \ char in front of any " chars it finds (needed for the JSON stuff)
std::string MwsRemoteJobManager::escapeQuoteChars( const std::string & str)
{
  std::string out;
  size_t start = 0;
  size_t end = 0;
  do {
    end = str.find('"', start);
    if (end == std::string::npos)
    {
      out += str.substr(start);
    }
    else
    {
      out += str.substr(start, end-start);
      out += "\\\"";  // append a single backslash char and a single quote char
      start = end+1;
    }
  } while (end != std::string::npos && start < str.length());

  return out;
}
