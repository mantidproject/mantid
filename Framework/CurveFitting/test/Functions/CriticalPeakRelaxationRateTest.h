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

  void test_function_gives_expected_value_for_given_input() {
    auto critprr = createTestCriticalPeakRelaxationRate();

    const double Scale = critprr->getParameter("Scaling");
    const double Tc = critprr->getParameter("CriticalTemp");
    const double Exp = critprr->getParameter("Exponent");
    const double Bg1 = critprr->getParameter("Background1");
    const double Bg2 = critprr->getParameter("Background2");
    const double Delta = critprr->getAttribute("Delta").asDouble();

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    critprr->function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      double expression = fabs(xValues[i] - Tc);
      if (xValues[i] + Delta < Tc || xValues[i] - Delta > Tc) {
        if (xValues[i] < Tc) {
          TS_ASSERT_DELTA(yValues[i], Bg1 + Scale / pow(expression, Exp), 1e-4);
        }
        if (xValues[i] > Tc) {
          TS_ASSERT_DELTA(yValues[i], Bg2 + Scale / pow(expression, Exp), 1e-4);
        }
      } else {
        TS_ASSERT_DELTA(yValues[i], 1e6, 1e-4);
      }
    }
  }

private:
  class TestableCriticalPeakRelaxationRate : public CriticalPeakRelaxationRate {
  public:
    void function1D(double *out, const double *xValues, const size_t nData) const override {
      CriticalPeakRelaxationRate::function1D(out, xValues, nData);
    }
  };

  std::shared_ptr<TestableCriticalPeakRelaxationRate> createTestCriticalPeakRelaxationRate() {
    auto func = std::make_shared<TestableCriticalPeakRelaxationRate>();
    func->initialize();
    func->setParameter("Scaling", 2.3);
    func->setParameter("CriticalTemp", 7.0);
    func->setParameter("Exponent", 4.0);
    func->setParameter("Background1", 1.3);
    func->setParameter("Background2", 4.5);
    return func;
  }
};
