#ifndef MANTID_GEOMETRY_QUADRILATERALTEST_H_
#define MANTID_GEOMETRY_QUADRILATERALTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Math/Quadrilateral.h"
#include <cfloat>

using namespace Mantid::Geometry;
using Mantid::Kernel::V2D;
using Mantid::Geometry::Vertex2D;

class QuadrilateralTest : public CxxTest::TestSuite
{
public:

  void test_Rectangle_Constructor()
  {
    Quadrilateral rect(0.0, 2.0, 0.0, 1.5);
    TS_ASSERT_EQUALS(rect[0], V2D());
    TS_ASSERT_EQUALS(rect[1], V2D(0.0,1.5));
    TS_ASSERT_EQUALS(rect[2], V2D(2.0,1.5));
    TS_ASSERT_EQUALS(rect[3], V2D(2.0,0.0));

    TS_ASSERT_DELTA(rect.smallestX(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(rect.largestX(), 2.0, DBL_EPSILON);
    TS_ASSERT_DELTA(rect.smallestY(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(rect.largestY(), 1.5, DBL_EPSILON);
  }

  void test_Head_Retrieval()
  {
    Quadrilateral rectangle = makeRectangle();
    const Vertex2D *head = rectangle.head();
    TS_ASSERT_EQUALS(*head, Vertex2D(0.0,0.0));
  }

  void test_Area()
  {
    Quadrilateral rectangle = makeRectangle();
    TS_ASSERT_DELTA(rectangle.area(), 3.0, DBL_EPSILON);
  }

  void test_Copy()
  {
    Quadrilateral rectangle = makeRectangle();
    TS_ASSERT_EQUALS(rectangle[0], V2D());
    TS_ASSERT_EQUALS(rectangle[1], V2D(0.0,1.5));
    TS_ASSERT_EQUALS(rectangle[2], V2D(2.0,1.5));
    TS_ASSERT_EQUALS(rectangle[3], V2D(2.0,0.0));

    // Force a copy
    Quadrilateral copied(rectangle);
    TS_ASSERT_EQUALS(copied[0], V2D());
    TS_ASSERT_EQUALS(copied[1], V2D(0.0,1.5));
    TS_ASSERT_EQUALS(copied[2], V2D(2.0,1.5));
    TS_ASSERT_EQUALS(copied[3], V2D(2.0,0.0));

    TS_ASSERT_DELTA(copied.smallestX(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(copied.largestX(), 2.0, DBL_EPSILON);
    TS_ASSERT_DELTA(copied.smallestY(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(copied.largestY(), 1.5, DBL_EPSILON);
  }

  void test_Assignment()
  {
    Quadrilateral rectangle = makeRectangle();
    Quadrilateral assign(0.0,1.0,1.0,0.0);
    assign = rectangle;
    TS_ASSERT_EQUALS(assign[0], V2D());
    TS_ASSERT_EQUALS(assign[1], V2D(0.0,1.5));
    TS_ASSERT_EQUALS(assign[2], V2D(2.0,1.5));
    TS_ASSERT_EQUALS(assign[3], V2D(2.0,0.0));

    TS_ASSERT_DELTA(assign.smallestX(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(assign.largestX(), 2.0, DBL_EPSILON);
    TS_ASSERT_DELTA(assign.smallestY(), 0.0, DBL_EPSILON);
    TS_ASSERT_DELTA(assign.largestY(), 1.5, DBL_EPSILON);
  }
  
private:
  Quadrilateral makeRectangle()
  {
    return Quadrilateral(V2D(), V2D(2.0,0.0), V2D(2.0,1.5), V2D(0.0,1.5));
  }

};


//------------------------------------------------------------------------------
// Performance Test
//------------------------------------------------------------------------------
class QuadrilateralTestPerformance : public CxxTest::TestSuite
{
public:
  
  void test_Area_Calls()
  {
    const size_t ntests(50000000);

    double totalArea(0.0);
    for( size_t i = 0; i < ntests; ++i)
    {
      Quadrilateral test(V2D(), V2D(2.0,0.0), V2D(2.0,1.5), V2D(0.0,1.5));
      totalArea += test.area();
    }
    
  }

};

#endif /* MANTID_GEOMETRY_QUADRILATERALTEST_H_ */

