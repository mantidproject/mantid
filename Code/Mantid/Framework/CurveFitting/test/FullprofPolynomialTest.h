#ifndef MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_
#define MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FullprofPolynomial.h"

using Mantid::CurveFitting::FullprofPolynomial;

class FullprofPolynomialTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FullprofPolynomialTest *createSuite() { return new FullprofPolynomialTest(); }
  static void destroySuite( FullprofPolynomialTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_ */