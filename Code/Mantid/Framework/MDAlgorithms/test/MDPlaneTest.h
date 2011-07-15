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

  /// Helper function for the 2D case
  bool try2Dpoint(MDPlane & p, coord_t x, coord_t y)
  {
    coord_t centers[2] = {x,y};
    return p.isPointBounded(centers);
  }

  /// 2D test with some simple linear inequations
  void test_2D_line()
  {
    coord_t coeff1[2] = {1., 0};
    // Plane where x < 5
    MDPlane p1(2, coeff1, 5.0);
    TS_ASSERT(  try2Dpoint(p1, 4.0, 12.) );
    TS_ASSERT( !try2Dpoint(p1, 6.0, -5.) );
    TS_ASSERT( !try2Dpoint(p1, 5.0, 1. ) );

    // Plane where x > 5
    coord_t coeff2[2] = {-1., 0};
    MDPlane p2(2, coeff2, -5.0);
    TS_ASSERT( !try2Dpoint(p2, 4.0, 12.) );
    TS_ASSERT(  try2Dpoint(p2, 6.0, -5.) );
    TS_ASSERT( !try2Dpoint(p2, 5.0, 1. ) );

    // Plane where y < 10
    coord_t coeff3[2] = {0., 1.};
    MDPlane p3(2, coeff3, 10.0);
    TS_ASSERT(  try2Dpoint(p3, 100.,  9.0) );
    TS_ASSERT( !try2Dpoint(p3, -99., 11.0) );

    // Plane where y-x < 0 (below a 45 degree line)
    coord_t coeff4[2] = {-1., 1.};
    MDPlane p4(2, coeff4, 0.0);
    TS_ASSERT(  try2Dpoint(p4, 1., 0.9) );
    TS_ASSERT(  try2Dpoint(p4, 1., -5.) );
    TS_ASSERT( !try2Dpoint(p4, 1., 1.1) );
    TS_ASSERT( !try2Dpoint(p4, 0., 0.1) );
  }

};


#endif /* MANTID_MDALGORITHMS_MDPLANETEST_H_ */

