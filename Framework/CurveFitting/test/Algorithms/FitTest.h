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

  void test_function_Keren() {

    // Mock data
    int ndata = 41;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    x = {0,       0.922276, 1.84455, 2.76683, 3.68911, 4.61138, 5.53366,
         6.45594, 7.37821,  8.30049, 9.22276, 10.145,  11.0673, 11.9896,
         12.9119, 13.8341,  14.7564, 15.6787, 16.601,  17.5233, 18.4455,
         19.3678, 20.2901,  21.2124, 22.1346, 23.0569, 23.9792, 24.9015,
         25.8237, 26.746,   27.6683, 28.5906, 29.5128, 30.4351, 31.3574,
         32.2797, 33.202,   34.1242, 35.0465, 35.9688, 36.8911};
    y = {1,        0.950342, 0.875263, 0.848565, 0.859885, 0.8632,   0.839704,
         0.808929, 0.790497, 0.782535, 0.772859, 0.75648,  0.738228, 0.723282,
         0.711316, 0.69916,  0.685455, 0.671399, 0.658356, 0.646277, 0.634338,
         0.622165, 0.610055, 0.598363, 0.587083, 0.575999, 0.565007, 0.554178,
         0.543602, 0.533278, 0.523147, 0.513177, 0.503385, 0.493792, 0.484394,
         0.475175, 0.466123, 0.45724,  0.448529, 0.439988, 0.43161};
    e = {0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01,
         0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01,
         0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01,
         0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=Keren, Field=80, Fluct=0.2, Delta=0.2");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("WorkspaceIndex", 0);

    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());
    std::string status = fit.getPropertyValue("OutputStatus");
    TS_ASSERT_EQUALS("success", status);

    // check the output
    const double field = 100;
    const double delta =
        Mantid::PhysicalConstants::MuonGyromagneticRatio * field * 0.2;
    const double fluct = delta;
    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("Field"), field, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Delta"), delta, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Fluct"), fluct, 0.001);
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

  void test_function_StaticKuboToyabeTimesStretchExp() {

    // Mock data
    int ndata = 18;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i);
      e[i] = 1.0;
    }
    // Calculated with A = 0.24, Delta = 0.06, Lambda = 0.63 and
    // Beta = 0.63
    y = {0.24,        0.113248409, 0.074402367, 0.052183632, 0.037812471,
         0.027927981, 0.020873965, 0.015717258, 0.011885418, 0.009005914,
         0.006825573, 0.005166593, 0.003900885, 0.002934321, 0.002196637,
         0.001634742, 0.001208136, 0.000885707};

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=StaticKuboToyabeTimesStretchExp");
    fit.setProperty("InputWorkspace", ws);
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0001, 0.0001);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 0.24, 0.0001);
    TS_ASSERT_DELTA(func->getParameter("Delta"), 0.06, 0.001);
    TS_ASSERT_DELTA(func->getParameter("Lambda"), 0.63, 0.001);
    TS_ASSERT_LESS_THAN(func->getParameter("Beta"), 1.00);
  }

  void test_function_StretchExpMuon() {

    // Mock data
    int ndata = 20;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    // values extracted from y(x)=2*exp(-(x/4)^0.5)
    y = {2,          1.2130613,  0.98613738, 0.84124005, 0.73575888,
         0.65384379, 0.58766531, 0.53273643, 0.48623347, 0.44626032,
         0.41148132, 0.38092026, 0.35384241, 0.32968143, 0.30799199,
         0.28841799, 0.27067057, 0.25451242, 0.2397465,  0.22620756};
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i);
      e[i] = 0.1 * y[i];
    }

    Fit fit;
    fit.initialize();
    fit.setProperty("Function",
                    "name=StretchExpMuon, A=1.5, Lambda=0.2, Beta=0.4");
    fit.setProperty("InputWorkspace", ws);
    fit.setPropertyValue("StartX", "0");
    fit.setPropertyValue("EndX", "19");
    fit.execute();

    // Test the goodness of the fit
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.001, 0.001);

    // Test the fitting parameters
    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("A"), 2.0, 0.02);
    TS_ASSERT_DELTA(func->getParameter("Lambda"), 0.25, 0.0025);
    TS_ASSERT_DELTA(func->getParameter("Beta"), 0.5, 0.05);
  }

  void test_function_EndErfc() {

    // Mock data
    int ndata = 13;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    // values extracted from y(x)=2*exp(-(x/4)^0.5)
    y = {1, 3, 4, 28, 221, 872, 1495, 1832, 1830, 1917, 2045, 1996, 0};
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(5 * i);
      e[i] = 1.0;
    }

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=EndErfc, A=2000, B=50, C=6, D=0");
    fit.setProperty("InputWorkspace", ws);
    fit.setPropertyValue("StartX", "5");
    fit.setPropertyValue("EndX", "55");
    fit.execute();

    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.0001, 20000);

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A"), 1000, 30.0);
    TS_ASSERT_DELTA(out->getParameter("B"), 26, 0.1);
    TS_ASSERT_DELTA(out->getParameter("C"), 7.7, 0.1);
    TS_ASSERT_DELTA(out->getParameter("D"), 0, 0.1);
  }

  void test_function_Bk2BkExpConvPV() {

    // Mock data
    int ndata = 35;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    // values extracted from y(x)=2*exp(-(x/4)^0.5)
    x = {54999.094000, 55010.957000, 55022.820000, 55034.684000, 55046.547000,
         55058.410000, 55070.273000, 55082.137000, 55094.000000, 55105.863000,
         55117.727000, 55129.590000, 55141.453000, 55153.320000, 55165.184000,
         55177.047000, 55188.910000, 55200.773000, 55212.637000, 55224.500000,
         55236.363000, 55248.227000, 55260.090000, 55271.953000, 55283.816000,
         55295.680000, 55307.543000, 55319.406000, 55331.270000, 55343.133000,
         55354.996000, 55366.859000, 55378.727000, 55390.590000, 55402.453000};
    y = {2.628336,    4.034647,   6.193415,   9.507247,   14.594171,
         22.402889,   34.389721,  52.790192,  81.035973,  124.394840,
         190.950440,  293.010220, 447.602290, 664.847780, 900.438170,
         1028.003700, 965.388730, 787.024410, 603.501770, 456.122890,
         344.132350,  259.611210, 195.848420, 147.746310, 111.458510,
         84.083313,   63.431709,  47.852318,  36.099365,  27.233042,
         20.544367,   15.498488,  11.690837,  8.819465,   6.653326};
    for (int i = 0; i < ndata; i++) {
      e[i] = std::sqrt(fabs(y[i]));
    }

    Fit fit;
    fit.initialize();
    TS_ASSERT(fit.isInitialized());
    fit.setProperty("Function", "name=Bk2BkExpConvPV, Height=1000");
    fit.setProperty("Ties", "TOF_h=55175.79, Alpha=0.03613, Beta=0.02376, "
                            "Sigma2=187.50514, Gamma=0");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
    fit.setProperty("CostFunction", "Least squares");
    fit.setProperty("MaxIterations", 100);
    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());

    // Test fitting results

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT(chi2 < 1.5);

    std::string fitStatus = fit.getProperty("OutputStatus");
    TS_ASSERT_EQUALS(fitStatus, "success");

    IFunction_sptr func = fit.getProperty("Function");
    TS_ASSERT_DELTA(func->getParameter("TOF_h"), 55175.79, 1.0E-8);
    TS_ASSERT_DELTA(func->getParameter("Height"), 96000, 100);
  }

  void test_function_Gaussian_LMMinimizer() {

    // Mock data
    int ndata = 20;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    y = {3.56811123,   3.25921675,  2.69444562,  3.05054488,  2.86077216,
         2.29916480,   2.57468876,  3.65843827,  15.31622763, 56.57989073,
         101.20662386, 76.30364797, 31.54892552, 8.09166673,  3.20615343,
         2.95246554,   2.75421444,  3.70180447,  2.77832668,  2.29507565};
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i + 1);
      y[i] -= 2.8765;
    }
    e = {1.72776328,  1.74157482, 1.73451042, 1.73348562, 1.74405622,
         1.72626701,  1.75911386, 2.11866496, 4.07631054, 7.65159052,
         10.09984173, 8.95849024, 5.42231173, 2.64064858, 1.81697576,
         1.72347732,  1.73406310, 1.73116711, 1.71790285, 1.72734254};

    Fit fit;
    fit.initialize();
    TS_ASSERT(fit.isInitialized());
    fit.setProperty("Function",
                    "name=Gaussian, PeakCentre=11.2, Height=100.7, Sigma=2.2");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());

    // test the output from fit
    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.035, 0.01);

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("Height"), 97.8036, 0.0001);
    TS_ASSERT_DELTA(out->getParameter("PeakCentre"), 11.2356, 0.0001);
    TS_ASSERT_DELTA(out->getParameter("Sigma") * 2.0 * sqrt(2.0 * M_LN2),
                    2.6237, 0.0001);
  }

  void test_function_Gaussian_SimplexMinimizer() {

    // Mock data
    int ndata = 20;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    y = {3.56811123,   3.25921675,  2.69444562,  3.05054488,  2.86077216,
         2.29916480,   2.57468876,  3.65843827,  15.31622763, 56.57989073,
         101.20662386, 76.30364797, 31.54892552, 8.09166673,  3.20615343,
         2.95246554,   2.75421444,  3.70180447,  2.77832668,  2.29507565};
    for (int i = 0; i < ndata; i++) {
      x[i] = static_cast<double>(i + 1);
      y[i] -= 2.8765;
    }
    e = {1.72776328,  1.74157482, 1.73451042, 1.73348562, 1.74405622,
         1.72626701,  1.75911386, 2.11866496, 4.07631054, 7.65159052,
         10.09984173, 8.95849024, 5.42231173, 2.64064858, 1.81697576,
         1.72347732,  1.73406310, 1.73116711, 1.71790285, 1.72734254};

    Fit fit;
    fit.initialize();
    TS_ASSERT(fit.isInitialized());
    fit.setProperty(
        "Function",
        "name=Gaussian, PeakCentre=11.2, Height=100.7, Sigma=0.934254");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Minimizer", "Simplex");
    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());

    std::string minimizer = fit.getProperty("Minimizer");
    TS_ASSERT(minimizer.compare("Simplex") == 0);

    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.035, 0.01);

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("Height"), 97.8091, 0.01);
    TS_ASSERT_DELTA(out->getParameter("PeakCentre"), 11.2356, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Sigma") * 2.0 * sqrt(2.0 * M_LN2),
                    2.6240, 0.001);
  }

  void test_function_Gaussian_HRP38692Data() {

    // Pick values taken from HRPD_for_UNIT_TESTING.xml
    // here we have an example where an upper constraint on Sigma <= 100 makes
    // the Gaussian fit below success. The starting value of Sigma is here 300.
    // Note that the fit is equally successful if we had no constraint on Sigma
    // and used a starting of Sigma = 100.

    int ndata = 41;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    // x-values in time-of-flight
    for (int i = 0; i < 8; i++)
      x[i] = 79292.4375 + 7.875 * double(i);
    for (int i = 8; i < ndata; i++)
      x[i] = 79347.625 + 8.0 * (double(i) - 8.0);
    // y-values
    y = {7,   8,   4,   9,   4,   10,  10,  5,   8,   7,  10, 18, 30, 71,
         105, 167, 266, 271, 239, 221, 179, 133, 126, 88, 85, 52, 37, 51,
         32,  31,  17,  21,  15,  13,  12,  12,  10,  7,  5,  9,  6};
    // errors are the square root of the Y-value
    for (int i = 0; i < ndata; i++)
      e[i] = sqrt(y[i]);

    Fit fit;
    fit.initialize();
    TS_ASSERT(fit.isInitialized());
    fit.setProperty("Function", "name=LinearBackground, A0=0, A1=0; "
                                "name=Gaussian, PeakCentre=79450.0, "
                                "Height=200.0, Sigma=300");
    fit.setProperty("Constraints", "20 < f1.Sigma < 100");
    fit.setProperty("Ties", "f0.A1=0");
    fit.setProperty("InputWorkspace", ws);
    fit.setPropertyValue("StartX", "79300");
    fit.setPropertyValue("EndX", "79600");
    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());

    // test the output from fit is what you expect
    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 5.2, 0.1);
    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("f1.Height"), 232., 1);
    TS_ASSERT_DELTA(out->getParameter("f1.PeakCentre"), 79430.1, 10);
    TS_ASSERT_DELTA(out->getParameter("f1.Sigma"), 26.0, 0.1);
    TS_ASSERT_DELTA(out->getParameter("f0.A0"), 8.09, 0.1);
    TS_ASSERT_DELTA(out->getParameter("f0.A1"), 0.0, 0.01);
  }

  void test_function_Gaussian_HRP38692Data_SimplexMinimizer() {

    // here we have an example where an upper constraint on Sigma <= 100
    // makes
    // the Gaussian fit below success. The starting value of Sigma is here
    // 300.
    // Note that the fit is equally successful if we had no constraint on
    // Sigma
    // and used a starting of Sigma = 100.
    // Note that the no constraint simplex with Sigma = 300 also does not
    // locate
    // the correct minimum but not as badly as levenberg-marquardt

    int ndata = 41;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    // x-values in time-of-flight
    for (int i = 0; i < 8; i++)
      x[i] = 79292.4375 + 7.875 * double(i);
    for (int i = 8; i < ndata; i++)
      x[i] = 79347.625 + 8.0 * (double(i) - 8.0);
    // y-values
    y = {7,   8,   4,   9,   4,   10,  10,  5,   8,   7,  10, 18, 30, 71,
         105, 167, 266, 271, 239, 221, 179, 133, 126, 88, 85, 52, 37, 51,
         32,  31,  17,  21,  15,  13,  12,  12,  10,  7,  5,  9,  6};
    // errors are the square root of the Y-value
    for (int i = 0; i < ndata; i++)
      e[i] = sqrt(y[i]);

    Fit fit;
    fit.initialize();
    TS_ASSERT(fit.isInitialized());
    fit.setProperty("Function", "name=LinearBackground, A0=0, A1=0; "
                                "name=Gaussian, PeakCentre=79450.0, "
                                "Height=200.0, Sigma=10.0");
    fit.setProperty("Constraints", "20 < f1.Sigma < 100");
    fit.setProperty("Ties", "f0.A1=0");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Minimizer", "Simplex");
    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());

    std::string minimizer = fit.getProperty("Minimizer");
    TS_ASSERT(minimizer.compare("Simplex") == 0);

    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 2.5911, 1);

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("f1.Height"), 232, 1);
    TS_ASSERT_DELTA(out->getParameter("f1.PeakCentre"), 79430, 1);
    TS_ASSERT_DELTA(out->getParameter("f1.Sigma"), 26.08, 1);
    TS_ASSERT_DELTA(out->getParameter("f0.A0"), 8, 1);
    TS_ASSERT_DELTA(out->getParameter("f0.A1"), 0.0, 0.01);
  }

