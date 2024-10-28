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
#include "MantidCurveFitting/Functions/MagneticOrderParameter.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using Mantid::MantidVec;

class MagneticOrderParameterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MagneticOrderParameterTest *createSuite() { return new MagneticOrderParameterTest(); }
  static void destroySuite(MagneticOrderParameterTest *suite) { delete suite; }

  void test_category() {
    MagneticOrderParameter fn;
    TS_ASSERT_EQUALS(fn.category(), "Muon\\MuonModelling\\Magnetism");
  }

  void test_function_parameter_settings() {
    auto mop = createTestMagneticOrderParameter();

    TS_ASSERT_THROWS(mop->setParameter("X", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(mop->setParameter("A9", 1.0), const std::invalid_argument &);
  }

  void test_function_gives_expected_value_for_given_input() {
    auto mop = createTestMagneticOrderParameter();

    const double amp = mop->getParameter("A0");
    const double alpha = mop->getParameter("Alpha");
    const double beta = mop->getParameter("Beta");
    const double tc = mop->getParameter("CriticalTemp");

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    mop->function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], amp * pow((1 - pow(xValues[i] / tc, alpha)), beta), 1e-12);
    }
  }

  void test_jacobian_gives_expected_values() {
    auto mop = createTestMagneticOrderParameter();

    const size_t nData(1);
    std::vector<double> xValues(nData, 3.5);

    Mantid::CurveFitting::Jacobian jacobian(nData, 4);
    mop->functionDeriv1D(&jacobian, xValues.data(), nData);

    double dfdamp = jacobian.get(0, 0);
    double dfdalpha = jacobian.get(0, 1);
    double dfdbeta = jacobian.get(0, 2);
    double dfdtc = jacobian.get(0, 3);

    TS_ASSERT_DELTA(dfdamp, 0.9999999629, 1e-8);
    TS_ASSERT_DELTA(dfdalpha, 0.0000003793, 1e-8);
    TS_ASSERT_DELTA(dfdbeta, -0.0000000426, 1e-8);
    TS_ASSERT_DELTA(dfdtc, 0.0000000011, 1e-8);
  }

  // test ensures that certain variables return 0 if calculation hits nan or inf
  void test_jacobian_gives_expected_values_not_finite_numbers() {
    auto mop = createTestMagneticOrderParameter_infinite();

    const size_t nData(1);
    std::vector<double> xValues(nData, 4.5);

    Mantid::CurveFitting::Jacobian jacobian(nData, 4);
    mop->functionDeriv1D(&jacobian, xValues.data(), nData);

    double dfdamp = jacobian.get(0, 0);
    double dfdalpha = jacobian.get(0, 1);
    double dfdbeta = jacobian.get(0, 2);
    double dfdtc = jacobian.get(0, 3);

    TS_ASSERT_DELTA(dfdamp, 0.0, 1e-8);
    TS_ASSERT_DELTA(dfdalpha, 0.0, 1e-8);
    TS_ASSERT_DELTA(dfdbeta, 0.0, 1e-8);
    TS_ASSERT_DELTA(dfdtc, 0.0, 1e-8);
  }

private:
  class TestableMagneticOrderParameter : public MagneticOrderParameter {
  public:
    void function1D(double *out, const double *xValues, const size_t nData) const override {
      MagneticOrderParameter::function1D(out, xValues, nData);
    }
    void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) override {
      MagneticOrderParameter::functionDeriv1D(out, xValues, nData);
    }
  };

  std::shared_ptr<TestableMagneticOrderParameter> createTestMagneticOrderParameter() {
    auto func = std::make_shared<TestableMagneticOrderParameter>();
    func->initialize();
    func->setParameter("A0", 2.3);
    func->setParameter("Alpha", 4.0);
    func->setParameter("Beta", 2.0);
    func->setParameter("CriticalTemp", 300.0);
    return func;
  }

  std::shared_ptr<TestableMagneticOrderParameter> createTestMagneticOrderParameter_infinite() {
    auto func = std::make_shared<TestableMagneticOrderParameter>();
    func->initialize();
    func->setParameter("A0", 3.3);
    func->setParameter("Alpha", 4.0);
    func->setParameter("Beta", 0.5);
    func->setParameter("CriticalTemp", 3.0);
    return func;
  }
};
