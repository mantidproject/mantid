// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CURVEFITTING_FITTEST_H_
#define CURVEFITTING_FITTEST_H_

#include "MantidTestHelpers/FakeObjects.h"
#include <cxxtest/TestSuite.h>

#include "FitTestHelpers.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IPawleyFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/PawleyFunction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

#include "MantidTestHelpers/FunctionCreationHelper.h"
#include "MantidTestHelpers/MultiDomainFunctionHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::DataObjects;

using Mantid::TestHelpers::MultiDomainFunctionTest_Function;

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

  void test_empty_function() {
    boost::shared_ptr<Mantid::API::MultiDomainFunction> multi;

    auto alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    TS_ASSERT_THROWS(
        alg->setProperty("Function",
                         boost::dynamic_pointer_cast<IFunction>(multi)),
        const std::invalid_argument &);
  }

  void test_empty_function_str() {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    TS_ASSERT_THROWS(alg->setPropertyValue("Function", ""),
                     const std::invalid_argument &);
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
    y = {
        1.e-4 * sqrh,  0.00,  -1.2e-2 * sqrh, -5.6e-2, -18.2e-2 * sqrh, 0.0,
        0.8008 * sqrh, 1.144, 1.287 * sqrh,   0.0,     -0.8008 * sqrh,  -0.4368,
        -0.182 * sqrh, 0.0,   1.2e-2 * sqrh,  0.16e-2, 1.e-4 * sqrh,    0.00};

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
    TS_ASSERT_DELTA(func->getParameter("A"), 1.29300, 0.00001);
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
    for (int i = 0; i < ndata; i++) {
      x[i] = 0.146785 * static_cast<double>(i);
    }
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
    const double delta = Mantid::PhysicalConstants::MuonGyromagneticRatio *
                         field * 2.0 * M_PI * 0.2;
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
    data->initialize(1, 100, 100);

    auto &x = data->dataX(0);
    auto &y = data->dataY(0);
    auto &e = data->dataE(0);

    y = {0.00679397551246448, 0.00684266083126313, 0.00698285916556982,
         0.00719965548825388, 0.00747519954546736, 0.00779445649068509,
         0.00814796531751759, 0.0085316132498512,  0.00894499942724,
         0.00938983058044737, 0.0098689280357672,  0.0103857911674609,
         0.0109444805899566,  0.0115496436468315,  0.0122065986210473,
         0.0129214505517302,  0.0137012349575442,  0.0145540939647495,
         0.0154894928726603,  0.0165184880798197,  0.0176540608380039,
         0.01891153608981,    0.0203091122610038,  0.021868537134057,
         0.0236159780401305,  0.0255831534292171,  0.0278088202944704,
         0.0303407524938984,  0.0332384060776671,  0.0365765613911014,
         0.0404503783689891,  0.0449825362752094,  0.0503335145708212,
         0.0567167210280417,  0.0644212970503862,  0.0738474209204705,
         0.085562497139828,   0.10039290319273,    0.119576178650528,
         0.145011665152563,   0.17965292804199,    0.228047317644744,
         0.296874083423821,   0.394987350612542,   0.532006328704948,
         0.714364415633021,   0.938739703160756,   1.18531948194073,
         1.41603503739802,    1.58257225395956,    1.64354644127685,
         1.58257225395956,    1.41603503739802,    1.18531948194073,
         0.938739703160756,   0.714364415633021,   0.532006328704948,
         0.394987350612542,   0.296874083423821,   0.228047317644743,
         0.17965292804199,    0.145011665152563,   0.119576178650528,
         0.10039290319273,    0.085562497139828,   0.0738474209204706,
         0.0644212970503863,  0.0567167210280418,  0.0503335145708214,
         0.0449825362752095,  0.0404503783689893,  0.0365765613911016,
         0.0332384060776675,  0.0303407524938988,  0.0278088202944705,
         0.0255831534292172,  0.0236159780401305,  0.0218685371340571,
         0.0203091122610038,  0.0189115360898101,  0.0176540608380039,
         0.0165184880798196,  0.0154894928726603,  0.0145540939647495,
         0.0137012349575443,  0.0129214505517302,  0.0122065986210471,
         0.0115496436468314,  0.0109444805899566,  0.0103857911674609,
         0.00986892803576708, 0.00938983058044717, 0.00894499942723977,
         0.00853161324985108, 0.0081479653175175,  0.00779445649068496,
         0.00747519954546727, 0.00719965548825406, 0.00698285916556974,
         0.00684266083126313};

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

    // Changed output intensity from barn to mb/sr
    const double c_mbsr = 79.5774715459;

    auto data = TableWorkspace_sptr(new TableWorkspace);
    data->addColumn("double", "Energy");
    data->addColumn("double", "Intensity");

    TableRow row = data->appendRow();
    row << 0.0 << 2.74937 * c_mbsr;
    row = data->appendRow();
    row << 29.3261 << 0.7204 * c_mbsr;
    row = data->appendRow();
    row << 44.3412 << 0.429809 * c_mbsr;

    Fit fit;
    fit.initialize();
    fit.setProperty("Function",
                    "name=CrystalFieldPeaks,Ion=Ce,Symmetry=Ci,Temperature="
                    "44,ToleranceEnergy=1e-10,ToleranceIntensity=0.1,"
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

    TS_ASSERT_DELTA(outF->getParameter("B20"), 0.366336, 0.0001 * c_mbsr);
    TS_ASSERT_DELTA(outF->getParameter("B22"), 3.98132, 0.0001 * c_mbsr);
    TS_ASSERT_DELTA(outF->getParameter("B40"), -0.0304001, 0.0001 * c_mbsr);
    TS_ASSERT_DELTA(outF->getParameter("B42"), -0.119605, 0.0001 * c_mbsr);
    TS_ASSERT_DELTA(outF->getParameter("B44"), -0.130124, 0.0001 * c_mbsr);

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
      TS_ASSERT_DELTA(column->toDouble(0), 2.74937 * c_mbsr, 0.0001 * c_mbsr);
      TS_ASSERT_DELTA(column->toDouble(1), 0.7204 * c_mbsr, 0.0001 * c_mbsr);
      TS_ASSERT_DELTA(column->toDouble(2), 0.429809 * c_mbsr, 0.0001 * c_mbsr);
      column = output->getColumn("Energy_calc");
      TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.0001);
      column = output->getColumn("Intensity_calc");
      TS_ASSERT_DELTA(column->toDouble(0), 2.74937 * c_mbsr, 0.0001 * c_mbsr);
      TS_ASSERT_DELTA(column->toDouble(1), 0.7204 * c_mbsr, 0.0001 * c_mbsr);
      TS_ASSERT_DELTA(column->toDouble(2), 0.429809 * c_mbsr, 0.0001 * c_mbsr);
    }
  }

  void test_function_exp_decay_fit() {
    Algorithms::Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // create mock data to test against
    const std::string wsName = "ExpDecayMockData";
    const int histogramNumber = 1;
    const int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, timechannels, timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    Mantid::MantidVec &x = ws2D->dataX(0); // x-values
    for (int i = 0; i < timechannels; i++)
      x[i] = i;
    Mantid::MantidVec &y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec &e = ws2D->dataE(0); // error values of counts

    y = {5,
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
         0.01239376088333,
         0};

    e.assign(timechannels, 1.0);

    // alg2.setFunction(fn);
    alg2.setPropertyValue("Function", "name=ExpDecay,Height=1,Lifetime=1");

    // Set which spectrum to fit against and initial starting values
    alg2.setProperty("InputWorkspace", ws);
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

  void test_function_Multidomain_resetting_properties() {
    auto multi = Mantid::TestHelpers::makeMultiDomainFunction3();

    auto alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setProperty("Function", boost::dynamic_pointer_cast<IFunction>(multi));
    auto ws1 = Mantid::TestHelpers::makeMultiDomainWorkspace1();
    alg->setProperty("InputWorkspace", ws1);
    alg->setProperty("WorkspaceIndex", 0);
    auto ws2 = Mantid::TestHelpers::makeMultiDomainWorkspace2();
    alg->setProperty("InputWorkspace", ws2);
    alg->setProperty("WorkspaceIndex", 1);
    alg->setProperty("InputWorkspace_1", ws2);
    alg->setProperty("InputWorkspace_1", ws1);
  }

  void test_function_Multidomain_Fit() {
    auto multi = Mantid::TestHelpers::makeMultiDomainFunction3();
    multi->getFunction(0)->setParameter("A", 1);
    multi->getFunction(0)->setParameter("B", 1);
    multi->getFunction(0)->setAttributeValue("Order", 1);
    multi->getFunction(1)->setParameter("A", 1);
    multi->getFunction(1)->setParameter("B", 1);
    multi->getFunction(1)->setAttributeValue("Order", 3);
    multi->getFunction(2)->setParameter("A", 1);
    multi->getFunction(2)->setParameter("B", 1);
    multi->getFunction(2)->setAttributeValue("Order", 5);

    Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", boost::dynamic_pointer_cast<IFunction>(multi));

    auto ws1 = Mantid::TestHelpers::makeMultiDomainWorkspace1();
    fit.setProperty("InputWorkspace", ws1);
    fit.setProperty("WorkspaceIndex", 0);
    auto ws2 = Mantid::TestHelpers::makeMultiDomainWorkspace2();
    fit.setProperty("InputWorkspace_1", ws2);
    fit.setProperty("WorkspaceIndex_1", 0);
    auto ws3 = Mantid::TestHelpers::makeMultiDomainWorkspace3();
    fit.setProperty("InputWorkspace_2", ws3);
    fit.setProperty("WorkspaceIndex_2", 0);

    fit.execute();
    TS_ASSERT(fit.isExecuted());

    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("f0.A"), 0.5, 1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f0.B"), 5, 1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f1.A"), -4, 1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f1.B"), -20, 1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f2.A"), 4, 1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f2.B"), 16, 1e-8);
  }

  void test_function_Multidomain_one_function_to_two_parts_of_workspace() {

    const double A0 = 1, A1 = 100;
    const double B0 = 2;

    // Set up a workspace which is divided into 3 parts: 0 <= x < 10, 10 <= x <
    // 20, 20 <= x < 30
    // The first and last parts have their data on line A0 + B0 * x, and the
    // middle part has
    // constant value A1

    MatrixWorkspace_sptr ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(1, 30, 30);
    {
      const double dx = 1.0;
      Mantid::MantidVec &x = ws->dataX(0);
      Mantid::MantidVec &y = ws->dataY(0);
      for (size_t i = 0; i < 10; ++i) {
        x[i] = dx * double(i);
        y[i] = A0 + B0 * x[i];
      }
      for (size_t i = 10; i < 20; ++i) {
        x[i] = dx * double(i);
        y[i] = A1;
      }
      for (size_t i = 20; i < 30; ++i) {
        x[i] = dx * double(i);
        y[i] = A0 + B0 * x[i];
      }
    }

    // Set up a multi-domain function and Fit algorithm to fit a single function
    // to two
    // parts of the workspace

    boost::shared_ptr<MultiDomainFunction> mf =
        boost::make_shared<MultiDomainFunction>();
    mf->addFunction(boost::make_shared<MultiDomainFunctionTest_Function>());
    mf->setParameter(0, 0.0);
    mf->setParameter(1, 0.0);
    std::vector<size_t> ind(2);
    ind[0] = 0;
    ind[1] = 1;
    mf->setDomainIndices(0, ind);
    TS_ASSERT_EQUALS(mf->getMaxIndex(), 1);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("Fit");
    Mantid::API::IAlgorithm &fit = *alg;
    fit.initialize();
    fit.setProperty("Function", boost::dynamic_pointer_cast<IFunction>(mf));
    // at this point Fit knows the number of domains and creates additional
    // properties InputWorkspace_#
    TS_ASSERT(fit.existsProperty("InputWorkspace_1"));

    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("WorkspaceIndex", 0);
    fit.setProperty("StartX", 0.0);
    fit.setProperty("EndX", 9.9999);
    fit.setProperty("InputWorkspace_1", ws);

    // at this point Fit knows the type of InputWorkspace_1 (MatrixWorkspace)
    // and creates additional
    // properties for it
    TS_ASSERT(fit.existsProperty("WorkspaceIndex_1"));
    TS_ASSERT(fit.existsProperty("StartX_1"));
    TS_ASSERT(fit.existsProperty("EndX_1"));

    fit.setProperty("WorkspaceIndex_1", 0);
    fit.setProperty("StartX_1", 20.0);
    fit.setProperty("EndX_1", 30.0);

    fit.execute();
    TS_ASSERT(fit.isExecuted());

    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter(0), 1.0, 1e-15); // == A0
    TS_ASSERT_DELTA(fun->getParameter(1), 2.0, 1e-15); // == B0
  }

  void test_function_Pawley_FitSi() {
    /* This example generates a spectrum with the first two reflections
     * of Silicon with lattice parameter a = 5.4311946 Angstr.
     *    hkl      d       height      fwhm
     *   1 1 1  3.13570    40.0       0.006
     *   2 2 0  1.92022    110.0      0.004
     */
    auto ws = getWorkspacePawley(
        "name=Gaussian,PeakCentre=3.13570166,Height=40.0,Sigma=0.003;name="
        "Gaussian,PeakCentre=1.92021727,Height=110.0,Sigma=0.002",
        1.85, 3.2, 400);

    // needs to be a PawleyFunction_sptr to have getPawleyParameterFunction()
    Mantid::CurveFitting::Functions::PawleyFunction_sptr pawleyFn =
        boost::make_shared<Mantid::CurveFitting::Functions::PawleyFunction>();
    pawleyFn->initialize();
    pawleyFn->setLatticeSystem("Cubic");
    pawleyFn->addPeak(V3D(1, 1, 1), 0.0065, 35.0);
    pawleyFn->addPeak(V3D(2, 2, 0), 0.0045, 110.0);
    pawleyFn->setUnitCell("5.4295 5.4295 5.4295");

    // fix ZeroShift
    pawleyFn->fix(pawleyFn->parameterIndex("f0.ZeroShift"));

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",
                     boost::dynamic_pointer_cast<IFunction>(pawleyFn));
    fit->setProperty("InputWorkspace", ws);
    fit->execute();

    IFunction_sptr parameters = pawleyFn->getPawleyParameterFunction();

    TS_ASSERT_DELTA(parameters->getParameter("a"), 5.4311946, 1e-6);
  }

  void test_function_Pawley_FitSiZeroShift() {
    /* This example generates a spectrum with the first three reflections
     * of Silicon with lattice parameter a = 5.4311946 Angstr.
     *    hkl      d       height     ca. fwhm
     *   1 1 1  3.13570    40.0       0.006
     *   2 2 0  1.92022    110.0      0.004
     *   3 1 1  1.63757    101.0      0.003
     */
    auto ws = getWorkspacePawley(
        "name=Gaussian,PeakCentre=3.13870166,Height=40.0,Sigma=0.003;name="
        "Gaussian,PeakCentre=1.92321727,Height=110.0,Sigma=0.002;name=Gaussian,"
        "PeakCentre=1.6405667,Height=105.0,Sigma=0.0016",
        1.6, 3.2, 800);

    Mantid::CurveFitting::Functions::PawleyFunction_sptr pawleyFn =
        boost::make_shared<Mantid::CurveFitting::Functions::PawleyFunction>();
    pawleyFn->initialize();
    pawleyFn->setLatticeSystem("Cubic");
    pawleyFn->addPeak(V3D(1, 1, 1), 0.0065, 35.0);
    pawleyFn->addPeak(V3D(2, 2, 0), 0.0045, 115.0);
    pawleyFn->addPeak(V3D(3, 1, 1), 0.0035, 115.0);
    pawleyFn->setUnitCell("5.433 5.433 5.433");
    pawleyFn->setParameter("f0.ZeroShift", 0.001);

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",
                     boost::dynamic_pointer_cast<IFunction>(pawleyFn));
    fit->setProperty("InputWorkspace", ws);
    fit->execute();

    Mantid::CurveFitting::Functions::PawleyParameterFunction_sptr parameters =
        pawleyFn->getPawleyParameterFunction();

    TS_ASSERT_DELTA(parameters->getParameter("a"), 5.4311946, 1e-5);
    TS_ASSERT_DELTA(parameters->getParameter("ZeroShift"), 0.003, 1e-4);
  }

  // Peak functions:
  // ---------------------

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
        "name=IkedaCarpenterPV, I=3000, SigmaSquared=25.0, Gamma=0.1, X0=50.0");
    fit.setProperty("InputWorkspace", ws);
    fit.setPropertyValue("StartX", "30");
    fit.setPropertyValue("EndX", "100");
    // from the fitfunctions documentation:
    // In general when fitting a single peak it is not recommended to refine
    // both Alpha0 and Alpha1 at the same time since these two parameters will
    // effectively be 100% correlated because the wavelength over a single peak
    // is likely effectively constant. All parameters are constrained to be
    // non-negative.
    fit.setProperty("Ties", "Alpha0=1.6666");
    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());

    // test the output from fit is what you expect
    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 0.01);

    IFunction_sptr out = fit.getProperty("Function");
    // test that all parameters are non-negative
    TS_ASSERT_DELTA(out->getParameter("I"), 3140.1444, 40.0);
    TS_ASSERT_DELTA(out->getParameter("Alpha1"), 1.4276, 0.005);
    TS_ASSERT_DELTA(out->getParameter("Beta0"), 40.0000, 10.0);
    TS_ASSERT_DELTA(out->getParameter("Kappa"), 45.8482, 0.2);
    TS_ASSERT_DELTA(out->getParameter("SigmaSquared"), 100.921, 1.0);
    TS_ASSERT_DELTA(out->getParameter("Gamma"), 0.05, 0.05);
    TS_ASSERT_DELTA(out->getParameter("X0"), 50.0, 0.2);
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

  // Get data out for the singled out test!
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

      std::cout << "[D] data_set.append([" << x[i] << ", " << y[i] << ", "
                << e[i] << "])\n";
    }

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=PseudoVoigt, PeakCentre=0.0, FWHM=0.15, "
                                "Intensity=112.78, Mixing=0.7");
    fit.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(fit.execute()))
    TS_ASSERT(fit.isExecuted());

    IFunction_sptr fitted = fit.getProperty("Function");
    TS_ASSERT_DELTA(fitted->getError(0), 0.0, 1e-6);
    TS_ASSERT_DELTA(fitted->getError(2), 0.0, 1e-6);
    TS_ASSERT_DELTA(fitted->getError(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(fitted->getError(3), 0.0, 1e-6);

    std::cout << "Mixing = " << fitted->getParameter("Mixing") << "\n";
    std::cout << "PeakCentre = " << fitted->getParameter("PeakCentre") << "\n";
    std::cout << "Intensity = " << fitted->getParameter("Intensity") << "\n";
    std::cout << "FWHM = " << fitted->getParameter("FWHM") << "\n";

    TS_ASSERT_DELTA(fitted->getParameter("Mixing"), 0.62, 1e-2);
    TS_ASSERT_DELTA(fitted->getParameter("PeakCentre"), 0.0, 1e-4);
    TS_ASSERT_DELTA(fitted->getParameter("Intensity"), 20.51, 0.5);
    TS_ASSERT_DELTA(fitted->getParameter("FWHM"), 0.15, 1e-2);
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

  void test_PeakRadius() {
    size_t nbins = 100;
    auto ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, nbins, nbins);
    FunctionDomain1DVector x(-10, 10, nbins);
    ws->mutableX(0) = x.toVector();
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=Lorentzian,Amplitude=5,FWHM=1");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("MaxIterations", 0);
      fit.setProperty("Output", "out");
      fit.execute();
      auto res = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          "out_Workspace");
      const auto &y = res->y(1);
      TS_ASSERT_DIFFERS(y.front(), 0.0);
      TS_ASSERT_DIFFERS(y.back(), 0.0);
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=Lorentzian,Amplitude=5,FWHM=1");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("PeakRadius", 5);
      fit.setProperty("MaxIterations", 0);
      fit.setProperty("Output", "out");
      fit.execute();
      auto res = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          "out_Workspace");
      const auto &y = res->y(1);
      for (size_t i = 0; i < 25; ++i) {
        TS_ASSERT_EQUALS(y[i], 0.0);
        TS_ASSERT_EQUALS(y[nbins - i - 1], 0.0);
      }
      TS_ASSERT_DIFFERS(y[26], 0.0);
      TS_ASSERT_DIFFERS(y[26], 0.0);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_fit_size_change() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2 * exp(-(5 * x + x * x - 3 * x * x * x)); },
        1, 0, 1, 0.1);
    {
      API::IFunction_sptr fun =
          boost::make_shared<TestHelpers::FunctionChangesNParams>();
      TS_ASSERT_EQUALS(fun->nParams(), 1);

      Fit fit;
      fit.initialize();
      fit.setRethrows(true);
      fit.setProperty("Function", fun);
      fit.setProperty("InputWorkspace", ws);
      TS_ASSERT_THROWS_NOTHING(fit.execute());
      TS_ASSERT_EQUALS(fun->nParams(), 5);
      TS_ASSERT_DELTA(fun->getParameter(0), 1.9936, 0.1);
      TS_ASSERT_DELTA(fun->getParameter(1), -9.4991, 0.1);
      TS_ASSERT_DELTA(fun->getParameter(2), 19.1074, 0.1);
      TS_ASSERT_DELTA(fun->getParameter(3), -17.8434, 0.1);
      TS_ASSERT_DELTA(fun->getParameter(4), 6.3465, 0.1);
    }
    {
      API::IFunction_sptr fun =
          boost::make_shared<TestHelpers::FunctionChangesNParams>();
      TS_ASSERT_EQUALS(fun->nParams(), 1);

      Fit fit;
      fit.initialize();
      fit.setRethrows(true);
      fit.setProperty("Function", fun);
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
      TS_ASSERT_THROWS_NOTHING(fit.execute());
      TS_ASSERT_EQUALS(fun->nParams(), 5);
      TS_ASSERT_DELTA(fun->getParameter(0), 1.9936, 0.1);
      TS_ASSERT_DELTA(fun->getParameter(1), -9.4991, 0.1);
      TS_ASSERT_DELTA(fun->getParameter(2), 19.1074, 0.1);
      TS_ASSERT_DELTA(fun->getParameter(3), -17.8434, 0.1);
      TS_ASSERT_DELTA(fun->getParameter(4), 6.3465, 0.1);
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "success");
    }

    AnalysisDataService::Instance().clear();
  }

  void test_fit_size_change_1() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2 + x - 0.1 * x * x; }, 1, 0, 1, 0.1);
    {
      API::IFunction_sptr fun =
          boost::make_shared<TestHelpers::FunctionChangesNParams>();
      TS_ASSERT_EQUALS(fun->nParams(), 1);

      Fit fit;
      fit.initialize();
      fit.setRethrows(true);
      fit.setProperty("Function", fun);
      fit.setProperty("InputWorkspace", ws);
      TS_ASSERT_THROWS_NOTHING(fit.execute());
      TS_ASSERT_EQUALS(fun->nParams(), 5);
      TS_ASSERT_DELTA(fun->getParameter(0), 2.0, 0.0001);
      TS_ASSERT_DELTA(fun->getParameter(1), 1.0, 0.0001);
      TS_ASSERT_DELTA(fun->getParameter(2), -0.1, 0.0001);
      TS_ASSERT_DELTA(fun->getParameter(3), 0.0, 0.0001);
      TS_ASSERT_DELTA(fun->getParameter(4), 0.0, 0.0001);
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "success");
    }
    {
      API::IFunction_sptr fun =
          boost::make_shared<TestHelpers::FunctionChangesNParams>();
      TS_ASSERT_EQUALS(fun->nParams(), 1);

      Fit fit;
      fit.initialize();
      fit.setRethrows(true);
      fit.setProperty("Function", fun);
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
      TS_ASSERT_THROWS_NOTHING(fit.execute());
      TS_ASSERT_EQUALS(fun->nParams(), 5);
      TS_ASSERT_DELTA(fun->getParameter(0), 2.0, 0.0001);
      TS_ASSERT_DELTA(fun->getParameter(1), 1.0, 0.0001);
      TS_ASSERT_DELTA(fun->getParameter(2), -0.1, 0.0001);
      TS_ASSERT_DELTA(fun->getParameter(3), 0.0, 0.0001);
      TS_ASSERT_DELTA(fun->getParameter(4), 0.0, 0.0001);
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "success");
    }

    AnalysisDataService::Instance().clear();
  }

  void test_exclude_ranges_with_unweighted_least_squares() {
    HistogramData::Points points{-2, -1, 0, 1, 2};
    HistogramData::Counts counts(points.size(), 0.0);
    // This value should be excluded.
    counts.mutableData()[2] = 10.0;
    MatrixWorkspace_sptr ws(DataObjects::create<Workspace2D>(
                                1, HistogramData::Histogram(points, counts))
                                .release());
    Fit fit;
    fit.initialize();
    fit.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(
        fit.setProperty("Function", "name=FlatBackground,A0=0.1"))
    TS_ASSERT_THROWS_NOTHING(fit.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(fit.setPropertyValue("Exclude", "-0.5, 0.5"))
    TS_ASSERT_THROWS_NOTHING(
        fit.setProperty("Minimizer", "Levenberg-MarquardtMD"))
    TS_ASSERT_THROWS_NOTHING(
        fit.setProperty("CostFunction", "Unweighted least squares"))
    TS_ASSERT_THROWS_NOTHING(fit.setProperty("Output", "fit_test_output"))
    TS_ASSERT_THROWS_NOTHING(fit.execute())
    const std::string status = fit.getProperty("OutputStatus");
    TS_ASSERT_EQUALS(status, "success")
    API::IFunction_sptr function = fit.getProperty("Function");
    TS_ASSERT_DELTA(function->getParameter(0), 0.0, 1e-12)
    AnalysisDataService::Instance().clear();
  }

private:
  /// build test input workspaces for the Pawley function Fit tests
  MatrixWorkspace_sptr getWorkspacePawley(const std::string &functionString,
                                          double xMin, double xMax, size_t n) {
    IFunction_sptr siFn =
        FunctionFactory::Instance().createInitialized(functionString);

    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, n, n);

    FunctionDomain1DVector xValues(xMin, xMax, n);
    FunctionValues yValues(xValues);
    std::vector<double> eValues(n, 1.0);

    siFn->function(xValues, yValues);

    std::vector<double> &xData = ws->dataX(0);
    std::vector<double> &yData = ws->dataY(0);
    std::vector<double> &eData = ws->dataE(0);

    for (size_t i = 0; i < n; ++i) {
      xData[i] = xValues[i];
      yData[i] = yValues[i];
      eData[i] = eValues[i];
    }

    WorkspaceCreationHelper::addNoise(ws, 0, -0.1, 0.1);

    return ws;
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
    runFitAlgorithm(m_onePeakWS, FitTestHelpers::SingleB2BPeak,
                    "Damped GaussNewton");
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
    runFitAlgorithm(m_smoothWS, FitTestHelpers::SmoothishGaussians,
                    "Damped GaussNewton");
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
