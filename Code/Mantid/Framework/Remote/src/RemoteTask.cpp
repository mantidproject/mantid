#include "MantidRemote/RemoteTask.h"

#include <sstream>
#include <string>
using namespace std;


// sets the m_name member, replacing all whitespace with '_' chars.  (Moab, and
// possibly other job managers, doesn't allow spaces in job names.)
void RemoteTask::setName( const string &name)
{
  if (name.length() == 0)
    return;

  m_name = name;

  // replace every whitespace char with an underscore
  for (unsigned i=0; i < m_name.length(); i++)
  {
    if (isspace( m_name[i]))
      m_name[i] = '_';
  }
}

    
// Queries the user for any user-supplied params and builds up the complete
// list of command line paramters from user's responses and the list of 'fixed'
// parameters.  Returns it all in a single string.
string RemoteTask::getCmdLineParams() const
{

    ostringstream params;

    for (unsigned i = 0; i < m_cmdLineParams.size(); i++)
    {
        string param = m_cmdLineParams[i];
        // for each parameter, check to see if we need to substitute a user-supplied value instead...
        for (unsigned j = 0; j < m_substitutionParamIds.size(); j++)
        {
            string searchString = string("%") + m_substitutionParamIds[j] + string("%");
            size_t pos = param.find( searchString);
            if (pos != string::npos)
            {
                // Ahah! Substitute the user supplied value....
                string newParam = param.substr(0, pos) + m_substitutionParamValues[j] + param.substr(pos + searchString.length());
                param = newParam;
                break;  // break out of the j loop
            }
        }
        params << param << " ";
    }

    return params.str();
}
    

// Retrieves the specified resource value.  Will check the user-supplied params
// substitute the user-supplied value if required.
string RemoteTask::getResourceValue (const string &name) const
{
    // First, look up the value.
    // Note: We can't do this the easy way because map::operator[] is not const (see
    // the STL docs for the reason why) and I want this function to be const.
    string value;
    map<string,string>::const_iterator it = m_resources.find( name);
    if (it != m_resources.end())
    {
        value = (*it).second;
    }
    else
    {
        value = "";
    }

    // Check to see if the value actually exists...
    if (value.length() > 0)
    {
        // Check to see if we need to substitute a user-supplied value instead...
        for (unsigned i = 0; i < m_substitutionParamIds.size(); i++)
        {
            string searchString = string("%") + m_substitutionParamIds[i] + string("%");
            size_t pos = value.find( searchString);
            if (pos != string::npos)
            {
                // Ahah! Substitute the user supplied value....
                string newValue = value.substr(0, pos) + m_substitutionParamValues[i] + value.substr(pos + searchString.length());
                value = newValue;
                break;  // break out of the loop
            }
        }
    }

    return value;
}

