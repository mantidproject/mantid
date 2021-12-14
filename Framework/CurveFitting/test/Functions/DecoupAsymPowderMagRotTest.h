// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/DecoupAsymPowderMagRot.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidCurveFitting/MuonHelpers.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::MuonHelper;
using Mantid::MantidVec;

class DecoupAsymPowderMagRotTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DecoupAsymPowderMagRotTest *createSuite() { return new DecoupAsymPowderMagRotTest(); }
  static void destroySuite(DecoupAsymPowderMagRotTest *suite) { delete suite; }

  void test_category() {
    DecoupAsymPowderMagRot fn;
    TS_ASSERT_EQUALS(fn.category(), "Muon\\MuonModelling\\Magnetism");
  }

  void test_function_parameter_settings() {
    auto dapmr = createTestDecoupAsymPowderMagRot();

    TS_ASSERT_THROWS(dapmr->setParameter("X", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(dapmr->setParameter("A9", 1.0), const std::invalid_argument &);
  }

  void test_function_gives_expected_value_for_given_input() {
    auto dapmr = createTestDecoupAsymPowderMagRot();

    const double asymm = dapmr->getParameter("Asymmetry");
    const double charField = dapmr->getParameter("CharField");

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0.1);
    std::array<double, numPoints> yValues;
    dapmr->function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      auto A_z = getAz(xValues[i], charField);
      TS_ASSERT_DELTA(yValues[i], asymm * (1 - A_z), 1e-12);
    }
  }

  void test_jacobian_gives_expected_values() {
    auto dapmr = createTestDecoupAsymPowderMagRot();

    const size_t nData(1);
    std::vector<double> xValues(nData, 1100.0);

    Mantid::CurveFitting::Jacobian jacobian(nData, 2);
    dapmr->functionDeriv1D(&jacobian, xValues.data(), nData);

    double dfdasym = jacobian.get(0, 0);
    double dfdcharField = jacobian.get(0, 1);

    TS_ASSERT_DELTA(dfdasym, 0.3789116772, 1e-8);
    TS_ASSERT_DELTA(dfdcharField, -0.0002968811, 1e-8);
    ;
  }

private:
  class TestableDecoupAsymPowderMagRot : public DecoupAsymPowderMagRot {
  public:
    void function1D(double *out, const double *xValues, const size_t nData) const override {
      DecoupAsymPowderMagRot::function1D(out, xValues, nData);
    }
    void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) override {
      DecoupAsymPowderMagRot::functionDeriv1D(out, xValues, nData);
    }
  };

  std::shared_ptr<TestableDecoupAsymPowderMagRot> createTestDecoupAsymPowderMagRot() {
    auto func = std::make_shared<TestableDecoupAsymPowderMagRot>();
    func->initialize();
    func->setParameter("Asymmetry", 2.3);
    func->setParameter("CharField", 900.0);
    return func;
  }
};
