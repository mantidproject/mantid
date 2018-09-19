#ifndef TEIXEIRAWATERSQETEST_H_
#define TEIXEIRAWATERSQETEST_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/TeixeiraWaterSQE.h"
// Mantid Headers from the same project (n/a)
// Mantid headers from other projects
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include <cxxtest/TestSuite.h>
// third party library headers (n/a)
// standard library headers (n/a)
#include <numeric>
#include <random>

#include <boost/make_shared.hpp>
using Mantid::CurveFitting::Functions::TeixeiraWaterSQE;

class TeixeiraWaterSQETest : public CxxTest::TestSuite {
public:
  void test_categories() {
    TeixeiraWaterSQE func;
    const std::vector<std::string> categories = func.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "QuasiElastic");
  }

  /**
   * @brief Parameters can be set and read
   */
  void test_parameters() {
    auto func = createTestTeixeiraWaterSQE();
    TS_ASSERT_EQUALS(func->nParams(), 4);
    TS_ASSERT_EQUALS(func->getParameter("Height"), 1.0);
    TS_ASSERT_EQUALS(func->getParameter("DiffCoeff"), 1.0);
    TS_ASSERT_EQUALS(func->getParameter("Tau"), 1.0);
    TS_ASSERT_EQUALS(func->getParameter("Centre"), 0.001);
  }

  /**
   * @brief Evaluate the function at one particular enery value
   */
  void test_function_gives_expected_value_for_given_input() {
    auto func = createTestTeixeiraWaterSQE();
    const size_t nData(1);
    std::vector<double> xValues(nData, 0.1); // Evaluate at E=0.1meV
    std::vector<double> calculatedValues(nData, 0);
    func->function1D(calculatedValues.data(), xValues.data(), nData);
    TS_ASSERT_DELTA(calculatedValues[0], 1.423369463, 1e-8);
  }

  /**
   * @brief Test function is normalized in the Energy axis
   */
  void test_normalization() {
    auto func = createTestTeixeiraWaterSQE();
    func->setParameter("Tau", 50.0); // make it peaky
    double dE(0.0001);               // dE is 1micro-eV
    const size_t nData(20000);
    // Create the domain of energy values
    std::vector<double> xValues(nData, 0);
    std::iota(xValues.begin(), xValues.end(), -10000.0);
    std::transform(xValues.begin(), xValues.end(), xValues.begin(),
                   std::bind1st(std::multiplies<double>(), dE));
    // Evaluate the function on the domain
    std::vector<double> calculatedValues(nData, 0);
    func->function1D(calculatedValues.data(), xValues.data(), nData);
    // Integrate the evaluation
    std::transform(calculatedValues.begin(), calculatedValues.end(),
                   calculatedValues.begin(),
                   std::bind1st(std::multiplies<double>(), dE));
    auto integral =
        std::accumulate(calculatedValues.begin(), calculatedValues.end(), 0.0);
    TS_ASSERT_DELTA(integral, 1.0, 0.01);
  }

private:
  class TestableTeixeiraWaterSQE : public TeixeiraWaterSQE {
  public:
    void function1D(double *out, const double *xValues,
                    const size_t nData) const override {
      TeixeiraWaterSQE::function1D(out, xValues, nData);
    }
  };

  boost::shared_ptr<TestableTeixeiraWaterSQE> createTestTeixeiraWaterSQE() {
    auto func = boost::make_shared<TestableTeixeiraWaterSQE>();
    func->initialize();
    func->setParameter("Height", 1.0);
    func->setParameter("DiffCoeff", 1.0); // 1Angstrom
    func->setParameter("Tau", 1.0);       // 1ps
    func->setParameter("Centre", 0.001);  // shifted by 1micro-eV
    func->setAttributeValue("Q", 1.0);    // 1Angstrom^{-1}
    // HWHM=0.329105813
    return func;
  }
};

#endif /*TEIXEIRAWATERSQETEST_H_*/
