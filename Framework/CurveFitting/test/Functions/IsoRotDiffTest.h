#ifndef ISOROTDIFFTEST_H_
#define ISOROTDIFFTEST_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/IsoRotDiff.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include <cxxtest/TestSuite.h>
// third party library headers
#include <boost/make_shared.hpp>
// standard library headers (n/a)
#include <limits>

using Mantid::API::FunctionDomain;
using Mantid::API::FunctionDomain1DVector;
using Mantid::API::FunctionValues;
using Mantid::API::IFunction;
using Mantid::CurveFitting::Functions::IsoRotDiff;

class IsoRotDiffTest : public CxxTest::TestSuite {
public:
  void test_categories() {
    IsoRotDiff func;
    const std::vector<std::string> categories = func.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "QuasiElastic");
  }

  /**
   * @brief Parameters can be set and read
   */
  void test_parameters() {
    auto func = createTestIsoRotDiff();
    TS_ASSERT_EQUALS(func->getParameter("Height"), 0.88);
    TS_ASSERT_EQUALS(func->getParameter("Radius"), 1.06);
    TS_ASSERT_EQUALS(func->getParameter("Tau"), 2.03);
    TS_ASSERT_EQUALS(func->getParameter("Centre"), 0.0004);
    TS_ASSERT_EQUALS(func->getAttribute("Q").asDouble(), 0.3);
    TS_ASSERT_EQUALS(func->getAttribute("N").asInt(), 25);
  }

private:
  class TestableIsoRotDiff : public IsoRotDiff {
  public:
    void function(const FunctionDomain &domain,
                  FunctionValues &values) const override {
      IsoRotDiff::function(domain, values);
    }
  };

  boost::shared_ptr<TestableIsoRotDiff> createTestIsoRotDiff() {
    auto func = boost::make_shared<TestableIsoRotDiff>();
    func->initialize();
    func->setParameter("Height", 0.88);
    func->setParameter("Radius", 1.06); // Angstrom
    func->setParameter("Tau", 2.03);    // picosecond
    func->setParameter("Centre", 0.0004);
    return func;
  }
};
#endif /*ISOROTDIFFTEST_H_*/