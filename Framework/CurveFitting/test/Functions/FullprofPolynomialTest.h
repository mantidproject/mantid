// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_
#define MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/FullprofPolynomial.h"

#include <array>

using namespace Mantid::API;
using Mantid::CurveFitting::Functions::FullprofPolynomial;

class FullprofPolynomialTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FullprofPolynomialTest *createSuite() {
    return new FullprofPolynomialTest();
  }
  static void destroySuite(FullprofPolynomialTest *suite) { delete suite; }

  void test_category() {
    FullprofPolynomial cfn;
    cfn.initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = cfn.categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enfonce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void test_negative() {
    FullprofPolynomial tofbkgd;
    tofbkgd.initialize();
    TS_ASSERT_THROWS(tofbkgd.setAttributeValue("n", -3),
                     const std::invalid_argument &);
  }

  void test_zero() {
    FullprofPolynomial tofbkgd;
    tofbkgd.initialize();
    TS_ASSERT_THROWS(tofbkgd.setAttributeValue("n", 0),
                     const std::runtime_error &);
  }

  void test_calculate() {
    FullprofPolynomial tofbkgd;
    tofbkgd.initialize();
    TS_ASSERT_THROWS_NOTHING(tofbkgd.setAttributeValue("n", 6));
    TS_ASSERT_THROWS_NOTHING(tofbkgd.setAttributeValue("Bkpos", 10000.));
    TS_ASSERT_THROWS_NOTHING(tofbkgd.setParameter("A0", 0.3));
    TS_ASSERT_THROWS_NOTHING(tofbkgd.setParameter("A1", 1.0));
    TS_ASSERT_THROWS_NOTHING(tofbkgd.setParameter("A2", -0.5));
    TS_ASSERT_THROWS_NOTHING(tofbkgd.setParameter("A3", 0.05));
    TS_ASSERT_THROWS_NOTHING(tofbkgd.setParameter("A4", -0.02));

    const int timeChannels = 1000;
    std::array<double, timeChannels> xvals;
    const double tof0 = 8000.;
    const double dtof = 5.;

    for (int i = 0; i < timeChannels; i++) {
      xvals[i] = static_cast<double>(i) * dtof + tof0;
    }

    std::array<double, timeChannels> yValues;
    tofbkgd.function1D(yValues.data(), xvals.data(), timeChannels);

    // Test result
    TS_ASSERT_DELTA(yValues[400], 0.3, 1.0E-10); // Y[10000] = B0
    TS_ASSERT_DELTA(yValues[0], 0.079568, 1.0E-5);
    TS_ASSERT_DELTA(yValues[605], 0.39730, 1.0E-5);
    TS_ASSERT_DELTA(yValues[999], 0.55583, 1.0E-5);
  }
};

#endif /* MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_ */
