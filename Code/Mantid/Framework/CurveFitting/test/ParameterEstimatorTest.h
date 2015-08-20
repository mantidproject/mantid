#ifndef MANTID_CURVEFITTING_PARAMETERESTIMATORTEST_H_
#define MANTID_CURVEFITTING_PARAMETERESTIMATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ParameterEstimator.h"

using Mantid::CurveFitting::ParameterEstimator;
using namespace Mantid::API;

class ParameterEstimatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ParameterEstimatorTest *createSuite() { return new ParameterEstimatorTest(); }
  static void destroySuite( ParameterEstimatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_CURVEFITTING_PARAMETERESTIMATORTEST_H_ */