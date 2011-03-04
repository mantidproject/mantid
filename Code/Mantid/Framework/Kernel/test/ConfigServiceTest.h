#ifndef MANTID_CONFIGSERVICETEST_H_
#define MANTID_CONFIGSERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TestChannel.h"
#include <Poco/Path.h>
#include <Poco/File.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include <fstream>

#include <Poco/NObserver.h>

using namespace Mantid::Kernel;
using Mantid::TestChannel;

class ConfigServiceTest : public CxxTest::TestSuite
{
public: 

  void testLogging()
  {
    //attempt some logging
    Logger& log1 = Logger::get("logTest");

    TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string"));
    TS_ASSERT_THROWS_NOTHING(log1.information("an information string"));
    TS_ASSERT_THROWS_NOTHING(log1.information("a notice string"));
    TS_ASSERT_THROWS_NOTHING(log1.warning("a warning string"));
    TS_ASSERT_THROWS_NOTHING(log1.error("an error string"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string"));

    TS_ASSERT_THROWS_NOTHING(
    log1.fatal()<<"A fatal message from the stream operators " << 4.5 << std::endl;
    log1.error()<<"A error message from the stream operators " << -0.2 << std::endl;
    log1.warning()<<"A warning message from the stream operators " << 999.99 << std::endl;
    log1.notice()<<"A notice message from the stream operators " << 0.0 << std::endl;
    log1.information()<<"A information message from the stream operators " << -999.99 << std::endl;
    log1.debug()<<"A debug message from the stream operators " << 5684568 << std::endl;


    );

    //checking the level - this should be set to debug in the config file
    //therefore this should only return false for debug
    TS_ASSERT(log1.is(Logger::PRIO_DEBUG) == false); //debug
    TS_ASSERT(log1.is(Logger::PRIO_INFORMATION)); //information
    TS_ASSERT(log1.is(Logger::PRIO_NOTICE)); //information
    TS_ASSERT(log1.is(Logger::PRIO_WARNING)); //warning
    TS_ASSERT(log1.is(Logger::PRIO_ERROR)); //error
    TS_ASSERT(log1.is(Logger::PRIO_FATAL)); //fatal
  }

  void testEnabled()
  {
    //attempt some logging
    Logger& log1 = Logger::get("logTestEnabled");
    TS_ASSERT(log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with enabled=true"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"A fatal message from the stream operators with enabled=true " << 4.5 << std::endl;);
    
    TS_ASSERT_THROWS_NOTHING(log1.setEnabled(false));
    TS_ASSERT(!log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("YOU SHOULD NEVER SEE THIS"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"YOU SHOULD NEVER SEE THIS VIA A STREAM" << std::endl;);
    
    TS_ASSERT_THROWS_NOTHING(log1.setEnabled(true));
    TS_ASSERT(log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("you are allowed to see this"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"you are allowed to see this via a stream" << std::endl;);

  }

  void testChangeName()
  {
    //attempt some logging
    Logger& log1 = Logger::get("logTestName1");
    TS_ASSERT_THROWS_NOTHING(log1.error("This should be from logTestName1"));
    TS_ASSERT_THROWS_NOTHING(log1.error()<<"This should be from logTestName1 via a stream" << std::endl;);
    
    TS_ASSERT_THROWS_NOTHING(log1.setName("logTestName2"));
    TS_ASSERT_THROWS_NOTHING(log1.error("This should be from logTestName2"));
    TS_ASSERT_THROWS_NOTHING(log1.error()<<"This should be from logTestName2 via a stream" << std::endl;);
    
    TS_ASSERT_THROWS_NOTHING(log1.setName("logTestName1"));
    TS_ASSERT_THROWS_NOTHING(log1.error("This should be from logTestName1"));
    TS_ASSERT_THROWS_NOTHING(log1.error()<<"This should be from logTestName1 via a stream" << std::endl;);
    
  }

  void TestSystemValues()
  {
    //we cannot test the return values here as they will differ based on the environment.
    //therfore we will just check they return a non empty string.
    std::string osName = ConfigService::Instance().getOSName();
    TS_ASSERT_LESS_THAN(0, osName.length()); //check that the string is not empty
    std::string osArch = ConfigService::Instance().getOSArchitecture();
    TS_ASSERT_LESS_THAN(0, osArch.length()); //check that the string is not empty
    std::string osCompName = ConfigService::Instance().getComputerName();
    TS_ASSERT_LESS_THAN(0, osCompName.length()); //check that the string is not empty
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getOSVersion().length()); //check that the string is not empty
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getCurrentDir().length()); //check that the string is not empty
//	  TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getHomeDir().length()); //check that the string is not empty
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getTempDir().length()); //check that the string is not empty
  }

  void TestCustomProperty()
  {
    //Mantid.legs is defined in the properties script as 6
    std::string countString = ConfigService::Instance().getString("ManagedWorkspace.DataBlockSize");
    TS_ASSERT_EQUALS(countString, "4000");
  }

   void TestCustomPropertyAsValue()
  {
    //Mantid.legs is defined in the properties script as 6
    int value = 0;
    int retVal = ConfigService::Instance().getValue("ManagedWorkspace.DataBlockSize",value);
    double dblValue = 0;
    retVal = ConfigService::Instance().getValue("ManagedWorkspace.DataBlockSize",dblValue);

    TS_ASSERT_EQUALS(value, 4000);
    TS_ASSERT_EQUALS(dblValue, 4000.0);
  }

  void TestMissingProperty()
  {
    //Mantid.noses is not defined in the properties script 
    std::string noseCountString = ConfigService::Instance().getString("mantid.noses");
    //this should return an empty string

    TS_ASSERT_EQUALS(noseCountString, "");
  }

  void TestRelativeToAbsolute()
  {
    std::string path = ConfigService::Instance().getString("defaultsave.directory");
    TS_ASSERT( Poco::Path(path).isAbsolute() );
  } 

  void TestAppendProperties()
  {

    //This should clear out all old properties
    const std::string propfilePath = getDirectoryOfExecutable();
    const std::string propfile = propfilePath + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);
    //this should return an empty string
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "");
    //this should pass
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "6");
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

