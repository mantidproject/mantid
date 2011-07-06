#ifndef MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_
#define MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include <cfloat>
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::V2D;
using Mantid::Geometry::Vertex2DList;
using Mantid::Geometry::ConvexPolygon;

class PolygonIntersectionTest : public CxxTest::TestSuite
{
public:
  
  void test_Intersection_Of_Axis_Aligned_Squares()
  {
    // Define two squares that partially overlap
    ConvexPolygon squareOne(0.0, 2.0, 0.0, 2.0); //2x2, bottom left-hand corner at origin
    ConvexPolygon squareTwo(1.0, 3.0, 1.0, 3.0); //2x2, bottom left-hand corner at centre of first

    ConvexPolygon overlap = chasingEdgeIntersect(squareOne,squareTwo);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(2,1));
    TS_ASSERT_EQUALS(overlap[1], V2D(2,2));
    TS_ASSERT_EQUALS(overlap[2], V2D(1,2));
    TS_ASSERT_EQUALS(overlap[3], V2D(1,1));
  }

  void test_House()
  {
    Vertex2DList vertices;
    vertices.insert(V2D());
    vertices.insert(V2D(200,0));
    vertices.insert(V2D(200,100));
    vertices.insert(V2D(100,200));
    vertices.insert(V2D(0,100));
    ConvexPolygon house(vertices);

    vertices = Vertex2DList();
    vertices.insert(V2D(100,100));
    vertices.insert(V2D(300,100));
    vertices.insert(V2D(300,200));
    vertices.insert(V2D(100,200));
    ConvexPolygon rectangle(vertices);
    
    ConvexPolygon overlap = chasingEdgeIntersect(house,rectangle);
    TS_ASSERT_EQUALS(overlap.numVertices(), 3);
  }


  void test_Intersection_Of_Parallelogram_And_Square()
  {
    Vertex2DList vertices;
    vertices.insert(V2D(100,50));
    vertices.insert(V2D(175,50));
    vertices.insert(V2D(175,125));
    vertices.insert(V2D(100,125));
    ConvexPolygon square(vertices);

    vertices = Vertex2DList();
    vertices.insert(V2D());
    vertices.insert(V2D(200,0));
    vertices.insert(V2D(300,100));
    vertices.insert(V2D(100,100));
    ConvexPolygon parallelogram(vertices);

    ConvexPolygon overlap = chasingEdgeIntersect(square,parallelogram);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(100,100));
    TS_ASSERT_EQUALS(overlap[1], V2D(100,50));
    TS_ASSERT_EQUALS(overlap[2], V2D(175,50));
    TS_ASSERT_EQUALS(overlap[3], V2D(175,100));
  }

  void test_Squares_With_Side_Length_Less_Than_One()
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

    ConvexPolygon overlap = chasingEdgeIntersect(squareOne,squareTwo);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(0,0.2));
    TS_ASSERT_EQUALS(overlap[1], V2D(0,0.1));
    TS_ASSERT_EQUALS(overlap[2], V2D(0.2,0.1));
    TS_ASSERT_EQUALS(overlap[3], V2D(0.2,0.2));
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
        chasingEdgeIntersect(squareOne, squareTwo);
      }
      catch(std::runtime_error&)
      {
      }
    }

  }


};

#endif //MANTID_GEOMETRY_POLYGONINTERSECTIONTEST_H_

