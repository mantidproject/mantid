#ifndef MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_
#define MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/BinDetectorScan.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::BinDetectorScan;

using namespace Mantid::API;

class BinDetectorScanTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinDetectorScanTest *createSuite() {
    return new BinDetectorScanTest();
  }
  static void destroySuite(BinDetectorScanTest *suite) { delete suite; }

  void test_normal_operation() {
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            2, 10, 1);
    AnalysisDataService::Instance().add("testWS", testWS);

    BinDetectorScan alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "0.5");
    alg.setProperty("ComponentForHeightAxis", "bank1");
    //    alg.setProperty("HeightBinning", "-1.0, 0.1, 1.0");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }
};

#endif /* MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_ */
