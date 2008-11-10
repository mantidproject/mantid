//-----------------------------------
//Includes
//-----------------------------------
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/IAlgorithm.h"

namespace Mantid
{

  namespace PythonAPI
  {

    /// Static filename variable
    std::string SimplePythonAPI::g_strFilename = "mantidsimple.py";

    //------------------------------
    //Public methods
    //------------------------------
    /**
     * Return the name of the Python module to be created
     * @returns A string containing the name of the module file
     */
    const std::string & SimplePythonAPI::getModuleName()
    {
      return g_strFilename;
    }

    /**
     * Create the python module with function definitions in the
     * file whose name is returned by getModule()
     */
    void SimplePythonAPI::createModule()
    {
      //open file
      std::ofstream module(getModuleName().c_str());

      // Need to import definitions from main Python API
#ifdef _WIN32
      module << "from MantidPythonAPI import FrameworkManager\n\n";
#else
      module << "from libMantidPythonAPI import FrameworkManager\n\n";
#endif

      //Algorithm keys
      using namespace Mantid::API;
      StringVector algKeys = AlgorithmFactory::Instance().getKeys();
      VersionMap vMap;
      createVersionMap(vMap, algKeys);
      writeGlobalHelp(module, vMap);
      //Function definitions for each algorithm
      IndexVector helpStrings;
      for( VersionMap::const_iterator vIter = vMap.begin(); vIter != vMap.end();
          ++vIter)
          {
            IAlgorithm* algm = FrameworkManager::Instance().createAlgorithm(vIter->first);
	    PropertyVector orderedProperties(algm->getProperties());
	    std::sort(orderedProperties.begin(), orderedProperties.end(),
		      SimplePythonAPI::PropertyOrdering());
	    std::string name(vIter->first);
            writeFunctionDef(module, name , orderedProperties);
	    helpStrings.push_back(make_pair(name, createHelpString(vIter->first, orderedProperties)));
          }
      writeFunctionHelp(module, helpStrings);
      module.close();
    }
    
    /**
     * Construct a map between an algorithm name and it's highest version
     * @param vMap A reference to the vMap
     * @param algKeys A list of strings representing the algorithms mangled with version numbers
     */
    void SimplePythonAPI::createVersionMap(VersionMap & vMap, const StringVector & algKeys)
    {
      for(StringVector::const_iterator sIter = algKeys.begin(); sIter != algKeys.end();
	  ++sIter)
	{
	  std::string name = extractAlgName(*sIter); 
	  VersionMap::iterator vIter = vMap.find(name);
	  if( vIter == vMap.end() ) vMap.insert(make_pair(name, 1));
	  else ++(vIter->second);
	}
    }

     /**
     * Extract the algorithm name from an algorithm key
     * @param name The algorithm key
     * @returns The name of the algorithm
     */
    std::string SimplePythonAPI::extractAlgName(const std::string & name)
    {
      std::string::size_type idx = name.find('|');
      if( idx != std::string::npos ) return name.substr(0, idx);
      else return name;
    }

