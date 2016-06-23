#ifndef MANTID_CURVEFITTING_POLYNOMIALTEST_H_
#define MANTID_CURVEFITTING_POLYNOMIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Polynomial.h"

#include <array>
#include <numeric>

using Mantid::CurveFitting::Functions::Polynomial;

class PolynomialTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolynomialTest *createSuite() { return new PolynomialTest(); }
  static void destroySuite(PolynomialTest *suite) { delete suite; }

  void test_category() {
    Polynomial cfn;
    cfn.initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = cfn.categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enforce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void test_parametersAttributes() {
    Polynomial pol;
    pol.initialize();

    TS_ASSERT_THROWS(pol.setParameter("X", 1.0), std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(pol.setAttributeValue("n", 3));
    TS_ASSERT_THROWS(pol.setParameter("A99", 0.0), std::invalid_argument);

    Polynomial pol2;
    pol2.initialize();
    TS_ASSERT_THROWS(pol2.setAttributeValue("n", -1), std::invalid_argument);

    Polynomial pol3;
    pol3.initialize();
    TS_ASSERT_THROWS_NOTHING(pol2.setAttributeValue("n", 0));
  }

  void test_Polynomial() {
    Polynomial pol;
    pol.initialize();

    const double a3 = 0.2;
    const double a1 = 1.3;
    const double a0 = 0.3;
    pol.setAttributeValue("n", 3);
    pol.setParameter("A3", a3);
    pol.setParameter("A1", a1);
    pol.setParameter("A0", a0);

    const std::size_t numPoints = 50;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);

    std::array<double, numPoints> yValues;
    pol.function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      auto i3 = static_cast<double>(i);
      i3 = i3 * i3 * i3;
      TS_ASSERT_DELTA(yValues[i], a0 + a1 * static_cast<double>(i) + a3 * i3,
                      1e-12);
    }
  }
};

#endif /* MANTID_CURVEFITTING_POLYNOMIALTEST_H_ */
