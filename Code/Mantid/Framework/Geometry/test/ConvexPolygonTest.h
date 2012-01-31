#ifndef CONVEXPOLYGONTEST_H_
#define CONVEXPOLYGONTEST_H_

#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/Vertex2D.h"
#include "MantidKernel/Exception.h"
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <cfloat>

using Mantid::Kernel::V2D;
using Mantid::Geometry::ConvexPolygon;
using Mantid::Geometry::Vertex2D;
using Mantid::Geometry::PolygonEdge;

class ConvexPolygonTest : public CxxTest::TestSuite
{
public:

  void test_Building_With_A_Minimal_Valid_Set_Does_Not_Throw()
  {
    TS_ASSERT_THROWS_NOTHING(makeEquilateralTriangle());
  }

  void test_Building_With_An_Isolated_Vertex_Throws_Invalid_Arg()
  {
    Vertex2D *vertex = new Vertex2D();
    doExceptionCheck<std::invalid_argument>(vertex);
    delete vertex;
  }

   void test_Building_With_A_Line_Throws_Invalid_Arg()
  {
    Vertex2D *vertex = new Vertex2D();
    Vertex2D *second = vertex->insert(new Vertex2D(1.0,1.0));
    doExceptionCheck<std::invalid_argument>(vertex);
    delete vertex;
    delete second;
  }

  void test_Building_With_Head_Vertex_With_Two_Other_Points_Doesnt_Not_Throw()
  {
    Vertex2D *origin = new Vertex2D;
    origin = origin->insert(new Vertex2D(1.0,1.0));
    origin = origin->insert(new Vertex2D(0.0,1.0));

    TS_ASSERT_THROWS_NOTHING(new ConvexPolygon(origin));
  }

  void test_Building_With_Head_Vertex_Gives_Correct_Number_Of_Vertices()
  {
    Vertex2D *origin = new Vertex2D;
    origin = origin->insert(new Vertex2D(1.0,1.0));
    origin = origin->insert(new Vertex2D(0.0,1.0));
    ConvexPolygon poly(origin);
    TS_ASSERT_EQUALS(poly.numVertices(), 3);
  }

  void test_Buildling_With_Head_Vertex_With_Less_Than_Two_Other_Points_Throws()
  {
    Vertex2D *origin = new Vertex2D;
    TS_ASSERT_THROWS(new ConvexPolygon(origin), std::invalid_argument);
    Vertex2D *second = new Vertex2D(1.0,1.0);
    origin->insert(second);
    TS_ASSERT_THROWS(new ConvexPolygon(origin), std::invalid_argument);
    delete second;
    delete origin;
  }

  void test_Copying_Preserves_Polygon()
  {
    // Returns by value but with some optimizations may not copy the object
    ConvexPolygon rect = makeRectangle();
    TS_ASSERT_EQUALS(rect.numVertices(), 4);
    TS_ASSERT_EQUALS(rect[0], V2D());
    TS_ASSERT_EQUALS(rect[1], V2D(0.0, 1.0));
    TS_ASSERT_EQUALS(rect[2], V2D(2.0, 1.0));
    TS_ASSERT_EQUALS(rect[3], V2D(2.0, 0.0));

    ConvexPolygon copy(rect);
    TS_ASSERT_EQUALS(copy[0], V2D());
    TS_ASSERT_EQUALS(copy[1], V2D(0.0, 1.0));
    TS_ASSERT_EQUALS(copy[2], V2D(2.0, 1.0));
    TS_ASSERT_EQUALS(copy[3], V2D(2.0, 0.0));
  }

  void test_Head_Returns_Correct_Vertex()
  {
    ConvexPolygon poly = makeRectangle();
    TS_ASSERT_EQUALS(poly.head()->point(), V2D(0.0, 0.0));
  }

  void test_Index_Access_Returns_Correct_Object_For_Valid_Index()
  {
    ConvexPolygon triangle = makeEquilateralTriangle();
    const V2D & apex = triangle[1];
    TS_ASSERT_EQUALS(apex, V2D(1.0, std::sqrt(3.0)));
  }
  
  void test_Invalid_Index_Access_Throws()
  {
    using Mantid::Kernel::Exception::IndexError;
    ConvexPolygon triangle = makeEquilateralTriangle();
    TS_ASSERT_THROWS(triangle[3], IndexError);
    TS_ASSERT_THROWS(triangle[-1], IndexError);
  }

