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
  void test_2D_point()
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


  /// Helper function for the 2D case of a line intersecting the plane
  bool try2Dline(MDPlane & p, coord_t x1, coord_t y1, coord_t x2, coord_t y2)
  {
    coord_t centers1[2] = {x1,y1};
    coord_t centers2[2] = {x2,y2};
    return p.doesLineIntersect(centers1, centers2);
  }

  void test_2D_line()
  {
    coord_t coeff1[2] = {1., 0};
    // Plane where x < 5
    MDPlane p1(2, coeff1, 5.0);
    TS_ASSERT(  try2Dline(p1,   1,2,     6,2) );
    TS_ASSERT(  try2Dline(p1, 10,12,   4.99,8) );
    TS_ASSERT( !try2Dline(p1, 5.01,2,  5.02,2) );
    TS_ASSERT( !try2Dline(p1, 4.99,2,  4.25,2) );

    // Plane where y-x < 0 (below a 45 degree line)
    coord_t coeff4[2] = {-1., 1.};
    MDPlane p4(2, coeff4, 0.0);
    TS_ASSERT(  try2Dline(p4,   0.1,0.0,   0.1,0.2));
    TS_ASSERT( !try2Dline(p4,   0.1,0.0,   0.3,0.2));
    TS_ASSERT(  try2Dline(p4,   0.1,0.2,   0.3,0.2));
  }

};



class MDPlaneTestPerformance : public CxxTest::TestSuite
{
public:

  void test_3D_point()
  {
    coord_t coeff[3] = {1.23, 2.34, 3.45};
    coord_t pointA[3] = {0.111, 0.222, 0.333};

    MDPlane p(3, coeff, 5.67);
    bool res = false;
    for (size_t i=0; i<5*1000000 /*5 million*/; i++)
    {
      res = p.isPointBounded(pointA);
      (void) res;
    }
    TS_ASSERT(res);
  }

  void test_4D_point()
  {
    coord_t coeff[4] = {1.23, 2.34, 3.45, 4.56};
    coord_t pointA[4] = {0.111, 0.222, 0.333, 0.444};

    MDPlane p(4, coeff, 6.78);
    bool res = false;
    for (size_t i=0; i<5*1000000 /*5 million*/; i++)
    {
      res = p.isPointBounded(pointA);
      (void) res;
    }
    TS_ASSERT(res);
  }


  void test_3D_line()
  {
    coord_t coeff[3] = {1.23, 2.34, 3.45};
    coord_t pointA[3] = {0.111, 0.222, 0.333};
    coord_t pointB[3] = {9.111, 9.222, 9.333};

    MDPlane p(3, coeff, 5.67);
    bool res = false;
    for (size_t i=0; i<5*1000000 /*5 million*/; i++)
    {
      res = p.doesLineIntersect(pointA, pointB);
      (void) res;
    }
    TS_ASSERT(res);
  }

};

#endif /* MANTID_MDALGORITHMS_MDPLANETEST_H_ */

