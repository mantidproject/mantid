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
using Mantid::Geometry::Vertex2DList;
using Mantid::Geometry::PolygonEdge;

class ConvexPolygonTest : public CxxTest::TestSuite
{
public:

  void test_Building_With_A_Minimal_Valid_Set_Does_Not_Throw()
  {
    TS_ASSERT_THROWS_NOTHING(makeEquilateralTriangle());
  }

  void test_Building_With_An_Too_Small_A_Set_Throws_Invalid_Arg()
  {
    Vertex2DList vertices;
    doExceptionCheck<std::invalid_argument>(vertices);
    // Single vertex
    vertices.insert(V2D());
    doExceptionCheck<std::invalid_argument>(vertices);
    // Line
    vertices.insert(V2D(1.,1.));
    doExceptionCheck<std::invalid_argument>(vertices);
  }

  void test_Building_With_Non_Unique_Vertices_Only_Keeps_Unique()
  {
    Vertex2DList vertices;
    vertices.insert(V2D());
    vertices.insert(V2D(2.0,0.0));
    vertices.insert(V2D(1.0,std::sqrt(3.0)));
    vertices.insert(V2D(2.0,0.0));
    ConvexPolygon poly(vertices);
    TS_ASSERT_EQUALS(poly.numVertices(), 3);
    TS_ASSERT_EQUALS(poly[0], V2D());
    TS_ASSERT_EQUALS(poly[1], V2D(2.0,0.0));
    TS_ASSERT_EQUALS(poly[2], V2D(1.0,std::sqrt(3.0)));
  }

  void test_Building_With_Head_Vertex_With_Two_Other_Points_Doesnt_Not_Throw()
  {
    Vertex2D *origin = new Vertex2D;
    origin = origin->insert(new Vertex2D(1.0,1.0));
    origin = origin->insert(new Vertex2D(0.0,1.0));

    TS_ASSERT_THROWS_NOTHING(new ConvexPolygon(*origin));
  }

  void test_Building_With_Head_Vertex_Gives_Correct_Number_Of_Vertices()
  {
    Vertex2D *origin = new Vertex2D;
    origin = origin->insert(new Vertex2D(1.0,1.0));
    origin = origin->insert(new Vertex2D(0.0,1.0));
    ConvexPolygon poly(*origin);
    TS_ASSERT_EQUALS(poly.numVertices(), 3);
  }

  void test_Buildling_With_Head_Vertex_With_Less_Than_Two_Other_Points_Throws()
  {
    Vertex2D *origin = new Vertex2D;
    TS_ASSERT_THROWS(new ConvexPolygon(*origin), std::invalid_argument);
    Vertex2D *second = new Vertex2D(1.0,1.0);
    origin->insert(second);
    TS_ASSERT_THROWS(new ConvexPolygon(*origin), std::invalid_argument);
    delete second;
    delete origin;
  }

  void test_Copying_Preserves_Polygon()
  {
    // Returns by value but with some optimizations may not copy the object
    ConvexPolygon triangle = makeEquilateralTriangle();
    ConvexPolygon copy(triangle);

    TS_ASSERT_EQUALS(copy.numVertices(), 3);
    TS_ASSERT_EQUALS(copy.point(), V2D());
    copy.advance();
    TS_ASSERT_EQUALS(copy.point(), V2D(2.0, 0.0));
    copy.advance();
    TS_ASSERT_EQUALS(copy.point(), V2D(1.0,std::sqrt(3.0)));
  }

  void test_Point_Access_Returns_Current_Point()
  {
    ConvexPolygon poly = makeRectangle();
    TS_ASSERT_EQUALS(poly.point(), V2D());
    poly.advance();
    TS_ASSERT_EQUALS(poly.point(), V2D(2.0, 0.0));
    poly.advance();
    TS_ASSERT_EQUALS(poly.point(), V2D(2.0, 1.0));
    poly.advance();
    TS_ASSERT_EQUALS(poly.point(), V2D(0.0, 1.0));
  }

  void test_Edge_Gives_Back_Current_Edge()
  {
    ConvexPolygon poly = makeRectangle();
    PolygonEdge edge1 = poly.edge();
    TS_ASSERT_EQUALS(edge1.start(), V2D());
    TS_ASSERT_EQUALS(edge1.end(), V2D(2.0,0.0));
    poly.advance();
    PolygonEdge edge2 = poly.edge();
    TS_ASSERT_EQUALS(edge2.start(), V2D(2.0,0.0));
    TS_ASSERT_EQUALS(edge2.end(), V2D(2.0,1.0));

  }

  void test_Index_Access_Returns_Correct_Object_For_Valid_Index()
  {
    ConvexPolygon triangle = makeEquilateralTriangle();
    const V2D & apex = triangle[2];
    TS_ASSERT_EQUALS(apex, V2D(1.0, std::sqrt(3.0)));
  }
  
  void test_Invalid_Index_Access_Throws()
  {
    using Mantid::Kernel::Exception::IndexError;
    ConvexPolygon triangle = makeEquilateralTriangle();
    TS_ASSERT_THROWS(triangle[3], IndexError);
    TS_ASSERT_THROWS(triangle[-1], IndexError);
  }

  void test_Advance_Increments_The_Current_Vertex_And_Returns_The_Next()
  {
    ConvexPolygon triangle = makeEquilateralTriangle();
    V2D nextPoint;
    TS_ASSERT_THROWS_NOTHING(nextPoint = triangle.advance());
    TS_ASSERT_EQUALS(nextPoint, V2D(2.0,0.0));
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

private:
  /// Side length 2
  ConvexPolygon makeEquilateralTriangle()
  {
    Vertex2DList vertices;
    vertices.insert(V2D());
    vertices.insert(V2D(2.0,0.0));
    vertices.insert(V2D(1.0,std::sqrt(3.0)));
    return ConvexPolygon(vertices);
  }

  /// Short side 1, long side 2
  ConvexPolygon makeRectangle()
  {
    return ConvexPolygon(0.0, 2.0, 0.0, 1.0);
  }

  /// Short side 2-1-2-1
  ConvexPolygon makeParallelogram()
  {
    Vertex2DList vertices;
    vertices.insert(V2D());
    vertices.insert(V2D(2.0, 0.0));
    vertices.insert(V2D(2.0 + 0.5*std::sqrt(2.0), 0.5*std::sqrt(2.0)));
    vertices.insert(V2D(0.5*std::sqrt(2.0), 0.5*std::sqrt(2.0)));
    return ConvexPolygon(vertices);
  }

  // If a class has no accessible default constructor we cannot use
  // TS_ASSERT_THROWS()
  template <typename ExceptionType>
  void doExceptionCheck(const Vertex2DList & vertices)
  {
    try
    {
      ConvexPolygon p(vertices);
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


#endif //CONVEXPOLYGONTEST_H_
