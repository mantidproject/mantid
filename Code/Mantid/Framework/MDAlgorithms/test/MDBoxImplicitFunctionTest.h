#ifndef MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_
#define MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_

#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/MDBoxImplicitFunction.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::MDEvents::MDEvent;
using Mantid::MDEvents::MDBox;

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
    coord_t point[3] = {0.1,0.2,0.3};
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

  // Box that is fully contained in the implicit function
  void test_isBoxTouching_3D_allInside()
  {
    MDBox<MDEvent<3>,3> box;
    box.setExtents(0, 1.2, 1.8);
    box.setExtents(1, 2.2, 3.8);
    box.setExtents(2, 3.2, 3.8);
    MDBoxImplicitFunction f = get3DFunction();
    TS_ASSERT( f.isBoxTouching(&box) );
    bool res;
    for (size_t i=0; i<1000000; i++)
    {
      res = f.isBoxTouching(&box);
      (void) res;
    }
  }

  // Box that is completely outside of the implicit function
  void test_isBoxTouching_3D_allOutside()
  {
    MDBox<MDEvent<3>,3> box;
    box.setExtents(0, 3.2, 5.8);
    box.setExtents(1, -5.2, -3.8);
    box.setExtents(2, 12.2, 73.8);
    MDBoxImplicitFunction f = get3DFunction();
    TS_ASSERT( !f.isBoxTouching(&box) );
    bool res;
    for (size_t i=0; i<1000000; i++)
    {
      res = f.isBoxTouching(&box);
      (void) res;
    }
  }

  // Box that is fully contained in the implicit function
  void test_isBoxTouching_4D_allInside()
  {
    MDBox<MDEvent<4>,4> box;
    box.setExtents(0, 1.2, 1.8);
    box.setExtents(1, 2.2, 3.8);
    box.setExtents(2, 3.2, 3.8);
    box.setExtents(3, 4.2, 4.8);
    MDBoxImplicitFunction f = get4DFunction();
    TS_ASSERT( f.isBoxTouching(&box) );
    bool res;
    for (size_t i=0; i<1000000; i++)
    {
      res = f.isBoxTouching(&box);
      (void) res;
    }
  }

  // Box that is completely outside of the implicit function
  void test_isBoxTouching_4D_allOutside()
  {
    MDBox<MDEvent<4>,4> box;
    box.setExtents(0, 3.2, 5.8);
    box.setExtents(1, -5.2, -3.8);
    box.setExtents(2, 12.2, 73.8);
    box.setExtents(3, 18.2, 23.8);
    MDBoxImplicitFunction f = get4DFunction();
    TS_ASSERT( !f.isBoxTouching(&box) );
    bool res;
    for (size_t i=0; i<1000000; i++)
    {
      res = f.isBoxTouching(&box);
      (void) res;
    }
  }

};


#endif /* MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTIONTEST_H_ */

