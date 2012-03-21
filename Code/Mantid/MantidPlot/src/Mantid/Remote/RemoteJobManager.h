#ifndef REMOTEJOBMANAGER_H
#define REMOTEJOBMANAGER_H

// Note: I'm deliberately avoiding all Qt classes here just in case this code needs to be used
// outside of the MantidPlot hierarchy.

#include <string>

class RemoteJobManager;         // Top-level abstract class
class HttpRemoteJobManager;     // Mid-level abstract class. (Not sure we really need this.)
class MwsRemoteJobManager;      // Concrete class - communicates w/ Moab Web Services
//class CondorRemoteJobManager; // Concrete class - communicates w/ Condor.  Doesn't exist & probably never will
//class GlobusRemoteJobManager; // Concrete class - communicates w/ Globus.  Doesn't exist yet, but the ISIS folks need it

class RemoteJobManagerFactory;  // Knows how to create the various concrete classes

class RemoteAlg;                // Holds info about a particular remote algorithm (executable name, cmd line params, etc...)


class RemoteJobManager
{
public:
    RemoteJobManager( std::string displayName, std::string configFileUrl) :
        m_displayName(displayName),
        m_configFileUrl( configFileUrl) { }

    virtual ~RemoteJobManager() { }

    // The basic API: submit a job, abort a job and check on the status of a job
    virtual bool submitJob( const RemoteAlg &remoteAlg, std::string &retString) = 0;
    virtual int abortJob( std::string jobId) = 0;
    virtual bool jobHeld( std::string jobId) = 0;
    virtual bool jobRunning( std::string jobId) = 0;
    virtual bool jobComplete( std::string jobId) = 0;
    
    // This is provided as a standardized means of allowing the user to
    // enter authentication info.  For the MwsRemoteJobManager, I expect it
    // to pop up a dialog box asking for user name and password.  For Globus,
    // it'll probably ask for a certificate...
    //
    // NOTE: Actually - I'm not sure about this.  I really want to separate the UI from
    // the implementation of these classes....
    virtual void setAuthInfo() = 0;

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

class MwsRemoteJobManager : public HttpRemoteJobManager
{
public:
    MwsRemoteJobManager( std::string displayName, std::string configFileUrl,
                         std::string serviceBaseUrl, std::string userName) :
        HttpRemoteJobManager( displayName, configFileUrl),
        m_serviceBaseUrl( serviceBaseUrl),
        m_userName( userName) { }
    ~MwsRemoteJobManager() { }

    // Returns the type of job manager it actually is (MWS, Globus, etc..)
    virtual const std::string getType() const { return "MWS"; }

    virtual bool submitJob( const RemoteAlg &remoteAlg, std::string &retString);
    // TODO: IMPLEMENT THESE FUNCTIONS FOR REAL!
    virtual int abortJob( std::string jobId) { return 0; }
    virtual bool jobHeld( std::string jobId) { return false; }
    virtual bool jobRunning( std::string jobId) { return false; }
    virtual bool jobComplete( std::string jobId) { return false; }
    virtual void saveProperties( int itemNum);
    virtual void setAuthInfo()
    {
        // TODO: Implement this for real!
        return;
    }

private:
    std::string m_serviceBaseUrl;
    std::string m_userName;
    std::string m_password; // This does  **NOT** get saved in the properties file.  It's merely
                            // a convenient place to hold the password in memory (and I don't
                            // even like doing that, but the alternative is for the user to enter
                            // it every time and that would be way too tedious).  I'm expecting
                            // the GUI to pop up a dialog box asking for it before it's needed.

    std::string m_json; // This will hold the complete json text for the job submission.
                        // It exists merely for debugging purposes so I can look at the output
                        // in a debugger....


};

class RemoteJobManagerFactory
{
public:
    static RemoteJobManager *createFromProperties( int itemNum);

private:
    static MwsRemoteJobManager *createMwsManager( int itemNum);

};






#endif // REMOTEJOBMANAGER_H
