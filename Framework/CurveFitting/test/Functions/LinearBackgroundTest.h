// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_LINEARBACKGROUNDTEST_H_
#define MANTID_CURVEFITTING_LINEARBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/LinearBackground.h"

#include <array>
#include <numeric>

using namespace Mantid::API;
using Mantid::CurveFitting::Functions::LinearBackground;

class LinearBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LinearBackgroundTest *createSuite() {
    return new LinearBackgroundTest();
  }
  static void destroySuite(LinearBackgroundTest *suite) { delete suite; }

  void test_category() {
    LinearBackground lin;
    lin.initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = lin.categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enfonce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void test_calculate() {

    LinearBackground lin;
    lin.initialize();
    // set up fitting function
    TS_ASSERT_THROWS(lin.setParameter("X", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(lin.setParameter("A9", 1.0), const std::invalid_argument &);

    const double a1 = 2;
    const double a0 = 0.3;
    lin.setParameter("A1", a1);
    lin.setParameter("A0", a0);

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    lin.function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], a0 + a1 * static_cast<double>(i), 1e-12);
    }
  }
};

#endif /*MANTID_CURVEFITTING_LINEARBACKGROUNDTEST_H_*/
