// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/Functions/IsoRotDiff.h"
#include <cxxtest/TestSuite.h>
#include <limits>
#include <memory>

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
    TS_ASSERT_EQUALS(func->getParameter("f1.Height"), 0.88);
    TS_ASSERT_EQUALS(func->getParameter("f1.Radius"), 1.06);
    TS_ASSERT_EQUALS(func->getParameter("f1.Tau"), 2.03);
    TS_ASSERT_EQUALS(func->getParameter("f1.Centre"), 0.0004);
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
    func->setParameter("f1.Height", 0.88);
    func->setParameter("f1.Radius", 1.06); // Angstrom
    func->setParameter("f1.Tau", 2.03);    // picosecond
    func->setParameter("f1.Centre", 0.0004);
    return func;
  }
};
