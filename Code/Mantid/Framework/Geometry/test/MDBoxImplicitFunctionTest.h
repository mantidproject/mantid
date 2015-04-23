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

namespace {
typedef boost::tuple<Mantid::coord_t, Mantid::coord_t> Extent;
}

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
    coord_t centers[2] = {static_cast<coord_t>(x),static_cast<coord_t>(y)};
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


  void test_volume()
  {
    std::vector<coord_t> min, max;
    min.push_back(0);
    min.push_back(0);
    min.push_back(0);
    max.push_back(1);
    max.push_back(2);
    max.push_back(3);
    MDBoxImplicitFunction box(min, max);
    TS_ASSERT_EQUALS(1*2*3, box.volume());
  }

  void test_fraction_when_not_contained()
  {
      // The implicit function box
      const coord_t areaMin = 1.0f;
      const coord_t areaMax = 2.0f;
      std::vector<coord_t> min;
      min.push_back(areaMin);
      min.push_back(areaMin);
      std::vector<coord_t> max;
      max.push_back(areaMax);
      max.push_back(areaMax);
      MDBoxImplicitFunction f(min,max);

      // The box to test.
      const coord_t boxMin = 0.0f;
      const coord_t boxMax = 0.1f;
      std::vector<Extent> extents;
      // extent
      extents.push_back(Extent(boxMin, boxMax));
      // extent
      extents.push_back(Extent(boxMin, boxMax));

      TS_ASSERT_EQUALS(0.0, f.fraction(extents));
  }

  void test_fraction_when_fully_contained()
  {
      // The implicit function box
      const coord_t areaMin = 1.0f;
      const coord_t areaMax = 2.0f;
      std::vector<coord_t> min;
      min.push_back(areaMin);
      min.push_back(areaMin);
      std::vector<coord_t> max;
      max.push_back(areaMax);
      max.push_back(areaMax);
      MDBoxImplicitFunction f(min,max);

      // The box to test.
      const coord_t boxMin = 1.1f;
      const coord_t boxMax = 1.9f;
      std::vector<coord_t> boxVertexes;
      std::vector<Extent> extents;
      // extent
      extents.push_back(Extent(boxMin, boxMax));
      // extent
      extents.push_back(Extent(boxMin, boxMax));
      // extent
      extents.push_back(Extent(boxMin, boxMax));
      // extent
      extents.push_back(Extent(boxMin, boxMax));

      TS_ASSERT_EQUALS(1.0, f.fraction(extents));
  }

  void test_fraction_when_partially_contained_1D_simple()
  {
      // The implicit function box
      const coord_t areaMin = 0.9f;
      const coord_t areaMax = 2.0f;
      std::vector<coord_t> min;
      min.push_back(areaMin);
      std::vector<coord_t> max;
      max.push_back(areaMax);
      MDBoxImplicitFunction f(min,max);

      // The box to test.
      const coord_t boxMin = 0;
      const coord_t boxMax = 1;
      std::vector<Extent> extents;
      // extent
      extents.push_back(Extent(boxMin, boxMax));

      /*

                box to test
      (x = 0) *------------------* (x = 1)

                            implicit function 1D
                    (x = 0.9) *--------------------------* (x = 2)

      */

      TSM_ASSERT_DELTA("Overlap fraction is incorrectly calculated", 0.1, f.fraction(extents), 1e-4);
  }

  void test_fraction_when_partially_contained_1D_complex()
  {
      // The implicit function box
      const coord_t areaMin = 0.25;
      const coord_t areaMax = 0.75;
      std::vector<coord_t> min;
      min.push_back(areaMin);
      std::vector<coord_t> max;
      max.push_back(areaMax);
      MDBoxImplicitFunction f(min,max);

      // The box to test.
      const coord_t boxMin = 0;
      const coord_t boxMax = 1.0;
      std::vector<Extent> extents;
      // extent
      extents.push_back(Extent(boxMin, boxMax));

      /*

                                    box to test
      (x = 0) *------------------------------------------------------* (x = 1)

                                 implicit function 1D
                  (x = 0.25) *--------------------------* (x = 0.75)

      */

      TSM_ASSERT_DELTA("Overlap fraction is incorrectly calculated", 0.5, f.fraction(extents), 1e-4);
  }


  void test_fraction_when_partially_contained_2d_simple()
  {

      /*

        1/4 overlap

                ---------------
                |             |
                |             |
        ---------------       |
        |       |     |       |
        |       |     |       |
        |       ------|--------
        |             |
        |             |
        ---------------



      */

      // The implicit function box
      const coord_t areaMin = 0.5;
      const coord_t areaMax = 1.5;
      std::vector<coord_t> min;
      min.push_back(areaMin);
      min.push_back(areaMin);
      std::vector<coord_t> max;
      max.push_back(areaMax);
      max.push_back(areaMax);
      MDBoxImplicitFunction f(min,max);

      // The box to test.
      const coord_t boxMin = 0;
      const coord_t boxMax = 1;
      std::vector<Extent> extents;
      // extent
      extents.push_back(Extent(boxMin, boxMax));
      // extent
      extents.push_back(Extent(boxMin, boxMax));

      TSM_ASSERT_DELTA("2d overlap incorrectly calculated", 1.0/4, f.fraction(extents), 1e-3);
  }

  void test_fraction_when_partially_contained_2d_complex()
  {

      /*

        1/8 overlap

                ---------------
                |  function   |
                |             |
        ---------------       |
        |       |     |       |
        |       ------|--------
        |             |
        |   box       |
        |             |
        ---------------



      */

      // The implicit function box
      const coord_t areaMin = 0.5;
      const coord_t areaMax = 1.5;
      std::vector<coord_t> min;
      min.push_back(areaMin); // xmin at 0.5
      min.push_back(areaMin + (areaMin/2)); // ymin at 0.75
      std::vector<coord_t> max;
      max.push_back(areaMax);
      max.push_back(areaMax + (areaMin/2)); // ymax at 0.75
      MDBoxImplicitFunction f(min,max);

      // The box to test.
      const coord_t boxMin = 0;
      const coord_t boxMax = 1;
      std::vector<Extent> extents;
      // extent
      extents.push_back(Extent(boxMin, boxMax));
      // extent
      extents.push_back(Extent(boxMin, boxMax));

      TSM_ASSERT_DELTA("2d overlap incorrectly calculated", 1.0/8, f.fraction(extents), 1e-3);
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

