#ifndef MANTID_CURVEFITTING_SPLINEINTERPOLATIONTEST_H_
#define MANTID_CURVEFITTING_SPLINEINTERPOLATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/SplineInterpolation.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"

using Mantid::CurveFitting::Algorithms::SplineInterpolation;
using namespace Mantid::API;

class SplineInterpolationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SplineInterpolationTest *createSuite() {
    return new SplineInterpolationTest();
  }
  static void destroySuite(SplineInterpolationTest *suite) { delete suite; }

  // Functor to generate spline values
  struct SplineFunc {
    double operator()(double x, int) { return x * 2; }
  };

  void test_Init() {
    SplineInterpolation alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExec() {
    int order(2), spectra(1);

    // create binned workspaces
    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, false);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(
            SplineFunc(), spectra, 0, 20, 0.1, false);

    SplineInterpolation alg;
    runAlgorithm(alg, order, iws, mws);
    checkOutput(alg);
  }

  void testExecHistogramData() {
    int order(2), spectra(1);

    // create binned workspaces
    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, true);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(
            SplineFunc(), spectra, 0, 20, 1, true);

    SplineInterpolation alg;
    runAlgorithm(alg, order, iws, mws);
    checkOutput(alg);
  }

  void test2Points() {
    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, false);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               3, 4, 1, false);

    SplineInterpolation alg;
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "Anon");
    alg.setProperty("WorkspaceToInterpolate", iws);
    alg.setProperty("WorkspaceToMatch", mws);
    alg.setProperty("Linear2Points", true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  void test2PointsThrows() {
    // test input validation
    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, false);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               3, 4, 1, false);

    SplineInterpolation alg;
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "Anon");
    alg.setProperty("WorkspaceToInterpolate", iws);
    alg.setProperty("WorkspaceToMatch", mws);
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());
  }

  void testInterpolationRange() {

    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, false);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(
            SplineFunc(), 1, 3.3, 20, 1, false);

    SplineInterpolation alg;
    alg.initialize();
    alg.isInitialized();
    alg.setChild(true);
    alg.setPropertyValue("OutputWorkspace", "Anon");
    alg.setProperty("WorkspaceToInterpolate", iws);
    alg.setProperty("WorkspaceToMatch", mws);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_const_sptr ows = alg.getProperty("OutputWorkspace");
    const auto &yt = ows->y(0);
    const auto &yiws = iws->y(0);
    // check output: first four points should have constant y values
    for (size_t j = 0; j < 4; ++j) {
      TS_ASSERT_DELTA(yt[j], yiws[0], 1e-15);
    }
  }

  void testReferenceWorkspace() {

    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, true);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, false);

    // Add a unit
    iws->getAxis(0)->setUnit("Time");

    SplineInterpolation alg;
    alg.initialize();
    alg.isInitialized();
    alg.setChild(true);
    alg.setPropertyValue("OutputWorkspace", "Anon");
    alg.setProperty("WorkspaceToInterpolate", iws);
    alg.setProperty("WorkspaceToMatch", mws);
    alg.setProperty("ReferenceWorkspace", "WorkspaceToInterpolate");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check the axis values are preserved
    MatrixWorkspace_const_sptr ows = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(ows->getAxis(0)->unit()->caption(), "t");
    TS_ASSERT(mws->getAxis(0)->unit()->caption() != "t");
    TS_ASSERT(ows->isHistogramData());
  }

  void testExecMultipleSpectra() {
    int order(2), spectra(3);

    // create binned workspaces
    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, true);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(
            SplineFunc(), spectra, 0, 20, 1, true);

    SplineInterpolation alg;
    runAlgorithm(alg, order, iws, mws);
    checkOutput(alg);
  }

  void testAxisCopy() {
    int order(2), spectra(1);

    // create binned workspaces
    MatrixWorkspace_sptr mws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(SplineFunc(), 1,
                                                               0, 20, 1, true);
    MatrixWorkspace_sptr iws =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(
            SplineFunc(), spectra, 0, 20, 1, true);

    // Add an axis
    TextAxis *vAxis = new TextAxis(spectra);
    vAxis->setLabel(0, "a");
    iws->replaceAxis(1, vAxis);

    SplineInterpolation alg;
    runAlgorithm(alg, order, iws, mws);
    checkOutput(alg);

    // Check the axis values are preserved
    MatrixWorkspace_const_sptr ows = alg.getProperty("OutputWorkspace");
    TextAxis *vAxisOut = dynamic_cast<TextAxis *>(ows->getAxis(1));
    TS_ASSERT(vAxisOut);
    if (vAxisOut) {
      TS_ASSERT_EQUALS(vAxisOut->label(0), "a");
    }
  }

  void checkOutput(const SplineInterpolation &alg) const {
    MatrixWorkspace_const_sptr ows = alg.getProperty("OutputWorkspace");
    WorkspaceGroup_const_sptr derivs = alg.getProperty("OutputWorkspaceDeriv");

    for (size_t i = 0; i < ows->getNumberHistograms(); ++i) {
      MatrixWorkspace_const_sptr derivsWs =
          boost::dynamic_pointer_cast<const MatrixWorkspace>(
              derivs->getItem(i));

      NumericAxis *derivVAxis =
          dynamic_cast<NumericAxis *>(derivsWs->getAxis(1));
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

  void runAlgorithm(SplineInterpolation &alg, int order,
                    const Mantid::API::MatrixWorkspace_sptr &iws,
                    const Mantid::API::MatrixWorkspace_sptr &mws) const {
    alg.initialize();
    alg.isInitialized();
    alg.setChild(true);
    alg.setPropertyValue("OutputWorkspace", "Anon");
    alg.setPropertyValue("OutputWorkspaceDeriv", "AnonDeriv");

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DerivOrder", order));

    alg.setProperty("WorkspaceToInterpolate", iws);
    alg.setProperty("WorkspaceToMatch", mws);

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

    MatrixWorkspace_sptr matWs =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(
            SplineFunc(), spectra, xStartVal, xEndVal, xStepVal, false);

    MatrixWorkspace_sptr inWs =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(
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

  void testSplineInterpolationPerformance() {
    TS_ASSERT_THROWS_NOTHING(splineInterpAlg.execute());
  }

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
#endif /* MANTID_CURVEFITTING_SPLINEINTERPOLATIONTEST_H_ */
