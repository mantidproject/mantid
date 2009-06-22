#ifndef SIMPLEPYTHONAPITEST_H_
#define SIMPLEPYTHONAPITEST_H_

//--------------------------------
// Includes
//--------------------------------
#include <fstream>
#include <cxxtest/TestSuite.h>
#include "Poco/File.h"

#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/FrameworkManager.h"

class SimplePythonAPITest : public CxxTest::TestSuite
{
public:

  SimplePythonAPITest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void testCreateModule()
  {
    using namespace Mantid::PythonAPI;
    //first call the function to create the module file
    SimplePythonAPI::createModule(false);
    Poco::File apimodule(SimplePythonAPI::getModuleName());
    //has it been written ?
    TS_ASSERT(apimodule.exists());

    //does it contain what we expect
    std::ifstream is(apimodule.path().c_str());

    std::string line;
    getline(is, line);
    TS_ASSERT_EQUALS(line, "import sys");
    //skip lines containing append commands
    //This will leave line containing the next either non-blank or sys cmd line
    while( getline(is, line) && (line.empty() || line.find("sys") != std::string::npos) )
    {
    }

    //Next should import main Python API
    std::string modline("");
#ifdef _WIN32
    modline = "from MantidPythonAPI import *";
#else
    modline = "from libMantidPythonAPI import *";
#endif
        
    TS_ASSERT_EQUALS(line, modline);
    
    //next line should be os import
    getline( is, line );
    TS_ASSERT_EQUALS(line, std::string("import os"));
    //next line should be string import
    getline( is, line );
    TS_ASSERT_EQUALS(line, std::string("import string"));

    getline( is, line );
    getline( is, line );

    TS_ASSERT_EQUALS(line, std::string("PYTHONAPIINMANTIDPLOT = False"));

    //next non-blank line should be API objects
    //eat blank lines
    while( getline(is, line) && ( line.empty() || line[0] == '#' ) )
    {
    }
    
    TS_ASSERT_EQUALS(line, "mantid = FrameworkManager()");
    getline(is, line);
    getline(is, line);
    TS_ASSERT_EQUALS(line, "Mantid = mantid");
    getline(is, line);
    TS_ASSERT_EQUALS(line, "mtd = mantid");
    getline(is, line);
    TS_ASSERT_EQUALS(line, "Mtd = mantid");

    //next non-blank line should be setWorkingDirectory()
    //eat blank lines
    while( getline(is, line) && line.empty() ) 
    {
    }
    
    getline(is, line);
    TS_ASSERT_EQUALS(line, "def setWorkingDirectory(path):");
    getline(is, line);
    TS_ASSERT_EQUALS(line, "\tos.chdir(path)");

    //next non-blank line should be help()
    //eat blank lines
    while( getline(is, line) && line.empty() ) 
    {
    }
    modline = "def mtdGlobalHelp():";
    getline(is, line);
    TS_ASSERT_EQUALS(line, modline);

    //skip over help
    while( getline(is, line) && !line.empty() ) 
      {
      }
    //it should be an algorithm definition but first is a comment line
    getline(is,line);
    getline(is,line);
    bool found(false);
    if( line.find("def") == 0 ) found = true;
    TS_ASSERT_EQUALS(found, true);

    getline(is, line);
    found = false;
    if( line.find("createAlgorithm") != std::string::npos ) found = true;
    TS_ASSERT_EQUALS(found, true);

    int setprop(0);
    while( getline(is, line) )
      {
	if( line.find("setPropertyValue") != std::string::npos )
	  ++setprop;
      }
    TS_ASSERT( setprop > 0 );

    is.close();
    // remove
    TS_ASSERT_THROWS_NOTHING( apimodule.remove() );
    TS_ASSERT( !apimodule.exists() );

  }

};


#endif //SIMPLEPYTHONAPITEST_H_
