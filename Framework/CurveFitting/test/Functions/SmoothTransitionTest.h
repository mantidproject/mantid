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
#include "MantidCurveFitting/Functions/SmoothTransition.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using Mantid::MantidVec;

class SmoothTransitionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SmoothTransitionTest *createSuite() { return new SmoothTransitionTest(); }
  static void destroySuite(SmoothTransitionTest *suite) { delete suite; }

  void test_category() {
    SmoothTransition fn;
    TS_ASSERT_EQUALS(fn.category(), "Muon\\MuonModelling");
  }

  void test_function_gives_expected_value_for_given_input() {
    auto st = createTestSmoothTransition();

    TS_ASSERT_THROWS(st->setParameter("mid", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(st->setParameter("A9", 1.0), const std::invalid_argument &);

    const double a1 = st->getParameter("A1");
    const double a2 = st->getParameter("A2");
    const double midpoint = st->getParameter("Midpoint");
    const double gr = st->getParameter("GrowthRate");

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    st->function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], a2 + (a1 - a2) / (exp((xValues[i] - midpoint) / gr) + 1), 1e-12);
    }
  }

  void test_jacobian_gives_expected_values() {
    auto st = createTestSmoothTransition();

    const size_t nData(1);
    std::vector<double> xValues(nData, 3.5);

    Mantid::CurveFitting::Jacobian jacobian(nData, 4);
    st->functionDeriv1D(&jacobian, xValues.data(), nData);

    double dfda1 = jacobian.get(0, 0);
    double dfda2 = jacobian.get(0, 1);
    double dfdmp = jacobian.get(0, 2);
    double dfdgr = jacobian.get(0, 3);

    TS_ASSERT_DELTA(dfda1, 0.9758729786, 1e-8);
    TS_ASSERT_DELTA(dfda2, 0.0241270214, 1e-8);
    TS_ASSERT_DELTA(dfdmp, -0.0400263440, 1e-8);
    TS_ASSERT_DELTA(dfdgr, 0.1480974729, 1e-8);
  }

private:
  class TestableSmoothTransition : public SmoothTransition {
  public:
    void function1D(double *out, const double *xValues, const size_t nData) const override {
      SmoothTransition::function1D(out, xValues, nData);
    }
    void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) override {
      SmoothTransition::functionDeriv1D(out, xValues, nData);
    }
  };

  std::shared_ptr<TestableSmoothTransition> createTestSmoothTransition() {
    auto func = std::make_shared<TestableSmoothTransition>();
    func->initialize();
    func->setParameter("A1", 2.3);
    func->setParameter("A2", 4.0);
    func->setParameter("Midpoint", 7.2);
    func->setParameter("GrowthRate", 1.0);
    return func;
  }
};
