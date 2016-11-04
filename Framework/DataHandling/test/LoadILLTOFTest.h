#ifndef LOADILLTOFTEST_H_
#define LOADILLTOFTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadILLTOF.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLTOF;

class LoadILLTOFTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLTOFTest *createSuite() { return new LoadILLTOFTest(); }
  static void destroySuite(LoadILLTOFTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testName() {
    LoadILLTOF loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadILLTOF");
  }

  void testVersion() {
    LoadILLTOF loader;
    TS_ASSERT_EQUALS(loader.version(), 1);
  }

  void testInit() {
    LoadILLTOF loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  /*
   * This test only loads the Sample Data
   * The elastic peak is obtained on the fly from the sample data.
   */
  MatrixWorkspace_sptr loadDataFile(const std::string dataFile,
                                    const int numberOfHistograms) {
    LoadILLTOF loader;
    loader.initialize();
    loader.setPropertyValue("Filename", dataFile);

    std::string outputSpace = "LoadILLTOFTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);
    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), numberOfHistograms);

    // Check all detectors have a defined detector ID >= 0

    Mantid::detid2index_map detectorMap;
    TS_ASSERT_THROWS_NOTHING(detectorMap = output->getDetectorIDToWorkspaceIndexMap(true));

    // Check all detectors have a unique detector ID
    TS_ASSERT_EQUALS(detectorMap.size(), output->getNumberHistograms());


    for (auto value : detectorMap) {
      TS_ASSERT(value.first >= 0);
    }

    return output2D;
  }

  void test_IN4_load() {
    MatrixWorkspace_sptr ws = loadDataFile("ILL/IN4/084446.nxs", 397);

    double pulseInterval = ws->run().getLogAsSingleValue("pulse_interval");
    TS_ASSERT_DELTA(0.003, pulseInterval, 1e-10);
  }

  void test_IN5_load() { loadDataFile("ILL/IN5/104007.nxs", 98305); }

  void test_IN6_load() {
    MatrixWorkspace_sptr ws = loadDataFile("ILL/IN6/164192.nxs", 340);

    double pulseInterval = ws->run().getLogAsSingleValue("pulse_interval");
    TS_ASSERT_DELTA(0.0060337892, pulseInterval, 1e-10);
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadILLTOFTestPerformance : public CxxTest::TestSuite {
public:
  LoadILLTOFTestPerformance() : m_dataFile("ILL/IN5/104007.nxs") {}

  void testDefaultLoad() {
    Mantid::DataHandling::LoadILLTOF loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }

private:
  std::string m_dataFile;
};

#endif /*LOADILLTOFTEST_H_*/
