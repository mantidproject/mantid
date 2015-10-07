#ifndef CURVEFITTING_FITTEST_H_
#define CURVEFITTING_FITTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/FakeObjects.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidCurveFitting/Fit.h"
#include "FitTestHelpers.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

namespace {
class TestMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor setting a value for the relative error acceptance
  /// (default=0.01)
  TestMinimizer() {
    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(
                        "SomeOutput", "abc", Kernel::Direction::Output),
                    "Name of the output Workspace holding some output.");
  }

  /// Overloading base class methods.
  std::string name() const { return "TestMinimizer"; }
  /// Do one iteration.
  bool iterate(size_t iter) {
    m_data[iter] = iter;

    if (iter >= m_data.size() - 1) {
      API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
          "Workspace2D", 1, m_data.size(), m_data.size());
      auto &Y = ws->dataY(0);
      for (size_t i = 0; i < Y.size(); ++i) {
        Y[i] = static_cast<double>(m_data[i]);
      }
      setProperty("SomeOutput", ws);
      return false;
    }
    return true;
  }

  /// Return current value of the cost function
  double costFunctionVal() { return 0.0; }
  /// Initialize minimizer.
  virtual void initialize(API::ICostFunction_sptr, size_t maxIterations = 0) {
    m_data.resize(maxIterations);
  }

private:
  std::vector<size_t> m_data;
};

DECLARE_FUNCMINIMIZER(TestMinimizer, TestMinimizer)
}

class FitTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitTest *createSuite() { return new FitTest(); }
  static void destroySuite(FitTest *suite) { delete suite; }

  FitTest() {
    // need to have DataObjects loaded
    FrameworkManager::Instance();
  }

  // Test that Fit copies minimizer's output properties to Fit
  // Test that minimizer's iterate(iter) method is called maxIteration times
  //  and iter passed to iterate() has values within 0 <= iter < maxIterations
  void test_minimizer_output() {
    API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    Fit fit;
    fit.initialize();

    fit.setProperty("Function", "name=LinearBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("MaxIterations", 99);
    fit.setProperty("Minimizer", "TestMinimizer,SomeOutput=MinimizerOutput");
    fit.setProperty("CreateOutput", true);

    fit.execute();
    TS_ASSERT(fit.existsProperty("SomeOutput"));
    TS_ASSERT_EQUALS(fit.getPropertyValue("SomeOutput"), "MinimizerOutput");
    TS_ASSERT(
        API::AnalysisDataService::Instance().doesExist("MinimizerOutput"));

    API::MatrixWorkspace_sptr outWS =
        API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "MinimizerOutput");
    TS_ASSERT(outWS);
    auto &y = outWS->readY(0);
    TS_ASSERT_EQUALS(y.size(), 99);
    for (size_t iter = 0; iter < 99; ++iter) {
      TS_ASSERT_EQUALS(y[iter], static_cast<double>(iter));
    }

    API::AnalysisDataService::Instance().clear();
  }

  // Test that minimizer's output is'n passed to Fit if no other output is
  // created.
  // Other output are: fitting parameters table, calculated values.
  // To create output either CreateOutput must be set to true or Output be set
  // to non-empty string
  void test_minimizer_output_not_passed_to_Fit() {
    API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    Fit fit;
    fit.initialize();

    fit.setProperty("Function", "name=LinearBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("MaxIterations", 99);
    fit.setProperty("Minimizer", "TestMinimizer,SomeOutput=MinimizerOutput");

    fit.execute();
    TS_ASSERT(!fit.existsProperty("SomeOutput"));
    TS_ASSERT(
        !API::AnalysisDataService::Instance().doesExist("MinimizerOutput"));
  }
};

class FitTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitTestPerformance *createSuite() { return new FitTestPerformance(); }
  static void destroySuite(FitTestPerformance *suite) { delete suite; }

  FitTestPerformance() {
    m_smoothWS = FitTestHelpers::generateCurveDataForFit(
        FitTestHelpers::SmoothishGaussians);

    m_onePeakWS =
        FitTestHelpers::generateCurveDataForFit(FitTestHelpers::SingleB2BPeak);
  }

  // tests for a single peak (BackToBackExponential)

  // LM for Levenberg-Marquardt hereafter
  void test_fit_peaks_LM() {
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak,
                    "Levenberg-MarquardtMD");
  }

  void test_fit_peaks_Simplex() {
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak, "Simplex");
  }

  void test_fit_peaks_ConjG_FR() {
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak,
                    "Conjugate gradient (Fletcher-Reeves imp.)");
  }

  void test_fit_peaks_ConjG_PR() {
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak,
                    "Conjugate gradient (Polak-Ribiere imp.)");
  }

  void test_fit_peaks_BFGS() {
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak, "BFGS");
  }

  void test_fit_peaks_Damping() {
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak, "Damping");
  }

  void test_fit_peaks_SteepestDescent() {
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak,
                    "SteepestDescent");
  }

  // Note: does not converge unless you give a better initial guess of
  // parameters. So this is testing 500 iterations but not convergence.
  void test_fit_peaks_FABADA() {
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak, "FABADA");
  }

  // tests for a smooth function (2 Gaussians + linear background)

  void test_fit_smooth_LM() {
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians,
                    "Levenberg-MarquardtMD");
  }

  void test_fit_smooth_Simplex() {
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians, "Simplex");
  }

  // disabled because it is awfully slow: ~20s while others take <1s
  void disabled_test_fit_smooth_ConjG_FR() {
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians,
                    "Conjugate gradient (Fletcher-Reeves imp.)");
  }

  // disabled: awfully slow: ~20s
  void disabled_test_fit_smooth_ConjG_PR() {
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians,
                    "Conjugate gradient (Polak-Ribiere imp.)");
  }

  // disabled: slow: ~5s
  void disabled_test_fit_smooth_BFGS() {
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians, "BFGS");
  }

  void test_fit_smooth_Damping() {
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians, "Damping");
  }

  // disabled: too slow: ~17s
  void disabled_test_fit_smooth_SteepestDescent() {
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians,
                    "SteepestDescent");
  }

  // disabled: too slow: ~10s (and it doesn't converge)
  void disabled_test_fit_smooth_FABADA() {
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians, "FABADA");
  }

private:
  API::MatrixWorkspace_sptr m_smoothWS;
  API::MatrixWorkspace_sptr m_onePeakWS;
};

#endif /*CURVEFITTING_FITMWTEST_H_*/
