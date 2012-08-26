#ifndef REMOTETASK_H
#define REMOTETASK_H

#include <string>
#include <vector>
#include <map>

/*
 * This class contains most of the info needed to submit a remote job (executable, command line param, etc..)
 * This data comes from the XML config file that is downloaded from the cluster.
 * Notably absent is any kind of username & password info.  Presumably the GUI will ask the user for that.
 */

class RemoteTask
{
public:

    // Defines the type of user parameter.  Only two real types at the moment.
    // UNKNOWN_TYPE is used for error and sanity checks
    enum ParamType { TEXT_BOX, CHOICE_BOX, UNKNOWN_TYPE };
    
    RemoteTask( const std::string &taskName = "", const std::string &executable = "") :
        m_executable( executable) { setName( taskName);}
    
    // At this point, the default copy constructor and assignment operator are all
    // valid and useful.  If that changes, we'll either need to explicitly implement them
    // or else declare them private.
    
    // Getter funcs for task name and executable
    const std::string & getName() const { return m_name; }
    const std::string & getExecutable() const { return m_executable; }
    
    // Queries the user for any user-supplied params and builds up the complete
    // list of command line paramters from user's responses and the list of 'fixed'
    // parameters.  Returns it all in a single string.
    std::string getCmdLineParams() const;

    // Retrieves the specified resource value.  Will check the user-supplied params
    // substitute the user-supplied value if required.
    std::string getResourceValue (const std::string &name) const;

    // sets the m_name member, replacing all whitespace with '_' chars.  (Moab, and
    // possibly other job managers, doesn't allow spaces in job names.)
    void setName( const std::string &name);

    void setExecutable( const std::string  &executable) { m_executable = executable; }
    void appendCmdLineParam( const std::string &param) { m_cmdLineParams.push_back( param); }
    void appendSubstitutionParam( const std::string &paramName, const std::string &paramId, ParamType paramType=TEXT_BOX, const std::string &choiceString = "")
    {
        m_substitutionParamIds.push_back( paramId);
        m_substitutionParamNames.push_back( paramName);

        // Note: we really should check to see that there's a valid choice string for any type CHOICE_BOX
        m_substitutionParamTypes.push_back(paramType);
        m_substitutionChoiceStrings.push_back(choiceString);

        // The 'real' value will be filled in by the user before the job is submitted
        m_substitutionParamValues.push_back( "");
    }
    
    bool setSubstitutionParamValue( unsigned paramNum, const std::string &value)
    // Note: paramNum is zero based.
    {
        if (paramNum >= m_substitutionParamNames.size())  // sanity check
            return false;
        
        m_substitutionParamValues[paramNum] = value;
        return true;
    }

    // Assign a value to the parameter with the specified ID
    bool setSubstitutionParamValue( const std::string &paramId, const std::string &value)
    {
      int i = getSubstitutionParamIndex( paramId);
      if (i >= 0)
      {
        return setSubstitutionParamValue( i, value);
      }

      // If we get here, it's because we never found the ID
      return false;
    }
    
    unsigned long getNumSubstitutionParams() const { return m_substitutionParamIds.size(); }
    
    std::string getSubstitutionParamName( unsigned i) const
    {
        if (i < getNumSubstitutionParams())
            return m_substitutionParamNames[i];
        
        return "";
    }
    
    std::string getSubstitutionParamValue( unsigned i) const
    {
        if (i < getNumSubstitutionParams())
            return m_substitutionParamValues[i];
        
        return "";
    }

    std::string getSubstitutionParamValue( const std::string &paramId) const
    {
      int i = getSubstitutionParamIndex( paramId);
      if (i >= 0)
        return getSubstitutionParamValue( i);

      return "";  // didn't find the parameter ID so return empty string
    }

    ParamType getSubstitutionParamType( unsigned i) const
    {
        if (i < getNumSubstitutionParams())
            return m_substitutionParamTypes[i];

        return UNKNOWN_TYPE;
    }

    std::string getSubstitutionChoiceString( unsigned i) const
    {
        if (i < getNumSubstitutionParams())
        {
            // Only return a string if the corresponding type is CHOICE_BOX
            if (getSubstitutionParamType(i) == CHOICE_BOX)
                return m_substitutionChoiceStrings[i];
        }

        return "";
    }
    
    void appendResource( const std::string &name, const std::string &value)
    {
        m_resources[name] = value;
    }
    
    bool isValid() const
    { 
        // sanity check...
        if (m_substitutionParamIds.size() != m_substitutionParamNames.size())
            return false;

        if (m_substitutionParamIds.size() != m_substitutionParamValues.size())
            return false;

        if (m_substitutionParamIds.size() != m_substitutionParamTypes.size())
            return false;

        if (m_substitutionParamIds.size() != m_substitutionChoiceStrings.size())
            return false;
        
        // The only things that are really necessary are the task name and the
        // executable name.  (MWS also requires the the number of nodes, but other job
        // managers might not. Perhaps we create an MWSRemoteTask subclass?)
        return (m_name.length() > 0 && m_executable.length() > 0);
    }
    
protected:

    // given a substution param ID, return its index in the vector
    // returns -1 if the ID isn't found
    int getSubstitutionParamIndex( const std::string & paramId) const
    {
      // This is a bit of a nuisance since we have to do a linear search.  I looked into
      // using a std::map instead of the vectors, but it actually makes other parts of the
      // code more complex, so we'll just put up with the occasional linear search...
      int i=0;
      while (i < (int)m_substitutionParamIds.size() && m_substitutionParamIds[i] != paramId)
      {
        i++;
      }
      if (i >= (int)m_substitutionParamIds.size())
      {
        i=-1;
      }
      return i;
    }

private:
    std::string m_name;         // The name of the task.  Is sent over to the cluster (which will probably
                                // use it for naming the files for stdout and stderr).
    std::string m_executable;  // The name of the program to run.  Probably something like /usr/bin/mpirun...
    std::vector<std::string> m_cmdLineParams;

    std::vector<ParamType>   m_substitutionParamTypes;
    std::vector<std::string> m_substitutionParamNames;
    std::vector<std::string> m_substitutionParamIds;
    std::vector<std::string> m_substitutionParamValues;
    std::vector<std::string> m_substitutionChoiceStrings;

    // Note that param names are, in fact, optional.  The "global" parameters (such as outfile)
    // are set by MantidPlot itself.  Since we don't ask the user for the value, these parameters
    // shouldn't be displayed in the dialog box and therefore don't need a name.  The 5 vectors do
    // all need to be the same size, though (so that there's no confusion about the <type,name,
    // id,value,choice string> tuple), but the name may be the empty string.
    //
    // m_substitionChoiceStrings are similar:  choices are only valid for params of type CHOICE_BOX,
    // but other types of params must have an empty string here so that the vector sizes all match.

    std::map<std::string, std::string> m_resources;  // Maps resource names to values
};


#endif
