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
#include "MantidKernel/ConfigService.h"
#include "Poco/DirectoryIterator.h"
#include "Poco/Path.h"
#include "Poco/File.h"

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
    std::string SimplePythonAPI::getModuleName()
    {
      return Poco::Path(Mantid::Kernel::ConfigService::Instance().getOutputDir()).append(Poco::Path(g_strFilename)).toString(); 
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
      std::string filepath = getModuleName();	
      std::ofstream module(filepath.c_str());
      // Append the current directory and the scripts directory to the path to make importing
      // other Mantid things easier
      module << 
	"import sys\n"
	"if '.' in sys.path == False:\n"
	"\tsys.path.append('.')\n";

      std::string scripts_dir = Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory");
      //If there isn't one assume one relative to the executable directory
      if( scripts_dir.empty() ) 
      {
	scripts_dir = Mantid::Kernel::ConfigService::Instance().getBaseDir() + "../scripts";
      }
      Poco::File attr_test(scripts_dir);
      if( attr_test.exists() && attr_test.isDirectory() )
      {
	//Also append scripts directory and all sub directories
	module << "sys.path.append('" << convertPathToUnix(scripts_dir) << "')\n";
	Poco::DirectoryIterator iend;
	for( Poco::DirectoryIterator itr(scripts_dir); itr != iend; ++itr )
	{
	  Poco::File entry(itr->path());
	  bool is_dir(false);
	  // This avoids an error where the directory iterator gives a path to a file that Poco::File can't actually access.
	  // This happened when I had emacs open with an unsaved file which creates a hidden symbolic link to a strange location!
	  // that Poco can't seem to handle. - M. Gigg
	  try
	  {
	    is_dir = entry.isDirectory();
	  }
	  catch( Poco::FileNotFoundException &e)
	  {
	    continue;
	  }
	  if( is_dir )
	  {
	    module << "sys.path.append('" << convertPathToUnix(itr->path()) << "')\n";
	  }
	}
      }
      
      // Need to import definitions from main Python API
#ifdef _WIN32
      module << "from MantidPythonAPI import *\n";
#else
      module << "from libMantidPythonAPI import *\n";
#endif

      //If in gui mode also need sys and qti module
      if( gui )
      {
	module << "import qti\n";
      }
      //Need string and os module regardless
      module << "import os\n";
      module << "import string\n\n";

      //In GUI need to define a global variable to tell if we are using it or not
      if( gui )	module << "PYTHONAPIINMANTIDPLOT = True\n\n";
      else module << "PYTHONAPIINMANTIDPLOT = False\n\n";

      //Make the FrameworkManager object available with case variations
      module << "# The main API object\n"
	     << "mantid = FrameworkManager()\n"
	     << "# Aliases\n"
	     << "Mantid = mantid\n"
	     << "mtd = mantid\n"
	     << "Mtd = mantid\n\n";

      //First a simple function to change the working directory
      module << "# A wrapper for changing the directory\n"
	     << "def setWorkingDirectory(path):\n"
	     << "\tos.chdir(path)\n\n";
		  
      //Algorithm keys
      using namespace Mantid::API;
      //Ensure that a FrameworkManager object has been instantiated to initialise the framework
      FrameworkManager::Instance();
      StringVector algKeys = AlgorithmFactory::Instance().getKeys();
      VersionMap vMap;
      createVersionMap(vMap, algKeys);
      writeGlobalHelp(module, vMap);
      //Function definitions for each algorithm
      IndexVector helpStrings;
      for( VersionMap::const_iterator vIter = vMap.begin(); vIter != vMap.end();
	   ++vIter)
      {
	IAlgorithm_sptr algm = AlgorithmManager::Instance().createUnmanaged(vIter->first);
	algm->initialize();
	PropertyVector orderedProperties(algm->getProperties());
	std::sort(orderedProperties.begin(), orderedProperties.end(), SimplePythonAPI::PropertyOrdering());
	std::string name(vIter->first);
	writeFunctionDef(module, name , orderedProperties, false);
	if( gui ) writeFunctionDef(module, name , orderedProperties, true);
	std::transform(name.begin(), name.end(), name.begin(), tolower);
	helpStrings.push_back(make_pair(name, createHelpString(vIter->first, orderedProperties, false)));
	//The help for the dialog functions if necessary
	if( gui )
	{
	  helpStrings.push_back(make_pair(name + "dialog", createHelpString(vIter->first, orderedProperties, true)));
	}
      }
      //First function without dialog
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
	sanitizedNames[iarg] = removeCharacters((*pIter)->name(), "");
	os << sanitizedNames[iarg];

	//For gui mode, set all properties as optional
	if( gui )
	{
	  os << " = -1,";
	  ++pIter;
	}
	else
	{
	  //properties are optional unless their current value results in an error (isValid != ""
	  if( (*pIter)->isValid() != "" ) ++iMand;
	  else os  << " = -1";
	  if( ++pIter != pEnd ) os << ", ";
	}
      }
      //end of function parameters
      if( gui )
      {
	os << "message = \"\"):\n";
      }
      else
      {
	os << "):\n";
      }
      os << "\talgm = FrameworkManager().createAlgorithm(\"" << algm << "\")\n";

      if( gui ) os << "\tvalues = ''\n";

      pIter = properties.begin();
      iarg = 0;
      for( ; pIter != pEnd; ++pIter, ++iarg )
      {
	std::string pvalue = sanitizedNames[iarg];
	if( gui )
	{
	  os << "\tif " << pvalue << " != -1:\n"
	     << "\t\tvalues += '" << (*pIter)->name() << "=' + " << pvalue << " + '|'\n";
	}
	else
	{
	  if( iarg < iMand )
	  {
	    os << "\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << pvalue << ".lstrip('? '))\n";
	  }
	  else
	  {
	    os << "\tif " << pvalue << " != -1:\n"
	       << "\t\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << pvalue << ".lstrip('? '))\n";
	  }
	}
