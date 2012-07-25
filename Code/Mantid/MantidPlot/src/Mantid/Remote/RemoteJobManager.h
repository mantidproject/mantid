#ifndef REMOTEJOBMANAGER_H
#define REMOTEJOBMANAGER_H

// Note: I'm deliberately avoiding all Qt classes here just in case this code needs to be used
// outside of the MantidPlot hierarchy.

#include "RemoteJob.h"
#include <string>
#include <vector>

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

    // The basic API: submit a job, abort a job and check on the status of a job
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

class HttpRemoteJobManager : public RemoteJobManager
{
    // Currently, we don't actually need anything here.
    // Makes me wonder if we need this class at all...
public:
    HttpRemoteJobManager( std::string displayName, std::string configFileUrl) :
        RemoteJobManager( displayName, configFileUrl) { }
    ~HttpRemoteJobManager() { }

    virtual void saveProperties(int itemNum) { RemoteJobManager::saveProperties( itemNum); }

};

/*
 * Note: MwsRemoteJobManager is abstract!  (getPassword is a pure virtual function).
 * I don't really like doing this, but I need to ask for a password somehow.  In
 * MantidPlot, the best way to do that is to use a Qt dialog box.  However,
 * I really wanted to keep the Qt specific stuff separated.  (There's been some talk
 * about using MWS in other contexts where Qt may not be available.)
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

/*******
    TODO: IMPLEMENT THESE FUNCTIONS FOR REAL!
    virtual int abortJob( std::string jobId) { return 0; }
    virtual bool jobHeld( std::string jobId) { return false; }
    virtual bool jobRunning( std::string jobId) { return false; }
    virtual bool jobComplete( std::string jobId) { return false; }
*********/
    virtual void saveProperties( int itemNum);

protected:
    virtual bool getPassword() = 0; // This needs to be implemented in whatever way makes sense
                                    // for the environment where it's being used...

    std::string escapeQuoteChars( const std::string & str); // puts a \ char in front of any " chars it finds
                                                            // (needed for the JSON stuff)
    std::string m_password; // This does  **NOT** get saved in the properties file.  It's merely
                            // a convenient place to hold the password in memory (and I don't
                            // even like doing that, but the alternative is for the user to enter
                            // it every time and that would be way too tedious).  I'm expecting
                            // the GUI to pop up a dialog box asking for it before it's needed.

    std::string m_serviceBaseUrl;
    std::string m_userName;

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
