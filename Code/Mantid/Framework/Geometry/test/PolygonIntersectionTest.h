#ifndef MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_
#define MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/PolygonIntersection.h"
#include <cfloat>
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::V2D;
using Mantid::Geometry::Vertex2DList;
using Mantid::Geometry::ConvexPolygon;

class PolygonIntersectionTest : public CxxTest::TestSuite
{
public:

  void test_Stub()
  {
  }

  void xtest_Squares_With_Side_Length_Less_Than_One()
  {
    Vertex2DList vertices;
    vertices.insert(V2D());
    vertices.insert(V2D(1,0));
    vertices.insert(V2D(1,1));
    vertices.insert(V2D(0,1));
    ConvexPolygon squareOne(vertices);

    vertices = Vertex2DList();
    vertices.insert(V2D(0,0.1));
    vertices.insert(V2D(0.2,0.1));
    vertices.insert(V2D(0.2,0.2));
    vertices.insert(V2D(0,0.2));
    ConvexPolygon squareTwo(vertices);

    ConvexPolygon overlap = intersection(squareOne,squareTwo);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(0,0.2));
    TS_ASSERT_EQUALS(overlap[1], V2D(0,0.1));
    TS_ASSERT_EQUALS(overlap[2], V2D(0.2,0.1));
    TS_ASSERT_EQUALS(overlap[3], V2D(0.2,0.2));
    TS_ASSERT_DELTA(overlap.area(), squareTwo.area(), DBL_EPSILON);   
  }

  void xtest_Squares_With_One_Larger()
  {
    Vertex2DList vertices;
    vertices.insert(V2D(5.0, -0.5));
    vertices.insert(V2D(5.2, -0.5));
    vertices.insert(V2D(5.2, 0.5));
    vertices.insert(V2D(5.0, 0.5));
    ConvexPolygon squareOne(vertices);

    vertices = Vertex2DList();
    vertices.insert(V2D(5.0, -0.5));
    vertices.insert(V2D(5.1, -0.5));
    vertices.insert(V2D(5.1, 0.5));
    vertices.insert(V2D(5.0, 0.5));
    ConvexPolygon squareTwo(vertices);

    intersection(squareOne, squareTwo);

    ConvexPolygon overlap = intersection(squareOne, squareTwo);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);
    // Are they correct
    TS_ASSERT_DELTA(overlap.area(), squareTwo.area(), DBL_EPSILON); 
  }
};


//------------------------------------------------------------------------
// Performance Tests
//------------------------------------------------------------------------

class PolygonIntersectionTestPerformance : public CxxTest::TestSuite
{
public:
  void test_Intersection_Of_Large_Number()
  {
    const size_t niters(100000);
    for( size_t i = 0; i < niters; ++i )
    {
      // These are created each loop iteration to simulate a more real-life case
      // of constructing polygons inside a loop and then testing their intersection
      ConvexPolygon squareOne(0.0, 2.0, 0.0, 2.0); //2x2, bottom left-hand corner at origin
      ConvexPolygon squareTwo(1.0, 3.0, 1.0, 3.0); //2x2, bottom left-hand corner at centre of first
      try
      {
        intersection(squareOne, squareTwo);
      }
      catch(std::runtime_error&)
      {
      }
    }

  }


};

#endif //MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_