// 	if( !gui && iarg < iMand )
// 	  os << "\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << sanitizedNames[iarg] << ")\n";
// 	else {
// 	  os << "\tif " << sanitizedNames[iarg] << " != -1:\n"
// 	     << "\t\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << sanitizedNames[iarg] << ")\n";
      }


      if( gui )
      {
	os << "\tdialog = qti.app.mantidUI.createPropertyInputDialog(\"" << algm << "\" , message, values)\n"
	   << "\tif dialog == True:\n"
	   << "\t\tresult = qti.app.mantidUI.runAlgorithmAsynchronously(\"" << algm << "\")\n"
	   << "\telse:\n"
	   << "\t\tsys.exit('Information: Script execution cancelled')\n"
	   << "\tif result == False:\n"
	   << "\t\tsys.exit('An error occurred while running " << algm << "')\n"
	   << "\treturn algm\n";
      }
      else
      {
	os << "\tif PYTHONAPIINMANTIDPLOT == True:\n"
	   << "\t\tresult = qti.app.mantidUI.runAlgorithmAsynchronously(\"" << algm << "\")\n"
	   << "\t\tif result == False:\n"
	   << "\t\t\tsys.exit('An error occurred while running " << algm << "')\n"
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
      os << "\thelpmsg += \"For help with a specific command type: mantidHelp(\\\"cmd\\\")\\n\"\n"
       << "\tif PYTHONAPIINMANTIDPLOT == True:\n"
       << "\t\thelpmsg += \"Note: Each command also has a counterpart with the word 'Dialog'"
       << " appended to it, which when run will bring up a property input dialog for that algorithm.\\n\"\n"
       << "\tprint helpmsg,\n"
       << "\n";
    }

    /**
     * Construct a  help command for a specific algorithm
     * @param algm The name of the algorithm
     * @param properties The list of properties
     * @param dialog A boolean indicating whether this is a dialog function or not
	 * @return A help string for users
     */
    std::string SimplePythonAPI::createHelpString(const std::string & algm, const PropertyVector & properties, bool dialog)
    {
      std::ostringstream os;
      os << "\t\thelpmsg =  '" << algm;
      if( dialog ) os << "Dialog(";
      else os << "(";
      PropertyVector::const_iterator pIter = properties.begin();
      PropertyVector::const_iterator pEnd = properties.end();
      for( ; pIter != pEnd ; )
      {
	// Keep only alpha numeric characters
	os << removeCharacters((*pIter)->name(), "");
	if( ++pIter != pEnd ) os << ", ";
      }
      if( dialog ) os << ", message)\\n'\n";
      else os << ")\\n'\n";

      os << "\t\thelpmsg += 'Argument description:\\n'\n";
      std::string example(algm + std::string("("));
      pIter = properties.begin();
      for( ; pIter != pEnd ; ++pIter )
      {
	Mantid::Kernel::Property* prop = *pIter;
	std::string pname = removeCharacters((*pIter)->name(), "");
	os << "\t\thelpmsg += '     Name: " << pname << ", Optional: ";  
	if( prop->isValid() == "")
	{
	  os << "Yes, Default value: " << convertEOLToString(prop->value());
	}
	else 
	{
	  example += pname + std::string("=") + "\"param_value\",";
	  os << "No";
	}
	os << ", Direction: " << Mantid::Kernel::Direction::asText(prop->direction());
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
	os << ". Description: " << removeCharacters(prop->documentation(),"\n\r", true) << "\\n'\n";
      }
      if( dialog )
      {      
	os << "\t\thelpmsg += '     Name: message, Optional: Yes, Default value: '', Direction: Input\\n'\n";
      }
      os << "\t\thelpmsg += 'Note 1: All arguments must be wrapped in string quotes \"\", regardless of their type.\\n'\n"
	 << "\t\thelpmsg += 'Note 2: To include a particular optional argument, it should be given after the mandatory arguments in the form argumentname=\\'value\\'.\\n\\n'\n"
	 << "\t\thelpmsg += '     Example: " + std::string(example.begin(), example.end() - 1) + ")\\n'\n"
	 << "\t\tprint helpmsg,\n\n";
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
      os << "\tif cmd == '" << (*mIter).first << "':\n" 
	 << (*mIter).second;
      while( ++mIter != mEnd )
      {
	os << "\telif cmd == '" << (*mIter).first << "':\n" 
	   << (*mIter).second;
      }
      os << "\telse:\n"
	 << "\t\tprint 'mtdHelp() - '' + cmd + '' not found in help list'\n\n";
   
      //Aliases
      os << "# Help function aliases\n"
	 << "mtdhelp = mtdHelp\n"
	 << "Mtdhelp = mtdHelp\n"
	 << "MtdHelp = mtdHelp\n"
	 << "mantidhelp = mtdHelp\n"
	 << "mantidHelp = mtdHelp\n"
	 << "MantidHelp = mtdHelp\n";
    }

    /**
     * Takes a string and if only EOL characters are present then they are replaced with their string represenations
     * @param value The property value
     * @returns A string containing the sanitized property value
     */
    std::string SimplePythonAPI::convertEOLToString(const std::string & value)
    {
      if( value == "\n\r" )
	return std::string("\\\\") + std::string("n") + std::string("\\\\") + std::string("r");
      if( value == "\n" )
	return std::string("\\\\") + std::string("n");
      return value;
    }
    
    /**
     * Takes a string and removes the characters given in the optional second argument. If none are given then only alpha-numeric
     * characters are retained.
     * @param value The string to analyse
     * @param cs A string of characters to remove
     * @param eol_to_space Flag signalling whether end of line characters should be changed to a space
     * @returns The sanitized value
     */
    std::string SimplePythonAPI::removeCharacters(const std::string & value, const std::string & cs, bool eol_to_space)
    {
      if (value.empty())
        return value;

      std::string retstring;
      std::string::const_iterator sIter = value.begin();
      std::string::const_iterator sEnd = value.end();

      // No characeters specified, only keep alpha-numeric
      if (cs.empty())
      {
        for (; sIter != sEnd; ++sIter)
        {
          int letter = static_cast<int> (*sIter);
          if ((letter >= 48 && letter <= 57) || (letter >= 97 && letter <= 122) || (letter >= 65 && letter <= 90))
          {
            retstring.push_back(*sIter);
          }
        }
      }
      else
      {
        for (; sIter != sEnd; ++sIter)
        {
          const char letter = (*sIter);
          // If the letter is NOT one to remove
          if (cs.find_first_of(letter) == std::string::npos)
          {
            //This is because I use single-quotes to delimit my strings in the module and if any in strings
            //that I try to write contain these, it will confuse Python so I'll convert them
            if (letter == '\'')
            {
              retstring.push_back('\"');
            }
            // Keep the character
            else
            {
              retstring.push_back(letter);
            }
          }
          else
          {
            if (eol_to_space && letter == '\n')
            {
              retstring.push_back(' ');
            }
          }
        }
      }
      return retstring;
    }

    /**
     * Windows style backslash paths seems to cause problems sometimes. This function
     * converts all '\' characters for '/' ones
     * @param path The path string to check
     */
    std::string SimplePythonAPI::convertPathToUnix(const std::string & path)
    {
      size_t nchars = path.size();
      std::string retvalue;
      for( size_t i = 0; i < nchars; ++i )
      {
        char symbol = path.at(i);
        if( symbol == '\\' )
        {
          retvalue += '/';
        }
        else
        {
          retvalue += symbol;
        }
      }
      return retvalue;
    }

  } //namespace PythonAPI

} //namespace Mantid

