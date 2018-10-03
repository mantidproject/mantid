#ifndef INELASTICISOROTDIFFTEST_H_
#define INELASTICISOROTDIFFTEST_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/InelasticIsoRotDiff.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include <cxxtest/TestSuite.h>
// third party library headers (n/a)
// standard library headers (n/a)
#include <limits>
#include <numeric>

#include <boost/make_shared.hpp>
using Mantid::CurveFitting::Functions::InelasticIsoRotDiff;
using BConstraint = Mantid::CurveFitting::Constraints::BoundaryConstraint;

class InelasticIsoRotDiffTest : public CxxTest::TestSuite {
public:
  void test_categories() {
    InelasticIsoRotDiff func;
    const std::vector<std::string> categories = func.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "QuasiElastic");
  }

  /**
   * @brief Parameters can be set and read
   */
  void test_parameters() {
    auto func = createTestInelasticIsoRotDiff();
    TS_ASSERT_EQUALS(func->nParams(), 4);
    TS_ASSERT_EQUALS(func->getParameter("Height"), 0.88);
    TS_ASSERT_EQUALS(func->getParameter("Radius"), 1.06);
    TS_ASSERT_EQUALS(func->getParameter("Tau"), 2.03);
    TS_ASSERT_EQUALS(func->getParameter("Centre"), 0.0004);
    TS_ASSERT_EQUALS(func->getAttribute("Q").asDouble(), 0.7);
    TS_ASSERT_EQUALS(func->getAttribute("N").asInt(), 9);
  }

  /**
   * @brief Test default constraints are implemented
   */
  void test_constraints() {
    auto func = createTestInelasticIsoRotDiff();
    std::vector<std::string> parameters{"Height", "Radius", "Tau"};
    for (auto parameter : parameters) {
      auto i = func->parameterIndex(parameter);
      auto constraint = static_cast<BConstraint *>(func->getConstraint(i));
      TS_ASSERT(constraint);
      TS_ASSERT_EQUALS(constraint->hasLower(), true);
      TS_ASSERT_EQUALS(constraint->lower(),
                       std::numeric_limits<double>::epsilon());
    }
  }

  /**
   * @brief Evaluate the function at one particular enery value
   */
  void test_function_gives_expected_value_for_given_input() {
    auto func = createTestInelasticIsoRotDiff();
    const size_t nData(1);
    std::vector<double> xValues(nData, 0.1); // Evaluate at E=0.1meV
    std::vector<double> calculatedValues(nData, 0);
    func->function1D(calculatedValues.data(), xValues.data(), nData);
    TS_ASSERT_DELTA(calculatedValues[0], 0.0702102, 1e-6);
  }

  /**
   * @brief Test function is normalized in the Energy axis
   */
  void test_normalization() {
    auto func = createTestInelasticIsoRotDiff();
    func->setParameter("Tau", 50.0);  // make it peaky
    func->setAttributeValue("N", 25); // more terms for more precission
    double dE(0.0001);                // dE is 1micro-eV
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
    std::cout << integral << std::endl;
    TS_ASSERT_DELTA(integral, 0.147393, 1e-5);
  }

private:
  class TestableInelasticIsoRotDiff : public InelasticIsoRotDiff {
  public:
    void function1D(double *out, const double *xValues,
                    const size_t nData) const override {
      InelasticIsoRotDiff::function1D(out, xValues, nData);
    }
  };

  boost::shared_ptr<TestableInelasticIsoRotDiff>
  createTestInelasticIsoRotDiff() {
    auto func = boost::make_shared<TestableInelasticIsoRotDiff>();
    func->initialize();
    func->setParameter("Height", 0.88);
    func->setParameter("Radius", 1.06); // Angstrom
    func->setParameter("Tau", 2.03);    // picosecond
    func->setParameter("Centre", 0.0004);
    func->setAttributeValue("Q", 0.7); // inverse Angstrom
    func->setAttributeValue("N", 9);
    return func;
  }
};

#endif /*INELASTICISOROTDIFFTEST_H_*/
