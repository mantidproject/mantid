#ifndef MANTID_CURVEFITTING_AREAGAUSSIANTEST_H_
#define MANTID_CURVEFITTING_AREAGAUSSIANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/AreaGaussian.h"

using Mantid::CurveFitting::AreaGaussian;
using namespace Mantid::API;

class AreaGaussianTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AreaGaussianTest *createSuite() { return new AreaGaussianTest(); }
  static void destroySuite( AreaGaussianTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_CURVEFITTING_AREAGAUSSIANTEST_H_ */