  void test_Point_Inside_Polygon_Returns_True()
  {
    ConvexPolygon poly = makeRectangle();
    TS_ASSERT(poly.contains(V2D(1.0, 0.25)));
    TS_ASSERT(poly.contains(V2D(1.0, 0.0)));
    TS_ASSERT(poly.contains(poly.head()->point()));
  }

  void test_The_Determinant_For_A_Triangle()
  {
    ConvexPolygon triangle = makeEquilateralTriangle();
    TS_ASSERT_DELTA(triangle.determinant(), 2.0*std::sqrt(3.0), DBL_EPSILON);
  }

  void test_Area_Of_A_Triangle()
  {
    // Equilateral triangle of side length 2. Area = sqrt(3)
    ConvexPolygon triangle = makeEquilateralTriangle();
    TS_ASSERT_DELTA(triangle.area(), std::sqrt(3.0), DBL_EPSILON);
  }

  void test_Area_Of_A_Square()
  {
    ConvexPolygon rectangle = makeRectangle();
    TS_ASSERT_DELTA(rectangle.area(), 2.0, DBL_EPSILON);
  }
  
  void test_Area_Of_A_Parallelogram()
  {
    ConvexPolygon parallelogram = makeParallelogram();
    TS_ASSERT_DELTA(parallelogram.area(), std::sqrt(2.0), DBL_EPSILON);
  }
  
  void test_Extreme_Points_Are_Correct()
  {
    ConvexPolygon parallelogram = makeParallelogram();
    TS_ASSERT_DELTA(parallelogram.smallestX(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(parallelogram.largestX(), 2.0 + 0.5*std::sqrt(2.0), DBL_EPSILON);
    TS_ASSERT_DELTA(parallelogram.smallestY(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(parallelogram.largestY(), 0.5*std::sqrt(2.0), DBL_EPSILON);
  }

  void test_Polygon_Contains_Polygon()
  {
    Vertex2D *head = new Vertex2D(0.0,0.1);
    head->insert(new Vertex2D(2.0,0.1));
    head->insert(new Vertex2D(1.0,0.1 + std::sqrt(3.0)));
    ConvexPolygon smallTriangle(head);

    head = new Vertex2D(-1.0,0.0);
    head->insert(new Vertex2D(3.0,0.0));
    head->insert(new Vertex2D(2.0,2.0*std::sqrt(3.0)));
    ConvexPolygon largeTriangle(head);

    TS_ASSERT_EQUALS(largeTriangle.contains(smallTriangle), true);
    TS_ASSERT_EQUALS(smallTriangle.contains(largeTriangle), false);
  }

private:
  /// Side length 2
  ConvexPolygon makeEquilateralTriangle()
  {
    Vertex2D *head = new Vertex2D();
    head->insert(new Vertex2D(2.0,0.0));
    head->insert(new Vertex2D(1.0,std::sqrt(3.0)));
    return ConvexPolygon(head);
  }

  /// Short side 1, long side 2
  ConvexPolygon makeRectangle()
  {
    return ConvexPolygon(0.0, 2.0, 0.0, 1.0);
  }

  /// Short side 2-1-2-1
  ConvexPolygon makeParallelogram()
  {
    Vertex2D *head = new Vertex2D();
    head->insert(new Vertex2D(2.0,0.0));
    head->insert(new Vertex2D(2.0 + 0.5*std::sqrt(2.0), 0.5*std::sqrt(2.0)));
    head->insert(new Vertex2D(0.5*std::sqrt(2.0), 0.5*std::sqrt(2.0)));
    return ConvexPolygon(head);
  }

  // If a class has no accessible default constructor we cannot use
  // TS_ASSERT_THROWS()
  template <typename ExceptionType>
  void doExceptionCheck(Vertex2D * vertex)
  {
    try
    {
      ConvexPolygon p(vertex);
    }
    catch(ExceptionType &)
    {
      return;
    }
    catch(...)
    {
      TS_FAIL("Unexpected exception type thrown");
      return;
    }
    TS_FAIL("Expecting an exception but non was thrown.");
  }
};

//------------------------------------------------------------------------------
// Performance Test
//------------------------------------------------------------------------------
class ConvexPolygonTestPerformance : public CxxTest::TestSuite
{
public:
  
  void test_Area_Calls()
  {
    const size_t ntests(50000000);

    double totalArea(0.0);
    for( size_t i = 0; i < ntests; ++i)
    {
      ConvexPolygon test(0.0, 2.0, 0.0, 1.0);
      totalArea += test.area();
    }
    
  }

};


#endif //CONVEXPOLYGONTEST_H_