    //This should append a new properties file properties
    ConfigService::Instance().updateConfig(propfilePath+"MantidTest.user.properties",true);
    //this should now be valid
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "5");
    //this should have been overridden
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "76");
    //this should have been left alone
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

    //This should clear out all old properties
    ConfigService::Instance().updateConfig(propfile);
    //this should return an empty string
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "");
    //this should pass
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "6");
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

  }

  void testSaveConfigCleanFile()
  {
    const std::string propfile = getDirectoryOfExecutable() + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    const std::string filename("user.settings");
  
    // save any previous changed settings to make sure we're on a clean slate
    ConfigService::Instance().saveConfig(filename);
    
    Poco::File prop_file(filename);
    // Start with a clean state
    if( prop_file.exists() ) prop_file.remove();

    ConfigServiceImpl& settings = ConfigService::Instance();
    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));
    
    // No changes yet, so no file
    TS_ASSERT_EQUALS(prop_file.exists(), false);

    runSaveTest(filename);
  }

  void testSaveConfigExistingSettings()
  {
    const std::string filename("user.settings");
    Poco::File prop_file(filename);
    if( prop_file.exists() ) prop_file.remove();

    ConfigServiceImpl& settings = ConfigService::Instance();
    
    std::ofstream writer(filename.c_str(),std::ios_base::trunc);
    writer << "mantid.legs = 6";
    writer.close();

    runSaveTest(filename);
  }

  void testSaveConfigWithLineContinuation()
  {
    const std::string filename("user.settings");
    Poco::File prop_file(filename);
    if( prop_file.exists() ) prop_file.remove();

    ConfigServiceImpl& settings = ConfigService::Instance();
    
    std::ofstream writer(filename.c_str(),std::ios_base::trunc);
    writer << 
      "mantid.legs=6\n\n"
      "search.directories=/test1;\\\n"
      "/test2;/test3;\\\n"
      "/test4\n";
    writer.close();

    TS_ASSERT_THROWS_NOTHING(settings.setString("mantid.legs", "10"));

    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));
    // Should exist
    TS_ASSERT_EQUALS(prop_file.exists(), true);

    // Test the entry
    std::ifstream reader(filename.c_str(), std::ios::in);
    if( reader.bad() )
    {
      TS_FAIL("Unable to open config file for saving");
    }
    std::string line("");
    std::map<int, std::string> prop_lines;
    int line_index(0);
    while(getline(reader, line))
    {
      prop_lines.insert(std::make_pair(line_index, line));
      ++line_index;
    }
    reader.close();

    TS_ASSERT_EQUALS(prop_lines.size(), 5);
    TS_ASSERT_EQUALS(prop_lines[0], "mantid.legs=10");
    TS_ASSERT_EQUALS(prop_lines[1], "");
    TS_ASSERT_EQUALS(prop_lines[2], "search.directories=/test1;\\");
    TS_ASSERT_EQUALS(prop_lines[3], "/test2;/test3;\\");
    TS_ASSERT_EQUALS(prop_lines[4], "/test4");

    // Clean up
    prop_file.remove();
  }

  // Test that the ValueChanged notification is sent
  void testNotifications()
  {
    Poco::NObserver<ConfigServiceTest, ConfigServiceImpl::ValueChanged> changeObserver(*this, &ConfigServiceTest::handleConfigChange);
    m_valueChangedSent = false;

    ConfigServiceImpl& settings = ConfigService::Instance();

    TS_ASSERT_THROWS_NOTHING(settings.addObserver(changeObserver));

    settings.setString("default.facility", "SNS");

    TS_ASSERT(m_valueChangedSent);
    TS_ASSERT_EQUALS(m_key, "default.facility");
    TS_ASSERT_DIFFERS(m_preValue, m_curValue);
    TS_ASSERT_EQUALS(m_curValue, "SNS");

    // Need to set back to ISIS for later tests
    settings.setString("default.facility", "ISIS");
  }



protected:
  bool m_valueChangedSent;
  std::string m_key;
  std::string m_preValue;
  std::string m_curValue;
  void handleConfigChange(const Poco::AutoPtr<Mantid::Kernel::ConfigServiceImpl::ValueChanged>& pNf)
  {
    m_valueChangedSent = true;
    m_key = pNf->key();
    m_preValue = pNf->preValue();
    m_curValue = pNf->curValue();
  }

private:
  void runSaveTest(const std::string& filename)
  {
    ConfigServiceImpl& settings = ConfigService::Instance();
    // Make a change and save again
    std::string key("mantid.legs");
    std::string value("10");
    TS_ASSERT_THROWS_NOTHING(settings.setString(key, value));
    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));

    // Should exist
    Poco::File prop_file(filename);
    TS_ASSERT_EQUALS(prop_file.exists(), true);

    // Test the entry
    std::ifstream reader(filename.c_str(), std::ios::in);
    if( reader.bad() )
    {
      TS_FAIL("Unable to open config file for saving");
    }
    std::string line("");
    while(std::getline(reader, line))
    {
      if( line.empty() ) continue;
      else break;
    }
    reader.close();

    std::string key_value = key + "=" + value;
    TS_ASSERT_EQUALS(line, key_value);

    // Clean up
    prop_file.remove();

  }
  
};

#endif /*MANTID_CONFIGSERVICETEST_H_*/
