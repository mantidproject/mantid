#ifndef LOADILLTEST_H_
#define LOADILLTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;

class LoadILLTest : public CxxTest::TestSuite
{
public:
  LoadILLTest():testFile("C:/Users/hqs74821/Work/Mantid_stuff/ILL/068288.nxs"){}
  void testName()
  {
    TS_ASSERT_EQUALS( loader.name(), "LoadILL" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( loader.version(), 1 );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( loader.initialize() );
    TS_ASSERT( loader.isInitialized() );
  }

  void testFileCheck()
  {
    std::cerr << loader.fileCheck(testFile);
  }

  void testExec()
  {
    loader.setPropertyValue("Filename",testFile);
    loader.setPropertyValue("OutputWorkspace","out");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );

    AnalysisDataService::Instance().clear();
  }

private:
  Mantid::DataHandling::LoadILL loader;
  std::string testFile;
};

#endif /*LoadILLTEST_H_*/
