#ifndef LOADMLZTEST_H_
#define LOADMLZTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadMLZ.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadMLZ;

class LoadMLZTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMLZTest *createSuite() { return new LoadMLZTest(); }
  static void destroySuite(LoadMLZTest *suite) { delete suite; }

  LoadMLZTest() : m_dataFile("TOFTOFTestdata.nxs") {}

  void testName() {
    LoadMLZ loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadMLZ");
  }

  void testVersion() {
    LoadMLZ loader;
    TS_ASSERT_EQUALS(loader.version(), 1);
  }

  void testInit() {
    LoadMLZ loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  /*
   * This test only loads the Sample Data
   */
  void testExecJustSample() {
    LoadMLZ loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);

    std::string outputSpace = "LoadMLZTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);
    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 1006); // to check

    AnalysisDataService::Instance().clear();
  }

private:
  std::string m_dataFile;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMLZTestPerformance : public CxxTest::TestSuite {
public:
  LoadMLZTestPerformance() : m_dataFile("TOFTOFTestdata.nxs") {}

  void testDefaultLoad() {
    Mantid::DataHandling::LoadMLZ loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }

private:
  std::string m_dataFile;
};

#endif /*LoadMLZTEST_H_*/
