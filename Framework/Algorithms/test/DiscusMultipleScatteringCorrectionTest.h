// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Axis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DiscusMultipleScatteringCorrection.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class DiscusMultipleScatteringCorrectionHelper : public Mantid::Algorithms::DiscusMultipleScatteringCorrection {
public:
  double interpolateSquareRoot(const Mantid::HistogramData::Histogram &histToInterpolate, double x) {
    return DiscusMultipleScatteringCorrection::interpolateSquareRoot(histToInterpolate, x);
  }
  void updateTrackDirection(Mantid::Geometry::Track &track, const double cosT, const double phi) {
    DiscusMultipleScatteringCorrection::updateTrackDirection(track, cosT, phi);
  }
  void integrateCumulative(const Mantid::HistogramData::Histogram &h, double xmax, std::vector<double> &resultX,
                           std::vector<double> &resultY) {
    DiscusMultipleScatteringCorrection::integrateCumulative(h, xmax, resultX, resultY);
  }
};

class DiscusMultipleScatteringCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiscusMultipleScatteringCorrectionTest *createSuite() { return new DiscusMultipleScatteringCorrectionTest(); }
  static void destroySuite(DiscusMultipleScatteringCorrectionTest *suite) { delete suite; }

  DiscusMultipleScatteringCorrectionTest() {
    const int NBINS = 1;
    // wavelength = 1 in most tests, so k = 2 * M_PI. q max = 2k
    SofQWorkspace = WorkspaceCreationHelper::create2DWorkspaceBinned(1, NBINS, 0., 4 * M_PI / NBINS);
    for (size_t i = 0; i < SofQWorkspace->blocksize(); i++)
      SofQWorkspace->mutableY(0)[i] = 1.;
    SofQWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  }

  void test_testSQDeltaFunction() {
    const double THICKNESS = 0.001; // metres

    const int NTHETA = 900;
    auto inputWorkspace = SetupFlatPlateWorkspace(1, NTHETA, 0.2, 1, THICKNESS);

    auto SofQWorkspace = WorkspaceCreationHelper::create2DWorkspace(1, 3);
    SofQWorkspace->mutableX(0)[0] = 4.985;
    SofQWorkspace->mutableX(0)[1] = 4.995;
    SofQWorkspace->mutableX(0)[2] = 5.005;
    SofQWorkspace->mutableX(0)[3] = 5.015;
    // S(Q) zero everywhere apart from spike at Q=5
    SofQWorkspace->mutableY(0)[0] = 0.;
    SofQWorkspace->mutableY(0)[1] = 100.;
    SofQWorkspace->mutableY(0)[2] = 0.;
    SofQWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");

    auto alg = std::make_shared<Mantid::Algorithms::DiscusMultipleScatteringCorrection>();
    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SofqWorkspace", SofQWorkspace));
    alg->setPropertyValue("OutputWorkspace", "MuscatResults");
    // input workspace has single bin - centred at 1 Angstrom, so kinc=2*pi=6.28 inverse Angstroms
    // DiscusMultipleScatteringCorrection will sample q between 0 and 2k (12.56)
    // so q=5 requires sin(theta) = 5 /(4*pi) = 0.39789, theta=23.44 degrees, 2theta=46.88 degrees
    // So two scatters at max S(Q) will take the track to ~93.76 degrees
    alg->setProperty("InputWorkspace", inputWorkspace);
    const int NSCATTERINGS = 2;
    alg->setProperty("NumberScatterings", NSCATTERINGS);
    alg->setProperty("NeutronPathsSingle", 10000);
    alg->setProperty("NeutronPathsMultiple", 10000);
    alg->setProperty("ImportanceSampling", true);
    alg->execute();
    Mantid::API::WorkspaceGroup_sptr output =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
    Mantid::API::Workspace_sptr wsPtr = output->getItem("Scatter_2");
    auto doubleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr);

    // validate that the max scatter angle is ~94 degrees
    for (size_t i = 0; i < NTHETA; i++)
      if (doubleScatterResult->spectrumInfo().twoTheta(i) > M_PI * 94.0 / 180.0)
        TS_ASSERT_EQUALS(doubleScatterResult->y(i)[0], 0.);

    // crude check on peak positions at theta=0 and ~94 degrees
    double sum = 0.;
    for (size_t i = 0; i < NTHETA; i++)
      sum += doubleScatterResult->y(i)[0];
    double avgY = sum / NTHETA;
    std::vector<size_t> peakPos;
    int PEAKSPACING = NTHETA / 10;
    int lastPeakFound = -PEAKSPACING;
    for (size_t i = 0; i < NTHETA; i++)
      if ((doubleScatterResult->y(i)[0] > 3 * avgY) && (static_cast<int>(i - lastPeakFound) >= PEAKSPACING)) {
        peakPos.push_back(i);
        lastPeakFound = static_cast<int>(i);
      }
    TS_ASSERT_EQUALS(peakPos.size(), 2);
    if (peakPos.size() > 0) {
      TS_ASSERT_EQUALS(peakPos.front(), 0);
      TS_ASSERT((static_cast<double>(peakPos.back()) * 0.2 > 93) && (static_cast<double>(peakPos.back()) * 0.2 < 94));
    }

    Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
  }

  void test_output_workspaces() {
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(46, 1, 1.0, 1, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 3;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 10));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", 10));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());
    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      std::vector<std::string> wsNames = {"Scatter_1_NoAbs", "Scatter_1", "Scatter_2", "Scatter_3",
                                          "Scatter_2_3_Summed"};
      for (auto &name : wsNames) {
        Mantid::API::Workspace_sptr wsPtr = output->getItem(name);
        auto matrixWsPtr = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr);
        TS_ASSERT(matrixWsPtr);
      }
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_flat_plate_sample_single_scatter() {
    // generate a result corresponding to Figure 4 in the Mancinelli paper (flat
    // plate sample for once scattered neutrons) where there's an analytical solution
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(46, 1, 1.0, 1, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 1;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 10000));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr);
      // calculate result analytically
      const int SPECTRUMINDEXTOTEST = 1;
      const double analyticResult = calculateFlatPlateAnalyticalResult(
          singleScatterResult->histogram(SPECTRUMINDEXTOTEST).points()[0], inputWorkspace->sample().getMaterial(),
          inputWorkspace->spectrumInfo().twoTheta(SPECTRUMINDEXTOTEST), THICKNESS);
      const double delta(1e-05);
      TS_ASSERT_DELTA(singleScatterResult->y(SPECTRUMINDEXTOTEST)[0], analyticResult, delta);
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void run_flat_plate_sample_multiple_scatter(int nPaths, bool importanceSampling) {
    // same set up as previous test but increase nscatter to 2
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(2, 1, 1.0, 1, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", nPaths));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", nPaths));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ImportanceSampling", importanceSampling));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr1 = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr1);
      Mantid::API::Workspace_sptr wsPtr2 = output->getItem("Scatter_2");
      auto doubleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr2);
      // check single scatter result still matches analytical result
      const int SPECTRUMINDEXTOTEST = 1;
      const double analyticResult = calculateFlatPlateAnalyticalResult(
          singleScatterResult->histogram(SPECTRUMINDEXTOTEST).points()[0], inputWorkspace->sample().getMaterial(),
          inputWorkspace->spectrumInfo().twoTheta(SPECTRUMINDEXTOTEST), THICKNESS);
      const double delta(1e-05);
      TS_ASSERT_DELTA(singleScatterResult->y(SPECTRUMINDEXTOTEST)[0], analyticResult, delta);
      // no analytical result for double scatter so just check against current result that we assume is correct
      TS_ASSERT_DELTA(doubleScatterResult->y(SPECTRUMINDEXTOTEST)[0], 0.0019967315, delta);
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_flat_plate_sample_multiple_scatter_without_importance_sampling() {
    run_flat_plate_sample_multiple_scatter(100000, false);
  }

  void test_flat_plate_sample_multiple_scatter_with_importance_sampling() {
    // this test runs with flat S(Q). Not seeing the importance sampling having much effect but test ensures it hasn't
    // broken anything
    run_flat_plate_sample_multiple_scatter(100000, true);
  }

  void test_flat_plate_sample_multiple_scatter_with_wavelength_interp() {
    // same set up as previous test but increase nscatter to 2
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(2, 1, 1.0, 3, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 100000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", 100000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberOfWavelengthPoints", 2));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr1 = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr1);
      Mantid::API::Workspace_sptr wsPtr2 = output->getItem("Scatter_2");
      auto doubleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr2);
      // check single scatter result still matches analytical result
      const int SPECTRUMINDEXTOTEST = 1;
      const auto &mat = inputWorkspace->sample().getMaterial();
      const auto twoTheta = inputWorkspace->spectrumInfo().twoTheta(SPECTRUMINDEXTOTEST);
      const auto xPoints = singleScatterResult->histogram(SPECTRUMINDEXTOTEST).points();
      const double analyticResult1 = calculateFlatPlateAnalyticalResult(xPoints[0], mat, twoTheta, THICKNESS);
      const double analyticResult2 = calculateFlatPlateAnalyticalResult(xPoints[2], mat, twoTheta, THICKNESS);
      const auto &singleScatterY = singleScatterResult->y(SPECTRUMINDEXTOTEST);
      const double delta(1e-05);
      TS_ASSERT_DELTA(singleScatterY[0], analyticResult1, delta);
      TS_ASSERT_DELTA(singleScatterY[2], analyticResult2, delta);
      // check interpolated point is somewhere in between
      TS_ASSERT(singleScatterY[1] < analyticResult1 || singleScatterY[1] < analyticResult2);
      TS_ASSERT(singleScatterY[1] > analyticResult1 || singleScatterY[1] > analyticResult2);
      // no analytical result for double scatter so just check against current result that we assume is correct
      auto doubleScatterY = doubleScatterResult->y(SPECTRUMINDEXTOTEST);
      constexpr double expResult0 = 0.001997;
      constexpr double expResult2 = 0.001819;
      TS_ASSERT_DELTA(doubleScatterY[0], expResult0, delta);
      TS_ASSERT_DELTA(doubleScatterY[2], expResult2, delta);
      TS_ASSERT(doubleScatterY[1] < expResult0 || doubleScatterY[1] < expResult2);
      TS_ASSERT(doubleScatterY[1] > expResult0 || doubleScatterY[1] > expResult2);
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_SparseInstrument() {
    // set up instrument with five detectors at different latitudes (=5 different rows)
    // run simulation for detectors at latitude=0 and 2 degrees and interpolate at lat=1 degree
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(5, 2, 1.0, 1, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 10000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", 10000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SparseInstrument", true));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberOfDetectorRows", 3));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberOfDetectorColumns", 2));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr1 = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr1);
      // check single scatter result still matches analytical result
      const auto &mat = inputWorkspace->sample().getMaterial();
      auto twoTheta = inputWorkspace->spectrumInfo().twoTheta(0);
      auto xPoints = singleScatterResult->histogram(0).points();
      const double analyticResult1 = calculateFlatPlateAnalyticalResult(xPoints[0], mat, twoTheta, THICKNESS);
      twoTheta = inputWorkspace->spectrumInfo().twoTheta(4);
      xPoints = singleScatterResult->histogram(4).points();
      const double analyticResult2 = calculateFlatPlateAnalyticalResult(xPoints[0], mat, twoTheta, THICKNESS);
      const double delta(1e-05);
      auto singleScatterYLatZero = singleScatterResult->y(0)[0];
      TS_ASSERT_DELTA(singleScatterYLatZero, analyticResult1, delta);
      auto singleScatterYLatTwo = singleScatterResult->y(4)[0];
      TS_ASSERT_DELTA(singleScatterYLatTwo, analyticResult2, delta);
      // check interpolated result at lat=1 degree is in between the results at lat=0 and 2 degrees
      auto interpSingleScatterY = singleScatterResult->y(2)[0];
      TS_ASSERT(interpSingleScatterY < singleScatterYLatZero || interpSingleScatterY < singleScatterYLatTwo);
      TS_ASSERT(interpSingleScatterY > singleScatterYLatZero || interpSingleScatterY > singleScatterYLatTwo);
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_interpolateGaussian() {
    DiscusMultipleScatteringCorrectionHelper alg;

    auto ws = Mantid::DataObjects::create<Workspace2D>(
        1, Mantid::HistogramData::Histogram(Mantid::HistogramData::Points({0., 4., 16.}),
                                            Mantid::HistogramData::Counts({0., 2., 4.})));
    auto interpY = alg.interpolateSquareRoot(ws->histogram(0), 9.0);
    TS_ASSERT_EQUALS(interpY, 3.0);
  }

  void test_updateTrackDirection() {
    DiscusMultipleScatteringCorrectionHelper alg;
    const double twoTheta = M_PI * 60. / 180.;
    const double cosTwoTheta = cos(twoTheta);
    const double sinTwoTheta = sin(twoTheta);
    const double phi = M_PI;
    Mantid::Geometry::Track track(V3D(0, 0, 0), V3D(0.0, 0.0, 1.0));
    alg.updateTrackDirection(track, cosTwoTheta, phi);
    TS_ASSERT_EQUALS(track.direction(), V3D(0.0, -sinTwoTheta, cosTwoTheta));
    // special cases of track going vertically
    Mantid::Geometry::Track trackUp(V3D(0, 0, 0), V3D(0.0, 1.0, 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.updateTrackDirection(trackUp, cosTwoTheta, phi));
    TS_ASSERT_EQUALS(trackUp.direction(), V3D(0, cosTwoTheta, -sinTwoTheta))
    //...and vertically down
    Mantid::Geometry::Track trackDown(V3D(0, 0, 0), V3D(0.0, -1.0, 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.updateTrackDirection(trackDown, cosTwoTheta, phi));
    TS_ASSERT_EQUALS(trackDown.direction(), V3D(0, -cosTwoTheta, -sinTwoTheta))
  }

  void test_integrateCumulative() {
    DiscusMultipleScatteringCorrectionHelper alg;
    Mantid::HistogramData::Histogram test(Mantid::HistogramData::Points({0., 1., 2., 3.}),
                                          Mantid::HistogramData::Frequencies({1., 1., 1., 1.}));
    std::vector<double> testResultX, testResultY;
    alg.integrateCumulative(test, 2.2, testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[3], 2.2);
    TS_ASSERT_THROWS(alg.integrateCumulative(test, 3.2, testResultX, testResultY), std::runtime_error &);
    alg.integrateCumulative(test, 2.0, testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[2], 2.0);
    alg.integrateCumulative(test, 0., testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[0], 0.);
  }

  //---------------------------------------------------------------------------
  // Failure cases
  //---------------------------------------------------------------------------

  void test_invalidSOfQ() {
    DiscusMultipleScatteringCorrectionHelper alg;
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, THICKNESS);
    auto SofQWorkspaceTwoSp = WorkspaceCreationHelper::create2DWorkspace(2, 1);
    SofQWorkspaceTwoSp->mutableY(0)[0] = 1.;
    SofQWorkspaceTwoSp->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SofqWorkspace", SofQWorkspaceTwoSp));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);

    auto SofQWorkspaceNegative = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    SofQWorkspaceNegative->mutableY(0)[0] = -1.0;
    SofQWorkspaceNegative->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SofqWorkspace", SofQWorkspaceNegative));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_invalidZeroSOfQ() {
    DiscusMultipleScatteringCorrectionHelper alg;
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, THICKNESS);
    auto SofQWorkspaceZero = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    SofQWorkspaceZero->mutableY(0)[0] = 0.;
    SofQWorkspaceZero->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SofqWorkspace", SofQWorkspaceZero));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ImportanceSampling", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    alg.execute();
    TS_ASSERT(!alg.isExecuted());
  }

  void test_cant_run_withAlwaysStoreInADS_false() {
    const double THICKNESS = 0.001; // metres
    DiscusMultipleScatteringCorrectionHelper alg;
    alg.setAlwaysStoreInADS(false);
    alg.setRethrows(true);
    alg.initialize();
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, THICKNESS);
    alg.setProperty("InputWorkspace", inputWorkspace);
    alg.setProperty("SofqWorkspace", SofQWorkspace);
    alg.setPropertyValue("OutputWorkspace", "MuscatResults");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

