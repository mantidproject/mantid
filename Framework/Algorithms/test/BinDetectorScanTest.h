#ifndef MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_
#define MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/BinDetectorScan.h"

#include "MantidAPI/Axis.h"
#include "MantidDataObjects/ScanningWorkspaceBuilder.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::BinDetectorScan;

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;

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

  MatrixWorkspace_sptr createTestScanningWS(size_t nTubes,
                                            size_t nPixelsPerTube,
                                            std::vector<double> rotations) {
    const auto instrument =
        ComponentCreationHelper::createInstrumentWithPSDTubes(
            nTubes, nPixelsPerTube, true);
    size_t nTimeIndexes = 3;
    size_t nBins = 1;

    const std::vector<std::pair<DateAndTime, DateAndTime>> timeRanges = {
        {0, 1}, {1, 2}, {2, 3}};

    ScanningWorkspaceBuilder builder(instrument, nTimeIndexes, nBins);
    builder.setTimeRanges(timeRanges);
    builder.setRelativeRotationsForScans(rotations, V3D(0, 0, 0), V3D(0, 1, 0));

    Points x(nBins, LinearGenerator(0.0, 1.0));
    Counts y(std::vector<double>(nBins, 2.0));
    builder.setHistogram(Histogram(x, y));

    auto testWS = builder.buildWorkspace();

    // This has to be added to the ADS so that it can be used with the string
    // validator used in the algorithm.
    AnalysisDataService::Instance().add("testWS", testWS);

    return testWS;
  }

  void verifyHappyPathCase(double expectedCounts = 2.0) {
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

    for (size_t i = 0; i < N_TUBES; ++i)
      for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
        TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[i], expectedCounts, 1e-6)
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

  void test_with_scanning_workspaces_detectors_at_same_positions() {
    std::vector<double> rotations = {0, 0, 0};
    auto testWS = createTestScanningWS(N_TUBES, N_PIXELS_PER_TUBE, rotations);

    BinDetectorScan alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    verifyHappyPathCase(6.0);

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_with_scanning_workspaces_detectors_rotated_in_overlapping_scan() {
    std::vector<double> rotations = {0, -22.5, -45};
    auto testWS = createTestScanningWS(N_TUBES, N_PIXELS_PER_TUBE, rotations);

    BinDetectorScan alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    // Check x-axis goes from -90 -> 0 with 180 bins
    const auto &xAxis = outWS->getAxis(0);
    TS_ASSERT_EQUALS(xAxis->length(), 7)
    TS_ASSERT_DELTA(xAxis->getValue(0), -90.0, 1e-6)
    TS_ASSERT_DELTA(xAxis->getValue(1), -67.5, 1e-6)
    TS_ASSERT_DELTA(xAxis->getValue(7 - 1), 45.0, 1e-6)

    // Check y-axis goes from 0 to 0.027 with 10 points
    const auto &yAxis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(yAxis->length(), N_PIXELS_PER_TUBE)
    TS_ASSERT_DELTA(yAxis->getValue(0), 0.0, 1e-6)
    TS_ASSERT_DELTA(yAxis->getValue(N_PIXELS_PER_TUBE - 1), 0.027, 1e-6)

    size_t bin = 0;
    for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
      TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[bin], 2.0, 1e-6)

    bin = 1;
    for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
      TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[bin], 4.0, 1e-6)

    for (size_t i = 2; i < 5; ++i)
      for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
        TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[i], 6.0, 1e-6)

    bin = 5;
    for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
      TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[bin], 4.0, 1e-6)

    bin = 6;
    for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
      TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[bin], 2.0, 1e-6)

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void
  test_with_scanning_workspaces_detectors_rotated_in_non_overlapping_scan() {
    std::vector<double> rotations = {0, -28.125, -45};
    auto testWS = createTestScanningWS(N_TUBES, N_PIXELS_PER_TUBE, rotations);

    BinDetectorScan alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    // Check x-axis goes from -90 -> 0 with 180 bins
    const auto &xAxis = outWS->getAxis(0);
    TS_ASSERT_EQUALS(xAxis->length(), 7)
    TS_ASSERT_DELTA(xAxis->getValue(0), -90.0, 1e-6)
    TS_ASSERT_DELTA(xAxis->getValue(1), -67.5, 1e-6)
    TS_ASSERT_DELTA(xAxis->getValue(7 - 1), 45.0, 1e-6)

    // Check y-axis goes from 0 to 0.027 with 10 points
    const auto &yAxis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(yAxis->length(), N_PIXELS_PER_TUBE)
    TS_ASSERT_DELTA(yAxis->getValue(0), 0.0, 1e-6)
    TS_ASSERT_DELTA(yAxis->getValue(N_PIXELS_PER_TUBE - 1), 0.027, 1e-6)

    size_t bin = 0;
    for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
      TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[bin], 2.0, 1e-6)

    bin = 1;
    for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
      TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[bin], 3.5, 1e-6)

    for (size_t i = 2; i < 5; ++i)
      for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
        TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[i], 6.0, 1e-6)

    bin = 5;
    for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
      TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[bin], 4.0, 1e-6)

    bin = 6;
    for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
      TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[bin], 2.5, 1e-6)

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }
};

#endif /* MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_ */
