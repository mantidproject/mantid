#ifndef CURVEFITTING_FITTEST_H_
#define CURVEFITTING_FITTEST_H_

#include "MantidTestHelpers/FakeObjects.h"
#include <cxxtest/TestSuite.h>

#include "FitTestHelpers.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::DataObjects;

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

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A"), 1000, 30.0);
    TS_ASSERT_DELTA(out->getParameter("B"), 26, 0.1);
    TS_ASSERT_DELTA(out->getParameter("C"), 7.7, 0.1);
    TS_ASSERT_DELTA(out->getParameter("D"), 0, 0.1);
  }

  void test_function_ProductFunction() {

    // Mock data
    int ndata = 30;
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, ndata, ndata);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    x = {0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
         1, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9,
         2, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9};
    y = {0.001362, 0.00434468, 0.0127937, 0.0347769, 0.0872653, 0.202138,
         0.432228, 0.853165,   1.55457,   2.61483,   4.06006,   5.8194,
         7.69982,  9.40459,    10.6036,   11.0364,   10.6036,   9.40459,
         7.69982,  5.8194,     4.06006,   2.61483,   1.55457,   0.853165,
         0.432228, 0.202138,   0.0872653, 0.0347769, 0.0127937, 0.00434468};
    e.assign(ndata, 0.1);

    Mantid::CurveFitting::Algorithms::Fit fit;
    fit.initialize();

    fit.setPropertyValue(
        "Function", "composite=ProductFunction,NumDeriv=false;name="
                    "Gaussian,Height=3,PeakCentre=1,Sigma=0.5,ties=(Height="
                    "3.0,PeakCentre=1.0,Sigma=0.5);name=Gaussian,Height=15,"
                    "PeakCentre=2.5,Sigma=0.5,ties=(Sigma=0.5)");
    fit.setProperty("InputWorkspace", ws);
    fit.setPropertyValue("WorkspaceIndex", "0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(fit.execute()))
    TS_ASSERT(fit.isExecuted());

    // test the output from fit is what you expect

    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.0, 0.01);

    Mantid::API::IFunction_sptr outF = fit.getProperty("Function");

    TS_ASSERT_DELTA(outF->getParameter("f0.PeakCentre"), 1.0, 0.001);
    TS_ASSERT_DELTA(outF->getParameter("f0.Height"), 3.0, 0.001);
    TS_ASSERT_DELTA(outF->getParameter("f0.Sigma"), 0.5, 0.001);
    TS_ASSERT_DELTA(outF->getParameter("f1.PeakCentre"), 2.0, 0.001);
    TS_ASSERT_DELTA(outF->getParameter("f1.Height"), 10.0, 0.01);
    TS_ASSERT_DELTA(outF->getParameter("f1.Sigma"), 0.5, 0.001);
  }

  void setUp() override {
    std::string resFileName = "ResolutionTestResolution.res";
    std::ofstream fil(resFileName.c_str());

    double N = 117;
    double DX = 10;
    double X0 = -DX / 2;
    double dX = DX / (N - 1);
    double resS = acos(0.);
    double resH = 3;
    double yErr = 0;
    double y0 = 0;

    for (int i = 0; i < N; i++) {
      double x = X0 + i * dX;
      double y = resH * exp(-x * x * resS);
      double err = fabs(y - y0) / 10;
      yErr = std::max(err, yErr);
      fil << x << ' ' << y << " 0\n";
      y0 = y;
    }
  }

  void tearDown() override {
    std::string resFileName = "ResolutionTestResolution.res";
    Poco::File phandle(resFileName);
    if (phandle.exists()) {
      phandle.remove();
    }
  }

  void test_resolution_fit() {

    const int nX = 100;
    const int nY = nX - 1;

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, nX, nY));

    const double dx = 10 / 99;

    Mantid::MantidVec &X = ws->dataX(0);
    Mantid::MantidVec &Y = ws->dataY(0);
    Mantid::MantidVec &E = ws->dataE(0);

    X = {0.000000, 0.101010, 0.202020, 0.303030, 0.404040, 0.505051, 0.606061,
         0.707071, 0.808081, 0.909091, 1.010101, 1.111111, 1.212121, 1.313131,
         1.414141, 1.515152, 1.616162, 1.717172, 1.818182, 1.919192, 2.020202,
         2.121212, 2.222222, 2.323232, 2.424242, 2.525253, 2.626263, 2.727273,
         2.828283, 2.929293, 3.030303, 3.131313, 3.232323, 3.333333, 3.434343,
         3.535354, 3.636364, 3.737374, 3.838384, 3.939394, 4.040404, 4.141414,
         4.242424, 4.343434, 4.444444, 4.545455, 4.646465, 4.747475, 4.848485,
         4.949495, 5.050505, 5.151515, 5.252525, 5.353535, 5.454545, 5.555556,
         5.656566, 5.757576, 5.858586, 5.959596, 6.060606, 6.161616, 6.262626,
         6.363636, 6.464646, 6.565657, 6.666667, 6.767677, 6.868687, 6.969697,
         7.070707, 7.171717, 7.272727, 7.373737, 7.474747, 7.575758, 7.676768,
         7.777778, 7.878788, 7.979798, 8.080808, 8.181818, 8.282828, 8.383838,
         8.484848, 8.585859, 8.686869, 8.787879, 8.888889, 8.989899, 9.090909,
         9.191919, 9.292929, 9.393939, 9.494949, 9.595960, 9.696970, 9.797980,
         9.898990};
    Y = {0.000000, 0.000000, 0.000000, 0.000000, 0.000001, 0.000001, 0.000002,
         0.000004, 0.000006, 0.000012, 0.000021, 0.000036, 0.000063, 0.000108,
         0.000183, 0.000305, 0.000503, 0.000818, 0.001314, 0.002084, 0.003262,
         0.005041, 0.007692, 0.011586, 0.017229, 0.025295, 0.036664, 0.052465,
         0.074121, 0.103380, 0.142353, 0.193520, 0.259728, 0.344147, 0.450195,
         0.581418, 0.741323, 0.933166, 1.159690, 1.422842, 1.723466, 2.061013,
         2.433271, 2.836167, 3.263660, 3.707743, 4.158590, 4.604836, 5.034009,
         5.433072, 5.789067, 6.089806, 6.324555, 6.484675, 6.564144, 6.559937,
         6.472215, 6.304315, 6.062539, 5.755762, 5.394893, 4.992230, 4.560768,
         4.113514, 3.662855, 3.220017, 2.794656, 2.394584, 2.025646, 1.691721,
         1.394844, 1.135414, 0.912461, 0.723946, 0.567061, 0.438516, 0.334790,
         0.252343, 0.187776, 0.137950, 0.100055, 0.071644, 0.050648, 0.035348,
         0.024356, 0.016568, 0.011127, 0.007378, 0.004829, 0.003121, 0.001991,
         0.001254, 0.000780, 0.000479, 0.000290, 0.000174, 0.000103, 0.000060,
         0.000034};
    E.assign(nY, 1.0);

    X.back() = X[98] + dx;
    AnalysisDataService::Instance().add("ResolutionTest_WS", ws);

    Algorithms::Fit fit;
    fit.initialize();

    fit.setPropertyValue(
        "Function", "composite=Convolution,"
                    "FixResolution=true,NumDeriv=true;name=Resolution,FileName="
                    "\"ResolutionTestResolution.res\","
                    "WorkspaceIndex=0;name=ResolutionTest_Gauss,c=5,h=2,s=1");
    fit.setPropertyValue("InputWorkspace", "ResolutionTest_WS");
    fit.setPropertyValue("WorkspaceIndex", "0");
    fit.execute();
  }

  void getStretchExpMockData(Mantid::MantidVec &y, Mantid::MantidVec &e) {
    // values extracted from y(x)=2*exp(-(x/4)^0.5)
    y = {2,          1.2130613,  0.98613738, 0.84124005, 0.73575888,
         0.65384379, 0.58766531, 0.53273643, 0.48623347, 0.44626032,
         0.41148132, 0.38092026, 0.35384241, 0.32968143, 0.30799199,
         0.28841799, 0.27067057, 0.25451242, 0.2397465,  0.22620756};

    std::transform(
        y.begin(), y.end(), e.begin(),
        std::bind(std::multiplies<double>(), std::placeholders::_1, 0.1));
  }

  void test_function_StretchExp_Against_MockData() {
    Algorithms::Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // create mock data to test against
    std::string wsName = "StretchExpMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, timechannels, timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    // in this case, x-values are just the running index
    auto &x = ws2D->dataX(0);
    for (int i = 0; i < timechannels; i++)
      x[i] = 1.0 * i + 0.00001;

    Mantid::MantidVec &y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec &e = ws2D->dataE(0); // error values of counts
    getStretchExpMockData(y, e);

    // put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    alg2.setPropertyValue(
        "Function",
        "name=StretchExp, Height=1.5, Lifetime=5.0, Stretching=0.4");

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex", "0");
    alg2.setPropertyValue("StartX", "0");
    alg2.setPropertyValue("EndX", "19");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(alg2.execute()))

    TS_ASSERT(alg2.isExecuted());

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.001, 0.001);

    IFunction_sptr out = alg2.getProperty("Function");
    // golden standard y(x)=2*exp(-(x/4)^0.5)
    // allow for a 1% error in Height and Lifetime, and 10% error in the
    // Stretching exponent
    TS_ASSERT_DELTA(out->getParameter("Height"), 2.0, 0.02);
    TS_ASSERT_DELTA(out->getParameter("Lifetime"), 4.0, 0.04);
    TS_ASSERT_DELTA(out->getParameter("Stretching"), 0.5, 0.05);

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "General");

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_function_convolution_fit_resolution() {

    boost::shared_ptr<WorkspaceTester> data =
        boost::make_shared<WorkspaceTester>();
    data->init(1, 100, 100);

    auto &x = data->dataX(0);
    auto &y = data->dataY(0);
    auto &e = data->dataE(0);

    y = {0, -1.77636e-16, -1.77636e-16, 0, -1.77636e-16, -8.88178e-17,
         -1.33227e-16, 0, 0, 8.88178e-17, 3.33067e-17, 1.11022e-17, 1.27676e-16,
         6.66134e-17, 8.32667e-17, 3.88578e-17, 9.4369e-17, 1.44329e-16,
         2.66454e-16, 5.10703e-15, 9.80105e-14, 1.63027e-12, 2.31485e-11,
         2.80779e-10, 2.91067e-09, 2.58027e-08, 1.9575e-07, 1.27204e-06,
         7.08849e-06, 3.39231e-05, 0.000139678, 0.000496012, 0.00152387,
         0.0040672, 0.00948273, 0.0194574, 0.0354878, 0.0583005, 0.0877657,
         0.123662, 0.167048, 0.221547, 0.293962, 0.393859, 0.531629, 0.714256,
         0.938713, 1.18531, 1.41603, 1.58257, 1.64355, 1.58257, 1.41603,
         1.18531, 0.938713, 0.714256, 0.531629, 0.393859, 0.293962, 0.221547,
         0.167048, 0.123662, 0.0877657, 0.0583005, 0.0354878, 0.0194574,
         0.00948273, 0.0040672, 0.00152387, 0.000496012, 0.000139678,
         3.39231e-05, 7.08849e-06, 1.27204e-06, 1.9575e-07, 2.58027e-08,
         2.91067e-09, 2.80779e-10, 2.31486e-11, 1.63033e-12, 9.80771e-14,
         5.09592e-15, 2.77556e-16, 3.88578e-17, 2.22045e-17, -1.66533e-17,
         -1.11022e-17, 0, -7.21645e-17, -8.88178e-17, -1.11022e-16,
         -1.33227e-16, -4.44089e-17, -1.77636e-16, -1.33227e-16, -8.88178e-17,
         -3.55271e-16, -8.88178e-17, -1.77636e-16, -1.77636e-16};

    x = {-10,  -9.8, -9.6, -9.4, -9.2, -9,   -8.8, -8.6, -8.4, -8.2, -8,   -7.8,
         -7.6, -7.4, -7.2, -7,   -6.8, -6.6, -6.4, -6.2, -6,   -5.8, -5.6, -5.4,
         -5.2, -5,   -4.8, -4.6, -4.4, -4.2, -4,   -3.8, -3.6, -3.4, -3.2, -3,
         -2.8, -2.6, -2.4, -2.2, -2,   -1.8, -1.6, -1.4, -1.2, -1,   -0.8, -0.6,
         -0.4, -0.2, 0,    0.2,  0.4,  0.6,  0.8,  1,    1.2,  1.4,  1.6,  1.8,
         2,    2.2,  2.4,  2.6,  2.8,  3,    3.2,  3.4,  3.6,  3.8,  4,    4.2,
         4.4,  4.6,  4.8,  5,    5.2,  5.4,  5.6,  5.8,  6,    6.2,  6.4,  6.6,
         6.8,  7,    7.2,  7.4,  7.6,  7.8,  8,    8.2,  8.4,  8.6,  8.8,  9,
         9.2,  9.4,  9.6,  9.8};

    e.assign(y.size(), 1);

    Algorithms::Fit fit;
    fit.initialize();
    // fit.setPropertyValue("Function", conv->asString());

    fit.setPropertyValue("Function",
                         "composite=Convolution,FixResolution=true,NumDeriv="
                         "true;name=ConvolutionTest_Gauss,c=0,h=0.5,s=0.5;"
                         "name=ConvolutionTest_Lorentz,c=0,h=1,w=1");
    fit.setProperty("InputWorkspace", data);
    fit.setProperty("WorkspaceIndex", 0);
    fit.execute();

    IFunction_sptr out = fit.getProperty("Function");
    // by default convolution keeps parameters of the resolution (function
    // #0)
    // fixed
    TS_ASSERT_EQUALS(out->getParameter("f0.h"), 0.5);
    TS_ASSERT_EQUALS(out->getParameter("f0.s"), 0.5);
    // fit is not very good
    TS_ASSERT_LESS_THAN(0.1, fabs(out->getParameter("f1.w") - 1));

    Algorithms::Fit fit1;
    fit1.initialize();
    fit1.setProperty("Function",
                     "composite=Convolution,FixResolution=false,NumDeriv="
                     "true;name=ConvolutionTest_Gauss,c=0,h=0.5,s=0.5;"
                     "name=ConvolutionTest_Lorentz,c=0,h=1,w=1");
    fit1.setProperty("InputWorkspace", data);
    fit1.setProperty("WorkspaceIndex", 0);
    fit1.execute();

    out = fit1.getProperty("Function");
    // resolution parameters change and close to the initial values

    TS_ASSERT_DELTA(out->getParameter("f0.s"), 2.0, 0.0001);
    TS_ASSERT_DELTA(out->getParameter("f1.w"), 0.5, 0.0001);
  }

  void test_function_crystal_field_peaks_fit() {

    auto data = TableWorkspace_sptr(new TableWorkspace);
    data->addColumn("double", "Energy");
    data->addColumn("double", "Intensity");

    TableRow row = data->appendRow();
    row << 0.0 << 2.74937;
    row = data->appendRow();
    row << 29.3261 << 0.7204;
    row = data->appendRow();
    row << 44.3412 << 0.429809;

    Fit fit;
    fit.initialize();
    fit.setProperty("Function",
                    "name=CrystalFieldPeaks,Ion=Ce,Symmetry=Ci,Temperature="
                    "44,ToleranceEnergy=1e-10,ToleranceIntensity=0.001,"
                    "BmolX=0,BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=0,B20=0."
                    "37,B21=0,B22=3.9,B40=-0.03,B41=0,B42=-0.11,B43=0,B44=-"
                    "0.12,B60=0,B61=0,B62=0,B63=0,B64=0,B65=0,B66=0,IB21=0,"
                    "IB22=0,IB41=0,IB42=0,IB43=0,IB44=0,IB61=0,IB62=0,IB63="
                    "0,IB64=0,IB65=0,IB66=0,IntensityScaling=1");

    fit.setProperty("Ties", "BmolX=0,BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ="
                            "0,B21=0,B41=0,B43=0,B60=0,B61=0,B62=0,B63=0,"
                            "B64=0,B65=0,B66=0,IB21=0,IB22=0,IB41=0,IB42=0,"
                            "IB43=0,IB44=0,IB61=0,IB62=0,IB63=0,IB64=0,"
                            "IB65=0,IB66=0,IntensityScaling=1");

    fit.setProperty("InputWorkspace", data);
    fit.setProperty("DataColumn", "Energy");
    fit.setProperty("DataColumn_1", "Intensity");
    fit.setProperty("Output", "out");
    fit.execute();

    Mantid::API::IFunction_sptr outF = fit.getProperty("Function");

    TS_ASSERT_DELTA(outF->getParameter("B20"), 0.366336, 0.0001);
    TS_ASSERT_DELTA(outF->getParameter("B22"), 3.98132, 0.0001);
    TS_ASSERT_DELTA(outF->getParameter("B40"), -0.0304001, 0.0001);
    TS_ASSERT_DELTA(outF->getParameter("B42"), -0.119605, 0.0001);
    TS_ASSERT_DELTA(outF->getParameter("B44"), -0.130124, 0.0001);

    ITableWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "out_Workspace");
    TS_ASSERT(output);
    if (output) {
      TS_ASSERT_EQUALS(output->rowCount(), 3);
      TS_ASSERT_EQUALS(output->columnCount(), 4);
      auto column = output->getColumn("Energy");
      TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.0001);
      column = output->getColumn("Intensity");
      TS_ASSERT_DELTA(column->toDouble(0), 2.74937, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 0.7204, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 0.429809, 0.0001);
      column = output->getColumn("Energy_calc");
      TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.0001);
      column = output->getColumn("Intensity_calc");
      TS_ASSERT_DELTA(column->toDouble(0), 2.74937, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 0.7204, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 0.429809, 0.0001);
    }
  }

  void test_function_exp_decay_fit() {
    Algorithms::Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // create mock data to test against
    std::string wsName = "ExpDecayMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, timechannels, timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    Mantid::MantidVec &x = ws2D->dataX(0); // x-values
    for (int i = 0; i < timechannels; i++)
      x[i] = i;
    Mantid::MantidVec &y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec &e = ws2D->dataE(0); // error values of counts

    y = {5, 3.582656552869, 2.567085595163, 1.839397205857, 1.317985690579,
         0.9443780141878, 0.6766764161831, 0.484859839322, 0.347417256114,
         0.2489353418393, 0.1783699667363, 0.1278076660325, 0.09157819444367,
         0.0656186436847, 0.04701781275748, 0.03368973499543, 0.02413974996916,
         0.01729688668232, 0.01239376088333, 0};

    e.assign(19, 1.0);

    // put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // alg2.setFunction(fn);
    alg2.setPropertyValue("Function", "name=ExpDecay,Height=1,Lifetime=1");

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex", "0");
    alg2.setPropertyValue("StartX", "0");
    alg2.setPropertyValue("EndX", "20");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(alg2.execute()))

    TS_ASSERT(alg2.isExecuted());

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.0001, 0.0001);

    IFunction_sptr out = alg2.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("Height"), 5, 0.0001);
    TS_ASSERT_DELTA(out->getParameter("Lifetime"), 3, 0.001);
  }

  void test_function_lattice_fit() {
    // Fit Silicon lattice with three peaks.
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    table->addColumn("V3D", "HKL");
    table->addColumn("double", "d");

    TableRow newRow = table->appendRow();
    newRow << V3D(1, 1, 1) << 3.135702;
    newRow = table->appendRow();
    newRow << V3D(2, 2, 0) << 1.920217;
    newRow = table->appendRow();
    newRow << V3D(3, 1, 1) << 1.637567;

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=LatticeFunction,LatticeSystem=Cubic,"
                                "ProfileFunction=Gaussian,a=5,ZeroShift=0");
    fit.setProperty("Ties", "ZeroShift=0.0");
    fit.setProperty("InputWorkspace", table);
    fit.setProperty("CostFunction", "Unweighted least squares");
    fit.setProperty("CreateOutput", true);
    fit.execute();

    TS_ASSERT(fit.isExecuted());

    // test the output from fit is what you expect
    IFunction_sptr out = fit.getProperty("Function");

    TS_ASSERT_DELTA(out->getParameter("a"), 5.4311946, 1e-6);
    TS_ASSERT_LESS_THAN(out->getError(0), 1e-6);
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
