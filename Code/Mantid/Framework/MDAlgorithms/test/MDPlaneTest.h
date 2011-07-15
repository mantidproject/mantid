#ifndef MANTID_MDALGORITHMS_MDPLANETEST_H_
#define MANTID_MDALGORITHMS_MDPLANETEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/MDPlane.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
//using namespace Mantid::API;

class MDPlaneTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    std::vector<coord_t> coeff;
    TSM_ASSERT_THROWS_ANYTHING( "O-dimensions are not allowed.", MDPlane test(coeff, 2.5) );
    coeff.push_back(1.234);
    coeff.push_back(4.56);
    MDPlane p(coeff, 2.5);
    TS_ASSERT_EQUALS( p.getNumDims(), 2);
    TS_ASSERT_DELTA( p.getCoeff()[0], 1.234, 1e-5);
    TS_ASSERT_DELTA( p.getCoeff()[1], 4.56,  1e-5);
  }

  void test_constructor2()
  {
    coord_t coeff[2] = {1.234, 4.56};
    TSM_ASSERT_THROWS_ANYTHING( "O-dimensions are not allowed.", MDPlane test(0, coeff, 2.5) );
    MDPlane p(2, coeff, 2.5);
    TS_ASSERT_EQUALS( p.getNumDims(), 2);
    TS_ASSERT_DELTA( p.getCoeff()[0], 1.234, 1e-5);
    TS_ASSERT_DELTA( p.getCoeff()[1], 4.56,  1e-5);
  }


};


#endif /* MANTID_MDALGORITHMS_MDPLANETEST_H_ */

