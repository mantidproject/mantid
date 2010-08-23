//-----------------------------------
//Includes
//-----------------------------------
#include <vector>
#include <set>
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

    /// The module filename
    std::string SimplePythonAPI::g_module_name = "mantidsimple.py";

    //------------------------------
    //Public methods
    //------------------------------
    /**
    * Return the name of the Python module to be created
    * @returns A string containing the name of the module file
    */
    std::string SimplePythonAPI::getModuleFilename()
    {
      return Poco::Path(Mantid::Kernel::ConfigService::Instance().getOutputDir()).append(Poco::Path(g_module_name)).toString(); 
    }

    /**
    * Create the python module with function definitions in the
    * file whose name is returned by getModule()
    * @param gui If this is true create the necessary framework to use the dialog
    * boxes in qtiplot
    */
    void SimplePythonAPI::createModule(bool gui)
    {
      std::ofstream module(getModuleFilename().c_str());

      module << "from MantidFramework import *\n";
      //If in gui mode also need sys and qti module
      if( gui )
      {
        module << "import qti\n";
      }
      //Need string and os module regardless
      module << "import os\n";
      module << "import string\n\n";

      // A simple function to change the working directory
      module << "# A wrapper for changing the directory\n"
        << "def setWorkingDirectory(path):\n"
        << "\tos.chdir(path)\n\n";

      // A function to sort out whether the dialog parameters are disabled or not
      if( gui )
      {
        module << "# A utility function for the dialog routines that decides if the parameter\n"
          << "# should be added to the final list of parameters that have their widgets enabled\n"
          << "def convertToPair(param_name, param_value, enabled_list, disabled_list):\n"
          << "\tif param_value == None:\n"
          << "\t\tif not param_name in disabled_list:\n"
          << "\t\t\treturn ('', param_name)\n"
          << "\t\telse:\n"
          << "\t\t\treturn ('', '')\n"
          << "\telse:\n"
          << "\t\tstrval = makeString(param_value)\n"
          << "\t\tif param_name in enabled_list or (len(strval) > 0 and strval[0] == '?'):\n"
          << "\t\t\treturn (param_name + '=' + strval.lstrip('?'), param_name)\n"
          << "\t\telse:\n"
          << "\t\t\treturn (param_name + '=' + strval, '')\n\n";
      }

      // A couple of functions to aid in the formatting of help commands
      module << "def numberRows(descr, fw):\n"
        << "\tdes_len = len(descr)\n"
        << "\tif des_len == 0:\n"
        << "\t\treturn (1, [''])\n"
        << "\tnrows = 0\n"
        << "\ti = 0\n"
        << "\tdescr_split = []\n"
        << "\twhile i < des_len:\n"
        << "\t\tnrows += 1\n"
        << "\t\tdescr_split.append(descr[i:i+fw])\n"
        << "\t\ti += fw\n"
        << "\treturn (nrows, descr_split)\n\n";

      // A rather complicated function to format the help into a table
      module << "def createParamTable(param_list, dialog):\n"
        << "\tflw = 100\n"
        << "\tcol_widths = [flw/5, 6, 8, 6, flw/3, flw/4]\n"
        << "\ttopline = '|' + 'Param Name'.center(col_widths[0]) + '|' + 'In/Out'.center(col_widths[1]) + '|' + 'Type'.center(col_widths[2]) + '|' + 'Req\\'d?'.center(col_widths[3]) + '|' + 'Description'.center(col_widths[4])  + '|' + 'Allowed Values'.center(col_widths[5]) + '\\n'\n"
        << "\trow_delim = '-' * len(topline) + '\\n'\n"
        << "\thelpstr =  row_delim + topline + row_delim\n"
        << "\tif dialog == True:\n"
        << "\t\tparam_list.append(['Message','Input','string','','A message to display', ''])\n"
        << "\t\tparam_list.append(['Enable','Input','string','','Comma-separated list of param names to keep enabled in the dialog', ''])\n"
        << "\t\tparam_list.append(['Disable','Input','string','','Comma-separated list of param names to disable in the dialog', ''])\n"
        << "\tfor pstr in param_list:\n"
        << "\t\tndes, descr_split = numberRows(pstr[4], col_widths[4])\n"
        << "\t\tnall, allow_split = numberRows(pstr[5], col_widths[5])\n"
        << "\t\tif ndes  == 1 and nall == 1:\n"
        << "\t\t\thelpstr += ''.join(['|' + pstr[s].center(col_widths[s]) for s in range(0, 6)]) + '\\n'\n"
        << "\t\telse:\n"
        << "\t\t\tmidline = 0\n"
        << "\t\t\ttot_rows = max(ndes, nall)\n"
        << "\t\t\tif bool(tot_rows % 2):\n"
        << "\t\t\t\tmidline = (tot_rows + 1) / 2\n"
        << "\t\t\telse:\n"
        << "\t\t\t\tmidline = tot_rows / 2\n"
        << "\t\t\tfor r in range(0, tot_rows):\n"
        << "\t\t\t\tline = []\n"
        << "\t\t\t\tif ndes == nall:\n"
        << "\t\t\t\t\tif r != midline - 1:\n"
        << "\t\t\t\t\t\tline = ['','','','',descr_split[r], allow_split[r]]\n"
        << "\t\t\t\t\telse:\n"
        << "\t\t\t\t\t\tline = [pstr[0],pstr[1],pstr[2],pstr[3],descr_split[r], allow_split[r]]\n" 
        << "\t\t\t\telif ndes > nall:\n"
        << "\t\t\t\t\tif r < nall:\n"
        << "\t\t\t\t\t\tif r != midline - 1:\n"
        << "\t\t\t\t\t\t\tline = ['','','','',descr_split[r], allow_split[r]]\n"
        << "\t\t\t\t\t\telse:\n"
        << "\t\t\t\t\t\t\tline = [pstr[0],pstr[1],pstr[2],pstr[3],descr_split[r], allow_split[r]]\n" 
        << "\t\t\t\t\telse:\n"
        << "\t\t\t\t\t\tif r != midline - 1:\n"
        << "\t\t\t\t\t\t\tline = ['','','','',descr_split[r], '']\n"
        << "\t\t\t\t\t\telse:\n"
        << "\t\t\t\t\t\t\tline = [pstr[0],pstr[1],pstr[2],pstr[3],descr_split[r], '']\n"
        << "\t\t\t\telse:\n"
        << "\t\t\t\t\tif r < ndes:\n"
        << "\t\t\t\t\t\tif r != midline - 1:\n"
        << "\t\t\t\t\t\t\tline = ['','','','',descr_split[r], allow_split[r]]\n"
        << "\t\t\t\t\t\telse:\n"
        << "\t\t\t\t\t\t\tline = [pstr[0],pstr[1],pstr[2],pstr[3],descr_split[r], allow_split[r]]\n"
        << "\t\t\t\t\telse:\n"
        << "\t\t\t\t\t\tif r != midline - 1:\n"
        << "\t\t\t\t\t\t\tline = ['','','','', '',allow_split[r]]\n"
        << "\t\t\t\t\t\telse:\n"
        << "\t\t\t\t\t\t\tline = [pstr[0],pstr[1],pstr[2], pstr[3],'', allow_split[r]]\n"
        << "\t\t\t\thelpstr += ''.join(['|' + line[s].center(col_widths[s]) for s in  range(0,6)]) + '\\n'\n"
        << "\t\thelpstr += row_delim\n"
        << "\treturn helpstr\n\n";


      //Algorithm keys
      using namespace Mantid::API;
      //Ensure that a FrameworkManager object has been instantiated to initialise the framework
      FrameworkManager::Instance();
      StringVector algKeys = AlgorithmFactory::Instance().getKeys();
      VersionMap vMap;
      createVersionMap(vMap, algKeys);
      writeGlobalHelp(module, vMap, gui);
      //Function definitions for each algorithm
      IndexVector helpStrings;
      std::map<std::string, std::set<std::string> > categories;
      for( VersionMap::const_iterator vIter = vMap.begin(); vIter != vMap.end();
        ++vIter)
      {
        IAlgorithm_sptr algm = AlgorithmManager::Instance().createUnmanaged(vIter->first);
        algm->initialize();
        PropertyVector orderedProperties(algm->getProperties());
        std::sort(orderedProperties.begin(), orderedProperties.end(), SimplePythonAPI::PropertyOrdering());
        std::string name(vIter->first);
        writeFunctionDef(module, name , orderedProperties, gui);
        if( gui ) 
        {
          writeGUIFunctionDef(module, name, orderedProperties);
        }

        // Help strings
        std::transform(name.begin(), name.end(), name.begin(), tolower);
        helpStrings.push_back(make_pair(name, createHelpString(vIter->first, orderedProperties, false)));
        //The help for the dialog functions if necessary
        if( gui )
        {
          helpStrings.push_back(make_pair(name + "dialog", createHelpString(vIter->first, orderedProperties, true)));
        }

        // Get the category and save it to our map
        std::string category = algm->category();
        std::string::size_type idx = category.find("\\");
        std::string topcategory(category), tail(vIter->first); 
        if( idx != std::string::npos )
        {
          topcategory = category.substr(0, idx);
          tail = category.substr(idx + 1) + "\\" + vIter->first;
        }

        std::map<std::string, std::set<std::string> >::iterator itr = categories.find(topcategory);
        if( itr != categories.end() )
        {
          (*itr).second.insert(tail);
        }
        else
        {
          std::set<std::string> cat_algms;
          cat_algms.insert(tail);
          categories.insert(std::make_pair<std::string, std::set<std::string> >(topcategory, cat_algms));
        }
      }

      //Help strings
      writeFunctionHelp(module, helpStrings, categories);
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
     * Write the help text for the function definitions that look more pythonic.
     * @param os The stream to use to write the help
     * @param properties This list of properties
     * @param names The names associated with the properties
     *
     */
    void SimplePythonAPI::writeFunctionPyHelp(std::ostream& os, const PropertyVector& properties,
                                              const StringVector& names)
    {
      os << "\t\"\"\"\n";

      size_t size = properties.size();
      Mantid::Kernel::Property *prop;
      for (size_t i = 0; i < size; ++i)
      {
        prop = properties[i];
        os << "\t" << names[i] << "("
           << Mantid::Kernel::Direction::asText(prop->direction());
        if (!prop->isValid().empty())
          os << ":req";
        os << ") *" << prop->type() << "* "<< "\n";
        std::set<std::string> allowed = prop->allowedValues();
        if (!prop->documentation().empty() || !allowed.empty())
        {
          os << "\t    " << prop->documentation();
          if (!allowed.empty())
          {
            os << " [";
            std::set<std::string>::const_iterator sIter = allowed.begin();
            std::set<std::string>::const_iterator sEnd = allowed.end();
            for( ; sIter != sEnd ; )
            {
              os << (*sIter);
              if( ++sIter != sEnd ) os << ", ";
            }
            os << "]";
          }
          os << "\n";
        }
      }
      os << "\t\"\"\"\n";
    }

    /**
    * Write a Python function defintion
    * @param os The stream to use to write the definition
    * @param algm The name of the algorithm
    * @param properties The list of properties
    * @param async Whether the algorithm should be executed asynchonously or not
    */
    void SimplePythonAPI::writeFunctionDef(std::ostream & os, const std::string & algm,
      const PropertyVector & properties, bool async)
    {
      os << "# Definition of \"" << algm << "\" function.\n";
      //start of definition
      os << "def " << algm;
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

        //properties are optional unless their current value results in an error (isValid != "")
        if( (*pIter)->isValid() != "" ) ++iMand;
        else os  << " = None";
        if( ++pIter != pEnd ) os << ", ";
      }

      //end of function parameters
      os << "):\n";

      writeFunctionPyHelp(os, properties, sanitizedNames);

      os << "\talgm = mantid.createAlgorithm(\"" << algm << "\")\n";

      // Redo loop for setting values
      pIter = properties.begin();
      iarg = 0;
      for( ; pIter != pEnd; ++pIter, ++iarg )
      {
        std::string pvalue = sanitizedNames[iarg];
        if( iarg < iMand )
        {
          os << "\talgm.setPropertyValue(\"" << (*pIter)->name() 
            << "\", makeString(" << pvalue << ").lstrip('? '))\n";
        }
        else
        {
          os << "\tif " << pvalue << " != None:\n"
            << "\t\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", makeString(" 
            << pvalue << ").lstrip('? '))\n";
        }
      }

      if( async )
      {
        writeAsyncFunctionCall(os, algm, "\t");
        os << "\tif result == False:\n"
          << "\t\tsys.exit('An error occurred while running " << algm << ". See results log for details.')\n";
      }
      else
      {
        os << "\talgm.setRethrows(True)\n";
        os << "\talgm.execute()\n";
      }

      // Return the IAlgorithm object
      os << "\treturn mtd._createAlgProxy(algm)\n\n";
    }

    /**
    * Write the GUI version of the Python function that raises a Qt dialog
    * @param os The stream to use to write the definition
    * @param algm The name of the algorithm
    * @param properties The list of properties
    */
    void SimplePythonAPI::writeGUIFunctionDef(std::ostream & os, const std::string & algm,
      const PropertyVector & properties)
    {
      os << "# Definition of \"" << algm << "\" function.\n";
      //start of definition
      os << "def " << algm << "Dialog(";
      //Iterate through properties
      PropertyVector::const_iterator pIter = properties.begin();
      PropertyVector::const_iterator pEnd = properties.end();
      StringVector sanitizedNames(properties.size());
      for( int iarg = 0; pIter != pEnd; ++pIter, ++iarg)
      {
        sanitizedNames[iarg] = removeCharacters((*pIter)->name(), "");
        os << sanitizedNames[iarg];

        os << " = None,";
      }
      //end of algorithm function parameters but add other arguments
      os << "Message = \"\", Enable=\"\", Disable=\"\"):\n"
        << "\talgm = mantid.createAlgorithm(\"" << algm << "\")\n"
        << "\tenabled_list = [s.lstrip(' ') for s in Enable.split(',')]\n"
        << "\tdisabled_list = [s.lstrip(' ') for s in Disable.split(',')]\n"
        << "\tvalues = '|'\n"
        << "\tfinal_enabled = ''\n\n";

      pIter = properties.begin();
      for( int iarg = 0; pIter != pEnd; ++pIter, ++iarg)
      {
        os << "\tvalpair = convertToPair('" << (*pIter)->name() << "', " << sanitizedNames[iarg]
        << ", enabled_list, disabled_list)\n"
          << "\tvalues += valpair[0] + '|'\n"
          << "\tfinal_enabled += valpair[1] + ','\n\n";
      }

      os << "\tdialog = qti.app.mantidUI.createPropertyInputDialog(\"" << algm 
        << "\" , values, Message, final_enabled)\n"
        << "\tif dialog == True:\n";

      writeAsyncFunctionCall(os, algm, "\t\t");

      os << "\telse:\n"
        << "\t\tsys.exit('Information: Script execution cancelled')\n"
        << "\tif result == False:\n"
        << "\t\tsys.exit('An error occurred while running " << algm << ". See results log for details.')\n"
        << "\treturn mtd._createAlgProxy(algm)\n\n";
    }

    /**
    * Write a global help command called mtdHelp, which takes no arguments
    * @param os The stream to use to write the command
    * @param vMap The map of algorithm names to highest version numbers
    * @param gui Create GUI help message with extra information about dialog functions
    */
    void SimplePythonAPI::writeGlobalHelp(std::ostream & os, const VersionMap & vMap, bool gui)
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
      os << "\thelpmsg += \"For help with a specific command type: mantidHelp(\\\"cmd\\\")\\n\"\n";
      if( gui )
      {
        os << "\thelpmsg += \"Note: Each command also has a counterpart with the word 'Dialog'"
          << " appended to it, which when run will bring up a property input dialog for that algorithm.\\n\"\n";
      }
      os << "\tprint helpmsg,\n"
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
      os << "\t\tparams_list = [";
      std::string argument_list(algm);
      if( dialog )
      {
        argument_list += "Dialog";
      }
      argument_list += "(";

      PropertyVector::const_iterator pIter = properties.begin();
      PropertyVector::const_iterator pEnd = properties.end();
      for( ; pIter != pEnd ; )
      {
        Mantid::Kernel::Property *prop = *pIter;
        argument_list += removeCharacters((*pIter)->name(), "");
        os << "['" << prop->name() << "','" << Mantid::Kernel::Direction::asText(prop->direction()) << "', '" 
          << prop->type() << "','";
        if( !prop->isValid().empty() )
        {
          os << "X";
        }
        os << "', '" << removeCharacters(prop->documentation(),"\n\r", true) << "','";
        std::set<std::string> allowed = prop->allowedValues();
        if( !allowed.empty() )
        {
          std::set<std::string>::const_iterator sIter = allowed.begin();
          std::set<std::string>::const_iterator sEnd = allowed.end();
          for( ; sIter != sEnd ; )
          {
            os << (*sIter);
            if( ++sIter != sEnd ) os << ", ";
          }
        }
        os << "']";
        if( ++pIter != pEnd ) 
        {
          os << ",";
          argument_list += ",";
        }
      }
      if( dialog )
      {
        argument_list += ",Message,Enable,Disable";
      }
      argument_list += ")";

      os << "]\n"
        << "\t\thelpstring = '\\nUsage: ' + '" << argument_list << "\\n\\n'\n"
        << "\t\thelpstring += createParamTable(params_list,";
      if( dialog )
      {
        os << "True";
      }
      else
      {
        os << "False";
      }
      os << ")\n"
        << "\t\tprint helpstring,\n";
      return os.str();
    }

    /**
    * Write the help function that takes a command as an argument
    * @param os The stream to use for the output
    * @param helpStrings A map of function names to help strings
    * @param categories The list categories and their associated algorithms
    */
    void SimplePythonAPI::writeFunctionHelp(std::ostream & os, const IndexVector & helpStrings, 
      const std::map<std::string, std::set<std::string> > & categories)
    {
      if ( helpStrings.empty() ) return;

      os << "def mtdHelp(cmd = None):\n";

      os << "\tif cmd == None or cmd == '':\n"
        << "\t\tmtdGlobalHelp()\n"
        << "\t\treturn\n";
      os << "\n\ttry:\n"
          << "\t\tcmd = cmd.func_name\n"
          << "\texcept ValueError:\n"
          << "\t\tpass\n";
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

      //Categories
      std::map<std::string, std::set<std::string> >::const_iterator cat_end = categories.end();
      for( std::map<std::string, std::set<std::string> >::const_iterator cat_itr = categories.begin(); cat_itr != cat_end; ++cat_itr )
      {
        std::string topcategory = cat_itr->first;
        std::string lowercase = topcategory;
        std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), tolower);
        os << std::string("\telif cmd == '") << lowercase << std::string("':\n\t\thelpstr = 'The algorithms in the ") 
          << topcategory << std::string(" category are:\\n'\n");		
        std::set<std::string>::const_iterator alg_end = cat_itr->second.end();
        for( std::set<std::string>::const_iterator alg_itr = cat_itr->second.begin(); alg_itr != alg_end; ++alg_itr )
        {
          os << std::string("\t\thelpstr += '\t") << (*alg_itr) << "\\n'\n";
        }
        os << "\t\tprint helpstr,\n";
      }



      // Finally add a "default" clause if the name cannot be found
      os << "\telse:\n"
        << "\t\tprint 'mtdHelp() - ' + cmd + ' not found in help list'\n\n";

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
    * Write out Python code required to execute an algorithm asynchronously, ensuring the GIL is in the correct state
    * @param output The stream to contain the code
    * @param alg_name The name of the algorithm
    * @param prefix A prefix to apply to each line
    */
    void SimplePythonAPI::writeAsyncFunctionCall(std::ostream & output, const std::string & alg_name, 
      const std::string & prefix)
    {
      output << prefix << "mtd._setGILRequired(True)\n" 
        << prefix << "result = qti.app.mantidUI.runAlgorithmAsync_PyCallback(\"" << alg_name << "\")\n"
        << prefix << "mtd._setGILRequired(False)\n";
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
    * Split the string on the given delimiter
    * @param str The string to split
    * @param delim The delimiter to use
    * @return A vector of the sections of the string
    */
    std::vector<std::string> SimplePythonAPI::split(const std::string & str,  const std::string & delim)
    {
      std::vector<std::string> splitlist;
      std::string::size_type idx = str.find(delim);
      if( idx == std::string::npos ) return splitlist;

      splitlist.push_back( str.substr(0, idx) );
      std::string::size_type offset = delim.size();
      std::string::size_type start(idx + offset);
      std::string::size_type end(str.size());
      while( true )
      {
        idx = str.find(delim, start);
        if( idx == std::string::npos ) 
        {
          splitlist.push_back( str.substr(start, end - start) );
          break;
        }
        splitlist.push_back( str.substr(start, idx - start) );
        start = idx + offset;
      }

      return splitlist;
    }

  } //namespace PythonAPI

} //namespace Mantid

