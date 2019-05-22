// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef QUADRATICTEST_H_
#define QUADRATICTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Quadratic.h"

#include <array>
#include <numeric>

using Mantid::CurveFitting::Functions::Quadratic;

class QuadraticTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QuadraticTest *createSuite() { return new QuadraticTest(); }
  static void destroySuite(QuadraticTest *suite) { delete suite; }

  void test_category() {
    Quadratic cfn;
    cfn.initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = cfn.categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enfonce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void test_parameters() {
    Quadratic quad;
    quad.initialize();

    TS_ASSERT_THROWS(quad.setParameter("X", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(quad.setAttributeValue("n", 3), const std::invalid_argument &);
    TS_ASSERT_THROWS(quad.setParameter("A99", 0.0), const std::invalid_argument &);
  }

  void test_calculate() {
    Quadratic quad;
    quad.initialize();

    const double a2 = -0.2;
    const double a1 = 1.3;
    const double a0 = 34.5;
    quad.setParameter("A0", a0);
    quad.setParameter("A1", a1);
    quad.setParameter("A2", a2);

    const std::size_t numPoints = 50;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);

    std::array<double, numPoints> yValues;
    quad.function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      auto i2 = static_cast<double>(i);
      i2 *= i2;
      TS_ASSERT_DELTA(yValues[i], a0 + a1 * static_cast<double>(i) + a2 * i2,
                      1e-12);
    }
  }
};

#endif /*QUADRATICTEST_H_*/
