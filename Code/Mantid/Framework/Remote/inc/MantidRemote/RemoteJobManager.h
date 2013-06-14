#ifndef REMOTEJOBMANAGER_H
#define REMOTEJOBMANAGER_H

#include "RemoteJob.h"
#include <string>
#include <ostream>
#include <vector>

#include <Poco/Net/HTTPCookie.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/NameValueCollection.h>

/**
  * The basic class hierarchy looks like this:
  *
  * RemoteJobManager         Top-level abstract class
  * HttpRemoteJobManager     Mid-level abstract class - handles HTTP-specific stuff
  * MwsRemoteJobManager      Concrete class - communicates w/ Moab Web Services, inherits from HttpRemoteJobManager
  *
  * CondorRemoteJobManager   Concrete class - communicates w/ Condor.  Doesn't exist & probably never will
  * GlobusRemoteJobManager   Concrete class - communicates w/ Globus.  Doesn't exist yet, but the ISIS folks need it
  **/

// Forward declarations
class RemoteTask;               // Holds info about a particular remote task (executable name, cmd line params, etc...
                                // See comments in remotetask.h)
namespace Poco {
  namespace XML {
    class Element;
  }
}


class RemoteJobManager
{
public:
  RemoteJobManager( const Poco::XML::Element* elem);

  virtual ~RemoteJobManager() { }

  // This is intended to be a single enum for all of the errors that any function
  // (public or private) can return
  enum JobManagerErrorCode {
    JM_OK = 0,
    JM_HTTP_SERVER_ERR,       // The HTTP server returned something other than code 200.
                              // In cases where this can happen, there's proably a string
                              // with the exact message the server sent back.
    JM_CLEARTEXT_DISALLOWED,  // In cases where we're using HTTP Basic Authentication, we
                              // send the password in (obfuscated) cleartext.  As such,
                              // we have to insist on using HTTPS rather than HTTP.
    JM_LOCAL_FILE_ERROR,      // Problem writing to the local file (see downloadFile())
    JM_NOT_IMPLELEMTED        // The function hasn't been written (yet)

  };

  // The basic API: start/stop a transaction, upload/download files, submit/abort and
  // check the status of jobs
  virtual JobManagerErrorCode startTransaction( std::string &transId, std::string &directory, std::string &serverErr) = 0;
  virtual JobManagerErrorCode stopTransaction( std::string &transId, std::string &serverErr) = 0;

  // Returns a list of all the files on the remote machine associated with the specified transaction
  virtual JobManagerErrorCode listFiles( const std::string &transId,
                                         std::vector<std::string> &listing,
                                         std::string &serverErr) = 0;

  // Transfer files to/from the compute cluster.
  // Note: remoteFileName is just the file name (no path), but localFileName should include
  // the complete path
  virtual JobManagerErrorCode uploadFile( const std::string &transId,
                                          const std::string &remoteFileName,
                                          const std::string &localFileName,
                                          std::string &serverErr) = 0;
  virtual JobManagerErrorCode downloadFile( const std::string &transId,
                                            const std::string &remoteFileName,
                                            const std::string &localFileName,
                                            std::string &serverErr) = 0;

  // submit a job to the compute cluster
  virtual bool submitJob( const RemoteTask &remoteTask, std::string &retString) = 0;

  virtual bool jobStatus( const std::string &jobId,
                          RemoteJob::JobStatus &retStatus,
                          std::string &errMsg) = 0;
  virtual bool jobStatusAll( std::vector<RemoteJob> &jobList, std::string &errMsg) = 0;


/************
  TODO: Uncomment these when we're ready to implement
  virtual int abortJob( std::string jobId) = 0;
  virtual bool jobHeld( std::string jobId) = 0;
  virtual bool jobRunning( std::string jobId) = 0;
  virtual bool jobComplete( std::string jobId) = 0;
***********/

  const std::string & getDisplayName() const { return m_displayName; }
  const std::string & getConfigFileUrl() const { return m_configFileUrl; }

  // Returns the type of job manager it actually is (MWS, Globus, etc..)
  virtual const std::string getType() const = 0;

  void setUserName( const std::string &name) { m_userName = name; }
  void setPassword( const std::string &pwd) { m_password = pwd; }

protected:

  static Mantid::Kernel::Logger& g_log;   ///< reference to the logger class

  std::string m_displayName;      // This will show up in the list of configured clusters
  std::string m_configFileUrl;    // A URL for a file that describes the jobs that are available
                                  // on this particular cluster

