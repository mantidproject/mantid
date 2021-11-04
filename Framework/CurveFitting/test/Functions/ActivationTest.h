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
#include "MantidCurveFitting/Functions/Activation.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using Mantid::MantidVec;

class ActivationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ActivationTest *createSuite() { return new ActivationTest(); }
  static void destroySuite(ActivationTest *suite) { delete suite; }

  void test_category() {
    Activation fn;
    TS_ASSERT_EQUALS(fn.category(), "Muon\\MuonModelling");
  }

  void test_function_parameter_settings() {
    auto activ = createTestActivation();

    TS_ASSERT_THROWS(activ->setParameter("X", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(activ->setParameter("A9", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(activ->setAttributeValue("type", "thng"), const std::invalid_argument &);
  }

  void test_unit_checker() {
    auto activ = createTestActivation();

    activ->setAttributeValue("Unit", "K");
    TS_ASSERT_THROWS_NOTHING(activ->beforeFunctionSet());

    activ->setAttributeValue("Unit", "k");
    TS_ASSERT_THROWS_NOTHING(activ->beforeFunctionSet());

    activ->setAttributeValue("Unit", "meV");
    TS_ASSERT_THROWS_NOTHING(activ->beforeFunctionSet());

    activ->setAttributeValue("Unit", "mev");
    TS_ASSERT_THROWS_NOTHING(activ->beforeFunctionSet());

    activ->setAttributeValue("Unit", "mevk");
    TS_ASSERT_THROWS(activ->beforeFunctionSet(), const std::invalid_argument &);
  }

  void test_function_gives_expected_value_for_K_given_input() {
    auto activ = createTestActivation();
    activ->setAttributeValue("Unit", "K");

    const double attemptRate = activ->getParameter("AttemptRate");
    const double barrier = activ->getParameter("Barrier");

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    activ->function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], attemptRate * exp(-barrier / xValues[i]), 1e-12);
    }
  }

  void test_function_gives_expected_value_for_meV_given_input() {
    auto activ = createTestActivation();
    activ->setAttributeValue("Unit", "meV");

    const double attemptRate = activ->getParameter("AttemptRate");
    const double barrier = activ->getParameter("Barrier");
    const double meVConv = Mantid::PhysicalConstants::meVtoKelvin;

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    activ->function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], attemptRate * exp(-(meVConv * barrier) / xValues[i]), 1e-12);
    }
  }

  void test_jacobian_gives_expected_values_K() {
    auto activ = createTestActivation();
    activ->setAttributeValue("Unit", "K");

    const size_t nData(1);
    std::vector<double> xValues(nData, 3.5);

    Mantid::CurveFitting::Jacobian jacobian(nData, 2);
    activ->functionDeriv1D(&jacobian, xValues.data(), nData);

    double dfdar = jacobian.get(0, 0);
    double dfdbarrier = jacobian.get(0, 1);

    TS_ASSERT_DELTA(dfdar, 0.318906557, 1e-7);
    TS_ASSERT_DELTA(dfdbarrier, -0.209567166, 1e-7);
  }

  void test_jacobian_gives_expected_values_meV() {
    auto activ = createTestActivation();
    activ->setAttributeValue("Unit", "meV");

    const size_t nData(1);
    std::vector<double> xValues(nData, 3.5);

    Mantid::CurveFitting::Jacobian jacobian(nData, 2);
    activ->functionDeriv1D(&jacobian, xValues.data(), nData);

    double dfdar = jacobian.get(0, 0);
    double dfdbarrier = jacobian.get(0, 1);

    TS_ASSERT_DELTA(dfdar, 0.0000017388, 1e-7);
    TS_ASSERT_DELTA(dfdbarrier, -0.000013260, 1e-7);
  }

private:
  class TestableActivation : public Activation {
  public:
    void function1D(double *out, const double *xValues, const size_t nData) const override {
      Activation::function1D(out, xValues, nData);
    }
    void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) override {
      Activation::functionDeriv1D(out, xValues, nData);
    }
  };

  std::shared_ptr<TestableActivation> createTestActivation() {
    auto func = std::make_shared<TestableActivation>();
    func->initialize();
    func->setParameter("AttemptRate", 2.3);
    func->setParameter("Barrier", 4.0);
    return func;
  }
};
