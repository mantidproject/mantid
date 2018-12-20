#ifndef ELASTICISOROTDIFFTEST_H_
#define ELASTICISOROTDIFFTEST_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/ElasticIsoRotDiff.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include <cxxtest/TestSuite.h>
// third party library headers (n/a)
// standard library headers (n/a)
#include <limits>

#include <boost/make_shared.hpp>
using Mantid::CurveFitting::Functions::ElasticIsoRotDiff;
using BConstraint = Mantid::CurveFitting::Constraints::BoundaryConstraint;

class ElasticIsoRotDiffTest : public CxxTest::TestSuite {
public:
  void test_categories() {
    ElasticIsoRotDiff func;
    const std::vector<std::string> categories = func.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "QuasiElastic");
  }

  /**
   * @brief Parameters can be set and read
   */
  void test_parameters() {
    auto func = createTestElasticIsoRotDiff();
    TS_ASSERT_EQUALS(func->nParams(), 3);
    TS_ASSERT_EQUALS(func->getParameter("Height"), 0.88);
    TS_ASSERT_EQUALS(func->getParameter("Radius"), 1.06);
    TS_ASSERT_EQUALS(func->getParameter("Centre"), 0.0004);
    TS_ASSERT_EQUALS(func->getAttribute("Q").asDouble(), 0.7);
  }

  /**
   * @brief Test default constraints are implemented
   */
  void test_constraints() {
    auto func = createTestElasticIsoRotDiff();
    std::vector<std::string> parameters{"Height", "Radius"};
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
   * @brief Evaluate the prefactor
   */
  void test_function_gives_expected_prefactor() {
    auto func = createTestElasticIsoRotDiff();
    TS_ASSERT_DELTA(func->HeightPrefactor(), 0.829433650, 1e-8);
  }

private:
  class TestableElasticIsoRotDiff : public ElasticIsoRotDiff {
  public:
    double HeightPrefactor() const override {
      return ElasticIsoRotDiff::HeightPrefactor();
    }
  };

  boost::shared_ptr<TestableElasticIsoRotDiff> createTestElasticIsoRotDiff() {
    auto func = boost::make_shared<TestableElasticIsoRotDiff>();
    func->initialize();
    func->setParameter("Height", 0.88);
    func->setParameter("Radius", 1.06); // 1Angstrom
    func->setParameter("Centre", 0.0004);
    func->setAttributeValue("Q", 0.7); // 1Angstrom^{-1}
    return func;
  }
};

#endif /*ELASTICISOROTDIFFTEST_H_*/
