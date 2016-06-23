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
    y = {5.0, 3.582656552869, 2.567085595163, 1.839397205857, 1.317985690579,
         0.9443780141878, 0.6766764161831, 0.484859839322, 0.347417256114,
         0.2489353418393, 0.1783699667363, 0.1278076660325, 0.09157819444367,
         0.0656186436847, 0.04701781275748, 0.03368973499543, 0.02413974996916,
         0.01729688668232, 0.01239376088333};

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
    y = {5 * sqrh, 0.0, -2.567085595163 * sqrh, -1.839397205857,
         -1.317985690579 * sqrh, 0.0, 0.6766764161831 * sqrh, 0.484859839322,
         0.347417256114 * sqrh, 0.0, -0.1783699667363 * sqrh, -0.1278076660325,
         -0.09157819444367 * sqrh, 0.0, 0.04701781275748 * sqrh,
         0.03368973499543, 0.02413974996916 * sqrh, 0.0,
         -0.01239376088333 * sqrh, 0.0};

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

    // Test the goodness of the fit. This is fit looks really bad
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 4000, 500);

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A"), 1000, 30.0);
    TS_ASSERT_DELTA(out->getParameter("B"), 26, 0.1);
    TS_ASSERT_DELTA(out->getParameter("C"), 7.7, 0.1);
    TS_ASSERT_DELTA(out->getParameter("D"), 0, 0.1);
  }

  // Background functions:
  // ---------------------

  // A test for [-1, 1] range data
  void test_function_Chebyshev() {
    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 11, 11);

    Mantid::MantidVec &X = ws->dataX(0);
    Mantid::MantidVec &Y = ws->dataY(0);
    Mantid::MantidVec &E = ws->dataE(0);
    for (size_t i = 0; i < Y.size(); i++) {
      double x = -1. + 0.1 * static_cast<double>(i);
      X[i] = x;
      Y[i] = x * x * x;
      E[i] = 1;
    }

    Algorithms::Fit fit;
    fit.initialize();

    fit.setProperty("Function", "name=Chebyshev, n=3");
    fit.setProperty("InputWorkspace", ws);
    fit.setPropertyValue("WorkspaceIndex", "0");

    fit.execute();
    TS_ASSERT(fit.isExecuted());

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A0"), 0, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A1"), 0.75, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A2"), 0, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A3"), 0.25, 1e-12);
  }

  // A test for a random number data
  void test_function_Chebyshev_Background() {
    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 21, 21);

    Mantid::MantidVec &X = ws->dataX(0);
    Mantid::MantidVec &Y = ws->dataY(0);
    Mantid::MantidVec &E = ws->dataE(0);
    for (size_t i = 0; i < Y.size(); i++) {
      double x = -10. + 1 * static_cast<double>(i);
      X[i] = x;
      Y[i] = x * x * x;
      if (Y[i] < 1.0) {
        E[i] = 1.0;
      } else {
        E[i] = sqrt(Y[i]);
      }
    }

    Algorithms::Fit fit;
    fit.initialize();

    const std::string funcString =
        "name=Chebyshev, n=3, StartX=-10.0, EndX=10.0";
    fit.setPropertyValue("Function", funcString);
    fit.setProperty("InputWorkspace", ws);
    fit.setPropertyValue("WorkspaceIndex", "0");

    fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
    fit.setProperty("CostFunction", "Least squares");
    fit.setProperty("MaxIterations", 1000);

    fit.execute();
    TS_ASSERT(fit.isExecuted());

    const double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT(chi2 < 2.0);

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A0"), 0, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A1"), 750, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A2"), 0, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A3"), 250, 1e-12);
  }

  // Test function on a Fullprof polynomial function
  void test_function_FullprofPolynomial() {
    // Create a workspace
    const int histogramNumber = 1;
    const int timechannels = 1000;

    Mantid::API::MatrixWorkspace_sptr ws2D =
        WorkspaceFactory::Instance().create("Workspace2D", histogramNumber,
                                            timechannels, timechannels);

    double tof0 = 8000.;
    double dtof = 5.;

    Mantid::MantidVec &X = ws2D->dataX(0);
    Mantid::MantidVec &Y = ws2D->dataY(0);
    Mantid::MantidVec &E = ws2D->dataE(0);
    for (int i = 0; i < timechannels; i++) {
      X[i] = static_cast<double>(i) * dtof + tof0;
      Y[i] = X[i] * 0.013;
      E[i] = sqrt(Y[i]);
    }

    // Set up fit
    Algorithms::Fit fitalg;
    TS_ASSERT_THROWS_NOTHING(fitalg.initialize());
    TS_ASSERT(fitalg.isInitialized());

    const std::string funcStr = "name=FullprofPolynomial, n=6, Bkpos=10000, "
                                "A0=0.5, A1=1.0, A2=-0.5, A3=0.0, A4=-0.02";
    fitalg.setProperty("Function", funcStr);
    fitalg.setProperty("InputWorkspace", ws2D);
    fitalg.setPropertyValue("WorkspaceIndex", "0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(fitalg.execute()));
    TS_ASSERT(fitalg.isExecuted());

    // test the output from fit is what you expect
    double chi2 = fitalg.getProperty("OutputChi2overDoF");

    TS_ASSERT_DELTA(chi2, 0.0, 0.1);

    IFunction_sptr out = fitalg.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A0"), 130., 0.001);
    TS_ASSERT_DELTA(out->getParameter("A1"), 130., 0.001);
    TS_ASSERT_DELTA(out->getParameter("A3"), 0., 0.001);
  }

  void test_function_LinearBackground() {
    // create mock data to test against
    const int histogramNumber = 1;
    const int timechannels = 5;
    Mantid::API::MatrixWorkspace_sptr ws2D =
        WorkspaceFactory::Instance().create("Workspace2D", histogramNumber,
                                            timechannels, timechannels);

    for (int i = 0; i < timechannels; i++) {
      ws2D->dataX(0)[i] = i + 1;
      ws2D->dataY(0)[i] = i + 1;
      ws2D->dataE(0)[i] = 1.0;
    }

    Algorithms::Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    const std::string funcStr = "name=LinearBackground, A0=1.0";
    alg2.setProperty("Function", funcStr);
    // Set which spectrum to fit against and initial starting values
    alg2.setProperty("InputWorkspace", ws2D);
    alg2.setPropertyValue("WorkspaceIndex", "0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(alg2.execute()))

    TS_ASSERT(alg2.isExecuted());

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");

    TS_ASSERT_DELTA(dummy, 0.0, 0.1);
    IFunction_sptr out = alg2.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A0"), 0.0, 0.01);
    TS_ASSERT_DELTA(out->getParameter("A1"), 1.0, 0.0003);
  }

  void test_function_Polynomial_QuadraticBackground() {
    const int histogramNumber = 1;
    const int timechannels = 5;
    Mantid::API::MatrixWorkspace_sptr ws2D =
        WorkspaceFactory::Instance().create("Workspace2D", histogramNumber,
                                            timechannels, timechannels);

    for (int i = 0; i < timechannels; i++) {
      ws2D->dataX(0)[i] = i + 1;
      ws2D->dataY(0)[i] = (i + 1) * (i + 1) + 2 * (i + 1) + 3.0;
      ws2D->dataE(0)[i] = 1.0;
    }

    CurveFitting::Algorithms::Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // set up fitting function
    const std::string funcStr = "name=Polynomial, n=2, A0=0.0, A1=1.";
    alg2.setProperty("Function", funcStr);
    // Set which spectrum to fit against and initial starting values
    alg2.setProperty("InputWorkspace", ws2D);
    alg2.setPropertyValue("WorkspaceIndex", "0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(alg2.execute()))
    TS_ASSERT(alg2.isExecuted());

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");

    TS_ASSERT_DELTA(dummy, 0.0, 0.1);
    IFunction_sptr out = alg2.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A0"), 3.0, 0.01);
    TS_ASSERT_DELTA(out->getParameter("A1"), 2.0, 0.0003);
    TS_ASSERT_DELTA(out->getParameter("A2"), 1.0, 0.01);
  }

  void test_function_Quadratic() {
    // create mock data to test against
    int histogramNumber = 1;
    int timechannels = 5;
    Mantid::API::MatrixWorkspace_sptr ws2D =
        WorkspaceFactory::Instance().create("Workspace2D", histogramNumber,
                                            timechannels, timechannels);

    for (int i = 0; i < timechannels; i++) {
      ws2D->dataX(0)[i] = i + 1;
      ws2D->dataY(0)[i] = (i + 1) * (i + 1);
      ws2D->dataE(0)[i] = 1.0;
    }

    Fit fitalg;
    TS_ASSERT_THROWS_NOTHING(fitalg.initialize());
    TS_ASSERT(fitalg.isInitialized());

    // set up fitting function
    const std::string funcStr = "name=Quadratic, A0=1.0";
    fitalg.setPropertyValue("Function", funcStr);

    // Set which spectrum to fit against and initial starting values
    fitalg.setProperty("InputWorkspace", ws2D);
    fitalg.setPropertyValue("WorkspaceIndex", "0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(fitalg.execute()))

    TS_ASSERT(fitalg.isExecuted());

    // test the output from fit is what you expect
    double dummy = fitalg.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.0, 0.1);

    IFunction_sptr out = fitalg.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A0"), 0.0, 0.01);
    TS_ASSERT_DELTA(out->getParameter("A1"), 0.0, 0.01);
    TS_ASSERT_DELTA(out->getParameter("A2"), 1.0, 0.0001);
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
