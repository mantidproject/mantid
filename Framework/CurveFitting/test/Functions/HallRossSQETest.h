// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/HallRossSQE.h"
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
using Mantid::CurveFitting::Functions::HallRossSQE;

class HallRossSQETest : public CxxTest::TestSuite {
public:
  void test_categories() {
    HallRossSQE func;
    const std::vector<std::string> categories = func.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "QuasiElastic");
  }

  /**
   * @brief Parameters can be set and read
   */
  void test_parameters() {
    auto func = createTestHallRossSQE();
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
    auto func = createTestHallRossSQE();
    const size_t nData(1);
    std::vector<double> xValues(nData, 0.1); // Evaluate at E=0.1meV
    std::vector<double> calculatedValues(nData, 0);
    func->function1D(calculatedValues.data(), xValues.data(), nData);
    TS_ASSERT_DELTA(calculatedValues[0], 1.250758222, 1e-8);
  }

private:
  class TestableHallRossSQE : public HallRossSQE {
  public:
    void function1D(double *out, const double *xValues, const size_t nData) const override {
      HallRossSQE::function1D(out, xValues, nData);
    }
  };

  std::shared_ptr<TestableHallRossSQE> createTestHallRossSQE() {
    auto func = std::make_shared<TestableHallRossSQE>();
    func->initialize();
    func->setParameter("Height", 1.0);
    func->setParameter("L", 1.0);
    func->setParameter("Tau", 1.25);
    func->setParameter("Centre", 0.001);
    func->setAttributeValue("Q", 1.0);
    return func;
  }
};