    /**
     * Write a Python function defintion
     * @param os The stream to use to write the definition
     * @param algm The name of the algorithm
     * @param properties The list of properties
     */
    void SimplePythonAPI::writeFunctionDef(std::ostream & os, const std::string & algm,
    const PropertyVector & properties)
    {
      os << "# Definition of \"" << algm << "\" function.\n";
      //start of definition
      os << "def " << algm << "(";
      //parameter listing - ensuring the ones that are required are
      //first
      PropertyVector::const_iterator pIter = properties.begin();
      PropertyVector::const_iterator pEnd = properties.end();
      int cParam(97), iMand(0);
      for( ; pIter != pEnd; ++cParam )
      {
	if( !(*pIter)->isValid() ) {
	  ++iMand;
	  os << (char)cParam;
	}
	else {
	  os << (char)cParam << " = -1";
	}
        if( ++pIter != pEnd ) os << ", ";
      }
      //end of function parameters
      os << "):\n";
      os << "\talgm = FrameworkManager().createAlgorithm(\"" << algm << "\")\n";
      pIter = properties.begin();
      for( cParam = 97; pIter != pEnd; ++pIter, ++cParam )
      {
	if( cParam < 97 + iMand )
	  os << "\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << (char)cParam << ")\n";
	else {
	  os << "\tif " << (char)cParam << " != -1:\n"
	     << "\t\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << (char)cParam << ")\n";
	}
      }
      os << "\talgm.execute()\n";
      os << "\treturn algm\n";
      //Add space at end of definition
      os << "\n";
    }

    /**
     * Write a global help command called mtdHelp, which takes no arguments
     * @param os The stream to use to write the command
     * @param algms The names of the available algorithms
     */
    void SimplePythonAPI::writeGlobalHelp(std::ostream & os, const VersionMap & vMap)
    {
      os << "# The help command with no parameters\n";
      os << "def mtdGlobalHelp():\n";
      os << "\tprint \"The algorithms available are:\"\n";
      VersionMap::const_iterator vIter = vMap.begin();
      for( ; vIter != vMap.end(); ++vIter )
      {
 	if( vIter->second == 1 )
 	  os << "\tprint \"\\t" << vIter->first << "\"\n";
 	else {
	  os << "\tprint \"\\t" << vIter->first << " ";
	  int i = 0;
	  while( ++i <= vIter->second )
	    {
	      os << "v" << i << " ";
	    }
	  os << "\"\n";
	}
      }
      os << "\tprint \"For help with a specific command type: mtdHelp(\\\"cmd\\\")\"\n";
      os << "\n";
    }

    /**
     * Construct a  help command for a specific algorithm
     * @param algm The name of the algorithm
     * @param properties The list of properties
     */
    std::string SimplePythonAPI::createHelpString(const std::string & algm, const PropertyVector & properties)
    {
      std::ostringstream os;
      os << "\t\tprint \"Usage: " << algm << "(";
      PropertyVector::const_iterator pIter = properties.begin();
      PropertyVector::const_iterator pEnd = properties.end();
      for( ; pIter != pEnd ; )
      {
	os << (*pIter)->name();
	if( ++pIter != pEnd ) os << ", ";
	else os << ")\"\n";
      }
      os << "\t\tprint \"Argument description:\"\n";
      pIter = properties.begin();
      for( ; pIter != pEnd ; ++pIter )
      {
	Mantid::Kernel::Property* prop = *pIter;
	os << "\t\tprint \"\\tName: " << prop->name() << ", Optional: ";  
	if( prop->isValid() ) os << "Yes, Default value: " << santizePropertyValue(prop->value());
	else os << "No";
	os << ", Direction: " << Mantid::Kernel::Direction::asText(prop->direction()) << ", ";
	StringVector allowed = prop->allowedValues();
	if( !allowed.empty() )
	{
	  os << "Allowed values: ";
	  StringVector::const_iterator sIter = allowed.begin();
	  StringVector::const_iterator sEnd = allowed.end();
	  for( ; sIter != sEnd ; )
	  {
	    os << (*sIter);
	    if( ++sIter != sEnd ) os << ", ";
	  }
	}
	os << "\"\n";	
      }
      os << "\t\tprint \"Note: All arguments must be wrapped in string quotes \\\"\\\", regardless of their type.\"\n\n";
      return os.str();
    }

    /**
     * Write the help function that takes a command as an argument
     * @param os The stream to use for the output
     * @param helpStrings A map of function names to help strings
     */
    void SimplePythonAPI::writeFunctionHelp(std::ostream & os, const IndexVector & helpStrings)
    {
      os << "def mtdHelp(cmd = -1):\n";
      
      os << "\tif cmd == -1:\n"
	 << "\t\tmtdGlobalHelp()\n"
	 << "\t\treturn\n";
      //Functons help
      SimplePythonAPI::IndexVector::const_iterator mIter = helpStrings.begin();
      SimplePythonAPI::IndexVector::const_iterator mEnd = helpStrings.end();
      os << "\tif cmd == \"" << (*mIter).first << "\":\n" 
	 << (*mIter).second;
      while( ++mIter != mEnd )
      {
	os << "\telif cmd == \"" << (*mIter).first << "\":\n" 
	   << (*mIter).second;
      }
      os << "\telse:\n"
	 << "\t\tprint \"mtdHelp() - '\" + cmd + \"' not found in help list\"\n\n";
    }

    /**
     * Take a property value as a string and if only special characters are present, i.e.
     * \n or \n\r then replace them with their string represenations
     * @param value The property value
     * @returns A string containing the sanitized property value
     */
    std::string SimplePythonAPI::santizePropertyValue(const std::string & value)
    {
      if( value == "\n\r" )
	return std::string("\\\\") + std::string("n") + std::string("\\\\") + std::string("r");
      if( value == "\n" )
	return std::string("\\\\") + std::string("n");
      return value;
    }

  } //namespace PythonAPI

} //namespace Mantid

