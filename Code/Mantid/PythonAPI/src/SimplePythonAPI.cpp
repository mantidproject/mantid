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
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"

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
     * @param gui If this is true create the necessary framework to use the dialog
     * boxes in qtiplot
     */
    void SimplePythonAPI::createModule(bool gui)
    {
      //open file
      std::ofstream module(getModuleName().c_str());

      // Need to import definitions from main Python API
#ifdef _WIN32
      module << "from MantidPythonAPI import FrameworkManager\n";
#else
      module << "from libMantidPythonAPI import FrameworkManager\n";
#endif

      //If in gui mode also need sys and qti module
      if( gui )
      {
	module << "import sys\n";
	module << "import qti\n";
      }
      //Need string module regardless
      module << "import string\n\n";

      //In GUI need to define a global variable to tell if we are using it or not
      if( gui )	module << "PYTHONAPIINMANTIDPLOT = True\n\n";
      else module << "PYTHONAPIINMANTIDPLOT = False\n\n";
		  
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
	Algorithm_sptr algm = AlgorithmManager::Instance().createUnmanaged(vIter->first);
	algm->initialize();
	PropertyVector orderedProperties(algm->getProperties());
	std::sort(orderedProperties.begin(), orderedProperties.end(), SimplePythonAPI::PropertyOrdering());
	std::string name(vIter->first);
	writeFunctionDef(module, name , orderedProperties, false);
	if( gui ) writeFunctionDef(module, name , orderedProperties, true);
	std::transform(name.begin(), name.end(), name.begin(), tolower);
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
     * @param gui If this is true create the necessary framework to use the dialog
     * boxes in qtiplot
     */
    void SimplePythonAPI::writeFunctionDef(std::ostream & os, const std::string & algm,
					   const PropertyVector & properties, bool gui)
    {
      os << "# Definition of \"" << algm << "\" function.\n";
      //start of definition
      os << "def " << algm;
      if( gui ) os << "Dialog";
      os << "(";
      //Iterate through properties
      PropertyVector::const_iterator pIter = properties.begin();
      PropertyVector::const_iterator pEnd = properties.end();
      StringVector sanitizedNames(properties.size());
      unsigned int iMand(0), iarg(0);
      for( ; pIter != pEnd; ++iarg)
      {
	sanitizedNames[iarg] = sanitizePropertyName((*pIter)->name());
	os << sanitizedNames[iarg];

	//For gui mode, set all properties as optional
	if( gui )
	{
	  os << " = -1";
	}
	else
	{
	  if( !(*pIter)->isValid() ) ++iMand;
	  else os  << " = -1";
	}
        if( ++pIter != pEnd ) os << ", ";
      }
      //end of function parameters
      os << "):\n";
      os << "\talgm = FrameworkManager().createAlgorithm(\"" << algm << "\")\n";

      if( gui )
      os << "\tnset = 0\n";

      pIter = properties.begin();
      iarg = 0;
      for( ; pIter != pEnd; ++pIter, ++iarg )
      {

	if( !gui && iarg < iMand )
	  os << "\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << sanitizedNames[iarg] << ")\n";
	else {
	  os << "\tif " << sanitizedNames[iarg] << " != -1:\n"
	     << "\t\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << sanitizedNames[iarg] << ")\n";
	  if( gui )  os << "\t\tnset += 1\n";

	}
      }

      if( gui )
      {
	os << "\tif nset != " << iarg + 1 << ":\n"
	   << "\t\tdialog = qti.app.mantidUI.createPropertyInputDialog(\"" << algm << "\")\n"
	   << "\tif dialog == True:\n"
	   << "\t\tresult = qti.app.mantidUI.runAlgorithmAsynchronously(\"" << algm << "\")\n"
	   << "\telse:\n"
	   << "\t\tsys.exit(1)\n"
	   << "\tif result == False:\n"
	   << "\t\tsys.exit(1)\n"
	   << "\treturn algm\n";
      }
      else
      {
	os << "\tif PYTHONAPIINMANTIDPLOT == True:\n"
	   << "\t\tresult = qti.app.mantidUI.runAlgorithmAsynchronously(\"" << algm << "\")\n"
	   << "\t\tif result == False:\n"
	   << "\t\t\tsys.exit(1)\n"
	   << "\telse:\n"
	   << "\t\talgm.execute()\n"
	   << "\treturn algm\n";
      }
      //Add space at end of definition
      os << "\n";
    }

    /**
     * Write a global help command called mtdHelp, which takes no arguments
     * @param os The stream to use to write the command
     * @param vMap The map of algorithm names to highest version numbers
     */
    void SimplePythonAPI::writeGlobalHelp(std::ostream & os, const VersionMap & vMap)
    {
      os << "# The help command with no parameters\n";
      os << "def mtdGlobalHelp():\n";
      os << "\thelpmsg =  \"The algorithms available are:\\n\"\n";
      VersionMap::const_iterator vIter = vMap.begin();
      for( ; vIter != vMap.end(); ++vIter )
      {
 	if( vIter->second == 1 )
 	  os << "\thelpmsg += \"\\t" << vIter->first << "\\n\"\n"; 
 	else {
	  os << "\thelpmsg += \"\\t" << vIter->first << " "; 
	  int i = 0;
	  while( ++i <= vIter->second )
	    {
	      os << "v" << i << " ";
	    }
	  os << "\\n\"\n";
	}
      }
      os << "\thelpmsg += \"For help with a specific command type: mtdHelp(\\\"cmd\\\")\\n\"\n";
      os << "\thelpmsg += \"Note: Each command also has a counterpart with the word 'Dialog'"
	 << " appended to it, which when run will bring up a property input dialog for that algorithm.\\n\"\n";
      os << "\tprint helpmsg,\n";
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
      os << "\t\thelpmsg =  \"Usage: " << algm << "(";
      PropertyVector::const_iterator pIter = properties.begin();
      PropertyVector::const_iterator pEnd = properties.end();
      for( ; pIter != pEnd ; )
      {
	os << sanitizePropertyName((*pIter)->name());
	if( ++pIter != pEnd ) os << ", ";
      }
      os << ")\\n\"\n";
      os << "\t\thelpmsg += \"Argument description:\\n\"\n";
      pIter = properties.begin();
      for( ; pIter != pEnd ; ++pIter )
      {
	Mantid::Kernel::Property* prop = *pIter;
	os << "\t\thelpmsg += \"\\tName: " << sanitizePropertyName((*pIter)->name()) << ", Optional: ";  
	if( prop->isValid() ) os << "Yes, Default value: " << sanitizePropertyValue(prop->value());
	else os << "No";
	os << ", Direction: " << Mantid::Kernel::Direction::asText(prop->direction());// << ", ";
	StringVector allowed = prop->allowedValues();
	if( !allowed.empty() )
	{
	  os << ", Allowed values: ";
	  StringVector::const_iterator sIter = allowed.begin();
	  StringVector::const_iterator sEnd = allowed.end();
	  for( ; sIter != sEnd ; )
	  {
	    os << (*sIter);
	    if( ++sIter != sEnd ) os << ", ";
	  }
	}
	os << "\\n\"\n";	
      }
      os << "\t\thelpmsg += \"Note 1: All arguments must be wrapped in string quotes  \\\"\\\", regardless of their type.\\n\"\n";
      os << "\t\thelpmsg += \"Note 2: To include a particular optional argument, it should be given after the mandatory arguments in the form argumentname=\\\"value\\\".\\n\"\n";
      os << "\t\tprint helpmsg,\n\n";
      return os.str();
    }

    /**
     * Write the help function that takes a command as an argument
     * @param os The stream to use for the output
     * @param helpStrings A map of function names to help strings
     */
    void SimplePythonAPI::writeFunctionHelp(std::ostream & os, const IndexVector & helpStrings)
    {
      if ( helpStrings.empty() ) return;

      os << "def mtdHelp(cmd = -1):\n";
      
      os << "\tif cmd == -1:\n"
	 << "\t\tmtdGlobalHelp()\n"
	 << "\t\treturn\n";
      os << "\n\tcmd = string.lower(cmd)\n";
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
     * EOL characters then replace them with their string represenations
     * @param value The property value
     * @returns A string containing the sanitized property value
     */
    std::string SimplePythonAPI::sanitizePropertyValue(const std::string & value)
    {
      if( value == "\n\r" )
	return std::string("\\\\") + std::string("n") + std::string("\\\\") + std::string("r");
      if( value == "\n" )
	return std::string("\\\\") + std::string("n");
      return value;
    }
    
    /**
     * Takes a list of properties and makes a list of names that can be used as parameters
     * by removing special characters such as spaces etc
     * @param name The full property name
     * @returns The sanitized property names
     */
    std::string SimplePythonAPI::sanitizePropertyName(const std::string & name)
    {
      std::string arg;
      std::string::const_iterator sIter = name.begin();
      std::string::const_iterator sEnd = name.end();
      for( ; sIter != sEnd; ++sIter )
      {
	int letter = (int)(*sIter);
	if( (letter >= 48 && letter <= 57) || (letter >= 97 && letter <= 122) ||
	    (letter >= 65 && letter <= 90) )
	{
	  arg.push_back(*sIter);
	}
      }
      return arg;
    }

  } //namespace PythonAPI

} //namespace Mantid

