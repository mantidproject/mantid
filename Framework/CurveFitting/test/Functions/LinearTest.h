#ifndef MANTID_CURVEFITTING_LINEARTEST_H_
#define MANTID_CURVEFITTING_LINEARTEST_H_


#include "MantidCurveFitting/Functions/Linear.h"
#include <boost/make_shared.hpp>
#include <array>

using namespace Mantid::API;
using Mantid::CurveFitting::Functions::Linear;

typedef boost::shared_ptr<Mantid::CurveFitting::Functions::Linear> Linear_sptr;

class LinearTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LinearTest *createSuite() {
    return new LinearTest();
  }
  static void destroySuite(LinearTest *suite) { delete suite; }

  void test_category() {
    Linear_sptr lin = boost::make_shared<Linear>();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = lin->categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    TS_ASSERT(lin->name() == "Linear");
  }

  void test_calculate() {

    Linear_sptr lin = boost::make_shared<Linear>();
    lin->initialize();
    // set up linear function
    TS_ASSERT_THROWS(lin->setParameter("X", 1.0), std::invalid_argument);
    TS_ASSERT_THROWS(lin->setParameter("Y9", 1.0), std::invalid_argument);

    const double y0 = 0.0;
    const double y1 = 1.0;
    lin->setParameter("y0", y0);
    lin->setParameter("y1", y1);

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    std::array<double, numPoints> deriv;
    lin->function1D(yValues.data(), xValues.data(), numPoints);
    lin->derivative1D(deriv.data(), xValues.data(), numPoints, 1) ;

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], yValues[i], 1e-12);
      TS_ASSERT_EQUALS(deriv[i], 1.0)
    }
  }
};

#endif /*MANTID_CURVEFITTING_LINEARTEST_H_*/
