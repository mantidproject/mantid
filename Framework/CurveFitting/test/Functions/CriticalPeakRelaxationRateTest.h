// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Functions/CriticalPeakRelaxationRate.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using Mantid::MantidVec;

class CriticalPeakRelaxationRateTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CriticalPeakRelaxationRateTest *createSuite() { return new CriticalPeakRelaxationRateTest(); }
  static void destroySuite(CriticalPeakRelaxationRateTest *suite) { delete suite; }

  void test_category() {
    CriticalPeakRelaxationRate fn;
    TS_ASSERT_EQUALS(fn.category(), "Muon\\MuonModelling\\Magnetism");
  }

  void test_function_parameter_settings() {
    auto critprr = createTestCriticalPeakRelaxationRate();

    TS_ASSERT_THROWS(critprr->setParameter("X", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(critprr->setParameter("A9", 1.0), const std::invalid_argument &);
  }

  void test_x_tc_values() {
    auto critprr = createTestCriticalPeakRelaxationRate();

    // when x and Tc are not the same
    const std::size_t numPoints = 1;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 1.2);
    std::array<double, numPoints> yValues;
    critprr->function1D(yValues.data(), xValues.data(), numPoints);
    TS_ASSERT_THROWS_NOTHING(critprr->checkParams(xValues.data(), numPoints));

    // when x and Tc have the same value
    auto critprr2 = createTestCriticalPeakRelaxationRate();
    const std::size_t numPoints2 = 1;
    std::array<double, numPoints2> xValues2;
    std::iota(xValues2.begin(), xValues2.end(), 0.5);
    std::array<double, numPoints2> yValues2;
    TS_ASSERT_THROWS(critprr2->function1D(yValues2.data(), xValues2.data(), numPoints2), const std::invalid_argument &);
  }

  void test_function_gives_expected_value_for_given_input() {
    auto critprr = createTestCriticalPeakRelaxationRate();

    const double scale = critprr->getParameter("Scaling");
    const double tc = critprr->getParameter("CriticalTemp");
    const double exp = critprr->getParameter("Exponent");
    const double bg = critprr->getParameter("Background");

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0.6);
    std::array<double, numPoints> yValues;
    critprr->function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      double expression = abs(xValues[i] - tc);
      TS_ASSERT_DELTA(yValues[i], scale / pow(expression, exp) + bg, 1e-12);
    }
  }

  void test_jacobian_gives_expected_values() {
    auto activ = createTestCriticalPeakRelaxationRate();

    const size_t nData(1);
    std::vector<double> xValues(nData, 3.5);

    Mantid::CurveFitting::Jacobian jacobian(nData, 4);
    activ->functionDeriv1D(&jacobian, xValues.data(), nData);

    double dfdsc = jacobian.get(0, 0);
    double dfdtc = jacobian.get(0, 1);
    double dfdexp = jacobian.get(0, 2);
    double dfdbg = jacobian.get(0, 3);

    TS_ASSERT_DELTA(dfdsc, 0.012345679, 1e-7);
    TS_ASSERT_DELTA(dfdtc, 0.037860082, 1e-7);
    TS_ASSERT_DELTA(dfdexp, 0.031195163, 1e-7);
    TS_ASSERT_DELTA(dfdbg, 1.0, 1e-7);
  }

private:
  class TestableCriticalPeakRelaxationRate : public CriticalPeakRelaxationRate {
  public:
    void function1D(double *out, const double *xValues, const size_t nData) const override {
      CriticalPeakRelaxationRate::function1D(out, xValues, nData);
    }
    void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) override {
      CriticalPeakRelaxationRate::functionDeriv1D(out, xValues, nData);
    }
  };

  std::shared_ptr<TestableCriticalPeakRelaxationRate> createTestCriticalPeakRelaxationRate() {
    auto func = std::make_shared<TestableCriticalPeakRelaxationRate>();
    func->initialize();
    func->setParameter("Scaling", 2.3);
    func->setParameter("CriticalTemp", 0.5);
    func->setParameter("Exponent", 4.0);
    func->setParameter("Background", 4.0);
    return func;
  }
};
