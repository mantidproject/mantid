#ifndef MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_
#define MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/MDImplicitFunction.h"
#include "MantidMDAlgorithms/MDPlane.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid;

class MDImplicitFunctionTest : public CxxTest::TestSuite
{
public:

  void test_addPlane()
  {
    MDImplicitFunction f;

    coord_t normal[3] = {1.234, 4.56, 6.78};
    coord_t point[3] = {1,2,3};
    MDPlane p1(3, normal, point);
    MDPlane p2(2, normal, point);
    MDPlane p3(3, normal, point);

    TS_ASSERT_EQUALS( f.getNumDims(), 0);
    TS_ASSERT_THROWS_NOTHING(f.addPlane(p1) );
    TS_ASSERT_EQUALS( f.getNumDims(), 3);
    TS_ASSERT_THROWS_ANYTHING( f.addPlane(p2) );
    TS_ASSERT_THROWS_NOTHING(f.addPlane(p3) );
    TS_ASSERT_EQUALS( f.getNumDims(), 3);
  }


  /// Helper function for the 2D case
  bool try2Dpoint(MDImplicitFunction & f, coord_t x, coord_t y)
  {
    coord_t centers[2] = {x,y};
    return f.isPointContained(centers);
  }


  void test_isPointContained()
  {
    MDImplicitFunction f;
    coord_t origin[2] = {0,0};

    // Everything below a 45 degree line
    coord_t normal1[2] = {1,-1};
    f.addPlane( MDPlane(2, normal1, origin) );

    // These points will be blocked by adding the second plane
    TS_ASSERT(  try2Dpoint(f, -1, -2) );
    TS_ASSERT(  try2Dpoint(f, 0.2, -0.1) );

    // Everything above y=0
    coord_t normal2[2] = {0,1};
    f.addPlane( MDPlane(2, normal2, origin) );

    // Are both planes doing the checking?
    TS_ASSERT(  try2Dpoint(f, 0.2, 0.1) );
    TS_ASSERT( !try2Dpoint(f, 0.2, -0.1) );
    TS_ASSERT( !try2Dpoint(f, 0.2, 0.3) );
    TS_ASSERT(  try2Dpoint(f, 2000, 1999) );
    TS_ASSERT( !try2Dpoint(f, -1, -2) );
  }

  void test_everythingIsContained_ifNoPlanes()
  {
    MDImplicitFunction f;
    TS_ASSERT(  try2Dpoint(f, -1, -2) );
    TS_ASSERT(  try2Dpoint(f, 0.2, -0.1) );
    TS_ASSERT(  try2Dpoint(f, 12, 33) );
  }

};


#endif /* MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_ */

