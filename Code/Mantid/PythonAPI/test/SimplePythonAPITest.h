#ifndef SIMPLEPYTHONAPITEST_H_
#define SIMPLEPYTHONAPITEST_H_

//--------------------------------
// Includes
//--------------------------------
#include <fstream>
#include <cxxtest/TestSuite.h>
#include "boost/filesystem.hpp"

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
    SimplePythonAPI::createModule();
    std::string modname = SimplePythonAPI::getModuleName();
    //has it been written ?
    TS_ASSERT(boost::filesystem::exists(modname));

    //does it contain what we expect
    std::ifstream is(modname.c_str());
    //first line should import main Python API
    std::string modline("");
#ifdef _WIN32
    modline = "from MantidPythonAPI import FrameworkManager";
#else
    modline = "from libMantidPythonAPI import FrameworkManager";
#endif
    
    std::string line;
    getline( is, line );
    TS_ASSERT_EQUALS(line, modline);
    
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
    boost::filesystem::remove_all(modname);

  }

};


#endif //SIMPLEPYTHONAPITEST_H_
