#ifndef REMOTEJOBMANAGER_H
#define REMOTEJOBMANAGER_H

// Note: I'm deliberately avoiding all Qt classes here just in case this code needs to be used
// outside of the MantidPlot hierarchy.

#include "RemoteJob.h"
#include <string>
#include <ostream>
#include <vector>

#include <Poco/Net/HTTPCookie.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/NameValueCollection.h>

class RemoteJobManager;         // Top-level abstract class
class HttpRemoteJobManager;     // Mid-level abstract class. (Not sure we really need this.)
class MwsRemoteJobManager;      // Abstract class - communicates w/ Moab Web Services
class QtMwsRemoteJobManager;    // Concrete class - Uses a Qt dialog box to ask for the password for MWS
//class CondorRemoteJobManager; // Concrete class - communicates w/ Condor.  Doesn't exist & probably never will
//class GlobusRemoteJobManager; // Concrete class - communicates w/ Globus.  Doesn't exist yet, but the ISIS folks need it

class RemoteJobManagerFactory;  // Knows how to create the various concrete classes

class RemoteTask;               // Holds info about a particular remote task (executable name, cmd line params, etc...
                                // See comments in remotetask.h)


class RemoteJobManager
{
public:
    RemoteJobManager( std::string displayName, std::string configFileUrl) :
        m_displayName(displayName),
        m_configFileUrl( configFileUrl) { }

    virtual ~RemoteJobManager() { }

    // This is intended to be a single enum for all of the errors that any function
    // (public or private) can return
    enum JobManagerErrorCode {
      JM_OK = 0,
      JM_HTTP_SERVER_ERR,       // The HTTP server returned something other than code 200.
                                // In cases where this can happen, there's proably a string
                                // with the exact message the server sent back.
      JM_CLEARTEXT_DISALLOWED   // In cases where we're using HTTP Basic Authentication, we
                                // send the password in (obfuscated) cleartext.  As such,
                                // we have to insist on using HTTPS rather than HTTP.

    };

    // The basic API: start/stop a transaction, upload/download files, submit/abort and
    // check the status of jobs
    virtual JobManagerErrorCode startTransaction( std::string &transId, std::string &directory, std::string &serverErr) = 0;
    virtual JobManagerErrorCode stopTransaction( std::string &transId, std::string &serverErr) = 0;

    /*************
      TODO: Uncomment these when I'm ready to implement them

    // returns the ID's of all the user's open transactions
    virtual bool openTransactions(std::vector<std::string> &transIds) = 0;
    **********/

    // NOTE: I haven't decided on the function signatures for upload/download yet.
    // Probably ought to use std::istream and std::ostream....
    //virtual bool uploadFile( std::string & TransId, ???? ) = 0;
    //virtual bool downloadFile( std::string & TransId, ???? ) = 0;

    virtual bool submitJob( const RemoteTask &remoteTask, std::string &retString) = 0;
    virtual bool jobStatus( const std::string &jobId,
                            RemoteJob::JobStatus &retStatus,
                            std::string &errMsg) = 0;
    virtual bool jobStatusAll( std::vector<RemoteJob> &jobList, std::string &errMsg) = 0;

    // returns true if there's an output file associated with the specified job ID and the file is readable
    virtual bool jobOutputReady( const std::string &jobId) = 0;

    // Fetches the job's output file from the remote cluster
    virtual bool getJobOutput( const std::string &jobId, std::ostream &outstream) = 0;
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

    // Save the necessary properties so the factory class can re-create the object
    virtual void saveProperties( int itemNum);



protected:
    std::string m_displayName;      // This will show up in the list of configured clusters
    std::string m_configFileUrl;    // A URL for a file that describes the jobs that are available
                                    // on this particular cluster

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
// Note that getPassword() is new (it's not declared in RemoteJobManager)
// and is protected.  I expect the final child class to implement it
// in whatever makes sense (presumably using a Qt dialog box if we're
// in MantidPlot).
class HttpRemoteJobManager : public RemoteJobManager
{
public:
    HttpRemoteJobManager( std::string displayName, std::string configFileUrl,
                          std::string serviceBaseUrl, std::string userName) :
        RemoteJobManager( displayName, configFileUrl),
        m_userName( userName), m_serviceBaseUrl( serviceBaseUrl){ }
    ~HttpRemoteJobManager() { }

