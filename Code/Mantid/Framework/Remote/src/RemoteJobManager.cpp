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
using namespace std;


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
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::startTransaction( string &transId, string &directory, string &serverErr)
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
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::stopTransaction( string &transId, string &serverErr)
{
  JobManagerErrorCode reqErr = JM_OK;

  // Create an HTTPS session
  // TODO: Notice that we've set the context to VERIFY_NONE.  I think that means we're not checking the SSL certificate that the server
  // sends to us.  That's BAD!!
  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( Poco::URI(m_serviceBaseUrl).getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);
  // We need to send a GET request to the server with a query string of "action=start"
  Poco::Net::HTTPRequest req;
  string queryString = "action=stop&transid=" + transId;
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


// Wrapper for a lot of the boilerplate code needed to perform an HTTPS GET
RemoteJobManager::JobManagerErrorCode HttpRemoteJobManager::initGetRequest( Poco::Net::HTTPRequest &req, string extraPath, string queryString)
{
  // Open an HTTP connection to the cluster
  Poco::URI uri(m_serviceBaseUrl);

  if (uri.getScheme() != "https")
  {
    // Disallow unencrypted channels (because we're sending the password in the
    // HTTP auth header)
    return JM_CLEARTEXT_DISALLOWED;
  }

  std::string path = uri.getPath();
  // Path should be something like "/mws/rest", append extraPath to it.
  path += extraPath;

  uri.setPath( path);
  uri.setQuery(queryString);

  req.setVersion(Poco::Net::HTTPRequest::HTTP_1_1);
  req.setMethod( Poco::Net::HTTPRequest::HTTP_GET);
  req.setURI(uri.toString());

  // Set the Authorization header (base64 encoded)
  ostringstream encodedAuth;
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
bool MwsRemoteJobManager::submitJob( const RemoteTask &remoteTask, string &retString)
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

    json << "{\n ";
    json << "\"commandFile\": \"" << m_mpirunExecutable << "\",\n";
    json << "\"commandLineArguments\": \"" << escapeQuoteChars( remoteTask.getCmdLineParams() )<< "\",\n";
    json << "\"user\": \"" << m_userName << "\",\n";
    json << "\"group\": \"" << remoteTask.getResourceValue( "group") << "\",\n";
    json << "\"name\": \"" << remoteTask.getName() << "\",\n";
    json << "\"variables\": {\"SUBMITTING_APP\": \"MantidPlot\"},\n";
    json << "\"requirements\": [{\n";
    json << "\t\"requiredProcessorCountMinimum\": \"" << remoteTask .getResourceValue("nodes") << "\"}]\n";  // don't forget the , before the \n if this is no longer the last line in the json
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
    ostringstream encodedAuth;
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
                                     string &errMsg)
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
  ostringstream encodedAuth;
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
                                        string &errMsg)
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
  ostringstream encodedAuth;
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
          string jobId;
          RemoteJob::JobStatus status;
          string algName;

          (*oneJob.find("id")).second.getValue( jobId);
          (*oneJob.find( "name")).second.getValue( algName);
          string submitTimeString;
          (*oneJob.find( "submitDate")).second.getValue( submitTimeString);
          // Unfortunately, the string that MWS returns is not quite in ISO 8601 format
          convertToISO8601( submitTimeString);

          string statusString;
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

          jobList.push_back(RemoteJob(jobId, this, status, algName, Mantid::Kernel::DateAndTime( submitTimeString)));
        }

      }
      it++;
    }
  }

  return retVal;
}


