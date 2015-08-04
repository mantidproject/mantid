#ifndef CURVEFITTING_FITTEST_H_
#define CURVEFITTING_FITTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/FakeObjects.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidCurveFitting/Fit.h"

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
    m_smoothWS = generateSmoothCurveWorkspace();
    m_onePeakWS = generatePeaksCurveWorkspace();
  }

  // Equivalent Python script. Fit a back-to-back exponential:
  // Fit(InputWorkspace=pws, Function='name=BackToBackExponential')
  void test_peaks_fit() {
    Fit fit;
    fit.setChild(true);
    fit.initialize();

    // example X0, S values after a good fit are 10079.0, 404.5
    fit.setProperty("Function", "name=BackToBackExponential, X0=8500, S=800");
    fit.setProperty("InputWorkspace", m_onePeakWS);
    // fit.setProperty("MaxIterations", 99);
    fit.setProperty("CreateOutput", true);

    fit.execute();
  }

  // Equivalent Python script. Fit with a BSpline function:
  // Fit(InputWorkspace=ws, Function='name=BSpline, Order=40')
  void test_smooth_curve_fit() {
    Fit fit;
    fit.setChild(true);
    fit.initialize();

    // From a quick test, order 30 => ~2.5s; order 40 => ~6s; order 50 =>
    // ~14s
    fit.setProperty("Function", "name=BSpline, Order=20, StartX=0, EndX=10");
    fit.setProperty("InputWorkspace", m_smoothWS);
    fit.setProperty("CreateOutput", true);

    fit.execute();
  }

private:
  // Equivalent python script. Create data with a peak and a bit of noise:
  // pws = CreateSampleWorkspace(Function="User Defined",
  // UserDefinedFunction="name=BackToBackExponential, I=15000, A=1, B=1.2,
  // X0=10000, S=400", NumBanks=1, BankPixelWidth=1, Random=True)
  API::MatrixWorkspace_sptr generatePeaksCurveWorkspace() {
    Mantid::API::IAlgorithm_sptr sampleAlg =
        Mantid::API::AlgorithmManager::Instance().create(
            "CreateSampleWorkspace");
    sampleAlg->initialize();
    sampleAlg->setChild(true);
    sampleAlg->setProperty("Function", "User Defined");
    sampleAlg->setProperty(
        "UserDefinedFunction",
        "name=BackToBackExponential, I=15000, A=1, B=1.2, X0=10000, S=400");
    sampleAlg->setProperty("NumBanks", 1);
    sampleAlg->setProperty("BankPixelWidth", 1);
    sampleAlg->setProperty("XMin", 0.0);
    sampleAlg->setProperty("XMax", 100.0);
    sampleAlg->setProperty("BinWidth", 0.1);
    sampleAlg->setProperty("Random", true);
    sampleAlg->setPropertyValue("OutputWorkspace", "sample_peak_curve_ws");

    sampleAlg->execute();
    API::MatrixWorkspace_sptr ws = sampleAlg->getProperty("OutputWorkspace");

    return ws;
  }

  // Equivalent python script. Create smooth-ish data curve:
  // ws = CreateSampleWorkspace(Function="User Defined",
  // UserDefinedFunction="name=LinearBackground, A0=0.4, A1=0.4; name=Gaussian,
  // PeakCentre=1.3, Height=7, Sigma=1.7; name=Gaussian, PeakCentre=5,
  // Height=10, Sigma=0.7; name=Gaussian, PeakCentre=8, Height=9, Sigma=1.8",
  // NumBanks=1, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.01, Random=True)
  API::MatrixWorkspace_sptr generateSmoothCurveWorkspace() {
    Mantid::API::IAlgorithm_sptr sampleAlg =
        Mantid::API::AlgorithmManager::Instance().create(
            "CreateSampleWorkspace");
    sampleAlg->initialize();
    sampleAlg->setChild(true);
    sampleAlg->setProperty("Function", "User Defined");
    sampleAlg->setProperty(
        "UserDefinedFunction",
        "name=LinearBackground, A0=0.4, A1=0.4; name=Gaussian, PeakCentre=1.3, "
        "Height=7, Sigma=1.7; name=Gaussian, PeakCentre=5, Height=10, "
        "Sigma=0.7; name=Gaussian, PeakCentre=8, Height=9, Sigma=1.8");
    sampleAlg->setProperty("NumBanks", 1);
    sampleAlg->setProperty("BankPixelWidth", 1);
    sampleAlg->setProperty("XMin", 0.0);
    sampleAlg->setProperty("XMax", 10.0);
    sampleAlg->setProperty("BinWidth", 0.01);
    sampleAlg->setProperty("Random", true);
    sampleAlg->setPropertyValue("OutputWorkspace", "sample_smooth_curve_ws");

    sampleAlg->execute();
    API::MatrixWorkspace_sptr ws = sampleAlg->getProperty("OutputWorkspace");

    return ws;
  }

  API::MatrixWorkspace_sptr m_smoothWS;
  API::MatrixWorkspace_sptr m_onePeakWS;
};

#endif /*CURVEFITTING_FITMWTEST_H_*/
