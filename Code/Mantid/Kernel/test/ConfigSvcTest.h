#ifndef MANTID_CONFIGSVCTEST_H_
#define MANTID_CONFIGSVCTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/ConfigSvc.h"
#include "../inc/Logger.h"
#include <string>
#include <iostream>

using namespace Mantid::Kernel;

class ConfigSvcTest : public CxxTest::TestSuite
{
public: 

  ConfigSvcTest()
  {
	  configSvc = ConfigSvc::Instance();
	  configSvc->loadConfig("MantidTest.properties");
  }

  void testLogging()
  {
	  //attempt some logging
	  Logger& log1 = Logger::get("logTest");

	  TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string"));
	  TS_ASSERT_THROWS_NOTHING(log1.information("an information string"));
	  TS_ASSERT_THROWS_NOTHING(log1.warning("a warning string"));
	  TS_ASSERT_THROWS_NOTHING(log1.error("an error string"));
	  TS_ASSERT_THROWS_NOTHING(log1.critical("a critical string"));
	  TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string"));

	  //checking the level - this should be set to debug in the config file
	  //therefore this should only return false for debug
	  TS_ASSERT(log1.is(Logger::PRIO_DEBUG) == false); //debug
	  TS_ASSERT(log1.is(Logger::PRIO_INFORMATION)); //information
	  TS_ASSERT(log1.is(Logger::PRIO_WARNING)); //warning
	  TS_ASSERT(log1.is(Logger::PRIO_ERROR)); //error
	  TS_ASSERT(log1.is(Logger::PRIO_CRITICAL)); //critical
	  TS_ASSERT(log1.is(Logger::PRIO_FATAL)); //fatal

  }

  void TestSystemValues()
  {
	  //we cannot test the return values here as they will differ based on the environment.
	  //therfore we will just check they return a non empty string.
	  std::string osName = configSvc->getOSName();
	  TS_ASSERT_LESS_THAN(0, osName.length()); //check that the string is not empty
	  std::string osArch = configSvc->getOSArchitecture();
	  TS_ASSERT_LESS_THAN(0, osArch.length()); //check that the string is not empty
	  std::string osCompName = configSvc->getComputerName();
	  TS_ASSERT_LESS_THAN(0, osCompName.length()); //check that the string is not empty
  }

  void TestCustomProperty()
  {
	  //Mantid.legs is defined in the properties script as 6
	  std::string legCountString = configSvc->getString("mantid.legs");
	  TS_ASSERT_EQUALS(legCountString, "6");
  }

   void TestCustomPropertyAsValue()
  {
	  //Mantid.legs is defined in the properties script as 6
	  int value;
	  int retVal = configSvc->getValue("mantid.legs",value);
	  double dblValue;
	  retVal = configSvc->getValue("mantid.legs",dblValue);
 std::cerr << std::endl << "mantid.legs=" << dblValue << std::endl;

	  TS_ASSERT_EQUALS(value, 6);
	  TS_ASSERT_EQUALS(dblValue, 6.0);
  }

  void TestMissingProperty()
  {
	  //Mantid.noses is not defined in the properties script 
	  TS_ASSERT_THROWS_ANYTHING( std::string legCountString = configSvc->getString("mantid.noses"));
  }
  
 
private:
	ConfigSvc *configSvc;
};

#endif /*MANTID_CONFIGSVCTEST_H_*/