#if !(defined __APPLE__)
  /**
  * Changing compiler on OS X has yet again caused this (and only this) test to
  * fail.
  * Switch it off until it is clear why the other Fit tests are okay on OS X
  * using Intel
  */
  void test_Function_IkedaCarpenterPV_NoInstrument() {
    // Try to fit an IC peak to a Gaussian mock data peak
    // Note that fitting a none-totally optimized IC to a Gaussian peak so
    // not a perfect fit - but pretty ok result

    int ndata = 31;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    y = {0.0000,  0.0003,  0.0028,  0.0223,  0.1405,  0.6996,  2.7608,  8.6586,
         21.6529, 43.3558, 69.8781, 91.2856, 97.5646, 86.4481, 64.7703, 42.3348,
         25.3762, 15.0102, 9.4932,  6.7037,  5.2081,  4.2780,  3.6037,  3.0653,
         2.6163,  2.2355,  1.9109,  1.6335,  1.3965,  1.1938,  1.0206};
    e = {0.0056, 0.0176, 0.0539, 0.1504, 0.3759, 0.8374, 1.6626, 2.9435,
         4.6543, 6.5855, 8.3603, 9.5553, 9.8785, 9.2987, 8.0490, 6.5075,
         5.0385, 3.8753, 3.0821, 2.5902, 2.2831, 2.0693, 1.8993, 1.7518,
         1.6185, 1.4962, 1.3833, 1.2791, 1.1827, 1.0936, 1.0112};
    for (int i = 0; i < ndata; i++) {
      x[i] = i * 5;
    }

    Fit fit;
    fit.initialize();
    TS_ASSERT(fit.isInitialized());
    fit.setProperty(
        "Function",
        "name=IkedaCarpenterPV, I=1000, SigmaSquared=25.0, Gamma=0.1, X0=50.0");
    fit.setProperty("Ties", "Alpha0=1.6, Alpha1=1.5, Beta0=31.9, Kappa=46.0");
    fit.setProperty("InputWorkspace", ws);
    fit.setPropertyValue("StartX", "0");
    fit.setPropertyValue("EndX", "150");
    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());

    // test the output from fit is what you expect
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 0.1);

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("I"), 3101.672, 0.1);
    TS_ASSERT_DELTA(out->getParameter("Alpha0"), 1.6, 0.0001);
    TS_ASSERT_DELTA(out->getParameter("Alpha1"), 1.5, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Beta0"), 31.9, 0.0001);
    TS_ASSERT_DELTA(out->getParameter("Kappa"), 46.0, 0.0001);
    TS_ASSERT_DELTA(out->getParameter("SigmaSquared"), 99.935, 0.1);
    TS_ASSERT_DELTA(out->getParameter("Gamma"), 0.0, 0.1);
    TS_ASSERT_DELTA(out->getParameter("X0"), 49.984, 0.1);
  }

  void test_Function_IkedaCarpenterPV_FullInstrument_DeltaE() {
    // create mock data to test against
    int ndata = 31;
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        2, ndata, false, false, false);
    ws->getAxis(0)->setUnit("DeltaE");
    for (int i = 0; i < ndata; i++) {
      ws->dataX(0)[i] = i * 5;
    }
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    y = {0.0000,  0.0003,  0.0028,  0.0223,  0.1405,  0.6996,  2.7608,  8.6586,
         21.6529, 43.3558, 69.8781, 91.2856, 97.5646, 86.4481, 64.7703, 42.3348,
         25.3762, 15.0102, 9.4932,  6.7037,  5.2081,  4.2780,  3.6037,  3.0653,
         2.6163,  2.2355,  1.9109,  1.6335,  1.3965,  1.1938,  1.0206};
    e = {0.0056, 0.0176, 0.0539, 0.1504, 0.3759, 0.8374, 1.6626, 2.9435,
         4.6543, 6.5855, 8.3603, 9.5553, 9.8785, 9.2987, 8.0490, 6.5075,
         5.0385, 3.8753, 3.0821, 2.5902, 2.2831, 2.0693, 1.8993, 1.7518,
         1.6185, 1.4962, 1.3833, 1.2791, 1.1827, 1.0936, 1.0112};
    for (int i = 0; i < ndata; i++) {
      x[i] = i * 5;
    }

    // Direct

    Fit fitDirect;
    fitDirect.initialize();
    TS_ASSERT(fitDirect.isInitialized());
    fitDirect.setProperty("Function", "name=IkedaCarpenterPV, I=1000, "
                                      "SigmaSquared=25.0, Gamma=0.1, X0=50.0");
    fitDirect.setProperty("InputWorkspace", ws);
    fitDirect.setProperty("Ties",
                          "Alpha0=1.6, Alpha1=1.5, Beta0=31.9, Kappa=46.0");
    fitDirect.setPropertyValue("StartX", "0");
    fitDirect.setPropertyValue("EndX", "150");

    // Set efixed for direct
    ws->mutableRun().addProperty<std::string>("deltaE-mode", "direct");
    ws->mutableRun().addProperty<double>("Ei", 11.0);
    TS_ASSERT_THROWS_NOTHING(fitDirect.execute());
    TS_ASSERT(fitDirect.isExecuted());

    double chi2 = fitDirect.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 22.745, 0.1);

    // Indirect

    Fit fitIndirect;
    fitIndirect.initialize();
    TS_ASSERT(fitIndirect.isInitialized());
    fitIndirect.setProperty("Function",
                            "name=IkedaCarpenterPV, I=1000, "
                            "SigmaSquared=25.0, Gamma=0.1, X0=50.0");
    fitIndirect.setProperty("InputWorkspace", ws);
    fitIndirect.setProperty("Ties",
                            "Alpha0=1.6, Alpha1=1.5, Beta0=31.9, Kappa=46.0");
    fitIndirect.setPropertyValue("StartX", "0");
    fitIndirect.setPropertyValue("EndX", "150");

    // Set efixed for indirect
    ws->mutableRun().addProperty<std::string>("deltaE-mode", "indirect", true);
    auto &pmap = ws->instrumentParameters();
    auto inst = ws->getInstrument()->baseInstrument();
    pmap.addDouble(inst.get(), "EFixed", 20.0);
    TS_ASSERT_THROWS_NOTHING(fitIndirect.execute());
    TS_ASSERT(fitIndirect.isExecuted());

    chi2 = fitIndirect.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.5721, 1);
  }
