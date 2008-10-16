//-----------------------------------
//Includes
//-----------------------------------
#include <vector>
#include <string>
#include <fstream>
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
      
      //Algorithm names
      StringSet algmNames = getAlgorithmNames();
      writeGlobalHelp(module, algmNames);

      //Function definitions for each algorithm
      for( StringSet::const_iterator sIter = algmNames.begin(); sIter != algmNames.end();
          ++sIter)
          {
            Mantid::API::IAlgorithm* algm = Mantid::API::FrameworkManager::Instance().createAlgorithm(*sIter);
            writeFunctionDef(module, *sIter, algm->getProperties());
          }
      //close file stream
      module.close();
    }

    /**
     * Get a list of unique algorithm names
     * @returns A set of strings denoting the loaded algorithms 
     */
    SimplePythonAPI::StringSet SimplePythonAPI::getAlgorithmNames()
    {
      StringVector algKeys = Mantid::API::AlgorithmFactory::Instance().getKeys();
      StringVector::const_iterator sIter = algKeys.begin();
      StringSet algNames;
      for( ; sIter != algKeys.end(); ++sIter )
      {
        algNames.insert(extractAlgName(*sIter));
      }
      return algNames;
    }
    
    /**
     * Strip the version information from the algorithm keys
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
      //parameter listing
      PropertyVector::const_iterator pIter = properties.begin();
      PropertyVector::const_iterator pEnd = properties.end();
      int cParam(97);
      for( ; pIter != pEnd; ++cParam )
      {
        os << (char)cParam;
        if( ++pIter != pEnd ) os << ", ";
      }
      //end of function parameters 
      os << "):\n";
      os << "\talgm = FrameworkManager().createAlgorithm(\"" << algm << "\")\n";
      pIter = properties.begin();
      for( cParam = 97; pIter != pEnd; ++pIter, ++cParam )
      {
        os << "\talgm.setPropertyValue(\"" << (*pIter)->name() << "\", " << (char)cParam << ")\n";
      }
      os << "\treturn algm\n"; 
      //Add space at end of definition
      os << "\n";
    }
    
    /**
     * Write a global help command called mtdHelp, which takes no arguments
     * @param os The stream to use to write the command
     * @param algms The names of the available algorithms
     */
    void SimplePythonAPI::writeGlobalHelp(std::ostream & os, const StringSet & algNames)
    {
      os << "# The help command with no parameters\n";
      os << "def mtdHelp():\n";
      os << "\tprint \"The algorithms available are:\"\n";
      StringSet::const_iterator sIter = algNames.begin();
      for( ; sIter != algNames.end(); ++sIter )
      {
        os << "\tprint \"\t" << *sIter << "\"\n";
      }
      os << "\n";
    }
    
  }
    
}
