//-----------------------------------
//Includes
//-----------------------------------
#include <vector>
#include <string>
#include <fstream>
#include <cassert>
#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/IAlgorithm.h"

namespace Mantid
{

  namespace PythonAPI
  {

    /// Static filename variable
    std::string SimplePythonAPI::m_strFilename = "mantidsimple.py";

    //------------------------------
    //Public methods
    //------------------------------
    /**
     * Return the name of the Python module to be created
     * @returns A string containing the name of the module file
     */
    const std::string & SimplePythonAPI::getModuleName()
    {
      return m_strFilename;
    }

    /**
     * Create the python module with function definitions in the
     * file whose name is stored in "m_strFilename"
     */
    void SimplePythonAPI::createModule()
    {
      //open file
      std::ofstream module(m_strFilename.c_str());

      // Need to import definitions from main Python API
#ifdef _WIN32
      module << "from MantidPythonAPI import FrameworkManager\n\n";
#else
      module << "from libMantidPythonAPI import FrameworkManager\n\n";
#endif

      //Algorithm keys
      StringVector algKeys = Mantid::API::AlgorithmFactory::Instance().getKeys();
      VersionMap vMap;
      createVersionMap(vMap, algKeys);
      writeGlobalHelp(module, vMap);

      //Function definitions for each algorithm
      for( VersionMap::const_iterator vIter = vMap.begin(); vIter != vMap.end();
          ++vIter)
          {
            Mantid::API::IAlgorithm* algm = Mantid::API::FrameworkManager::Instance().createAlgorithm(vIter->first);
            writeFunctionDef(module, vIter->first, algm->getProperties());
          }
      //close file stream
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
	  if( vIter == vMap.end() )
	    vMap.insert(make_pair(name, 1));
	  else
	    ++(vIter->second);
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
     * @param propertiesThe list of properties
     */
    void SimplePythonAPI::writeFunctionDef(std::ostream & os, std::string algm,
    const PropertyVector & properties)
    {
      os << "# Definition of \"" << algm << "\" function.\n";
      //start of definition
      os << "def " << algm << "(";
      //parameter listing - ensuring the ones that are required are
      //first
      PropertyVector orderedProperties(properties);
      std::sort(orderedProperties.begin(), orderedProperties.end(),
		SimplePythonAPI::PropertyOrdering());
      PropertyVector::const_iterator pIter = orderedProperties.begin();
      PropertyVector::const_iterator pEnd = orderedProperties.end();
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
      pIter = orderedProperties.begin();
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
      os << "def mtdHelp():\n";
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
      os << "\n";
    }

  }

}
