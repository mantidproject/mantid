#ifndef REMOTETASK_H
#define REMOTETASK_H

#include <string>
#include <vector>
#include <map>

/*
 * This class contains most of the info needed to submit a remote job (executable, command line param, etc..)
 * The calling algorithm is expected to set the values prior to calling RemoteJobManager::submitJob.
 * Notably absent is any kind of username & password info.  Presumably the GUI will ask the user for that.
 * This class is mainly a convenince so that we can pass this one object to submitJob instead of multiple strings.
 */

class RemoteTask
{
public:

  RemoteTask( const std::string &taskName = "", const std::string &transId = "") :
      m_transactionId( transId) { setName( taskName);}
    
    // At this point, the default copy constructor and assignment operator are all
    // valid and useful.  If that changes, we'll either need to explicitly implement them
    // or else declare them private.
    
    // Getter funcs for task name, executable, and transaction ID
    const std::string & getName() const { return m_name; }
    const std::string & getTransactionId() const { return m_transactionId; }
    
    // Builds up a string of all the command line parameters and returns it
    std::string getCmdLineParams() const;

    /* Resources are name/value pairs.  MWS uses them to control the job (such as
     * the number of nodes it needs to reserve).  These are separate from the command
     * lime params.
     *
     * Note that this is somewhat specific to MWS.
     */
    // Retrieves the specified resource value. Returns an empty string if the named resource
    // doesn't exist
    std::string getResourceValue (const std::string &name) const;

    // sets the m_name member, replacing all whitespace with '_' chars.  (Moab, and
    // possibly other job managers, doesn't allow spaces in job names.)
    void setName( const std::string &name);
    void setTransactionId( const std::string &transId) { m_transactionId = transId; }

    void appendCmdLineParam( const std::string &param) { m_cmdLineParams.push_back( param); }

    
    void appendResource( const std::string &name, const std::string &value)
    {
        m_resources[name] = value;
    }
    
    bool isValid() const
    {       
        // The only things that are really necessary are the task name, and
        // the transaction ID.  (MWS also requires the number of nodes, but
        // other job managers might not. Perhaps we create
        // an MWSRemoteTask subclass?)
        return (m_name.length() > 0 &&
                m_transactionId.length() > 0);
    }
    

private:
    std::string m_name;         // The name of the task.  Is sent over to the cluster (which will probably
                                // use it for naming the files for stdout and stderr).
    std::vector<std::string> m_cmdLineParams;

    std::string m_transactionId;  // The transaction that this task is associated with

    std::map<std::string, std::string> m_resources;  // Maps resource names to values
};


#endif
