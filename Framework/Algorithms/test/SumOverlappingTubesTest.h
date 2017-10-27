#ifndef MANTID_ALGORITHMS_SUMOVERLAPPINGTUBESTEST_H_
#define MANTID_ALGORITHMS_SUMOVERLAPPINGTUBESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SumOverlappingTubes.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/ScanningWorkspaceBuilder.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTypes/Core/DateAndTime.h"

using Mantid::Algorithms::SumOverlappingTubes;

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Indexing;
using namespace Mantid::Kernel;
using namespace Mantid::Types::Core;

class SumOverlappingTubesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SumOverlappingTubesTest *createSuite() {
    return new SumOverlappingTubesTest();
  }
  static void destroySuite(SumOverlappingTubesTest *suite) { delete suite; }

  const size_t N_TUBES = 5;
  const size_t N_PIXELS_PER_TUBE = 10;

  MatrixWorkspace_sptr createTestWS(size_t nTubes, size_t nPixelsPerTube) {
    const size_t nSpectra = nTubes * nPixelsPerTube;
    const size_t nBins = 1;

    MatrixWorkspace_sptr testWS = create<Workspace2D>(
        ComponentCreationHelper::createInstrumentWithPSDTubes(
            nTubes, nPixelsPerTube, true),
        IndexInfo(nSpectra),
        Histogram(BinEdges(nBins + 1, LinearGenerator(0.0, 1.0)),
                  Counts(nBins, 2.0)));

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

  void verifySuccessCase(double expectedCounts = 2.0) {
    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    verifyScatteringAngleAxis(outWS, N_TUBES);
    verifyHeightAxis(outWS);
    verifySpectraHaveSameCounts(outWS, expectedCounts);
  }

  void verifyScatteringAngleAxis(const MatrixWorkspace_sptr &outWS,
                                 const size_t nEntries) {
    // Check x-axis goes from -90 -> 0 with 180 bins
    const auto &xAxis = outWS->getAxis(0);
    TS_ASSERT_EQUALS(xAxis->length(), nEntries)
    for (size_t i = 0; i < N_TUBES; ++i)
      TS_ASSERT_DELTA(xAxis->getValue(i), -90.0 + 22.5 * double(i), 1e-6)
  }

  void verifyHeightAxis(const MatrixWorkspace_sptr &outWS) {
    // Check y-axis goes from 0 to 0.027 with 10 points
    const auto &yAxis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(yAxis->length(), N_PIXELS_PER_TUBE)
    for (size_t i = 0; i < N_PIXELS_PER_TUBE; ++i)
      TS_ASSERT_DELTA(yAxis->getValue(i), 0.003 * double(i), 1e-6)
  }

  void verifySpectraHaveSameCounts(MatrixWorkspace_sptr outWS,
                                   double expectedCounts = 2.0) {
    for (size_t i = 0; i < N_TUBES; ++i)
      for (size_t j = 0; j < N_PIXELS_PER_TUBE; ++j)
        TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[i], expectedCounts, 1e-6)
  }

  void verifySpectraCountsForScan(MatrixWorkspace_sptr outWS) {
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
  }

  void test_normal_operation_with_component_specified() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    verifySuccessCase();

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_normal_operation_explicit_height_axis() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("HeightAxis", "0.0, 0.003, 0.027");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    verifySuccessCase();

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_normal_operation_manual_scattering_angle_bins() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "-90.0, 22.5, 0.0");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    verifySuccessCase();

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_non_existent_component() {
    auto testWS = createTestWS(N_TUBES, N_PIXELS_PER_TUBE);

    SumOverlappingTubes alg;
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

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("HeightAxis", "0.003");
    TS_ASSERT_THROWS_EQUALS(
        alg.execute(), std::runtime_error & e, std::string(e.what()),
        "Height binning must have start, step and end values.");
    AnalysisDataService::Instance().remove("testWS");
  }

  void test_with_scanning_workspaces_detectors_at_same_positions() {
    std::vector<double> rotations = {0, 0, 0};
    auto testWS = createTestScanningWS(N_TUBES, N_PIXELS_PER_TUBE, rotations);

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    verifySuccessCase(6.0);

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_with_scanning_workspaces_detectors_rotated_in_overlapping_scan() {
    std::vector<double> rotations = {0, -22.5, -45};
    auto testWS = createTestScanningWS(N_TUBES, N_PIXELS_PER_TUBE, rotations);

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    alg.setProperty("Normalise", false);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    verifyScatteringAngleAxis(outWS, N_TUBES + 2);
    verifyHeightAxis(outWS);
    verifySpectraCountsForScan(outWS);

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void
  test_with_scanning_workspaces_detectors_rotated_in_non_overlapping_scan() {
    std::vector<double> rotations = {0, -28.125, -45};
    auto testWS = createTestScanningWS(N_TUBES, N_PIXELS_PER_TUBE, rotations);

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    alg.setProperty("Normalise", false);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    verifyScatteringAngleAxis(outWS, N_TUBES + 2);
    verifyHeightAxis(outWS);

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

  void
  test_with_scanning_workspaces_detectors_rotated_in_non_overlapping_scan_with_large_tolerance() {
    std::vector<double> rotations = {0, -22.5, -45};
    auto testWS = createTestScanningWS(N_TUBES, N_PIXELS_PER_TUBE, rotations);

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    alg.setProperty("ScatteringAngleTolerance", "0.3");
    alg.setProperty("Normalise", false);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    verifyScatteringAngleAxis(outWS, N_TUBES + 2);
    verifyHeightAxis(outWS);
    verifySpectraCountsForScan(outWS);

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }

  void
  test_with_scanning_workspaces_detectors_rotated_in_non_overlapping_scan_with_normalisation() {
    std::vector<double> rotations = {0, -28.125, -45};
    auto testWS = createTestScanningWS(N_TUBES, N_PIXELS_PER_TUBE, rotations);

    SumOverlappingTubes alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    verifyScatteringAngleAxis(outWS, N_TUBES + 2);
    verifyHeightAxis(outWS);

    verifySpectraHaveSameCounts(outWS, 6.0);

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS");
  }
};

#endif /* MANTID_ALGORITHMS_SUMOVERLAPPINGTUBESTEST_H_ */
