// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/ChudleyElliotSQE.h"
// Mantid Headers from the same project (n/a)
// Mantid headers from other projects
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include <cxxtest/TestSuite.h>
// third party library headers (n/a)
// standard library headers (n/a)
#include <numeric>
#include <random>

#include <memory>
using Mantid::CurveFitting::Functions::ChudleyElliotSQE;

class ChudleyElliotSQETest : public CxxTest::TestSuite {
public:
  void test_categories() {
    ChudleyElliotSQE func;
    const std::vector<std::string> categories = func.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "QuasiElastic");
  }

  /**
   * @brief Parameters can be set and read
   */
  void test_parameters() {
    auto func = createTestChudleyElliotSQE();
    TS_ASSERT_EQUALS(func->nParams(), 4);
    TS_ASSERT_EQUALS(func->getParameter("Height"), 1.0);
    TS_ASSERT_EQUALS(func->getParameter("L"), 1.0);
    TS_ASSERT_EQUALS(func->getParameter("Tau"), 1.25);
    TS_ASSERT_EQUALS(func->getParameter("Centre"), 0.001);
  }

  /**
   * @brief Evaluate the function at one particular enery value
   */
  void test_function_gives_expected_value_for_given_input() {
    auto func = createTestChudleyElliotSQE();
    const size_t nData(1);
    std::vector<double> xValues(nData, 0.1); // Evaluate at E=0.1meV
    std::vector<double> calculatedValues(nData, 0);
    func->function1D(calculatedValues.data(), xValues.data(), nData);
    TS_ASSERT_DELTA(calculatedValues[0], 1.584523780, 1e-8);
  }

  /**
   * @brief Test function is normalized in the Energy axis
   */
  void test_normalization() {
    auto func = createTestChudleyElliotSQE();
    double dE(0.0001); // dE is 1micro-eV
    const size_t nData(20000);
    // Create the domain of energy values
    std::vector<double> xValues(nData, 0);
    std::iota(xValues.begin(), xValues.end(), -10000.0);
    using std::placeholders::_1;
    std::transform(xValues.begin(), xValues.end(), xValues.begin(), std::bind(std::multiplies<double>(), dE, _1));
    // Evaluate the function on the domain
    std::vector<double> calculatedValues(nData, 0);
    func->function1D(calculatedValues.data(), xValues.data(), nData);
    // Integrate the evaluation
    std::transform(calculatedValues.begin(), calculatedValues.end(), calculatedValues.begin(),
                   std::bind(std::multiplies<double>(), dE, _1));
    auto integral = std::accumulate(calculatedValues.begin(), calculatedValues.end(), 0.0);
    TS_ASSERT_DELTA(integral, 1.0, 0.1);
  }

private:
  class TestableChudleyElliotSQE : public ChudleyElliotSQE {
  public:
    void function1D(double *out, const double *xValues, const size_t nData) const override {
      ChudleyElliotSQE::function1D(out, xValues, nData);
    }
  };

  std::shared_ptr<TestableChudleyElliotSQE> createTestChudleyElliotSQE() {
    auto func = std::make_shared<TestableChudleyElliotSQE>();
    func->initialize();
    func->setParameter("Height", 1.0);
    func->setParameter("L", 1.0);
    func->setParameter("Tau", 1.25);
    func->setParameter("Centre", 0.001);
    func->setAttributeValue("Q", 1.0);
    // HWHM=0.329105813
    return func;
  }
};
