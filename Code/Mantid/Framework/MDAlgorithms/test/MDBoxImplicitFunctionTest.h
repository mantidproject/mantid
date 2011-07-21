#ifndef MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_
#define MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_

#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/MDBoxImplicitFunction.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;

class MDBoxImplicitFunctionTest : public CxxTest::TestSuite
{
public:

  void test_constructor_throws()
  {
    std::vector<coord_t> min;
    std::vector<coord_t> max;
    TSM_ASSERT_THROWS_ANYTHING( "0 dimensions is bad.", MDBoxImplicitFunction f(min,max) );
    min.push_back(1.234);
    TSM_ASSERT_THROWS_ANYTHING( "Mismatch in nd", MDBoxImplicitFunction f(min,max) );
    max.push_back(4.56);
    TS_ASSERT_THROWS_NOTHING( MDBoxImplicitFunction f(min,max) );
  }

  /// Helper function for the 2D case
  bool try2Dpoint(MDImplicitFunction & f, coord_t x, coord_t y)
  {
    coord_t centers[2] = {x,y};
    return f.isPointContained(centers);
  }

  /** Make a box from 1,1 - 2,2 */
  void test_2D()
  {
    std::vector<coord_t> min;
    min.push_back(1.0);
    min.push_back(1.0);
    std::vector<coord_t> max;
    max.push_back(2.0);
    max.push_back(2.0);
    MDBoxImplicitFunction f(min,max);
    TS_ASSERT( try2Dpoint(f, 1.5, 1.5) );
    TS_ASSERT( !try2Dpoint(f, 0.9,1.5) );
    TS_ASSERT( !try2Dpoint(f, 2.1,1.5) );
    TS_ASSERT( !try2Dpoint(f, 1.5,0.9) );
    TS_ASSERT( !try2Dpoint(f, 1.5,2.1) );
  }

};


#endif /* MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_ */

