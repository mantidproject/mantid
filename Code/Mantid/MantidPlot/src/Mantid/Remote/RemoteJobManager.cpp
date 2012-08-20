#include "RemoteJobManager.h"
#include "RemoteTask.h"
#include "MantidKernel/ConfigService.h"
#include "SimpleJSON.h"

#include <Poco/Base64Encoder.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>

#include <ostream>
#include <sstream>
using namespace std;


// Get a reference to the logger
Mantid::Kernel::Logger& MwsRemoteJobManager::g_log = Mantid::Kernel::Logger::get("MwsRemoteJobManager");


void RemoteJobManager::saveProperties( int itemNum)
{
    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
    ostringstream tempStr;
    string prefix("Cluster.");
    tempStr << itemNum;
    prefix += tempStr.str() + string(".");

    config.setString( prefix + string( "DisplayName"), m_displayName);
    config.setString( prefix + string( "ConfigFileUrl"), m_configFileUrl);
}


MwsRemoteJobManager::MwsRemoteJobManager( std::string displayName, std::string configFileUrl,
                                          std::string serviceBaseUrl, std::string userName) :
    HttpRemoteJobManager( displayName, configFileUrl),
    m_serviceBaseUrl( serviceBaseUrl),
    m_userName( userName)
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
}

void MwsRemoteJobManager::saveProperties( int itemNum)
{
    HttpRemoteJobManager::saveProperties( itemNum);

    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
    ostringstream tempStr;
    string prefix("Cluster.");
    tempStr << itemNum;
    prefix += tempStr.str() + string(".");

    config.setString( prefix + string( "Type"), getType());
    config.setString( prefix + string( "ServiceBaseUrl"), m_serviceBaseUrl);
    config.setString( prefix + string( "UserName"), m_userName);
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
    json << "\"commandFile\": \"" << remoteTask .getExecutable() << "\",\n";
    json << "\"commandLineArguments\": \"" << escapeQuoteChars( remoteTask .getCmdLineParams() )<< "\",\n";
    json << "\"user\": \"" << m_userName << "\",\n";
    json << "\"group\": \"" << remoteTask .getResourceValue( "group") << "\",\n";
    json << "\"name\": \"" << remoteTask .getName() << "\",\n";
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
    Poco::Net::HTTPClientSession session( uri.getHost(), uri.getPort());

    std::string path = uri.getPathAndQuery();
    // Path should be something like "/mws/rest", append "/jobs" to it.
    path += "/jobs";

    // append the outfile variable to the URL (the PHP remembers this so we can download the file later)
    path += "?outfile=" + remoteTask.getSubstitutionParamValue( "outfile");

    Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_POST, path, Poco::Net::HTTPRequest::HTTP_1_1);
    req.setContentType( "application/json");

    // Set the Authorization header (base64 encoded)
    ostringstream encodedAuth;
    Poco::Base64Encoder encoder( encodedAuth);

    if (m_password.empty())
    {
      getPassword();
    }

    encoder << m_userName << ":" << m_password;
    encoder.close();

    req.setCredentials( "Basic", encodedAuth.str());

    // Apparently, the Content Length header is required.  Without it, MWS never receives the request
    // body.  (Apperently, either MWS or maybe Tomcat assumes the length to be 0 if it's not specified?!?)
    req.setContentLength( (int)(json.str().length()) );

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
  Poco::Net::HTTPClientSession session( uri.getHost(), uri.getPort());

  std::ostringstream path;
  path << uri.getPathAndQuery();
  // Path should be something like "/mws/rest", append "/jobs/<job_id>" to it.
  path << "/jobs/" << jobId;

  Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path.str(), Poco::Net::HTTPRequest::HTTP_1_1);
  req.setContentType( "application/json");

  // Set the Authorization header (base64 encoded)
  ostringstream encodedAuth;
  Poco::Base64Encoder encoder( encodedAuth);

  if (m_password.empty())
  {
    getPassword();
  }

  encoder << m_userName << ":" << m_password;
  encoder.close();

  req.setCredentials( "Basic", encodedAuth.str());

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;
  std::istream &responseStream = session.receiveResponse( response);

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
  Poco::Net::HTTPClientSession session( uri.getHost(), uri.getPort());

  std::ostringstream path;
  path << uri.getPathAndQuery();
  // Path should be something like "/mws/rest", append "/jobs" to it.
  path << "/jobs";

  Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path.str(), Poco::Net::HTTPRequest::HTTP_1_1);
  req.setContentType( "application/json");

  // Set the Authorization header (base64 encoded)
  ostringstream encodedAuth;
  Poco::Base64Encoder encoder( encodedAuth);

  if (m_password.empty())
  {
    getPassword();
  }

  encoder << m_userName << ":" << m_password;
  encoder.close();

  req.setCredentials( "Basic", encodedAuth.str());

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;
  std::istream &responseStream = session.receiveResponse( response);

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
        if (itVar != varObj.end())
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
  Poco::Net::HTTPClientSession session( uri.getHost(), uri.getPort());

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

  if (m_password.empty())
  {
    getPassword();
  }

  encoder << m_userName << ":" << m_password;
  encoder.close();

  req.setCredentials( "Basic", encodedAuth.str());

  session.sendRequest( req);

  // All we need to check is the return code.  And all we really care about is whether
  // the code is 200 or not.  We're not going to differentiate between the various
  // error codes...

  Poco::Net::HTTPResponse response;
  session.receiveResponse( response);
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
  Poco::Net::HTTPClientSession session( uri.getHost(), uri.getPort());

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

  if (m_password.empty())
  {
    getPassword();
  }

  encoder << m_userName << ":" << m_password;
  encoder.close();

  req.setCredentials( "Basic", encodedAuth.str());

  session.sendRequest( req);

  Poco::Net::HTTPResponse response;

  // Check the return code.  If it's good, then get the session stream and pull
  // down the rest of the file.
  if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
  {
    retVal = true;
    istream & respStream = session.receiveResponse(response);

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

// On success, creates a new object and returns pointer to it.  On failure,
// returns NULL
RemoteJobManager *RemoteJobManagerFactory::createFromProperties( int itemNum)
{
    // All the properties should start with the key "Cluster", followed by a key for their
    // item number, followed by the keys remaining keys they need.  ie: Cluster.0.DisplayName

    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
    ostringstream tempStr;
    string prefix("Cluster.");
    tempStr << itemNum;
    prefix += tempStr.str();

    std::vector< std::string> keys = config.getKeys( prefix);

    if (keys.size() == 0)
        return NULL;

    // Need a key for Type and it must have a value we recognize
    if (find( keys.begin(), keys.end(), std::string( "Type")) == keys.end())
        return NULL;

    std::string managerType;
    config.getValue( prefix + std::string(".Type"), managerType);

    if ( managerType == "MWS") return RemoteJobManagerFactory::createQtMwsManager( itemNum);
    // else if.....
    // else if.....


    // Type not recognized
    return NULL;

}


MwsRemoteJobManager *RemoteJobManagerFactory::createQtMwsManager( int itemNum)
{
    // There's 4 values that we need:  ConfigFileUrl, DisplayName, ServiceBaseUrl and UserName

    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
    ostringstream tempStr;
    string prefix("Cluster.");
    tempStr << itemNum;
    prefix += tempStr.str();

    std::vector< std::string> keys = config.getKeys( prefix);

    if (keys.size() == 0)
        return NULL;

    if (find( keys.begin(), keys.end(), std::string( "ConfigFileUrl")) == keys.end())
        return NULL;
    if (find( keys.begin(), keys.end(), std::string( "DisplayName")) == keys.end())
        return NULL;
    if (find( keys.begin(), keys.end(), std::string( "ServiceBaseUrl")) == keys.end())
        return NULL;
    if (find( keys.begin(), keys.end(), std::string( "UserName")) == keys.end())
        return NULL;


    std::string configFileUrl, displayName, serviceBaseUrl, userName;
    config.getValue( prefix + std::string(".ConfigFileUrl"), configFileUrl);
    config.getValue( prefix + std::string(".DisplayName"), displayName);
    config.getValue( prefix + std::string(".ServiceBaseUrl"), serviceBaseUrl);
    config.getValue( prefix + std::string(".UserName"), userName);

    // Do some quick sanity checks on the values...
    if (configFileUrl.length() == 0)
        return NULL;
    if (displayName.length() == 0)
        return NULL;
    if (serviceBaseUrl.length() == 0)
        return NULL;
    if (userName.length() == 0)
        return NULL;

    // Validation checks passed.  Create the object
    return new QtMwsRemoteJobManager( displayName, configFileUrl, serviceBaseUrl, userName);
}

