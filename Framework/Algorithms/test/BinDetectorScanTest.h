#ifndef MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_
#define MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/BinDetectorScan.h"

#include "MantidAPI/Axis.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
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

  const size_t N_TUBES = 5;
  const size_t N_PIXELS_PER_TUBE = 10;

  MatrixWorkspace_sptr createTestWS(size_t nTubes, size_t nPixelsPerTube) {
    const size_t nSpectra = nTubes * nPixelsPerTube;
    const size_t nBins = 1;

    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(int(nSpectra),
                                                         int(nBins));

    testWS->setInstrument(ComponentCreationHelper::createInstrumentWithPSDTubes(
        nTubes, nPixelsPerTube, true));

    // This has to be added to the ADS so that it can be used with the string
    // validator used in the algorithm.
    AnalysisDataService::Instance().add("testWS", testWS);

    return testWS;
  }

  void verifyHappyPathCase() {
    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    // Check x-axis goes from -90 -> 0 with 180 bins
    const auto &xAxis = outWS->getAxis(0);
    TS_ASSERT_EQUALS(xAxis->length(), N_TUBES)
    TS_ASSERT_DELTA(xAxis->getValue(0), -90.0, 1e-6)
    TS_ASSERT_DELTA(xAxis->getValue(1), -67.5, 1e-6)
    TS_ASSERT_DELTA(xAxis->getValue(N_TUBES - 1), 0.0, 1e-6)

    // Check y-axis goes from 0 to 0.027 with 10 points
    const auto &yAxis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(yAxis->length(), N_PIXELS_PER_TUBE)
    TS_ASSERT_DELTA(yAxis->getValue(0), 0.0, 1e-6)
    TS_ASSERT_DELTA(yAxis->getValue(N_PIXELS_PER_TUBE - 1), 0.027, 1e-6)

    for (size_t i = 0; i < N_TUBES; ++i) {
      for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j) {
        TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[i], 2.0, 1e-6)
      }
    }
  }

  void test_normal_operation_with_component_specified() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    BinDetectorScan alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    verifyHappyPathCase();

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_normal_operation_explicit_height_axis() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    BinDetectorScan alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("HeightBinning", "0.0, 0.003, 0.027");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    verifyHappyPathCase();

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_normal_operation_manual_scattering_angle_bins() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    BinDetectorScan alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "-90.0, 22.5, 0.0");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    verifyHappyPathCase();

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_non_existent_component() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    BinDetectorScan alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "not_a_component");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error & e,
                            std::string(e.what()),
                            "Component not_a_component could not be found.");
    AnalysisDataService::Instance().remove("testWS");
  }

  void test_non_incomplete_height_bins_component() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    BinDetectorScan alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("HeightBinning", "0.003");
    TS_ASSERT_THROWS_EQUALS(
        alg.execute(), std::runtime_error & e, std::string(e.what()),
        "Currently height binning must have start, step and end values.");
    AnalysisDataService::Instance().remove("testWS");
  }
};

#endif /* MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_ */