#endif

  void test_function_LogNormal() {

    // Mock data
    int ndata = 20;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    y = {0.0,        1.52798e-15, 6.4577135e-07, 0.0020337351, 0.12517292,
         1.2282908,  4.3935083,   8.5229866,     11.127883,    11.110426,
         9.1925694,  6.6457304,   4.353104,      2.6504159,    1.5279732,
         0.84552286, 0.45371715,  0.23794487,    0.12268847,   0.0624878};
    for (int i = 0; i < ndata; i++) {
      x[i] = i;
      e[i] = 0.1 * y[i];
    }

    Fit fit;
    fit.initialize();
    fit.setProperty("Function",
                    "name=LogNormal, Height=90., Location=2., Scale=0.2");
    fit.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(fit.execute()))
    TS_ASSERT(fit.isExecuted());

    // test the output from fit is what you expect
    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.001, 0.001);

    IFunction_sptr out = fit.getProperty("Function");
    // golden standard y(x) = 100.0 / x * exp( -(log(x)-2.2)^2/(2*0.25^2) )
    TS_ASSERT_DELTA(out->getParameter("Height"), 100.0, 0.1);
    TS_ASSERT_DELTA(out->getParameter("Location"), 2.2, 0.1);
    TS_ASSERT_DELTA(out->getParameter("Scale"), 0.25, 0.01);
  }

  void test_function_PseudoVoigt() {

    // Mock data
    int ndata = 100;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    y = {0.680508, 0.459591, 0.332266, 1.2717,   0.925787, 1.36216,  0.890605,
         0.983653, 0.965918, 0.916039, 0.979414, 0.861061, 0.973214, 1.53418,
         1.52668,  1.10537,  1.36965,  1.64708,  1.52887,  2.0042,   2.11257,
         2.44183,  2.29917,  2.61657,  2.25268,  2.82788,  3.089,    3.45517,
         3.41001,  4.39168,  5.0277,   5.2431,   6.8158,   7.80098,  9.45674,
         11.6082,  14.9449,  17.964,   22.4709,  28.9806,  35.2087,  42.7603,
         51.2697,  61.032,   71.2193,  81.0546,  90.7571,  99.5076,  106.364,
         111.216,  112.877,  111.288,  106.463,  99.5477,  90.7675,  81.7059,
         71.0115,  61.3214,  51.5543,  42.6311,  35.1712,  28.3785,  22.593,
         18.2557,  14.7387,  11.8552,  9.44558,  8.04787,  6.46706,  5.64766,
         4.62926,  4.28496,  4.01921,  3.85923,  3.15543,  2.44881,  2.2804,
         2.08211,  2.47078,  2.47588,  2.45599,  1.88098,  1.76205,  1.37918,
         1.95951,  1.97868,  1.24903,  1.15062,  1.33571,  0.965367, 1.07663,
         1.40468,  0.982297, 0.85258,  1.23184,  0.882275, 0.911729, 0.614329,
         1.26008,  1.07271};
    for (int i = 0; i < ndata; ++i) {
      x[i] = static_cast<double>(i) * 0.01 - 0.5;
      e[i] = sqrt(fabs(y[i]));
    }

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=PseudoVoigt, PeakCentre=0.0, FWHM=0.15, "
                                "Height=112.78, Mixing=0.7");
    fit.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(fit.execute()))
    TS_ASSERT(fit.isExecuted());

    IFunction_sptr fitted = fit.getProperty("Function");
    TS_ASSERT_DELTA(fitted->getError(0), 0.0, 1e-6);
    TS_ASSERT_DELTA(fitted->getError(2), 0.0, 1e-6);
    TS_ASSERT_DELTA(fitted->getError(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(fitted->getError(3), 0.0, 1e-6);
    TS_ASSERT_DELTA(fitted->getParameter("Mixing"), 0.7, 1e-2);
    TS_ASSERT_DELTA(fitted->getParameter("PeakCentre"), 0.0, 1e-4);
    TS_ASSERT_DELTA(fitted->getParameter("Height"), 112.78, 0.5);
    TS_ASSERT_DELTA(fitted->getParameter("FWHM"), 0.15, 1e-2);
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
