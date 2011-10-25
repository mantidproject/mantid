#ifndef MANTID_CURVEFITTING_QUADRATICBACKGROUNDTEST_H_
#define MANTID_CURVEFITTING_QUADRATICBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCurveFitting/QuadraticBackground.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class QuadraticBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QuadraticBackgroundTest *createSuite() { return new QuadraticBackgroundTest(); }
  static void destroySuite( QuadraticBackgroundTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_CURVEFITTING_QUADRATICBACKGROUNDTEST_H_ */

