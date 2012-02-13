#ifndef MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_
#define MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_

#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Geometry;

class MDBoxImplicitFunctionTest : public CxxTest::TestSuite
{
public:

  void test_constructor_throws()
  {
    std::vector<coord_t> min;
    std::vector<coord_t> max;
    TSM_ASSERT_THROWS_ANYTHING( "0 dimensions is bad.", MDBoxImplicitFunction f(min,max) );
    min.push_back(1.234f);
    TSM_ASSERT_THROWS_ANYTHING( "Mismatch in nd", MDBoxImplicitFunction f(min,max) );
    max.push_back(4.56f);
    TS_ASSERT_THROWS_NOTHING( MDBoxImplicitFunction f(min,max) );
  }

  /// Helper function for the 2D case
  bool try2Dpoint(MDImplicitFunction & f, double x, double y)
  {
    coord_t centers[2] = {coord_t(x),coord_t(y)};
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



class MDBoxImplicitFunctionTestPerformance : public CxxTest::TestSuite
{
public:
  MDBoxImplicitFunction get3DFunction()
  {
    std::vector<coord_t> min;
    min.push_back(1.0);
    min.push_back(2.0);
    min.push_back(3.0);
    std::vector<coord_t> max;
    max.push_back(2.0);
    max.push_back(3.0);
    max.push_back(4.0);
    return MDBoxImplicitFunction(min,max);
  }

  MDBoxImplicitFunction get4DFunction()
  {
    std::vector<coord_t> min;
    min.push_back(1.0);
    min.push_back(2.0);
    min.push_back(3.0);
    min.push_back(4.0);
    std::vector<coord_t> max;
    max.push_back(2.0);
    max.push_back(3.0);
    max.push_back(4.0);
    max.push_back(5.0);
    return MDBoxImplicitFunction(min,max);
  }



  void test_isPointBounded_3D()
  {
    coord_t point[3] = {1.5,2.5,3.5};
    MDBoxImplicitFunction f = get3DFunction();
    TS_ASSERT( f.isPointContained(point) );
    bool res;
    for (size_t i=0; i<1000000; i++)
    {
      res = f.isPointContained(point);
      (void) res;
    }
  }

  void test_isPointBounded_3D_pointIsOutside()
  {
    coord_t point[3] = {0.25,0.25,0.25};
    MDBoxImplicitFunction f = get3DFunction();
    TS_ASSERT(!f.isPointContained(point) );
    bool res;
    for (size_t i=0; i<1000000; i++)
    {
      res = f.isPointContained(point);
      (void) res;
    }
  }

  void test_isPointBounded_4D()
  {
    coord_t point[4] = {1.5,2.5,3.5,4.5};
    MDBoxImplicitFunction f = get4DFunction();
    TS_ASSERT( f.isPointContained(point) );
    bool res;
    for (size_t i=0; i<1000000; i++)
    {
      res = f.isPointContained(point);
      (void) res;
    }
  }

};


#endif /* MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_ */

