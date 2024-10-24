// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidCurveFitting/Algorithms/SplineInterpolation.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::Algorithms::SplineInterpolation;
using namespace Mantid::API;

class SplineInterpolationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SplineInterpolationTest *createSuite() { return new SplineInterpolationTest(); }
  static void destroySuite(SplineInterpolationTest *suite) { delete suite; }

  // Functor to generate spline values
  struct SplineFunc {
    double operator()(double x, int) { return x * 2; }
  };

  void test_Init() {
    SplineInterpolation alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void testExec() {
    int order(2), spectra(1);

    // create binned workspaces
    MatrixWorkspace_sptr mws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1, 0, 20, 1, false);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), spectra, 0, 20, 0.1, false);

    SplineInterpolation alg;
    runAlgorithm(alg, order, iws, mws);
    checkOutput(alg);
  }

  void testExecHistogramData() {
    int order(2), spectra(1);

    // create binned workspaces
    MatrixWorkspace_sptr mws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1, 0, 20, 1, true);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), spectra, 0, 20, 1, true);

    SplineInterpolation alg;
    runAlgorithm(alg, order, iws, mws);
    checkOutput(alg);
  }

  void testLinear2Point() {

    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 3, 2.1, 4.9, 1.4, true);

    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 2, 1.6, 5.6, 0.4, true);

    SplineInterpolation alg;
    runAlgorithm(alg, 1, iws, mws, true);

    checkOutputLinear(alg);
  }

  void checkOutputLinear(const SplineInterpolation &alg) const {
    MatrixWorkspace_const_sptr ows = alg.getProperty("OutputWorkspace");
    WorkspaceGroup_const_sptr derivs = alg.getProperty("OutputWorkspaceDeriv");

    TS_ASSERT(ows->isHistogramData())
    TS_ASSERT_EQUALS(ows->getNumberHistograms(), 3)
    TS_ASSERT_EQUALS(ows->blocksize(), 9)

    NumericAxis *xAxis = dynamic_cast<NumericAxis *>(ows->getAxis(0));
    TS_ASSERT(xAxis);

    const size_t numBins = ows->blocksize();
    for (size_t i = 0; i < numBins; i++) {
      double ref = 1.6 + static_cast<double>(i) * 0.4;
      TS_ASSERT_EQUALS((*xAxis)(i), ref);
    }

    const auto &y = ows->y(0).rawData();

    TS_ASSERT_EQUALS(y[0], 4.2)
    TS_ASSERT_EQUALS(y[1], 4.2)
    TS_ASSERT_EQUALS(y[2], 4.2)
    TS_ASSERT_DELTA(y[3], 4.6, 1e-10)
    TS_ASSERT_DELTA(y[4], 5.4, 1e-10)
    TS_ASSERT_DELTA(y[5], 6.2, 1e-10)
    TS_ASSERT_DELTA(y[6], 7., 1e-10)
    TS_ASSERT_EQUALS(y[7], 7.)
    TS_ASSERT_EQUALS(y[8], 7.)

    for (size_t i = 0; i < ows->getNumberHistograms(); ++i) {
      MatrixWorkspace_const_sptr derivsWs = std::dynamic_pointer_cast<const MatrixWorkspace>(derivs->getItem(i));

      NumericAxis *derivVAxis = dynamic_cast<NumericAxis *>(derivsWs->getAxis(1));
      TS_ASSERT(derivVAxis);
      TS_ASSERT_EQUALS((*derivVAxis)(0), 1);
      NumericAxis *derivXAxis = dynamic_cast<NumericAxis *>(derivsWs->getAxis(0));
      TS_ASSERT(derivXAxis);

      const size_t numBins = ows->blocksize();
      for (size_t i = 0; i < numBins; i++) {
        double ref = 1.6 + static_cast<double>(i) * 0.4;
        TS_ASSERT_EQUALS((*derivXAxis)(i), ref);
      }

      const auto &deriv = derivsWs->y(0);

      TS_ASSERT_EQUALS(deriv[0], 0.)
      TS_ASSERT_EQUALS(deriv[1], 0.)
      TS_ASSERT_EQUALS(deriv[2], 0.)
      TS_ASSERT_DELTA(deriv[3], 2., 1e-10)
      TS_ASSERT_DELTA(deriv[4], 2., 1e-10)
      TS_ASSERT_DELTA(deriv[5], 2., 1e-10)
      TS_ASSERT_DELTA(deriv[6], 2., 1e-10)
      TS_ASSERT_EQUALS(deriv[7], 0.)
      TS_ASSERT_EQUALS(deriv[8], 0.)
    }
  }

  void testExecMultipleSpectra() {
    int order(2), spectra(3);

    // create binned workspaces
    MatrixWorkspace_sptr mws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1, 0, 20, 1, true);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), spectra, 0, 20, 1, true);

    SplineInterpolation alg;
    runAlgorithm(alg, order, iws, mws);
    checkOutput(alg);
  }

  void testAxisCopy() {
    int order(2), spectra(3);

    // create binned workspaces
    MatrixWorkspace_sptr mws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1, 0, 20, 1, true);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), spectra, 0, 20, 1, true);

    // Add an axis
    auto vAxis = std::make_unique<TextAxis>(spectra);
    vAxis->setLabel(0, "a");
    vAxis->setLabel(1, "b");
    vAxis->setLabel(2, "c");
    iws->replaceAxis(1, std::move(vAxis));

    SplineInterpolation alg;
    runAlgorithm(alg, order, iws, mws);
    checkOutput(alg);

    // Check the axis values are preserved
    MatrixWorkspace_const_sptr ows = alg.getProperty("OutputWorkspace");
    TextAxis *vAxisOut = dynamic_cast<TextAxis *>(ows->getAxis(1));
    TS_ASSERT(vAxisOut);
    if (vAxisOut) {
      TS_ASSERT_EQUALS(vAxisOut->label(0), "a");
      TS_ASSERT_EQUALS(vAxisOut->label(1), "b");
      TS_ASSERT_EQUALS(vAxisOut->label(2), "c");
    }
  }

  void checkOutput(const SplineInterpolation &alg) const {
    MatrixWorkspace_const_sptr ows = alg.getProperty("OutputWorkspace");
    WorkspaceGroup_const_sptr derivs = alg.getProperty("OutputWorkspaceDeriv");

    for (size_t i = 0; i < ows->getNumberHistograms(); ++i) {
      MatrixWorkspace_const_sptr derivsWs = std::dynamic_pointer_cast<const MatrixWorkspace>(derivs->getItem(i));

      NumericAxis *derivVAxis = dynamic_cast<NumericAxis *>(derivsWs->getAxis(1));
      TS_ASSERT(derivVAxis);
      if (derivVAxis) {
        for (size_t i = 0; i < derivsWs->getNumberHistograms(); i++)
          TS_ASSERT_EQUALS((*derivVAxis)(i), i + 1);
      }

      const auto &xs = ows->x(i);
      const auto &ys = ows->y(i);
      const auto &d1 = derivsWs->y(0);
      const auto &d2 = derivsWs->y(1);

      // check output for consistency
      for (size_t j = 0; j < ys.size(); ++j) {
        TS_ASSERT_DELTA(ys[j], xs[j] * 2, 1e-15);
        TS_ASSERT_DELTA(d1[j], 2, 1e-15);
        TS_ASSERT_DELTA(d2[j], 0, 1e-15);
      }
    }
  }

  void runAlgorithm(SplineInterpolation &alg, int order, const Mantid::API::MatrixWorkspace_sptr &iws,
                    const Mantid::API::MatrixWorkspace_sptr &mws, const bool linear = false) const {
    alg.initialize();
    alg.isInitialized();
    alg.setChild(true);
    alg.setPropertyValue("OutputWorkspace", "Anon");
    alg.setPropertyValue("OutputWorkspaceDeriv", "AnonDeriv");

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DerivOrder", order));

    alg.setProperty("WorkspaceToInterpolate", iws);
    alg.setProperty("WorkspaceToMatch", mws);
    alg.setProperty("Linear2Points", linear);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }
};

class SplineInterpolationTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {

    constexpr int order(2), spectra(1);
    constexpr int xStartVal(0), xEndVal(100);
    constexpr int xStepVal(1);

    MatrixWorkspace_sptr matWs = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        SplineFunc(), spectra, xStartVal, xEndVal, xStepVal, false);

    MatrixWorkspace_sptr inWs = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        SplineFunc(), spectra, xStartVal, xEndVal, (xStepVal * 0.1), false);

    inputWs = inWs;
    matrixWs = matWs;

    splineInterpAlg.initialize();
    splineInterpAlg.setPropertyValue("OutputWorkspace", outputWsName);
    splineInterpAlg.setPropertyValue("OutputWorkspaceDeriv", outDerivWsName);

    splineInterpAlg.setProperty("DerivOrder", order);

    splineInterpAlg.setProperty("WorkspaceToInterpolate", inputWs);
    splineInterpAlg.setProperty("WorkspaceToMatch", matrixWs);

    splineInterpAlg.setRethrows(true);
  }

  void testSplineInterpolationPerformance() { TS_ASSERT_THROWS_NOTHING(splineInterpAlg.execute()); }

  void tearDown() override {
    AnalysisDataService::Instance().remove(outputWsName);
    AnalysisDataService::Instance().remove(outDerivWsName);
  }

private:
  SplineInterpolation splineInterpAlg;

  MatrixWorkspace_sptr inputWs;
  MatrixWorkspace_sptr matrixWs;

  const std::string outputWsName = "outputWs";
  const std::string outDerivWsName = "outputDerivativeWs";

  // Functor to generate spline values
  struct SplineFunc {
    double operator()(double x, int) { return x * 2; }
  };
};