// Note: This function does not actually use the Moab Web Services API.  (There's nothing in
// MWS for dealing with output files.)  Instead, it relies on some custom PHP code that must
// be installed on the server.  See https://github.com/neutrons/MWS-Front-End
bool MwsRemoteJobManager::jobOutputReady( const std::string &jobId)
{
  bool retVal = false;  // assume failure

  // Open an HTTP connection to the cluster
  Poco::URI uri(m_serviceBaseUrl);

  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( uri.getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);

  std::ostringstream path;
  path << uri.getPathAndQuery();
  // Path should be something like "/mws/rest", append "/filecheck" to it.
  path << "/filecheck";
  // Append the URL query string
  path << "?jobid=" << jobId;

  Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path.str(), Poco::Net::HTTPRequest::HTTP_1_1);
  req.setContentType( "text/html");

  // Set the Authorization header (base64 encoded)
  ostringstream encodedAuth;
  Poco::Base64Encoder encoder( encodedAuth);

  encoder << m_userName << ":" << m_password;
  encoder.close();

  req.setCredentials( "Basic", encodedAuth.str());

  // Attach any cookies we've got from previous responses
  req.setCookies( getCookies());

  session.sendRequest( req);

  // All we need to check is the return code.  And all we really care about is whether
  // the code is 200 or not.  We're not going to differentiate between the various
  // error codes...

  Poco::Net::HTTPResponse response;
  session.receiveResponse( response);
  std::vector<Poco::Net::HTTPCookie> newCookies;
  // For as yet unknown reasons, we don't always get a session cookie back from the
  // server.  In that case, we don't want to overwrite the cookie we're currently
  // using...
  response.getCookies( newCookies);
  if (newCookies.size() > 0)
  {
    m_cookies = newCookies;
  }

  if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
  {
    retVal = true;
  }

  return retVal;
}



// Note: This function does not actually use the Moab Web Services API.  (There's nothing in
// MWS for dealing with output files.)  Instead, it relies on some custom PHP code that must
// be installed on the server.  See https://github.com/neutrons/MWS-Front-End
bool MwsRemoteJobManager::getJobOutput( const std::string &jobId, std::ostream &outstream)
{

  bool retVal = false;  // assume failure

  // Open an HTTP connection to the cluster
  Poco::URI uri(m_serviceBaseUrl);

  Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  Poco::Net::HTTPSClientSession session( uri.getHost(), Poco::Net::HTTPSClientSession::HTTPS_PORT, context);

  std::ostringstream path;
  path << uri.getPathAndQuery();
  // Path should be something like "/mws/rest", append "/download" to it.
  path << "/download";
  // Append the URL query string
  path << "?jobid=" << jobId;

  Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path.str(), Poco::Net::HTTPRequest::HTTP_1_1);
  req.setContentType( "text/html");

  // Set the Authorization header (base64 encoded)
  ostringstream encodedAuth;
  Poco::Base64Encoder encoder( encodedAuth);

  encoder << m_userName << ":" << m_password;
  encoder.close();

  req.setCredentials( "Basic", encodedAuth.str());

  // Attach any cookies we've got from previous responses
  req.setCookies( getCookies());

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;

  // Check the return code.  If it's good, then get the session stream and pull
  // down the rest of the file.
  if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
  {
    retVal = true;
    istream & respStream = session.receiveResponse(response);
    std::vector<Poco::Net::HTTPCookie> newCookies;
    // For as yet unknown reasons, we don't always get a session cookie back from the
    // server.  In that case, we don't want to overwrite the cookie we're currently
    // using...
    response.getCookies( newCookies);
    if (newCookies.size() > 0)
    {
      m_cookies = newCookies;
    }

    outstream << respStream.rdbuf();
    outstream << flush;  // make sure we've got everything before the session goes out of scope
  }

  return retVal;
}




// Helper function used by jobStatusAll.  Converts a time string
// returned by MWS into a properly formatted ISO 8601 string.
// Note:  Only reason it's a member of MwsRemoteJobManager is so that it
// can access the logger.
bool MwsRemoteJobManager::convertToISO8601( string &time)
{
  // First the easy bit: insert a 'T' between the date and time fields
  size_t pos = time.find( ' ');
  if (pos == string::npos)
    return false;  // Give up.  String wasn't formatted the way we expected it to be.

  time[pos]='T';

  // Now the hard part: extract the time zone abbreviation and replace it with
  // the appropriate offset value.  Amazingly, there does not seem to be an easy
  // way to convert a timezone abbreviation into an offset, so I've had to make
  // my own map.
  string zone = time.substr(time.rfind(' ')+1);
  time.resize(time.rfind(' '));
  map<string, string>::const_iterator it = m_tzOffset.find(zone);
  if (it == m_tzOffset.end())
  {
    // Didn't recognize the timezone abbreviation.  Log a warning, but otherwise
    // ignore it and continue on...
    g_log.warning() << "Unrecognized timezone abbreviation \"" << zone
                    << "\".  Ignoring it and treating the time as UTC." << endl;
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
