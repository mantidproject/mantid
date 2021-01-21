// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
#include <memory>
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
    void function(const FunctionDomain &domain, FunctionValues &values) const override {
      IsoRotDiff::function(domain, values);
    }
  };

  std::shared_ptr<TestableIsoRotDiff> createTestIsoRotDiff() {
    auto func = std::make_shared<TestableIsoRotDiff>();
    func->initialize();
    func->setParameter("Height", 0.88);
    func->setParameter("Radius", 1.06); // Angstrom
    func->setParameter("Tau", 2.03);    // picosecond
    func->setParameter("Centre", 0.0004);
    return func;
  }
};