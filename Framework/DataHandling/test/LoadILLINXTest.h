#ifndef LOADILLINXTEST_H_
#define LOADILLINXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadILLINX.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLINX;

class LoadILLINXTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLINXTest *createSuite() { return new LoadILLINXTest(); }
  static void destroySuite(LoadILLINXTest *suite) { delete suite; }

  void testName() {
    LoadILLINX loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadILLINX");
  }

  void testVersion() {
    LoadILLINX loader;
    TS_ASSERT_EQUALS(loader.version(), 1);
  }

  void testInit() {
    LoadILLINX loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  /*
   * This test only loads the Sample Data
   * The elastic peak is obtained on the fly from the sample data.
   */
  void loadDataFile(const std::string dataFile, const int numberOfHistograms) {
    LoadILLINX loader;
    loader.initialize();
    loader.setPropertyValue("Filename", dataFile);

    std::string outputSpace = "LoadILLINXTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);
    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), numberOfHistograms);

    AnalysisDataService::Instance().clear();
  }

  void test_IN4_load() { loadDataFile("ILL/IN4/084446.nxs", 397); }
  void test_IN5_load() { loadDataFile("ILL/IN5/104007.nxs", 98305); }
  void test_IN6_load() { loadDataFile("ILL/IN6/164192.nxs", 340); }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadILLINXTestPerformance : public CxxTest::TestSuite {
public:
  LoadILLINXTestPerformance() : m_dataFile("ILL/IN5/104007.nxs") {}

  void testDefaultLoad() {
    Mantid::DataHandling::LoadILLINX loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }

private:
  std::string m_dataFile;
};

#endif /*LoadILLINXTEST_H_*/
