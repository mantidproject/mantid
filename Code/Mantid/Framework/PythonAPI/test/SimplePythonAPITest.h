#ifndef SIMPLEPYTHONAPITEST_H_
#define SIMPLEPYTHONAPITEST_H_

//--------------------------------
// Includes
//--------------------------------
#include <fstream>
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>

#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/LibraryManager.h"

class SimplePythonAPITest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SimplePythonAPITest *createSuite() { return new SimplePythonAPITest(); }
  static void destroySuite( SimplePythonAPITest *suite ) { delete suite; }

  SimplePythonAPITest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void testCreateModule()
  {
    using namespace Mantid::PythonAPI;
    //first call the function to create the module file
    SimplePythonAPI::createModule(false);
    Poco::File apimodule(SimplePythonAPI::getModuleFilename());
    //has it been written ?
    TS_ASSERT(apimodule.exists());

    //does it contain what we expect
    std::ifstream is(apimodule.path().c_str());

    std::string line;
    getline(is, line);
    //TS_ASSERT_EQUALS(line, std::string("from MantidFramework import mtd, _makeString"));
    //getline(is, line);

    //next line should be os import
    getline( is, line );
    TS_ASSERT_EQUALS(line, std::string("import os"));
    //next line should be sys import
    getline( is, line );
    TS_ASSERT_EQUALS(line, std::string("import sys"));
    //next line should be string import
    getline( is, line );
    TS_ASSERT_EQUALS(line, std::string("import string"));

    //next non-blank line should be help()
    //eat blank lines
    while( getline(is, line) && line.empty() ) 
    {
    }
    
    TS_ASSERT_EQUALS(line, std::string("def numberRows(descr, fw):"))
    while( getline(is, line) && !line.empty() ) 
    {
    }

    getline(is, line);
    getline(is, line);

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

    // Now the definition
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
