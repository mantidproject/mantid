#ifndef REMOTEALG_H
#define REMOTEALG_H

/*
 * This class contains most of the info needed to submit a remote job (executable, command line param, etc..)
 * This data comes from the XML config file that is downloaded from the cluster.
 * Notably absent is any kind of username & password info.  Presumably the GUI will ask the user for that.
 */

#include <string>
#include <vector>
#include <map>
class RemoteAlg
{
public:
    
    RemoteAlg( std::string executable = "") : m_executable( executable) { }
    
    // At this point, the default copy constructor and assignment operator are all
    // valid and useful.  If that changes, we'll either need to explicitly implement them
    // or else declare them private.
    
    const std::string & getExecutable() const { return m_executable; }
    
    // Queries the user for any user-supplied params and builds up the complete
    // list of command line paramters from user's responses and the list of 'fixed'
    // parameters.  Returns it all in a single string.
    std::string getCmdLineParams() const;

    // Retrieves the specified resource value.  Will check the user-supplied params
    // substitute the user-supplied value if required.
    std::string getResourceValue (const std::string &name) const;

    void setExecutable( const std::string  &executable) { m_executable = executable; }
    void appendCmdLineParam( const std::string &param) { m_cmdLineParams.push_back( param); }
    void appendUserSuppliedParam( const std::string &paramName, const std::string &paramId)
    {
        m_userSuppliedParamIds.push_back( paramId);
        m_userSuppliedParamNames.push_back( paramName);

        // The 'real' value will be filled in by the user before the job is submitted
        m_userSuppliedParamValues.push_back( "");
    }
    
    bool setUserSuppliedParamValue( unsigned paramNum, const std::string &value)
    // Note: paramNum is zero based.
    {
        if (paramNum >= m_userSuppliedParamNames.size())  // sanity check
            return false;
        
        m_userSuppliedParamValues[paramNum] = value;
        return true;
    }
    
    unsigned long getNumUserSuppliedParams() const { return m_userSuppliedParamIds.size(); }
    
    std::string getUserSuppliedParamName( unsigned i)
    {
        if (i < getNumUserSuppliedParams())
            return m_userSuppliedParamNames[i];
        
        return "";
    }
    
    std::string getUserSuppliedParamValue( unsigned i)
    {
        if (i < getNumUserSuppliedParams())
            return m_userSuppliedParamValues[i];
        
        return "";
    }
    
    void appendResource( const std::string &name, const std::string &value)
    {
        m_resources[name] = value;
    }
    
    bool isValid() const
    { 
        // sanity check...
        if (m_userSuppliedParamIds.size() != m_userSuppliedParamNames.size())
            return false;
        
        // Note: We're deliberately NOT checking m_userSuppliedParamValues.  Those
        // strings get filled in just before the job is submitted
        
        // The only thing that's really necessary is the executable name.
        // (MWS also requires the the number of nodes, but other job managers might not.
        // Perhaps we create an MWSRemoteAlg subclass?)
        return (m_executable.length() > 0);
    }
    
private:
    std::string m_executable;  // The name of the program to run.  Probably something like /usr/bin/mpirun...
    std::vector<std::string> m_cmdLineParams;
    std::vector<std::string> m_userSuppliedParamNames;
    std::vector<std::string> m_userSuppliedParamIds;
    std::vector<std::string> m_userSuppliedParamValues;

    std::map<std::string, std::string> m_resources;  // Maps resource names to values
};


#endif
