#include "RemoteJobManager.h"
#include "RemoteAlg.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/Base64Encoder.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>

#include <ostream>
#include <sstream>
using namespace std;



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
bool MwsRemoteJobManager::submitJob( const RemoteAlg &remoteAlg, string &retString)
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
    json << "\"commandFile\": \"" << remoteAlg.getExecutable() << "\",\n";
    json << "\"commandLineArguments\": \"" << escapeQuoteChars( remoteAlg.getCmdLineParams() )<< "\",\n";
    json << "\"user\": \"" << m_userName << "\",\n";
    json << "\"group\": \"" << remoteAlg.getResourceValue( "group") << "\",\n";
    json << "\"name\": \"" << remoteAlg.getName() << "\",\n";
    json << "\"requirements\": [{\n";
    json << "\t\"requiredProcessorCountMinimum\": \"" << remoteAlg.getResourceValue("nodes") << "\"}]\n";  // don't forget the , before the \n if this is no longer the last line in the json
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
      /* else if what else??? */
      else
      {
          retStatus = RemoteJob::JOB_STATUS_UNKNOWN;
          errMsg = "Unknown job state: " + statusString;
      }
  }

  return retVal;
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

