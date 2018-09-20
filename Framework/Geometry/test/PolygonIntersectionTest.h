#ifndef MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_
#define MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/Quadrilateral.h"

#include <cxxtest/TestSuite.h>

using Mantid::Geometry::ConvexPolygon;
using Mantid::Geometry::Quadrilateral;
using Mantid::Geometry::intersection;
using Mantid::Kernel::V2D;

class PolygonIntersectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolygonIntersectionTest *createSuite() {
    return new PolygonIntersectionTest();
  }
  static void destroySuite(PolygonIntersectionTest *suite) { delete suite; }

  void test_Intersection_Of_Axis_Aligned_Squares() {
    // Define two squares that partially overlap
    Quadrilateral squareOne(0.0, 2.0, 0.0,
                            2.0); // 2x2, bottom left-hand corner at origin
    Quadrilateral squareTwo(
        1.0, 3.0, 1.0, 3.0); // 2x2, bottom left-hand corner at centre of first

    ConvexPolygon overlap;
    TS_ASSERT(intersection(squareOne, squareTwo, overlap));
    TS_ASSERT(overlap.isValid());
    TS_ASSERT_EQUALS(overlap.npoints(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(1, 2));
    TS_ASSERT_EQUALS(overlap[1], V2D(2, 2));
    TS_ASSERT_EQUALS(overlap[2], V2D(2, 1));
    TS_ASSERT_EQUALS(overlap[3], V2D(1, 1));

    // Symmetry
    ConvexPolygon reverseOverlap;
    TS_ASSERT(intersection(squareTwo, squareOne, reverseOverlap));
    TS_ASSERT(reverseOverlap.isValid());
    TS_ASSERT_EQUALS(reverseOverlap.npoints(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(reverseOverlap[0], V2D(1, 2));
    TS_ASSERT_EQUALS(reverseOverlap[1], V2D(2, 2));
    TS_ASSERT_EQUALS(reverseOverlap[2], V2D(2, 1));
    TS_ASSERT_EQUALS(reverseOverlap[3], V2D(1, 1));
  }

  void test_House() {
    ConvexPolygon house;
    house.insert(0, 0);
    house.insert(0, 100);
    house.insert(100, 200);
    house.insert(200, 100);
    house.insert(200, 0);

    ConvexPolygon rectangle;
    rectangle.insert(100, 100);
    rectangle.insert(100, 200);
    rectangle.insert(300, 200);
    rectangle.insert(300, 100);

    ConvexPolygon overlap;
    TS_ASSERT(intersection(house, rectangle, overlap));

    TS_ASSERT(overlap.isValid());
    TS_ASSERT_EQUALS(overlap.npoints(), 3);
    TS_ASSERT_EQUALS(overlap[0], V2D(100, 200));
    TS_ASSERT_EQUALS(overlap[1], V2D(200, 100));
    TS_ASSERT_EQUALS(overlap[2], V2D(100, 100));

    ConvexPolygon overlapRev;
    TS_ASSERT(intersection(rectangle, house, overlapRev));
    TS_ASSERT(overlapRev.isValid());
    TS_ASSERT_EQUALS(overlapRev.npoints(), 3);
    TS_ASSERT_EQUALS(overlapRev[0], V2D(100, 200));
    TS_ASSERT_EQUALS(overlapRev[1], V2D(200, 100));
    TS_ASSERT_EQUALS(overlapRev[2], V2D(100, 100));
  }

  void test_Intersection_Of_Parallelogram_And_Square() {
    ConvexPolygon square;
    square.insert(100, 50);
    square.insert(100, 125);
    square.insert(175, 125);
    square.insert(175, 50);

    ConvexPolygon parallelogram;
    parallelogram.insert(0, 0);
    parallelogram.insert(100, 100);
    parallelogram.insert(300, 100);
    parallelogram.insert(200, 0);

    ConvexPolygon overlap;
    TS_ASSERT(intersection(square, parallelogram, overlap));

    TS_ASSERT(overlap.isValid());
    TS_ASSERT_EQUALS(overlap.npoints(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(100, 100));
    TS_ASSERT_EQUALS(overlap[1], V2D(175, 100));
    TS_ASSERT_EQUALS(overlap[2], V2D(175, 50));
    TS_ASSERT_EQUALS(overlap[3], V2D(100, 50));

    // Symmetry
    ConvexPolygon overlapRev;
    TS_ASSERT(intersection(parallelogram, square, overlapRev));

    TS_ASSERT(overlapRev.isValid());
    TS_ASSERT_EQUALS(overlapRev.npoints(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlapRev[0], V2D(100, 100));
    TS_ASSERT_EQUALS(overlapRev[1], V2D(175, 100));
    TS_ASSERT_EQUALS(overlapRev[2], V2D(175, 50));
    TS_ASSERT_EQUALS(overlapRev[3], V2D(100, 50));
  }

  void test_Intersection_With_Self() {
    Quadrilateral squareOne(0.0, 2.0, 0.0,
                            2.0); // 2x2, bottom left-hand corner at origin
    ConvexPolygon overlap;
    TS_ASSERT(intersection(squareOne, squareOne, overlap));

    TS_ASSERT(overlap.isValid());
    TS_ASSERT_EQUALS(overlap.npoints(), 4);
    TS_ASSERT_EQUALS(overlap[0], V2D(0.0, 2.0));
    TS_ASSERT_EQUALS(overlap[1], V2D(2.0, 2.0));
    TS_ASSERT_EQUALS(overlap[2], V2D(2.0, 0.0));
    TS_ASSERT_EQUALS(overlap[3], V2D());
  }

  void test_First_Shape_Engulfing_Second_Gives_Overlap_Of_Smaller() {
    Quadrilateral smallRectangle(7.0, 8.0, 0.5, 1.5);
    Quadrilateral largeRectange(6.8, 8.6, -0.5, 2.0);

    ConvexPolygon overlap;
    TS_ASSERT(intersection(smallRectangle, largeRectange, overlap));
    TS_ASSERT(overlap.isValid());
    TS_ASSERT_EQUALS(overlap.npoints(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], smallRectangle[0]);
    TS_ASSERT_EQUALS(overlap[1], smallRectangle[1]);
    TS_ASSERT_EQUALS(overlap[2], smallRectangle[2]);
    TS_ASSERT_EQUALS(overlap[3], smallRectangle[3]);
  }

  //---------------------------------------- Failure tests
  //--------------------------------

  void test_Shapes_Sharing_Return_No_Intersection() {
    ConvexPolygon parallelogram;
    parallelogram.insert(-3.0, -3.0);
    parallelogram.insert(-2.0, -1.0);
    parallelogram.insert(0.5, -1.0);
    parallelogram.insert(-0.5, -3.0);

    ConvexPolygon rect2;
    rect2.insert(1.0, -1.0);
    rect2.insert(-4.0, -1.0);
    rect2.insert(-4.0, 3.0);
    rect2.insert(1.0, 3.0);

    /**
     * The overlap here is a line-segment [-3,-1]->[0.5,-1] which is not
     * valid polygon
     */
    ConvexPolygon overlap;
    TS_ASSERT(!intersection(rect2, parallelogram, overlap));
    TS_ASSERT(!overlap.isValid());

    overlap = ConvexPolygon();
    TS_ASSERT(!intersection(parallelogram, rect2, overlap));
    TS_ASSERT(!overlap.isValid());
  }

  void test_No_Overlap_At_All_Returns_No_Intersection() {
    // Define two squares that do not overlap
    Quadrilateral squareOne(0.0, 2.0, 0.0,
                            2.0); // 2x2, bottom left-hand corner at origin
    Quadrilateral squareTwo(3.0, 5.0, 3.0,
                            5.0); // 2x2, bottom left-hand corner at (3,3)

    ConvexPolygon overlap;
    TS_ASSERT(!intersection(squareOne, squareTwo, overlap));
    TS_ASSERT(!overlap.isValid());

    overlap = ConvexPolygon();
    TS_ASSERT(!intersection(squareTwo, squareOne, overlap));
    TS_ASSERT(!overlap.isValid());
  }

  void test_Overlap_Small_Polygon() {

    V2D ll1(-1.06675e-06, 0.010364);
    V2D lr1(-9.335e-07, 0.010364);
    V2D ur1(-9.335e-07, 0.010376);
    V2D ul1(-1.06675e-06, 0.010376);
    Quadrilateral squareOne(ll1, lr1, ur1, ul1);

    V2D ll2(-1.4650627e-06, 0.010410755);
    V2D lr2(-7.4121856e-07, 0.01034716581);
    V2D ur2(-7.43843122e-07, 0.0103851998);
    V2D ul2(-1.47044797837e-06, 0.010449023);
    Quadrilateral squareTwo(ll2, lr2, ur2, ul2);

    ConvexPolygon overlap;
    TS_ASSERT(intersection(squareOne, squareTwo, overlap));
    TS_ASSERT(overlap.isValid());
  }
};

//------------------------------------------------------------------------
// Performance Tests
//------------------------------------------------------------------------

class PolygonIntersectionTestPerformance : public CxxTest::TestSuite {
public:
  void test_Intersection_Of_Large_Number() {
    const size_t niters(100000);
    for (size_t i = 0; i < niters; ++i) {
      // These are created each loop iteration to simulate a more real-life case
      // of constructing polygons inside a loop and then testing their
      // intersection
      Quadrilateral squareOne(0.0, 2.0, 0.0,
                              2.0); // 2x2, bottom left-hand corner at origin
      Quadrilateral squareTwo(
          1.0, 3.0, 1.0,
          3.0); // 2x2, bottom left-hand corner at centre of first
      ConvexPolygon overlap;
      intersection(squareOne, squareTwo, overlap);
    }
  }
};

#endif /* MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_ */
