#ifndef MANTID_CURVEFITTING_LINEARBACKGROUNDTEST_H_
#define MANTID_CURVEFITTING_LINEARBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/LinearBackground.h"

#include <boost/make_shared.hpp>
#include <array>
#include <numeric>

using namespace Mantid::API;
using Mantid::CurveFitting::Functions::LinearBackground;

typedef boost::shared_ptr<Mantid::CurveFitting::Functions::LinearBackground> Linear_sptr;

class LinearBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LinearBackgroundTest *createSuite() {
    return new LinearBackgroundTest();
  }
  static void destroySuite(LinearBackgroundTest *suite) { delete suite; }

  void test_category() {
    Linear_sptr lin = boost::make_shared<LinearBackground>();
    lin->initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = lin->categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enforce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void test_calculate_fit() {

    Linear_sptr lin = boost::make_shared<LinearBackground>();
    lin->initialize();
    // set up fitting function
    TS_ASSERT_THROWS(lin->setParameter("X", 1.0), std::invalid_argument);
    TS_ASSERT_THROWS(lin->setParameter("A9", 1.0), std::invalid_argument);

    const double a1 = 2;
    const double a0 = 0.3;
    lin->setParameter("A1", a1);
    lin->setParameter("A0", a0);

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    lin->function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], a0 + a1 * static_cast<double>(i), 1e-12);
    }
  }

  void test_calculate() {

    Linear_sptr lin = boost::make_shared<LinearBackground>();
    lin->initialize();
    // set up linear function
    TS_ASSERT_THROWS(lin->setAttributeValue("y", 1.0), std::invalid_argument);
    TS_ASSERT_THROWS(lin->setAttributeValue("y9", 1.0), std::invalid_argument);

    const double y0 = 0.0;
    const double y1 = 1.0;
    lin->setAttributeValue("y0", y0);
    lin->setAttributeValue("y1", y1);

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    std::array<double, numPoints> deriv;
    lin->function1D(yValues.data(), xValues.data(), numPoints);
    lin->derivative1D(deriv.data(), xValues.data(), numPoints, 1);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], yValues[i], 1e-12);
      TS_ASSERT_EQUALS(deriv[i], 1.0);
    }
  }
};

#endif /*MANTID_CURVEFITTING_LINEARBACKGROUNDTEST_H_*/
