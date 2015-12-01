#ifndef MANTID_CURVEFITTING_CHEBFUNTEST_H_
#define MANTID_CURVEFITTING_CHEBFUNTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Chebfun.h"
#include <iostream>

using Mantid::CurveFitting::Functions::Chebfun;

class ChebfunTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChebfunTest *createSuite() { return new ChebfunTest(); }
  static void destroySuite( ChebfunTest *suite ) { delete suite; }


  void test_quadratic()
  {
    auto fun = [](double x) { return x*x; };
    Chebfun cheb(fun, -1, 1);
    TS_ASSERT_EQUALS(cheb.numberOfParts(), 1);
    TS_ASSERT_EQUALS(cheb.size(), 3);
    TS_ASSERT_EQUALS(cheb.accuracy(), 1e-15);
    TS_ASSERT(cheb.isGood());
    for (double x = -1; x <= 1; x += 0.1) {
      auto y1 = fun(x);
      auto y2 = cheb(x);
      TS_ASSERT_DELTA(y1, y2, 1e-15);
    }
  }

  void test_Poisson()
  {
    auto fun = [](double x) { return 2 * (x - 1.0 + log(1.0) - log(x)); };
    Chebfun cheb(fun, 0.0001, 3, 1e-15);
    //std::cerr << "\n\nSize " << cheb.numberOfParts() << ' ' << cheb.size() << ' ' << cheb.accuracy() << ' ' << cheb.isGood() << std::endl;
    TS_ASSERT_EQUALS(cheb.numberOfParts(), 8);
    TS_ASSERT_EQUALS(cheb.size(), 1844);
    TS_ASSERT_EQUALS(cheb.accuracy(), 1e-15);
    TS_ASSERT(cheb.isGood());
    for (double x = cheb.startX(); x <= cheb.endX(); x += 0.1) {
      auto y1 = fun(x);
      auto y2 = cheb(x);
      TS_ASSERT_DELTA(y1, y2, 4e-14);
    }
    auto y1 = fun(cheb.endX());
    auto y2 = cheb(cheb.endX());
    TS_ASSERT_DELTA(y1, y2, 4e-14);
  }


};


#endif /* MANTID_CURVEFITTING_CHEBFUNTEST_H_ */