private:
  Mantid::API::IAlgorithm_sptr createAlgorithm() {
    using Mantid::Algorithms::DiscusMultipleScatteringCorrection;
    using Mantid::API::IAlgorithm;
    auto alg = std::make_shared<DiscusMultipleScatteringCorrection>();
    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SofqWorkspace", SofQWorkspace));
    alg->setPropertyValue("OutputWorkspace", "MuscatResults");
    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr SetupFlatPlateWorkspace(const int nlat, const int nlong, const double anginc,
                                                            const int nbins, const double thickness) {

    auto inputWorkspace =
        WorkspaceCreationHelper::create2DWorkspaceWithGeographicalDetectors(nlat, nlong, anginc, nbins);

    // create flat plate that is 1mm thick
    auto flatPlateShape = ComponentCreationHelper::createCuboid((10 * thickness) / 2, (10 * thickness) / 2,
                                                                thickness / 2, 0, V3D{0, 0, 1});
    auto mat = Mantid::Kernel::Material("Ni", Mantid::PhysicalConstants::getNeutronAtom(28, 0), 0.091337537);
    flatPlateShape->setMaterial(mat);
    inputWorkspace->mutableSample().setShape(flatPlateShape);

    return inputWorkspace;
  }

  double calculateFlatPlateAnalyticalResult(const double wavelength, const Mantid::Kernel::Material &mat,
                                            const double twoTheta, const double thickness) {
    const double totalXSection = mat.totalScatterXSection() + mat.absorbXSection(wavelength);
    const double alpha = mat.absorbXSection(wavelength) / totalXSection;
    const double mfp = 0.01 / (mat.numberDensity() * totalXSection);
    const double tau = thickness / mfp;
    const double secangle = 1 / cos(twoTheta);
    if (secangle == 1.)
      return (1 - alpha) * tau * exp(-tau) / (4 * M_PI);
    else
      return (1 - alpha) * (exp(-tau * secangle) - exp(-tau)) / (4 * M_PI * (1 - secangle));
  }
  Mantid::API::MatrixWorkspace_sptr SofQWorkspace;
};
