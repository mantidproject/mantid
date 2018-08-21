#ifndef MANTID_GEOMETRY_QUADRILATERALTEST_H_
#define MANTID_GEOMETRY_QUADRILATERALTEST_H_

#include "MantidGeometry/Math/Quadrilateral.h"
#include <cfloat>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::V2D;

class QuadrilateralTest : public CxxTest::TestSuite {
public:
  void test_Rectangle_Constructor() {
    Quadrilateral rect(0.0, 2.0, 0.0, 1.5);
    TS_ASSERT_EQUALS(rect[0], V2D());
    TS_ASSERT_EQUALS(rect[1], V2D(0.0, 1.5));
    TS_ASSERT_EQUALS(rect[2], V2D(2.0, 1.5));
    TS_ASSERT_EQUALS(rect[3], V2D(2.0, 0.0));

    TS_ASSERT_DELTA(rect.minX(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(rect.maxX(), 2.0, DBL_EPSILON);
    TS_ASSERT_DELTA(rect.minY(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(rect.maxY(), 1.5, DBL_EPSILON);
  }

  void test_Area() {
    Quadrilateral rectangle = makeRectangle();
    TS_ASSERT_DELTA(rectangle.area(), 3.0, DBL_EPSILON);
  }

  void test_Copy() {
    Quadrilateral rectangle = makeRectangle();
    TS_ASSERT_EQUALS(rectangle[0], V2D());
    TS_ASSERT_EQUALS(rectangle[1], V2D(0.0, 1.5));
    TS_ASSERT_EQUALS(rectangle[2], V2D(2.0, 1.5));
    TS_ASSERT_EQUALS(rectangle[3], V2D(2.0, 0.0));

    // Force a copy
    Quadrilateral copied(rectangle);
    TS_ASSERT_EQUALS(copied[0], V2D());
    TS_ASSERT_EQUALS(copied[1], V2D(0.0, 1.5));
    TS_ASSERT_EQUALS(copied[2], V2D(2.0, 1.5));
    TS_ASSERT_EQUALS(copied[3], V2D(2.0, 0.0));

    TS_ASSERT_DELTA(copied.minX(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(copied.maxX(), 2.0, DBL_EPSILON);
    TS_ASSERT_DELTA(copied.minY(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(copied.maxY(), 1.5, DBL_EPSILON);
  }

  void test_Assignment() {
    Quadrilateral rectangle = makeRectangle();
    Quadrilateral assign(0.0, 1.0, 1.0, 0.0);
    assign = rectangle;
    TS_ASSERT_EQUALS(assign[0], V2D());
    TS_ASSERT_EQUALS(assign[1], V2D(0.0, 1.5));
    TS_ASSERT_EQUALS(assign[2], V2D(2.0, 1.5));
    TS_ASSERT_EQUALS(assign[3], V2D(2.0, 0.0));

    TS_ASSERT_DELTA(assign.minX(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(assign.maxX(), 2.0, DBL_EPSILON);
    TS_ASSERT_DELTA(assign.minY(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(assign.maxY(), 1.5, DBL_EPSILON);
  }

  void test_contains_single_point() {
    Quadrilateral rect = makeRectangle();

    TS_ASSERT(rect.contains(V2D(1.0, 0.25)));
    // On edge
    TS_ASSERT(rect.contains(V2D(1.0, 0.0)));
    // Outside
    TS_ASSERT(!rect.contains(V2D(-3.0, 1.5)));
    TS_ASSERT(!rect.contains(V2D(3.0, 1.5)));
    TS_ASSERT(!rect.contains(V2D(1.0, 2.0)));
    TS_ASSERT(!rect.contains(V2D(1.0, -2.0)));
  }

  void test_contains_polygon() {
    Quadrilateral smallRectangle = makeRectangle();
    Quadrilateral largeRectangle(V2D(), V2D(3.0, 0.0), V2D(3.0, 2.0),
                                 V2D(0.0, 2.0));

    TS_ASSERT(largeRectangle.contains(smallRectangle));
    TS_ASSERT(!smallRectangle.contains(largeRectangle));
  }

  void test_point_outside_quadrilateral() {
    // Quadrilateral with vertexes at:
    // Lower left: (5, 0)
    // Upper left: (0, 10)
    // Upper right: (10, 10)
    // Lower right: (10, 0)
    Quadrilateral quad(V2D(5, 0), V2D(10, 0), V2D(10, 10), V2D(0, 10));

    // Single point at (2,2)
    TS_ASSERT(!quad.contains(V2D(1, 1)));
    // Single point at (5, -1)
    TS_ASSERT(!quad.contains(V2D(5, -1)));
    // Single point at (11, 5)
    TS_ASSERT(!quad.contains(V2D(11, 5)));
    // Single point at (5, 11)
    TS_ASSERT(!quad.contains(V2D(5, 11)));
  }

  void test_polygon_outside_quadrilateral() {

    // Quadrilateral with vertexes at:
    // Lower left: (5, 0)
    // Upper left: (0, 10)
    // Upper right: (10, 10)
    // Lower right: (10, 0)
    Quadrilateral quad(V2D(5, 0), V2D(10, 0), V2D(10, 10), V2D(0, 10));

    // Polygon with vertexes at:
    // Lower left: (1, 1)
    // Lower right: (2, 1)
    // Upper right: (2, 2)
    // Upper left: (1, 2)
    Quadrilateral smallQuad(V2D(1, 1), V2D(2, 1), V2D(2, 2), V2D(1, 2));
    // Create a polygon from smallQuad so we
    // can test Quadrilateral::contains
    ConvexPolygon poly = smallQuad.toPoly();
    // poly should not be inside quad
    TS_ASSERT(!quad.contains(poly));
  }

  void test_clockwise_rotation() {
    Quadrilateral quad(V2D(0.0, 0.0), V2D(1.0, 3.0), V2D(4.0, 4.0),
                       V2D(4.0, 1.0));

    quad.shiftVertexesClockwise();

    TS_ASSERT_EQUALS(quad.at(0).X(), 4.0);
    TS_ASSERT_EQUALS(quad.at(0).Y(), 1.0);
    TS_ASSERT_EQUALS(quad.at(1).X(), 4.0);
    TS_ASSERT_EQUALS(quad.at(1).Y(), 4.0);
    TS_ASSERT_EQUALS(quad.at(2).X(), 1.0);
    TS_ASSERT_EQUALS(quad.at(2).Y(), 3.0);
    TS_ASSERT_EQUALS(quad.at(3).X(), 0.0);
    TS_ASSERT_EQUALS(quad.at(3).Y(), 0.0);
  }

private:
  Quadrilateral makeRectangle() {
    return Quadrilateral(V2D(), V2D(2.0, 0.0), V2D(2.0, 1.5), V2D(0.0, 1.5));
  }
};

//------------------------------------------------------------------------------
// Performance Test
//------------------------------------------------------------------------------
class QuadrilateralTestPerformance : public CxxTest::TestSuite {
public:
  void test_update_values() {
    const size_t ntests{50000000};
    for (size_t i = 0; i < ntests; ++i) {
      Quadrilateral test(V2D(), V2D(2.0, 0.0), V2D(2.0, 1.5), V2D(0.0, 1.5));
    }
  }

  void test_Area_Calls() {
    const size_t ntests{50000000};
    double totalArea{0.0};
    Quadrilateral test(V2D(), V2D(2.0, 0.0), V2D(2.0, 1.5), V2D(0.0, 1.5));
    for (size_t i = 0; i < ntests; ++i) {
      totalArea += test.area();
    }
  }
};

#endif /* MANTID_GEOMETRY_QUADRILATERALTEST_H_ */
