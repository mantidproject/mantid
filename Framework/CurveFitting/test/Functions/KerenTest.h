#ifndef MANTID_CURVEFITTING_KERENTEST_H_
#define MANTID_CURVEFITTING_KERENTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Keren.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <numeric>

using Mantid::CurveFitting::Functions::Keren;
using Mantid::API::Workspace_sptr;
using Mantid::API::AlgorithmManager;
using Mantid::API::IFunction_sptr;
using Mantid::API::WorkspaceFactory;
using Mantid::HistogramData::HistogramX;

/**
 * Structure to hold Y, E data with X0 and DeltaX
 */
struct MockData {
  double x0;
  double dX;
  Mantid::MantidVec y;
  Mantid::MantidVec e;
};

/// Test class to test protected methods
class TestFunction : public Keren {
public:
  double wrapRelaxation(const double delta, const double larmor,
                        const double fluct, const double time) const {
    return relaxation(delta, larmor, fluct, time);
  }
  double wrapPolarization(const double delta, const double larmor,
                          const double fluct, const double time) const {
    return polarization(delta, larmor, fluct, time);
  }
  void wrapFunc1D(double *out, const double *xValues,
                  const size_t nData) const {
    function1D(out, xValues, nData);
  }
};

class KerenTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static KerenTest *createSuite() { return new KerenTest(); }
  static void destroySuite(KerenTest *suite) { delete suite; }

  void test_name() {
    Keren function;
    TS_ASSERT_EQUALS("Keren", function.name());
  }

  void test_category() {
    Keren function;
    TS_ASSERT_EQUALS("Muon", function.category());
  }

  /// Test the function against mock data
  void test_fitting() {
    const auto workspace = getMockDataWorkspace();
    Keren function;
    function.initialize();

    // set some reasonable starting values
    function.setParameter("Field", 80.0);
    function.setParameter("Fluct", 0.2);
    function.setParameter("Delta", 0.2);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->initialize();
    fit->setChild(true);
    fit->setPropertyValue("Function", function.asString());
    fit->setProperty("InputWorkspace", workspace);
    fit->setProperty("WorkspaceIndex", 0);
    TS_ASSERT_THROWS_NOTHING(fit->execute());
    TS_ASSERT(fit->isExecuted());
    std::string status = fit->getPropertyValue("OutputStatus");
    TS_ASSERT_EQUALS("success", status);

    // check the output
    const double field = 100;
    const double delta =
        Mantid::PhysicalConstants::MuonGyromagneticRatio * field * 0.2;
    const double fluct = delta;
    IFunction_sptr out = fit->getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("Field"), field, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Delta"), delta, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Fluct"), fluct, 0.001);
  }

  void test_relaxation() {
    TestFunction function;
    function.initialize();

    const double field = 100;
    const double larmor =
        Mantid::PhysicalConstants::MuonGyromagneticRatio * field;
    const double delta = larmor * 0.2;
    const double fluct = delta;

    TS_ASSERT_DELTA(function.wrapRelaxation(delta, larmor, fluct, 1.0), 0.0582,
                    0.0001);
    TS_ASSERT_DELTA(function.wrapRelaxation(delta, larmor, fluct, 5.0), 0.1555,
                    0.001);
    TS_ASSERT_DELTA(function.wrapRelaxation(delta, larmor, fluct, 10.0), 0.2753,
                    0.001);
  }

  void test_polarization() {
    TestFunction function;
    function.initialize();

    const double field = 100;
    const double larmor =
        Mantid::PhysicalConstants::MuonGyromagneticRatio * field;
    const double delta = larmor * 0.2;
    const double fluct = delta;

    TS_ASSERT_DELTA(function.wrapPolarization(delta, larmor, fluct, 1.0),
                    0.9434, 0.001);
    TS_ASSERT_DELTA(function.wrapPolarization(delta, larmor, fluct, 5.0),
                    0.8560, 0.001);
    TS_ASSERT_DELTA(function.wrapPolarization(delta, larmor, fluct, 10.0),
                    0.7594, 0.001);
  }

  void test_evaluateFunction() {
    TestFunction function;
    function.initialize();
    const double field = 100;
    const double delta =
        Mantid::PhysicalConstants::MuonGyromagneticRatio * field * 0.2;
    const double fluct = delta;
    function.setParameter("A", 1.0);
    function.setParameter("Delta", delta);
    function.setParameter("Field", field);
    function.setParameter("Fluct", fluct);

    double x = 1.0;
    double y;
    TS_ASSERT_THROWS_NOTHING(function.wrapFunc1D(&y, &x, 1));
    TS_ASSERT_DELTA(y, 0.9434, 0.001);
  }

private:
  /**
   * Mock data from an Excel spreadsheet with
   * B = 100 Gauss, omega_L = 5*Delta, nu = Delta and time from 0-10 Delta^-1
   * @returns :: Mock data structure
   */
  MockData getMockData() {
    MockData data;
    data.x0 = 0;
    data.dX = 0.922276444; // steps of 0.25/Delta
    data.y = {1,           0.950341815, 0.875262777, 0.848565312, 0.859885346,
              0.863200168, 0.839703519, 0.808928875, 0.790496951, 0.782534602,
              0.772858742, 0.75648003,  0.73822774,  0.723281868, 0.711316499,
              0.699160478, 0.685454747, 0.671399296, 0.658356469, 0.646276957,
              0.634337926, 0.622165429, 0.610055255, 0.598363028, 0.587082639,
              0.575998979, 0.565007276, 0.554178119, 0.543602216, 0.533277709,
              0.523146503, 0.51317749,  0.503385488, 0.493791756, 0.484393934,
              0.475174597, 0.466122922, 0.45724013,  0.448529464, 0.439988183,
              0.431609546};
    data.e = Mantid::MantidVec(41, 0.01);
    return data;
  }

  /**
   * Get a workspace with mock data in it
   * @returns :: Workspace with mock data in it
   */
  Workspace_sptr getMockDataWorkspace() {
    MockData data = getMockData();
    size_t N = data.y.size();
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, N, N);
    HistogramX x(N);
    std::iota(x.begin(), x.end(), data.x0);
    x *= data.dX;
    ws->mutableX(0) = x;
    ws->dataY(0) = data.y;
    ws->dataE(0) = data.e;
    return ws;
  }
};

#endif /* MANTID_CURVEFITTING_KERENTEST_H_ */
