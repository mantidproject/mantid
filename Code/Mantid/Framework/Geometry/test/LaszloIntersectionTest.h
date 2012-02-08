#ifndef MANTID_GEOMETRY_LASZLOINTERSECTIONTEST_H_
#define MANTID_GEOMETRY_LASZLOINTERSECTIONTEST_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/LaszloIntersection.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/Vertex2D.h"
#include <cfloat>
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::V2D;
using Mantid::Geometry::Vertex2D;
using Mantid::Geometry::ConvexPolygon;

class LaszloIntersectionTest : public CxxTest::TestSuite
{
public:
  
  void test_Intersection_Of_Axis_Aligned_Squares()
  {
    // Define two squares that partially overlap
    ConvexPolygon squareOne(0.0, 2.0, 0.0, 2.0); //2x2, bottom left-hand corner at origin
    ConvexPolygon squareTwo(1.0, 3.0, 1.0, 3.0); //2x2, bottom left-hand corner at centre of first

    ConvexPolygon overlap = intersectionByLaszlo(squareOne,squareTwo);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(1,2));
    TS_ASSERT_EQUALS(overlap[1], V2D(2,2));
    TS_ASSERT_EQUALS(overlap[2], V2D(2,1));
    TS_ASSERT_EQUALS(overlap[3], V2D(1,1));
    
    // Symmetry
    ConvexPolygon overlapRev = intersectionByLaszlo(squareTwo,squareOne);
    TS_ASSERT_EQUALS(overlapRev.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlapRev[0], V2D(1,2));
    TS_ASSERT_EQUALS(overlapRev[1], V2D(2,2));
    TS_ASSERT_EQUALS(overlapRev[2], V2D(2,1));
    TS_ASSERT_EQUALS(overlapRev[3], V2D(1,1));

  }

  void test_House()
  {
    Vertex2D *head = new Vertex2D();
    head->insert(new Vertex2D(200,0));
    head->insert(new Vertex2D(200,100));
    head->insert(new Vertex2D(100,200));
    head->insert(new Vertex2D(0,100));
    ConvexPolygon house(head);

    head = new Vertex2D(100,100);
    head->insert(new Vertex2D(300,100));
    head->insert(new Vertex2D(300,200));
    head->insert(new Vertex2D(100,200));
    ConvexPolygon rectangle(head);
    
    ConvexPolygon overlap = intersectionByLaszlo(house,rectangle);
    TS_ASSERT_EQUALS(overlap.numVertices(), 3);
    TS_ASSERT_EQUALS(overlap[0], V2D(100,200));
    TS_ASSERT_EQUALS(overlap[1], V2D(200,100));
    TS_ASSERT_EQUALS(overlap[2], V2D(100,100));

    ConvexPolygon overlapRev = intersectionByLaszlo(rectangle, house);
    TS_ASSERT_EQUALS(overlapRev.numVertices(), 3);
    TS_ASSERT_EQUALS(overlapRev[0], V2D(100,200));
    TS_ASSERT_EQUALS(overlapRev[1], V2D(200,100));
    TS_ASSERT_EQUALS(overlapRev[2], V2D(100,100));
  }

  void test_Intersection_Of_Parallelogram_And_Square()
  {
    Vertex2D *head = new Vertex2D(100,50);
    head->insert(new Vertex2D(175,50));
    head->insert(new Vertex2D(175,125));
    head->insert(new Vertex2D(100,125));
    ConvexPolygon square(head);

    head = new Vertex2D();
    head->insert(new Vertex2D(200,0));
    head->insert(new Vertex2D(300,100));
    head->insert(new Vertex2D(100,100));
    ConvexPolygon parallelogram(head);

    ConvexPolygon overlap = intersectionByLaszlo(square,parallelogram);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(100,100));
    TS_ASSERT_EQUALS(overlap[1], V2D(175,100));
    TS_ASSERT_EQUALS(overlap[2], V2D(175,50));
    TS_ASSERT_EQUALS(overlap[3], V2D(100,50));

    //Symmetry
    ConvexPolygon overlapRev = intersectionByLaszlo(parallelogram, square);
    TS_ASSERT_EQUALS(overlapRev.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlapRev[0], V2D(100,100));
    TS_ASSERT_EQUALS(overlapRev[1], V2D(175,100));
    TS_ASSERT_EQUALS(overlapRev[2], V2D(175,50));
    TS_ASSERT_EQUALS(overlapRev[3], V2D(100,50));
  }

  void test_Intersection_With_Self()
  {
    ConvexPolygon squareOne(0.0, 2.0, 0.0, 2.0); //2x2, bottom left-hand corner at origin
    ConvexPolygon overlap = intersectionByLaszlo(squareOne, squareOne);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);

    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], V2D(0.0, 2.0));
    TS_ASSERT_EQUALS(overlap[1], V2D(2.0, 2.0));
    TS_ASSERT_EQUALS(overlap[2], V2D(2.0, 0.0));
    TS_ASSERT_EQUALS(overlap[3], V2D());
  }

  void test_Shapes_Sharing_A_Line_Throws()
  {
    Vertex2D *plist = new Vertex2D(-3.0, -3.0);
    plist->insert(new Vertex2D(-0.5, -3.0));
    plist->insert(new Vertex2D(0.5, -1.0));
    plist->insert(new Vertex2D(-2.0, -1.0));
    ConvexPolygon parallelogram(plist);

    Vertex2D *s2list = new Vertex2D(1.0, -1.0);
    s2list->insert(new Vertex2D(1.0, 3.0));
    s2list->insert(new Vertex2D(-4.0, 3.0));
    s2list->insert(new Vertex2D(-4.0, -1.0));
    ConvexPolygon rect2(s2list);

    /**
     * The overlap here is a line-segment [-3,-1]->[0.5,-1] which is not 
     * valid polygon so this should throw
     */
    TS_ASSERT_THROWS(intersectionByLaszlo(rect2, parallelogram), Mantid::Geometry::NoIntersectionException);
    TS_ASSERT_THROWS(intersectionByLaszlo(parallelogram, rect2), Mantid::Geometry::NoIntersectionException);
  }

  void test_First_Shape_Engulfing_Second_Gives_Overlap_Of_Smaller()
  {
    ConvexPolygon smallRectangle = ConvexPolygon(7.0, 8.0,0.5, 1.5);
    ConvexPolygon largeRectange(6.8, 8.6, -0.5, 2.0);

    ConvexPolygon overlap = intersectionByLaszlo(smallRectangle,largeRectange);
    TS_ASSERT_EQUALS(overlap.numVertices(), 4);
    // Are they correct
    TS_ASSERT_EQUALS(overlap[0], smallRectangle[0]);
    TS_ASSERT_EQUALS(overlap[1], smallRectangle[1]);
    TS_ASSERT_EQUALS(overlap[2], smallRectangle[2]);
    TS_ASSERT_EQUALS(overlap[3], smallRectangle[3]);

  }


};

//------------------------------------------------------------------------
// Performance Tests
//------------------------------------------------------------------------

class LaszloIntersectionTestPerformance : public CxxTest::TestSuite
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
        intersectionByLaszlo(squareOne, squareTwo);
      }
      catch(Mantid::Geometry::NoIntersectionException&)
      {
      }
    }

  }
};


#endif /* MANTID_GEOMETRY_LASZLOINTERSECTIONTEST_H_ */

