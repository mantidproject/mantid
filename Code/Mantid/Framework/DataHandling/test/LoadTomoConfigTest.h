#ifndef LOADTOMOCONFIGTEST_H_
#define LOADTOMOCONFIGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidDataHandling/LoadTomoConfig.h" 

#include <Poco/File.h>

using Mantid::DataHandling::LoadTomoConfig;

class LoadTomoConfigTest : public CxxTest::TestSuite
{
public: 

  /// Tests casting, general algorithm properties: name, version, etc.
  void test_algorithm()
  {
    Mantid::API::IAlgorithm_sptr testAlg =
      Mantid::API::AlgorithmManager::Instance().create("LoadTomoConfig" /*, 1*/);
    TS_ASSERT(testAlg);
    if (!testAlg->isInitialized())
      testAlg->initialize();
    TS_ASSERT_EQUALS(testAlg->version(), 1 );
    TS_ASSERT_EQUALS(testAlg->name(), "LoadTomoConfig" );
    
  }

  void test_wrongExec()
  {
    Mantid::API::IAlgorithm_sptr testAlg =
      Mantid::API::AlgorithmManager::Instance().create("LoadTomoConfig" /*, 1*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    // exec without filename -> should throw
    TS_ASSERT_THROWS(testAlg->execute(), std::runtime_error);
    // try to set empty filename
    TS_ASSERT_THROWS(testAlg->setPropertyValue("Filename",""), std::invalid_argument);
  }

  void test_init()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  // one file with errors/unrecognized content
  void test_wrongContentsFile()
  {

  }

  // one example file that should load fine
  void test_loadOK()
  {
    //TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", m_filename));

    //if (!alg->isInitialized())
    //  alg->initialize();
    //TS_ASSERT(alg->isInitialized());

    //TS_ASSERT_THROWS_NOTHING(alg.execute());
    //TS_ASSERT(alg.isExecuted());
  }

private:

  LoadTomoConfig alg;
  std::string m_filename;
};

#endif /* LOADTOMOCONFIGTEST_H__*/