    virtual JobManagerErrorCode startTransaction( std::string &transId, std::string &directory, std::string &serverErr);
    virtual JobManagerErrorCode stopTransaction( std::string &transId, std::string &serverErr);


    virtual void saveProperties(int itemNum) { RemoteJobManager::saveProperties( itemNum); }


protected:
    // Wraps up some of the boilerplate code needed to execute an HTTP GET request
    JobManagerErrorCode initGetRequest( Poco::Net::HTTPRequest &req, std::string extraPath, std::string queryString);

    virtual bool getPassword() = 0; // This needs to be implemented in whatever way makes sense
                                    // for the environment where it's being used...

    // Username and password for HTTP Basic Auth
    std::string m_userName;
    std::string m_password; // This does  **NOT** get saved in the properties file.  It's merely
                            // a convenient place to hold the password in memory (and I don't
                            // even like doing that, but the alternative is for the user to enter
                            // it every time and that would be way too tedious).  I'm expecting
                            // the GUI to pop up a dialog box asking for it before it's needed.

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

/*
 * Note: MwsRemoteJobManager is abstract!  (getPassword is still a pure virtual
 * function).  I don't really like doing this, but I need to ask for a password
 * somehow.  In MantidPlot, the best way to do that is to use a Qt dialog box.
 * However, I really wanted to keep the Qt specific stuff separated.  (There's
 * been some talk about using MWS in other contexts where Qt may not be available.)
 * So, my solution is to have most of the implementation here, but create a child
 * class that implements the getPassword function.  If I ever need to use this
 * code someplace where Qt is unavailable, I'll have to create another child class
 * with a different implementation
 */
class MwsRemoteJobManager : public HttpRemoteJobManager
{
public:
    MwsRemoteJobManager( std::string displayName, std::string configFileUrl,
                         std::string serviceBaseUrl, std::string userName);
    ~MwsRemoteJobManager() { }

    // Returns the type of job manager it actually is (MWS, Globus, etc..)
    virtual const std::string getType() const { return "MWS"; }

    virtual bool submitJob( const RemoteTask &remoteTask, std::string &retString);
    virtual bool jobStatus( const std::string &jobId,
                            RemoteJob::JobStatus &retStatus,
                            std::string &errMsg);
    virtual bool jobStatusAll( std::vector<RemoteJob> &jobList, std::string &errMsg);
    virtual bool jobOutputReady( const std::string &jobId);
    virtual bool getJobOutput( const std::string &jobId, std::ostream &outstream);

/*******
    TODO: IMPLEMENT THESE FUNCTIONS FOR REAL!
    virtual int abortJob( std::string jobId) { return 0; }
    virtual bool jobHeld( std::string jobId) { return false; }
    virtual bool jobRunning( std::string jobId) { return false; }
    virtual bool jobComplete( std::string jobId) { return false; }
*********/
    virtual void saveProperties( int itemNum);

protected:


    std::string escapeQuoteChars( const std::string & str); // puts a \ char in front of any " chars it finds
                                                            // (needed for the JSON stuff

private:
    bool convertToISO8601( std::string &time);
    static Mantid::Kernel::Logger& g_log;   ///< reference to the logger class

    std::map<std::string, std::string> m_tzOffset;  // Maps timezone abbreviations to their offsets
                                                    // (See the comments in the constructor and in
                                                    // convertToISO8601()
};


class QtMwsRemoteJobManager : public MwsRemoteJobManager
{
public:
  QtMwsRemoteJobManager( std::string displayName, std::string configFileUrl,
                       std::string serviceBaseUrl, std::string userName) :
    MwsRemoteJobManager( displayName, configFileUrl, serviceBaseUrl, userName) { }

  ~QtMwsRemoteJobManager() { }

protected:
  virtual bool getPassword(); // Will pop up a Qt dialog box to request a password
};


class RemoteJobManagerFactory
{
public:
    static RemoteJobManager *createFromProperties( int itemNum);

private:
    static MwsRemoteJobManager *createQtMwsManager( int itemNum);

};






#endif // REMOTEJOBMANAGER_H
