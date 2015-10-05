#ifndef CONVEXPOLYGONTEST_H_
#define CONVEXPOLYGONTEST_H_

#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/V2D.h"
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <cfloat>

using Mantid::Kernel::V2D;
using Mantid::Geometry::ConvexPolygon;

class ConvexPolygonTest : public CxxTest::TestSuite {
public:
  void test_Default_Constructor_Produces_Invalid_Polygon() {
    ConvexPolygon poly;
    TS_ASSERT(!poly.isValid());
  }

  void test_Clear_Produces_Invalid_Polygon() {
    ConvexPolygon rect = makeRectangle();
    TS_ASSERT(rect.isValid());
    rect.clear();
    TS_ASSERT(!rect.isValid());
  }

  void test_Copying_Preserves_Polygon() {
    ConvexPolygon rect = makeRectangle();
    ConvexPolygon copy(rect);
    TS_ASSERT_EQUALS(copy.npoints(), 4);
    TS_ASSERT_EQUALS(copy[0], V2D());
    TS_ASSERT_EQUALS(copy[1], V2D(0.0, 1.0));
    TS_ASSERT_EQUALS(copy[2], V2D(2.0, 1.0));
    TS_ASSERT_EQUALS(copy[3], V2D(2.0, 0.0));
  }

  void test_Valid_Index_Returns_Expected_Point() {
    ConvexPolygon rect = makeRectangle();
    TS_ASSERT_EQUALS(rect.npoints(), 4);
    TS_ASSERT_EQUALS(rect[0], V2D());
    TS_ASSERT_EQUALS(rect[2], V2D(2.0, 1.0));
    TS_ASSERT_EQUALS(rect[3], V2D(2.0, 0.0));

    TS_ASSERT_EQUALS(rect.at(0), V2D());
    TS_ASSERT_EQUALS(rect.at(2), V2D(2.0, 1.0));
    TS_ASSERT_EQUALS(rect.at(3), V2D(2.0, 0.0));
  }

  void test_Point_Inside_Polygon_Returns_True() {
    ConvexPolygon poly = makeRectangle();
    TS_ASSERT(poly.contains(V2D(1.0, 0.25)));
    TS_ASSERT(poly.contains(V2D(1.0, 0.0)));
  }

  void test_The_Determinant_For_A_Triangle() {
    ConvexPolygon triangle = makeEquilateralTriangle();
    TS_ASSERT_DELTA(triangle.determinant(), 2.0 * std::sqrt(3.0), DBL_EPSILON);
  }

  void test_Area_Of_A_Triangle() {
    // Equilateral triangle of side length 2. Area = sqrt(3)
    ConvexPolygon triangle = makeEquilateralTriangle();
    TS_ASSERT_DELTA(triangle.area(), std::sqrt(3.0), DBL_EPSILON);
  }

  void test_Area_Of_A_Square() {
    ConvexPolygon rectangle = makeRectangle();
    TS_ASSERT_DELTA(rectangle.area(), 2.0, DBL_EPSILON);
  }

  void test_Area_Of_A_Parallelogram() {
    ConvexPolygon parallelogram = makeParallelogram();
    TS_ASSERT_DELTA(parallelogram.area(), std::sqrt(2.0), DBL_EPSILON);
  }

  void test_Extreme_Points_Are_Correct() {
    ConvexPolygon parallelogram = makeParallelogram();
    TS_ASSERT_DELTA(parallelogram.minX(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(parallelogram.maxX(), 2.0 + 0.5 * std::sqrt(2.0),
                    DBL_EPSILON);
    TS_ASSERT_DELTA(parallelogram.minY(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(parallelogram.maxY(), 0.5 * std::sqrt(2.0), DBL_EPSILON);
  }

  void test_Polygon_Contains_Polygon() {
    // base 2
    ConvexPolygon smallTriangle;
    smallTriangle.insert(0.0, 0.0);
    smallTriangle.insert(1.0, std::sqrt(3.0));
    smallTriangle.insert(2.0, 0.0);

    // base 4
    ConvexPolygon largeTriangle;
    largeTriangle.insert(0.0, 0.0);
    largeTriangle.insert(2.0, 2.0 * std::sqrt(3.0));
    largeTriangle.insert(4.0, 0.0);

    TS_ASSERT_EQUALS(largeTriangle.contains(smallTriangle), true);
    TS_ASSERT_EQUALS(smallTriangle.contains(largeTriangle), false);
  }

  // ------------------------ Failure cases --------------------------------

  void test_Invalid_Index_Access_Throws_For_At() {
    using Mantid::Kernel::Exception::IndexError;
    ConvexPolygon triangle = makeEquilateralTriangle();
    TS_ASSERT_THROWS(triangle.at(3), IndexError);
    TS_ASSERT_THROWS(triangle.at(-1), IndexError);
  }

private:
  /// Side length 2
  ConvexPolygon makeEquilateralTriangle() {
    ConvexPolygon triangle;
    triangle.insert(0.0, 0.0);
    triangle.insert(1.0, std::sqrt(3.0));
    triangle.insert(2.0, 0.0);

    return triangle;
  }

  /// Short side 1, long side 2
  ConvexPolygon makeRectangle() {
    ConvexPolygon rectangle;
    rectangle.insert(0.0, 0.0);
    rectangle.insert(0.0, 1.0);
    rectangle.insert(2.0, 1.0);
    rectangle.insert(2.0, 0.0);

    return rectangle;
  }

  /// Short side 2-1-2-1
  ConvexPolygon makeParallelogram() {
    ConvexPolygon parallelogram;
    parallelogram.insert(0.0, 0.0);
    parallelogram.insert(0.5 * std::sqrt(2.0), 0.5 * std::sqrt(2.0));
    parallelogram.insert(2.0 + 0.5 * std::sqrt(2.0), 0.5 * std::sqrt(2.0));
    parallelogram.insert(2.0, 0.0);
    return parallelogram;
  }
};

//------------------------------------------------------------------------------
// Performance Test
//------------------------------------------------------------------------------
class ConvexPolygonTestPerformance : public CxxTest::TestSuite {
public:
  void test_Area_Calls() {
    const size_t ntests(50000000);

    double totalArea(0.0);
    for (size_t i = 0; i < ntests; ++i) {
      ConvexPolygon test;
      test.insert(0.0, 0.0);
      test.insert(0.0, 1.0);
      test.insert(2.0, 1.0);
      test.insert(2.0, 0.0);

      totalArea += test.area();
    }
  }
};

#endif // CONVEXPOLYGONTEST_H_
