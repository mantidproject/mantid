#ifndef CONVEXPOLYGONTEST_H_
#define CONVEXPOLYGONTEST_H_

#include "MantidGeometry/Math/ConvexPolygon.h"
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <cfloat>

using Mantid::Geometry::ConvexPolygon;
using Mantid::Geometry::Vertex2D;
using Mantid::Geometry::Vertex2DList;

class ConvexPolygonTest : public CxxTest::TestSuite
{
public:

  void test_Construction_With_A_Minimal_Valid_Set_Does_Not_Throw()
  {
    TS_ASSERT_THROWS_NOTHING(makeEquilateralTriangle());
  }

  void test_Construction_With_An_Too_Small_A_Set_Throws_Invalid_Arg()
  {
    Vertex2DList vertices;
    doExceptionCheck<std::invalid_argument>(vertices);
    // Single vertex
    vertices.insert(vertices.end(),Vertex2D());
    doExceptionCheck<std::invalid_argument>(vertices);
    // Line
    vertices.insert(vertices.end(),Vertex2D(1.,1.));
    doExceptionCheck<std::invalid_argument>(vertices);
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

private:
  /// Side length 2
  ConvexPolygon makeEquilateralTriangle()
  {
    Vertex2DList vertices;
    vertices.insert(vertices.end(),Vertex2D());
    vertices.insert(vertices.end(),Vertex2D(2.0,0.0));
    vertices.insert(vertices.end(),Vertex2D(1.0,std::sqrt(3.0)));
    return ConvexPolygon(vertices);
  }

  /// Short side 1, long side 2
  ConvexPolygon makeRectangle()
  {
    Vertex2DList vertices;
    vertices.insert(vertices.end(),Vertex2D());
    vertices.insert(vertices.end(),Vertex2D(2.0, 0.0));
    vertices.insert(vertices.end(),Vertex2D(2.0, 1.0));
    vertices.insert(vertices.end(),Vertex2D(0.0, 1.0));
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
    }
    catch(...)
    {
      TS_FAIL("Unexpected exception type thrown");
    }
  }
  
  

};


#endif //CONVEXPOLYGONTEST_H_