  // Username and password for HTTP Basic Auth
  // NOTE: This is really an implementation detail and as such shouldn't reside up here
  // at the API level.  With the currently design, I don't have any way to avoid it,
  // though.  The best I can hope for is to fix this when I re-factor this code later.
  std::string m_userName;
  std::string m_password;

private:
  // Default constructor deliberately left unimplemented so it will cause a
  // compile error if anyone accidently tries to use it.  Currently, the default
  // copy constructor and assigment operator will work fine (though that's
  // subject to change).
  RemoteJobManager();

};

// Lots of HTTP-related stuff, including implementations for the
// transaction and file transfer functions (because these are all
// via HTTP and don't actually involve the particular job manager
// that we use.
class HttpRemoteJobManager : public RemoteJobManager
{
public:
  HttpRemoteJobManager( const Poco::XML::Element* elem);
   ~HttpRemoteJobManager() { }

  virtual JobManagerErrorCode startTransaction( std::string &transId, std::string &directory, std::string &serverErr);
  virtual JobManagerErrorCode stopTransaction( std::string &transId, std::string &serverErr);


  // Returns a list of all the files on the remote machine associated with the specified transaction
  virtual JobManagerErrorCode listFiles( const std::string &transId,
                                         std::vector<std::string> &listing,
                                         std::string &serverErr);

  // Transfer files to/from the compute cluster.
  // Note: remoteFileName is just the file name (no path), but localFileName should include
  // the complete path
  virtual JobManagerErrorCode uploadFile( const std::string &transId,
                                          const std::string &remoteFileName,
                                          const std::string &localFileName,
                                          std::string &serverErr);
  virtual JobManagerErrorCode downloadFile( const std::string &transId,
                                            const std::string &remoteFileName,
                                            const std::string &localFileName,
                                            std::string &serverErr);
protected:

  // Wraps up some of the boilerplate code needed to execute HTTP GET and POST requests
  JobManagerErrorCode initGetRequest( Poco::Net::HTTPRequest &req, std::string extraPath, std::string queryString)
  { return initHTTPRequest( req, Poco::Net::HTTPRequest::HTTP_GET, extraPath, queryString); }
  JobManagerErrorCode initPostRequest( Poco::Net::HTTPRequest &req, std::string extraPath)
  { return initHTTPRequest( req, Poco::Net::HTTPRequest::HTTP_POST, extraPath); }
  JobManagerErrorCode initHTTPRequest( Poco::Net::HTTPRequest &req, const std::string &method,
                                       std::string extraPath, std::string queryString="");


  std::string m_serviceBaseUrl; // What we're going to connect to.  The full URL will be
                                // built by appending a path (and possibly a query string)
                                // to this string.

  // Store any cookies that the HTTP server sends us so we can send them back
  // on future requests.  (In particular, the ORNL servers use session cookies
  // so we don't have to authenticate to the LDAP server on every single request.)
  //
  // NOTE: For reasons that are unclear, Poco's HTTPResponse class returns cookies
  // in a vector of HTTPCookie objects, but its HTTPRequest::setCookies() function
  // takes a NameValueCollection object, so we have to convert.  (WTF Poco devs?!?)
  std::vector<Poco::Net::HTTPCookie> m_cookies;
  Poco::Net::NameValueCollection getCookies();

};

class MwsRemoteJobManager : public HttpRemoteJobManager
{
public:

  MwsRemoteJobManager( const Poco::XML::Element* elem);
  ~MwsRemoteJobManager() { }

  // Returns the type of job manager it actually is (MWS, Globus, etc..)
  virtual const std::string getType() const { return "MWS"; }

  virtual bool submitJob( const RemoteTask &remoteTask, std::string &retString);
  virtual bool jobStatus( const std::string &jobId,
                          RemoteJob::JobStatus &retStatus,
                          std::string &errMsg);
  virtual bool jobStatusAll( std::vector<RemoteJob> &jobList, std::string &errMsg);

/*******
  TODO: IMPLEMENT THESE FUNCTIONS FOR REAL!
  virtual int abortJob( std::string jobId) { return 0; }
  virtual bool jobHeld( std::string jobId) { return false; }
  virtual bool jobRunning( std::string jobId) { return false; }
  virtual bool jobComplete( std::string jobId) { return false; }
*********/

protected:

  std::string escapeQuoteChars( const std::string & str); // puts a \ char in front of any " chars it finds
                                                          // (needed for the JSON stuff

  // Locations of the mpirun and python executables - specified in the Facilities.xml file
  std::string m_mpirunExecutable;
  std::string m_pythonExecutable;

private:
  bool convertToISO8601( std::string &time);

  std::map<std::string, std::string> m_tzOffset;  // Maps timezone abbreviations to their offsets
                                                  // (See the comments in the constructor and in
                                                  // convertToISO8601()
};


#endif // REMOTEJOBMANAGER_H
