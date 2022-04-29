// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Axis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DiscusMultipleScatteringCorrection.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/DeltaEMode.h"
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
  double interpolateGaussian(const Mantid::API::ISpectrum &histToInterpolate, double x) {
    return DiscusMultipleScatteringCorrection::interpolateGaussian(histToInterpolate, x);
  }
  double interpolateSquareRoot(const Mantid::API::ISpectrum &histToInterpolate, double x) {
    return DiscusMultipleScatteringCorrection::interpolateSquareRoot(histToInterpolate, x);
  }
  void updateTrackDirection(Mantid::Geometry::Track &track, const double cosT, const double phi) {
    DiscusMultipleScatteringCorrection::updateTrackDirection(track, cosT, phi);
  }
  void integrateCumulative(const Mantid::HistogramData::Histogram &h, double xmin, double xmax,
                           std::vector<double> &resultX, std::vector<double> &resultY) {
    DiscusMultipleScatteringCorrection::integrateCumulative(h, xmin, xmax, resultX, resultY);
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
    // k = 1 in most tests, so q max = 2k = 2
    IsotropicSofQWorkspace = WorkspaceCreationHelper::create2DWorkspaceBinned(1, NBINS, 0., 2.0 / NBINS);
    for (size_t i = 0; i < IsotropicSofQWorkspace->blocksize(); i++)
      IsotropicSofQWorkspace->mutableY(0)[i] = 1.;
    IsotropicSofQWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  }

  void test_testSQDeltaFunction() {
    const double THICKNESS = 0.001; // metres

    const int NTHETA = 900;
    const double ang_inc = 180.0 / NTHETA;
    auto inputWorkspace =
        SetupFlatPlateWorkspace(1, NTHETA, ang_inc, 1, 0.5, 1.0, 10 * THICKNESS, 10 * THICKNESS, THICKNESS);

    auto SofQWorkspace = WorkspaceCreationHelper::create2DWorkspace(1, 3);
    SofQWorkspace->mutableX(0) = {0.9985, 0.9995, 1.0005, 1.0015};
    // S(Q) zero everywhere apart from spike at Q=1
    SofQWorkspace->mutableY(0) = {0., 100., 0.};
    SofQWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");

    auto alg = std::make_shared<Mantid::Algorithms::DiscusMultipleScatteringCorrection>();
    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("StructureFactorWorkspace", SofQWorkspace));
    alg->setPropertyValue("OutputWorkspace", "MuscatResults");
    // input workspace has single bin - centred at 1.0 Angstrom-1
    // DiscusMultipleScatteringCorrection will sample q between 0 and 2k (2.0)
    // so q=1 requires sin(theta) = 0.5, theta=30 degrees, 2theta=60 degrees
    // So two scatters at max S(Q) will take the track to ~120 degrees
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

    // validate that the max scatter angle is ~120 degrees (peak is at 120.0 but slight tail)
    for (size_t i = 0; i < NTHETA; i++) {
      if (doubleScatterResult->spectrumInfo().twoTheta(i) > M_PI * (120.2 + 0.5 * ang_inc) / 180.0)
        TS_ASSERT_EQUALS(doubleScatterResult->y(i)[0], 0.);
    }

    // crude check on peak positions at theta=0 and ~120 degrees
    double sum = 0.;
    for (size_t i = 0; i < NTHETA; i++)
      sum += doubleScatterResult->y(i)[0];
    double avgY = sum / NTHETA;
    std::vector<size_t> peakPos;
    int PEAKSPACING = NTHETA / 10;
    for (int i = 0; i < NTHETA; i++) {
      bool maxInWindow = true;
      for (int j = std::max(0, i - PEAKSPACING); j <= std::min(NTHETA - 1, i + PEAKSPACING); j++) {
        if (doubleScatterResult->y(j)[0] > doubleScatterResult->y(i)[0])
          maxInWindow = false;
      }
      if ((doubleScatterResult->y(i)[0] > 3 * avgY) && maxInWindow)
        peakPos.push_back(i);
    }
    TS_ASSERT_EQUALS(peakPos.size(), 2);
    if (peakPos.size() > 0) {
      TS_ASSERT_EQUALS(peakPos.front(), 0);
      TS_ASSERT((static_cast<double>(peakPos.back()) * ang_inc >= 120) &&
                (static_cast<double>(peakPos.back()) * ang_inc < 121));
    }

    Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
  }

  void test_output_workspaces() {
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(46, 1, 1.0, 1, 0.5, 1.0, 10 * THICKNESS, 10 * THICKNESS, THICKNESS);

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
    auto inputWorkspace = SetupFlatPlateWorkspace(46, 1, 1.0, 1, 0.5, 1.0, 10 * THICKNESS, 10 * THICKNESS, THICKNESS,
                                                  DeltaEMode::Elastic);

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
    auto inputWorkspace =
        SetupFlatPlateWorkspace(2, 1, 1.0, 1, 0.5, 1.0, 10 * THICKNESS, 10 * THICKNESS, THICKNESS, DeltaEMode::Elastic);
    // overwrite x with single point centered at wavelength=1 Angstrom. Algorithm used to take x units of wavelength
    // so this allows test values to be preserved
    Mantid::HistogramData::Points xv = {2 * M_PI};
    inputWorkspace->setPoints(0, xv.cowData());
    inputWorkspace->setPoints(1, xv.cowData());

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

  void test_flat_plate_sample_multiple_scatter_with_bin_interp() {
    // same set up as previous test but increase nscatter to 2
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace =
        SetupFlatPlateWorkspace(2, 1, 1.0, 3, 0.5, 1.0, 10 * THICKNESS, 10 * THICKNESS, THICKNESS, DeltaEMode::Elastic);
    // overwrite x with points equivalent to wavelength=1,2,3 Angstroms. Algorithm used to take x units of wavelength
    // so this allows test values to be preserved
    Mantid::HistogramData::Points xv = {2 * M_PI / 3, M_PI, 2 * M_PI};
    inputWorkspace->setPoints(0, xv.cowData());
    inputWorkspace->setPoints(1, xv.cowData());

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 100000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", 100000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberOfSimulationPoints", 2));
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
      constexpr double expResult2 = 0.001997;
      constexpr double expResult0 = 0.001819;
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
    auto inputWorkspace = SetupFlatPlateWorkspace(5, 2, 1.0, 1, 0.5, 1.0, 10 * THICKNESS, 10 * THICKNESS, THICKNESS);

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
    const int NBINS = 10;

    auto ws2 = WorkspaceCreationHelper::create2DWorkspacePoints(1, NBINS, 0.5);
    for (auto i = 0; i < 4; i++) {
      ws2->mutableY(0)[i] = pow(2 * i, 2);
    }
    auto interpY = alg.interpolateGaussian(ws2->getSpectrum(0), 2.0);
    TS_ASSERT_EQUALS(interpY, exp(9.0));

    // check point beyond half way point uses same three points
    interpY = alg.interpolateGaussian(ws2->getSpectrum(0), 2.00000001);
    TS_ASSERT_DELTA(interpY, exp(9.0), 0.01);
  }

  void test_interpolateSquareRoot() {
    DiscusMultipleScatteringCorrectionHelper alg;

    auto ws = Mantid::DataObjects::create<Workspace2D>(
        1, Mantid::HistogramData::Histogram(Mantid::HistogramData::Points({0., 4., 16.}),
                                            Mantid::HistogramData::Counts({0., 2., 4.})));
    auto interpY = alg.interpolateSquareRoot(ws->getSpectrum(0), 9.0);
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
    alg.integrateCumulative(test, 0., 2.2, testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[3], 2.2);
    testResultX.clear();
    testResultY.clear();
    TS_ASSERT_THROWS(alg.integrateCumulative(test, 0., 3.2, testResultX, testResultY), std::runtime_error &);
    testResultX.clear();
    testResultY.clear();
    alg.integrateCumulative(test, 0., 2.0, testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[2], 2.0);
    testResultX.clear();
    testResultY.clear();
    alg.integrateCumulative(test, 0., 0., testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[0], 0.);
    testResultX.clear();
    testResultY.clear();
    alg.integrateCumulative(test, 1., 0., testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[0], 0.);
    testResultX.clear();
    testResultY.clear();
    alg.integrateCumulative(test, 0.5, 1.5, testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[2], 1.0);
    testResultX.clear();
    testResultY.clear();
    alg.integrateCumulative(test, 0.5, 0.9, testResultX, testResultY);
    TS_ASSERT_EQUALS(testResultY[1], 0.4);
  }

  void test_inelastic_with_importance_sampling() {
    // perform test on an S(Q,w) consisting of single spike at Q=1, w=-1
    // Not a realistic S(Q,w) for inelastic but useful for test to check sign conventions on w are correct and
    // also produces features at predictable w and two theta
    const double THICKNESS = 0.001; // metres

    const int NTHETA = 180;
    const double ang_inc = 180.0 / NTHETA;
    // set up k_inc=2.0 and work out where the peaks in the single and double scatter profiles should be
    const double kinitial = 2.0;
    const double deltaE = -1.0;
    const double deltaESpikeWidth = 0.01;
    const double qSpike = 1.0;
    const double qSpikeWidth = 0.01;
    const double Einitial = Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq * kinitial * kinitial;
    const double kafterfirst = sqrt((Einitial - deltaE) / Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq);
    const double cosTwoThetaScatter1 =
        (kafterfirst * kafterfirst + kinitial * kinitial - qSpike * qSpike) / (2 * kafterfirst * kinitial);
    const double TwoThetaScatter1InDeg = acos(cosTwoThetaScatter1) * 180.0 / M_PI;
    const double EAfterFirst = Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq * kafterfirst * kafterfirst;
    const double kaftersecond = sqrt((EAfterFirst - deltaE) / Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq);
    const double cosTwoThetaScatter2 =
        (kaftersecond * kaftersecond + kafterfirst * kafterfirst - qSpike * qSpike) / (2 * kaftersecond * kafterfirst);
    const double TwoThetaScatter2InDeg = acos(cosTwoThetaScatter2) * 180.0 / M_PI;
    const double expectedPeak1InDeg = std::abs(TwoThetaScatter1InDeg - TwoThetaScatter2InDeg);
    const double expectedPeak2InDeg = TwoThetaScatter1InDeg + TwoThetaScatter2InDeg;

    // set up workspace in direct mode with kinitial=2, w points at 1.0, 2.0, 3.0 meV
    auto inputWorkspace = SetupFlatPlateWorkspace(1, NTHETA, ang_inc, 3, -3.5, 1.0, 10 * THICKNESS, 10 * THICKNESS,
                                                  THICKNESS, 0., {0., 0., 1.}, DeltaEMode::Direct, Einitial);

    auto SofQWorkspace =
        WorkspaceCreationHelper::create2DWorkspacePoints(3, 3, deltaE - deltaESpikeWidth, deltaESpikeWidth);
    auto verticalAxis = std::make_unique<Mantid::API::NumericAxis>(3);
    // Now set the axis values
    for (int i = 0; i < 3; ++i) {
      verticalAxis->setValue(i, qSpike - qSpikeWidth + i * qSpikeWidth);
    }
    SofQWorkspace->replaceAxis(1, std::move(verticalAxis));
    // S(Q) zero everywhere apart from spike at Q=1, w=1
    for (size_t i = 0; i < 3; i++)
      for (size_t j = 0; j < 3; j++) {
        if ((SofQWorkspace->dataX(i)[j] == deltaE) && (SofQWorkspace->getAxis(1)->getValue(i) == qSpike))
          SofQWorkspace->mutableY(i)[j] = 1000.;
        else
          SofQWorkspace->mutableY(i)[j] = 0.;
      }
    SofQWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    SofQWorkspace->getAxis(1)->unit() = UnitFactory::Instance().create("MomentumTransfer");

    auto alg = std::make_shared<Mantid::Algorithms::DiscusMultipleScatteringCorrection>();
    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("StructureFactorWorkspace", SofQWorkspace);
    alg->setPropertyValue("OutputWorkspace", "MuscatResults");
    alg->setProperty("InputWorkspace", inputWorkspace);
    const int NSCATTERINGS = 2;
    alg->setProperty("NumberScatterings", NSCATTERINGS);
    alg->setProperty("NeutronPathsSingle", 1000);
    alg->setProperty("NeutronPathsMultiple", 10000);
    alg->setProperty("ImportanceSampling", true);
    alg->execute();

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr = output->getItem("Scatter_2");
      auto doubleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr);
      // validate that the max scatter angle is ~61.5 degrees
      for (size_t i = 0; i < NTHETA; i++) {
        if (doubleScatterResult->spectrumInfo().twoTheta(i) >
            M_PI * (std::ceil(expectedPeak2InDeg) + 0.5 * ang_inc) / 180.0)
          TS_ASSERT_EQUALS(doubleScatterResult->y(i)[1], 0.);
      }

      // crude check on peak positions at theta=2 and ~61.5 degrees with w overall=2
      double sum = 0.;
      for (size_t i = 0; i < NTHETA; i++)
        sum += doubleScatterResult->y(i)[1];
      double avgY = sum / NTHETA;
      std::vector<size_t> peakPos;
      int PEAKSPACING = NTHETA / 10;
      for (int i = 0; i < NTHETA; i++) {
        bool maxInWindow = true;
        for (int j = std::max(0, i - PEAKSPACING); j <= std::min(NTHETA - 1, i + PEAKSPACING); j++) {
          if (doubleScatterResult->y(j)[1] > doubleScatterResult->y(i)[1])
            maxInWindow = false;
        }
        if ((doubleScatterResult->y(i)[1] > 3 * avgY) && maxInWindow)
          peakPos.push_back(i);
      }
      TS_ASSERT_EQUALS(peakPos.size(), 2);
      if (peakPos.size() > 0) {
        TS_ASSERT((static_cast<double>(peakPos.front()) * ang_inc >= std::floor(expectedPeak1InDeg)) &&
                  (static_cast<double>(peakPos.front()) * ang_inc <= std::ceil(expectedPeak1InDeg)));
        TS_ASSERT((static_cast<double>(peakPos.back()) * ang_inc >= std::floor(expectedPeak2InDeg)) &&
                  (static_cast<double>(peakPos.back()) * ang_inc <= std::ceil(expectedPeak2InDeg)));
      }

      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void run_test_inelastic_on_realistic_structure_factor(const DeltaEMode::Type emode, int nPaths,
                                                        bool importanceSampling, bool simulateWSeparately,
                                                        int numberSimulationPoints, double expWeightMinusOne,
                                                        double expWeightPlusOne) {
    // run test on a realistic structure factor. Validate against results in original Discus paper

    // calculate the S(Q,w) values based on a Lorentzian
    double qmin = 0.; // 0.001
    double qmax = 4.0;
    int nqpts = 9;
    double wmin = -5.85;
    double wmax = 5.85;
    int nwpts = 79; // negative w is given explicitly so ~double number of pts in Discus
    double wwidth = (wmax - wmin) / (nwpts - 1);
    // D = 2.3E-05 #cm2 s - 1
    double D =
        0.15; // Angstom - 2 meV - 1(more accurate value is 0.151 but Discus seems to have used this rounded value)
    double TEMP = 300;
    double HOVERT = 11.6087 / TEMP;
    auto SofQWorkspace = WorkspaceCreationHelper::create2DWorkspacePoints(nqpts, nwpts, wmin, wwidth);
    auto verticalAxis = std::make_unique<Mantid::API::NumericAxis>(nqpts);
    for (int iq = 0; iq < nqpts; iq++) {
      std::vector<double> X, Y;
      double q = iq * (qmax - qmin) / (nqpts - 1) + qmin;
      for (int iw = 0; iw < nwpts; iw++) {
        double w = iw * wwidth + wmin;
        X.push_back(w);
        if (w * w + pow(D * q * q, 2) == 0.)
          // Discus S(Q, w) has zero here so do likewise
          Y.push_back(0.);
        else {
          double sqw = D * q * q / (M_PI * (w * w + pow(D * q * q, 2)));
          // Apply detailed balanace, neutrons more likely to lose energy in each scatter
          // w = Ei - Ef
          if (w > 0)
            sqw = sqw * exp(HOVERT * w);
          // S(Q, w) is capped at exactly 4.0 for some reason in Discus example
          Y.push_back(std::min(sqw, 4.0));
        }
      }
      SofQWorkspace->mutableX(iq) = X;
      SofQWorkspace->mutableY(iq) = Y;
      verticalAxis->setValue(iq, q);
    }
    SofQWorkspace->replaceAxis(1, std::move(verticalAxis));
    SofQWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    SofQWorkspace->getAxis(1)->unit() = UnitFactory::Instance().create("MomentumTransfer");

    std::vector<double> two_thetas = {20.0, 40.0, 60.0, 90.0};
    const double THICKNESS = 0.00065; // metres

    const int NTHETA = 18;
    const double ang_inc = 180.0 / NTHETA;
    const double EInitial = 5.1;
    auto inputWorkspace = SetupFlatPlateWorkspace(NTHETA, 1, ang_inc, nwpts, wmin - 0.5 * wwidth, wwidth, 0.05, 0.05,
                                                  THICKNESS, 45.0, {1.0, 0.0, 0.0}, emode, EInitial);
    auto alg = std::make_shared<Mantid::Algorithms::DiscusMultipleScatteringCorrection>();

    // override the material
    Mantid::PhysicalConstants::NeutronAtom neutron(0, 0.0, 0.0, 0.0, 0.0, 80., 0.);
    auto shape = std::shared_ptr<IObject>(
        inputWorkspace->sample().getShape().cloneWithMaterial(Material("dummy", neutron, 0.02)));
    inputWorkspace->mutableSample().setShape(shape);

    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("StructureFactorWorkspace", SofQWorkspace);
    alg->setPropertyValue("OutputWorkspace", "MuscatResults");
    alg->setProperty("InputWorkspace", inputWorkspace);
    const int NSCATTERINGS = 2;
    alg->setProperty("NumberScatterings", NSCATTERINGS);
    alg->setProperty("NeutronPathsSingle", 200);
    alg->setProperty("NeutronPathsMultiple", nPaths);
    if (numberSimulationPoints > 0)
      alg->setProperty("NumberOfSimulationPoints", numberSimulationPoints);
    alg->setProperty("ImportanceSampling", importanceSampling);
    alg->setProperty("SimulateEnergiesIndependently", simulateWSeparately);
    alg->execute();

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr1 = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr1);
      Mantid::API::Workspace_sptr wsPtr2 = output->getItem("Scatter_2");
      auto doubleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr2);

      const double delta(1e-04);
      const int SPECTRUMINDEXTOTEST = 2; // 20 degrees
      // check at the w=+/-1 points
      TS_ASSERT_DELTA(doubleScatterResult->y(SPECTRUMINDEXTOTEST)[33], expWeightMinusOne, delta);
      TS_ASSERT_DELTA(doubleScatterResult->y(SPECTRUMINDEXTOTEST)[46], expWeightPlusOne, delta);
      // double scatter intensity is larger than single at this point
      TS_ASSERT(doubleScatterResult->y(SPECTRUMINDEXTOTEST)[33] > singleScatterResult->y(SPECTRUMINDEXTOTEST)[33]);
      TS_ASSERT(doubleScatterResult->y(SPECTRUMINDEXTOTEST)[46] > singleScatterResult->y(SPECTRUMINDEXTOTEST)[46]);

      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_direct_on_realistic_structure_factor_with_importance_sampling() {
    run_test_inelastic_on_realistic_structure_factor(DeltaEMode::Direct, 1000, true, false, -1, 0.00022, 0.00017);
  }

  void test_direct_on_realistic_structure_factor_without_importance_sampling() {
    run_test_inelastic_on_realistic_structure_factor(DeltaEMode::Direct, 1000, false, false, -1, 0.00022, 0.00017);
  }

  void test_direct_on_realistic_structure_factor_without_importance_sampling_simulate_w_separately() {
    run_test_inelastic_on_realistic_structure_factor(DeltaEMode::Direct, 1000, false, true, -1, 0.00022, 0.00017);
  }

  void test_indirect_on_realistic_structure_factor_without_importance_sampling() {
    // results are not vastly different to the direct geometry
    run_test_inelastic_on_realistic_structure_factor(DeltaEMode::Indirect, 1000, false, false, -1, 0.00022, 0.00021);
  }

  void test_indirect_on_realistic_structure_factor_with_deltaE_interpolation() {
    // only run simulation on half of the deltaE bins (even indices) and interpolate the rest (odd indices)
    run_test_inelastic_on_realistic_structure_factor(DeltaEMode::Indirect, 1000, false, false, 40, 0.00023, 0.00021);
  }

  //---------------------------------------------------------------------------
  // Failure cases
  //---------------------------------------------------------------------------

  void test_invalidSOfQ() {
    DiscusMultipleScatteringCorrectionHelper alg;
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, 0.5, 1.0, 100 * THICKNESS, 100 * THICKNESS, THICKNESS,
                                                  DeltaEMode::Elastic);
    auto SofQWorkspaceTwoSp = WorkspaceCreationHelper::create2DWorkspace(2, 1);
    SofQWorkspaceTwoSp->mutableY(0)[0] = 1.;
    SofQWorkspaceTwoSp->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StructureFactorWorkspace", SofQWorkspaceTwoSp));
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
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StructureFactorWorkspace", SofQWorkspaceNegative));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_invalidZeroSOfQ() {
    DiscusMultipleScatteringCorrectionHelper alg;
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, 0.5, 1.0, 100 * THICKNESS, 100 * THICKNESS, THICKNESS);
    auto SofQWorkspaceZero = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    SofQWorkspaceZero->mutableY(0)[0] = 0.;
    SofQWorkspaceZero->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StructureFactorWorkspace", SofQWorkspaceZero));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ImportanceSampling", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    alg.execute();
    TS_ASSERT(!alg.isExecuted());
  }

  void test_elastic_SQW_supplied_for_inelastic() {
    DiscusMultipleScatteringCorrectionHelper alg;
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, 0.5, 1.0, 100 * THICKNESS, 100 * THICKNESS, THICKNESS,
                                                  0., {0., 0., 1.}, DeltaEMode::Direct);
    auto SofQWorkspaceZero = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    SofQWorkspaceZero->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StructureFactorWorkspace", SofQWorkspaceZero));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_invalid_SQW_wrong_units_supplied_for_inelastic() {
    DiscusMultipleScatteringCorrectionHelper alg;
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, 0.5, 1.0, 100 * THICKNESS, 100 * THICKNESS, THICKNESS,
                                                  0., {0., 0., 1.}, DeltaEMode::Direct);
    auto SofQWorkspaceZero = WorkspaceCreationHelper::create2DWorkspace(2, 1);
    auto verticalAxis = std::make_unique<Mantid::API::NumericAxis>(2);
    // Now set the axis q values
    for (int i = 0; i < 2; ++i) {
      verticalAxis->setValue(0, i * 1.0);
    }
    SofQWorkspaceZero->replaceAxis(1, std::move(verticalAxis));
    SofQWorkspaceZero->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");
    SofQWorkspaceZero->getAxis(1)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StructureFactorWorkspace", SofQWorkspaceZero));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_invalid_SQW_no_negative_w_supplied_for_inelastic() {
    DiscusMultipleScatteringCorrectionHelper alg;
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, 0.5, 1.0, 100 * THICKNESS, 100 * THICKNESS, THICKNESS,
                                                  0., {0., 0., 1.}, DeltaEMode::Direct);
    auto SofQWorkspaceWithOnlyPositiveW = WorkspaceCreationHelper::create2DWorkspaceBinned(2, 1, 1000);
    auto verticalAxis = std::make_unique<Mantid::API::NumericAxis>(2);
    // Now set the axis q values
    for (int i = 0; i < 2; ++i) {
      verticalAxis->setValue(0, i * 1.0);
    }
    SofQWorkspaceWithOnlyPositiveW->replaceAxis(1, std::move(verticalAxis));
    SofQWorkspaceWithOnlyPositiveW->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    SofQWorkspaceWithOnlyPositiveW->getAxis(1)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StructureFactorWorkspace", SofQWorkspaceWithOnlyPositiveW));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_cant_run_withAlwaysStoreInADS_false() {
    const double THICKNESS = 0.001; // metres
    DiscusMultipleScatteringCorrectionHelper alg;
    alg.setAlwaysStoreInADS(false);
    alg.setRethrows(true);
    alg.initialize();
    auto inputWorkspace = SetupFlatPlateWorkspace(1, 1, 1.0, 1, 0.5, 1.0, 100 * THICKNESS, 100 * THICKNESS, THICKNESS,
                                                  DeltaEMode::Elastic);
    alg.setProperty("InputWorkspace", inputWorkspace);
    alg.setProperty("StructureFactorWorkspace", IsotropicSofQWorkspace);
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
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("StructureFactorWorkspace", IsotropicSofQWorkspace));
    alg->setPropertyValue("OutputWorkspace", "MuscatResults");
    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr
  SetupFlatPlateWorkspace(const int nlat, const int nlong, const double anginc, const int nbins, const double xmin,
                          const double deltax, const double width, const double height, const double thickness,
                          const double angle = 0., const V3D axis = {0., 0., 1.},
                          const DeltaEMode::Type EMode = DeltaEMode::Elastic, const double efixed = 5.0) {
    std::string unitName = "Momentum";
    if (EMode != DeltaEMode::Elastic) {
      unitName = "DeltaE";
    }
    auto inputWorkspace = WorkspaceCreationHelper::create2DWorkspaceWithGeographicalDetectors(
        nlat, nlong, anginc, nbins, xmin, deltax, "testinst", unitName);

    auto flatPlateShape = ComponentCreationHelper::createCuboid(width / 2, height / 2, thickness / 2, angle, axis);
    auto mat = Mantid::Kernel::Material("Ni", Mantid::PhysicalConstants::getNeutronAtom(28, 0), 0.091337537);
    flatPlateShape->setMaterial(mat);
    inputWorkspace->mutableSample().setShape(flatPlateShape);

    auto inst = inputWorkspace->getInstrument();
    auto &pmap = inputWorkspace->instrumentParameters();
    if (EMode == DeltaEMode::Direct) {
      pmap.addString(inst.get(), "deltaE-mode", "Direct");
      inputWorkspace->mutableRun().addProperty<double>("Ei", efixed);
    } else if (EMode == DeltaEMode::Indirect) {
      pmap.addString(inst.get(), "deltaE-mode", "Indirect");
      pmap.addDouble(inst.get(), "Efixed", efixed);
    }

    return inputWorkspace;
  }

  double calculateFlatPlateAnalyticalResult(const double wavevector, const Mantid::Kernel::Material &mat,
                                            const double twoTheta, const double thickness) {
    const double wavelength = 2 * M_PI / wavevector;
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
  Mantid::API::MatrixWorkspace_sptr IsotropicSofQWorkspace;
};
