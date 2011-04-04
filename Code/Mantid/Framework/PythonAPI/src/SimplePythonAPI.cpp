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
#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/File.h>

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
      Poco::Path userPropDir(Mantid::Kernel::ConfigService::Instance().getUserPropertiesDir());
      return userPropDir.append(Poco::Path(g_module_name)).toString(); 
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
      module << "from MantidFramework import _makeString\n";
      module << "import os\n";
      module << "import sys\n";
      module << "import string\n\n";

      // A function to sort out whether the dialog parameters are disabled or not
      if( gui )
      {
        module << "# A utility function for the dialog routines that decides if the parameter\n"
          << "# should be added to the final list of parameters that have their widgets enabled\n"
          << "def _convertToPair(param_name, param_value, enabled_list, disabled_list):\n"
          << "  if param_value == None:\n"
          << "    if not param_name in disabled_list:\n"
          << "      return ('', param_name)\n"
          << "    else:\n"
          << "      return ('', '')\n"
          << "  else:\n"
          << "    strval = _makeString(param_value)\n"
          << "    if param_name in enabled_list or (len(strval) > 0 and strval[0] == '?'):\n"
          << "      return (param_name + '=' + strval.lstrip('?'), param_name)\n"
          << "    else:\n"
          << "      return (param_name + '=' + strval, '')\n\n";
      }

      // A couple of functions to aid in the formatting of help commands
      module << "def numberRows(descr, fw):\n"
        << "  des_len = len(descr)\n"
        << "  if des_len == 0:\n"
        << "    return (1, [''])\n"
        << "  nrows = 0\n"
        << "  i = 0\n"
        << "  descr_split = []\n"
        << "  while i < des_len:\n"
        << "    nrows += 1\n"
        << "    descr_split.append(descr[i:i+fw])\n"
        << "    i += fw\n"
        << "  return (nrows, descr_split)\n\n";

      //Algorithm keys
      using namespace Mantid::API;
      //Ensure that a FrameworkManager object has been instantiated to initialise the framework
      FrameworkManager::Instance();
      StringVector algKeys = AlgorithmFactory::Instance().getKeys();
      VersionMap vMap;
      createVersionMap(vMap, algKeys);
      //Function definitions for each algorithm
      //IndexVector helpStrings;
      std::map<std::string, std::set<std::string> > categories;
      for( VersionMap::const_iterator vIter = vMap.begin(); vIter != vMap.end();
        ++vIter)
      {
        IAlgorithm_sptr algm = AlgorithmManager::Instance().createUnmanaged(vIter->first);
        algm->initialize();
        PropertyVector orderedProperties(algm->getProperties());
        std::sort(orderedProperties.begin(), orderedProperties.end(), SimplePythonAPI::PropertyOrdering());
        std::string name(vIter->first);
	const int version(vIter->second);
        writeFunctionDef(module, name, version, orderedProperties, gui);
        if( gui ) 
        {
          writeGUIFunctionDef(module, name, version, orderedProperties);
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
    * @param version The default version of the algorithm
    * @param properties The list of properties
    * @param async Whether the algorithm should be executed asynchonously or not
    */
    void SimplePythonAPI::writeFunctionDef(std::ostream & os, const std::string & algm,
					   const int version,
					   const PropertyVector & properties, bool async)
    {
      if( algm == "Load" )
      {
	writeLoadFunction(os, async);
	return;
      }
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
      size_t nprops = properties.size();
      if( nprops > 0 ) os << ", ";
      os << "execute=True, Version=" << version << "):\n";

      os << "  algm = mtd.createAlgorithm(\"" << algm << "\", Version)\n";
      if( nprops > 0 )
      {
	os << "  try:\n";
	// Redo loop for setting values
	pIter = properties.begin();
	iarg = 0;
	for( ; pIter != pEnd; ++pIter, ++iarg )
	{
	  std::string pvalue = sanitizedNames[iarg];
	  if( iarg < iMand )
	  {
	    os << "    " << "  if execute:\n";
	    os << "    " << "    algm.setPropertyValue(\"" << (*pIter)->name()
	       << "\", _makeString(" << pvalue << ").lstrip('? '))\n";
	  }
	  else
	  {
	    os << "    " << "  if " << pvalue << " != None:\n"
	       << "    " << "    algm.setPropertyValue(\"" << (*pIter)->name() << "\", _makeString(" 
	       << pvalue << ").lstrip('? '))\n";
	  }
	}
	os << "  except RuntimeError, exc:\n"
	   << "      prefix = 'Unknown property search object'\n"
	   << "      error_msg = str(exc)\n"
	   << "      if error_msg.startswith(prefix):\n"
	   << "          prop_name = error_msg.lstrip(prefix).strip()\n"
	   << "          raise RuntimeError('Unknown property \"' + prop_name + '\" for " << algm << " version ' + str(algm.version()))\n"
	   << "      else:\n"
	   << "          raise\n";
      }
      os << "  algm = mtd._createAlgProxy(algm)\n"
         << "  if execute:\n"
         << "    algm.execute()\n";

      // Return the IAlgorithm object
      os << "  return algm\n";

      // add the help information as a static thing
      os << algm << ".__doc__ = mtd.createAlgorithmDocs(\"" << algm << "\")\n\n";
    }

    /**
     * Write a spcial function definition for the smarter Load algorithm
     * @param[in,out] os :: The file stream
     * @param async :: If true then the algorithm will run asynchrnously
     */
    void SimplePythonAPI::writeLoadFunction(std::ostream & os, bool async)
    {
      os <<
        "def Load(*args, **kwargs):\n"
        "    \"\"\"\n"
        "    Load is a more flexible algorithm than other Mantid algorithms.\n"
        "    It's aim is to discover the correct loading algorithm for a\n"
        "    given file. This flexibility comes at the expense of knowing the\n"
        "    properties out right before the file is specified.\n"
        "    \n"
        "    The argument list for the Load function has to be more flexible to\n"
        "    allow this searching to occur. Two arguments must be specified:\n"
        "    \n"
        "      - Filename :: The name of the file,\n"
        "      - OutputWorkspace :: The name of the workspace,\n"
        "    \n"
        "    either as the first two arguments in the list or as keywords. Any other\n"
        "    properties that the Load algorithm has can be specified by keyword only.\n"
        "    \n"
        "    Some common keywords are:\n"
        "     - SpectrumMin,\n"
        "     - SpectrumMax,\n"
        "     - SpectrumList,\n"
        "     - EntryNumber\n"
        "    \n"
        "    Example:\n"
        "      # Simple usage, ISIS NeXus file\n"
        "      Load('INSTR00001000.nxs', 'run_ws')\n"
        "      \n"
        "      # ISIS NeXus with SpectrumMin and SpectrumMax = 1\n"
        "      Load('INSTR00001000.nxs', 'run_ws', SpectrumMin=1,SpectrumMax=1)\n"
        "      \n"
        "      # SNS Event NeXus with precount on\n"
        "      Load('INSTR_1000_event.nxs', 'event_ws', Precount=True)\n"
        "      \n"
        "      # A mix of keyword and non-keyword is also possible\n"
        "      Load('event_ws', Filename='INSTR_1000_event.nxs',Precount=True)\n"
        "    \"\"\"\n"
        "    # Small inner function to grab the mandatory arguments and translate possible\n"
        "    # exceptions\n"
        "    def get_argument_value(key, kwargs):\n"
        "        try:\n"
        "            value = kwargs[key]\n"
        "            del kwargs[key]\n"
        "            return value\n"
        "        except KeyError:\n"
        "            raise RuntimeError('%s argument not supplied to Load function' % str(key))\n"
        "    \n"
        "    if len(args) == 2:\n"
        "        filename = args[0]\n"
        "        wkspace = args[1]\n"
        "    elif len(args) == 1:\n"
        "        if 'Filename' in kwargs:\n"
        "            wkspace = args[0]\n"
        "            filename = get_argument_value('Filename', kwargs)\n"
        "        elif 'OutputWorkspace' in kwargs:\n"
        "            filename = args[0]\n"
        "            wkspace = get_argument_value('OutputWorkspace', kwargs)\n"
        "        else:\n"
        "            raise RuntimeError('Cannot find \"Filename\" or \"OutputWorkspace\" in key word list. '\n"
        "                               'Cannot use single positional argument.')\n"
        "    elif len(args) == 0:\n"
        "        filename = get_argument_value('Filename', kwargs)\n"
        "        wkspace = get_argument_value('OutputWorkspace', kwargs)\n"
        "    else:\n"
        "        raise RuntimeError('Load() takes at most 2 positional arguments, %d found.' % len(args))\n"
        "    \n"
        "    # Create and execute\n"
        "    algm = mtd.createAlgorithm('Load')\n"
        "    if kwargs.get('execute', True) == True:\n"
        "        algm.setPropertyValue('Filename', filename)\n"
        "        algm.setPropertyValue('OutputWorkspace', wkspace)\n"
        "        for key, value in kwargs.iteritems():\n"
        "            try:\n"
        "                algm.setPropertyValue(key, _makeString(value).lstrip('? '))\n"
        "            except RuntimeError:\n"
        "                raise RuntimeError('Invalid argument \"%s\" to Load algorithm.' % str(key))\n"
        "            \n";
      // Execution
      if( async )
      {
        writeAsyncFunctionCall(os, "Load", "        ");
        os << "        if result == False:\n"
          << "            sys.exit('An error occurred while running Load. See results log for details.')\n";
      }
      else
      {
        os << "        algm.setRethrows(True)\n";
        os << "        algm.execute()\n";
      }
      os << "    return mtd._createAlgProxy(algm)\n\n";
    }

    /**
    * Write the GUI version of the Python function that raises a Qt dialog
    * @param os The stream to use to write the definition
    * @param algm The name of the algorithm
    * @param version The default version to create
    * @param properties The list of properties
    */
    void SimplePythonAPI::writeGUIFunctionDef(std::ostream & os, const std::string & algm,
					      const int version,
					      const PropertyVector & properties)
    {
      if( algm == "Load" )
      {
	writeLoadDialogDef(os);
	return;
      }

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
      os << "Message = \"\", Enable=\"\", Disable=\"\", Version=" << version << "):\n"
        << "  algm = mtd.createAlgorithm(\"" << algm << "\", Version)\n"
        << "  enabled_list = [s.lstrip(' ') for s in Enable.split(',')]\n"
        << "  disabled_list = [s.lstrip(' ') for s in Disable.split(',')]\n"
        << "  values = '|'\n"
        << "  final_enabled = ''\n\n";

      pIter = properties.begin();
      for( int iarg = 0; pIter != pEnd; ++pIter, ++iarg)
      {
	const std::string & propName = (*pIter)->name();
	os << "  if not algm.existsProperty('" << propName << "'):\n"
	   << "    raise RuntimeError('Unknown property \"" <<  propName << "\" for " << algm << " version ' + str(algm.version()))\n";
        os << "  valpair = _convertToPair('" << (*pIter)->name() << "', " << sanitizedNames[iarg]
	   << ", enabled_list, disabled_list)\n"
	   << "  values += valpair[0] + '|'\n"
	   << "  final_enabled += valpair[1] + ','\n\n";
      }

      os << "  dialog = qti.app.mantidUI.createPropertyInputDialog(\"" << algm 
         << "\" , values, Message, final_enabled)\n"
         << "  if dialog == True:\n"
         << "    algm = mtd._createAlgProxy(algm)\n"
         << "    algm.execute()\n"
         << "    return algm\n";
      os << "  else:\n"
         << "    sys.exit('Information: Script execution cancelled')\n\n";
    }

    /**
     * Write a command for the LoadDialog
     * @param os :: The file stream
     */
    void SimplePythonAPI::writeLoadDialogDef(std::ostream & os)
    {
      os << "def LoadDialog(*args, **kwargs):\n"
	"    \"\"\"Popup a dialog for the Load algorithm. More help on the Load function\n"
	"    is available via help(Load).\n"
	"\n"
	"    Additional arguments available here (as keyword only) are:\n"
	"      - Enable :: A CSV list of properties to keep enabled in the dialog\n"
	"      - Disable :: A CSV list of properties to keep enabled in the dialog\n"
	"      - Message :: An optional message string\n"
	"    \"\"\"\n"
	"    if 'Enable' in kwargs:\n"
	"        enabled_list = [s.lstrip(' ') for s in kwargs['Enable'].split(',')]\n"
	"    else:\n"
	"        enabled_list = []\n"
	"    if 'Disable' in kwargs:\n"
	"        disabled_list = [s.lstrip(' ') for s in kwargs['Disable'].split(',')]\n"
	"    else:\n"
	"        disabled_list = []\n"
	"\n"
	"    arguments = {}\n"
	"    filename = None\n"
	"    wkspace = None\n"
	"    if len(args) == 2:\n"
	"        filename = args[0]\n"
	"        wkspace = args[1]\n"
	"    elif len(args) == 1:\n"
	"        if 'Filename' in kwargs:\n"
	"            filename = kwargs['Filename']\n"
	"            wkspace = args[0]\n"
	"        elif 'OutputWorkspace' in kwargs:\n"
	"            wkspace = kwargs['OutputWorkspace']\n"
	"            filename = args[0]\n"
	"    arguments['Filename'] = filename\n"
	"    arguments['OutputWorkspace'] = wkspace\n"
	"    # Create lists to pass to create dialog function\n"
	"    final_enabled = ''\n"
	"    values = '|'\n"
	"    algm = mtd.createAlgorithm('Load')\n"
	"    if filename is not None:"
	"        algm.setPropertyValue('Filename', filename)\n"
	"    props = algm.getProperties()\n"
	"    for p in props:\n"
	"        p_name = p.name\n"
	"        if p_name not in arguments:\n"
	"            arguments[p_name] = kwargs.get(p_name, None)\n"
	"    # Everything else\n"
	"    for key, value in arguments.iteritems():\n"
	"        valpair = _convertToPair(key, value,enabled_list, disabled_list)\n"
	"        values += valpair[0] + '|'\n"
	"        final_enabled += valpair[1] + ','\n"
	"    final_enabled.rstrip(',')\n"
	"    # Running algorithm\n"
	"    dialog = qti.app.mantidUI.createPropertyInputDialog('Load' , values, kwargs.get('Message',''), final_enabled)\n"
	"    if dialog == True:\n"
	"        result = qti.app.mantidUI.runAlgorithmAsync_PyCallback('Load')\n"
	"    else:\n"
	"        sys.exit('Information: Script execution cancelled')\n"
	"    if result == False:\n"
	"        sys.exit('An error occurred while running Load. See results log for details.')\n"
	"    return mtd._createAlgProxy(algm)\n\n";
    }

    /**
    * Write out Python code required to execute an algorithm asynchronously.
    * @param output The stream to contain the code
    * @param alg_name The name of the algorithm
    * @param prefix A prefix to apply to each line
    */
    void SimplePythonAPI::writeAsyncFunctionCall(std::ostream & output, const std::string & alg_name, 
      const std::string & prefix)
    {
      output << prefix << "result = qti.app.mantidUI.runAlgorithmAsync_PyCallback(\"" << alg_name << "\")\n";
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

