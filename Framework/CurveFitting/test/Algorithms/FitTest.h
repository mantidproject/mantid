#ifndef CURVEFITTING_FITTEST_H_
#define CURVEFITTING_FITTEST_H_

#include "MantidTestHelpers/FakeObjects.h"
#include <cxxtest/TestSuite.h>

#include "FitTestHelpers.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/Fit.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::API;

namespace {
class TestMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor setting a value for the relative error acceptance
  /// (default=0.01)
  TestMinimizer() {
    declareProperty(
        Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
            "SomeOutput", "abc", Kernel::Direction::Output),
        "Name of the output Workspace holding some output.");
  }

  /// Overloading base class methods.
  std::string name() const override { return "TestMinimizer"; }
  /// Do one iteration.
  bool iterate(size_t iter) override {
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
  double costFunctionVal() override { return 0.0; }
  /// Initialize minimizer.
  void initialize(API::ICostFunction_sptr, size_t maxIterations = 0) override {
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

  void test_function_Abragam() {

    // create mock data to test against
    int ndata = 21;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i);
      e[i] = 0.01;
    }
    y = {0.212132034,  0.110872429,  -0.004130004, -0.107644046, -0.181984622,
         -0.218289678, -0.215908947, -0.180739307, -0.123016506, -0.054943061,
         0.011526466,  0.066481012,  0.103250678,  0.118929645,  0.114251678,
         0.092934753,  0.060672555,  0.023977227,  -0.010929869, -0.039018774,
         -0.057037526};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=Abragam");
    fit.setProperty("InputWorkspace", ws);
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.000001, 0.000001);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 0.3, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Omega"), 0.4, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Phi"), M_PI / 4.0, 0.01); // 45 degrees
    TS_ASSERT_DELTA(func->getParameter("Sigma"), 0.2, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Tau"), 2.0, 0.01);
  }

  void test_function_ExpDecayMuon() {

    // Mock data
    int ndata = 19;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i);
      e[i] = 1.0;
    }
    y = {5.0,
         3.582656552869,
         2.567085595163,
         1.839397205857,
         1.317985690579,
         0.9443780141878,
         0.6766764161831,
         0.484859839322,
         0.347417256114,
         0.2489353418393,
         0.1783699667363,
         0.1278076660325,
         0.09157819444367,
         0.0656186436847,
         0.04701781275748,
         0.03368973499543,
         0.02413974996916,
         0.01729688668232,
         0.01239376088333};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=ExpDecayMuon");
    fit.setProperty("InputWorkspace", ws);
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0001, 0.0001);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 5, 0.0001);
    TS_ASSERT_DELTA(func->getParameter("Lambda"), 0.3333, 0.001);
  }

  void test_function_ExpDecayOsc() {

    // Mock data
    int ndata = 20;
    const double sqrh = 0.70710678; // cos( 45 degrees )

    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i);
      e[i] = 1.;
    }
    y = {5 * sqrh,
         0.0,
         -2.567085595163 * sqrh,
         -1.839397205857,
         -1.317985690579 * sqrh,
         0.0,
         0.6766764161831 * sqrh,
         0.484859839322,
         0.347417256114 * sqrh,
         0.0,
         -0.1783699667363 * sqrh,
         -0.1278076660325,
         -0.09157819444367 * sqrh,
         0.0,
         0.04701781275748 * sqrh,
         0.03368973499543,
         0.02413974996916 * sqrh,
         0.0,
         -0.01239376088333 * sqrh,
         0.0};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=ExpDecayOsc");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Constraints", "0.01 < Frequency < 0.2, 0.01 < Phi < 1.0");
    fit.execute();

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 5, 0.01);
    TS_ASSERT_DELTA(func->getParameter("Lambda"), 1 / 3.0, 0.01);
    TS_ASSERT_DELTA(func->getParameter("Frequency"), 1 / 8.0,
                    0.01);                                    // Period of 8
    TS_ASSERT_DELTA(func->getParameter("Phi"), M_PI_4, 0.01); // 45 degrees
  }

  void test_function_GausDecay() {

    // Mock data
    int ndata = 18;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i - 8);
      e[i] = 1.0;
    }
    y = {0.01,  0.16,  1.2,   5.6,  18.2, 43.68, 80.08, 114.4, 128.7,
         114.4, 80.08, 43.68, 18.2, 5.6,  1.2,   0.16,  0.01,  0.00};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=GausDecay");
    fit.setProperty("InputWorkspace", ws);
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1.0);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 129.194, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Sigma"), 0.348, 0.001);
  }

  void test_function_GausOsc() {

    // Mock data
    int ndata = 18;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i - 8);
      e[i] = 1.0;
    }
    const double sqrh = 0.70710678; // cos( 45 degrees )
    y = {0.01 * sqrh,  0.00,  -1.2 * sqrh,  -5.6, -18.2 * sqrh,  0.0,
         80.08 * sqrh, 114.4, 128.7 * sqrh, 0.0,  -80.08 * sqrh, -43.68,
         -18.2 * sqrh, 0.0,   1.2 * sqrh,   0.16, 0.01 * sqrh,   0.00};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=GausOsc");
    fit.setProperty("InputWorkspace", ws);
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1.0);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 129.300, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Sigma"), 0.348, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Frequency"), 1 / 8.0,
                    0.01);                                    // Period of 8
    TS_ASSERT_DELTA(func->getParameter("Phi"), M_PI_4, 0.01); //  45 degrees
  }

  void test_function_StaticKuboToyabe() {

    // Mock data
    int ndata = 18;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i);
      e[i] = 0.01;
    }
    // Calculated with A = 0.24 and Delta = 0.16 on an Excel spreadsheet
    y = {0.24,        0.233921146, 0.216447929, 0.189737312, 0.156970237,
         0.121826185, 0.08791249,  0.058260598, 0.034976545, 0.019090369,
         0.01060189,  0.008680652, 0.011954553, 0.018817301, 0.027696749,
         0.037247765, 0.046457269, 0.054669182};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=StaticKuboToyabe");
    fit.setProperty("InputWorkspace", ws);
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0001, 0.0001);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 0.24, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Delta"), 0.16, 0.001);
  }

  void test_function_StaticKuboToyabeTimesExpDecay() {

    // Mock data
    int ndata = 15;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i);
      e[i] = 1.0;
    }
    // A = 0.24, Delta = 0.16, Lambda = 0.1
    y = {0.24,       0.211661,   0.177213,   0.140561,   0.10522,
         0.0738913,  0.0482474,  0.0289314,  0.015716,   0.00776156,
         0.00390022, 0.00288954, 0.00360064, 0.00512831, 0.00682993};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=StaticKuboToyabeTimesExpDecay");
    fit.setProperty("InputWorkspace", ws);
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0001, 0.0001);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 0.24, 0.0001);
    TS_ASSERT_DELTA(func->getParameter("Delta"), 0.16, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Lambda"), 0.1, 0.001);
  }

  void test_function_StaticKuboToyabeTimesGausDecay() {

    // Mock data
    int ndata = 15;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i);
      e[i] = 1.0;
    }
    // A = 0.24, Delta = 0.16, Sigma = 0.1
    y = {0.24,       0.231594,   0.207961,   0.173407,   0.133761,
         0.0948783,  0.0613345,  0.035692,   0.0184429,  0.0084925,
         0.00390022, 0.00258855, 0.00283237, 0.00347216, 0.00390132};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=StaticKuboToyabeTimesGausDecay");
    fit.setProperty("InputWorkspace", ws);
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0001, 0.0001);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 0.24, 0.0001);
    TS_ASSERT_DELTA(func->getParameter("Delta"), 0.16, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Sigma"), 0.1, 0.001);
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
