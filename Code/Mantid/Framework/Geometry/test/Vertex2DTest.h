#ifndef MANTID_GEOMETRY_VERTEX2DTEST_H_
#define MANTID_GEOMETRY_VERTEX2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Math/Vertex2D.h"

using Mantid::Geometry::Vertex2D;
using Mantid::Kernel::V2D;

class Vertex2DTest : public CxxTest::TestSuite
{
public:

  void test_Default_Construction()
  {
    Vertex2D origin;
    TS_ASSERT_EQUALS(origin.X(), 0.0);
    TS_ASSERT_EQUALS(origin.Y(), 0.0);
  }

  void test_Construction_With_Pt_Yields_Correct_Values()
  {
    Vertex2D vertex(V2D(5.1, 10.9));
    TS_ASSERT_EQUALS(vertex.X(), 5.1);
    TS_ASSERT_EQUALS(vertex.Y(), 10.9);
  }

  void test_Construction_With_XY_Values_Yields_Correct_Values()
  {
    Vertex2D vertex(5.1, 10.9);
    TS_ASSERT_EQUALS(vertex.X(), 5.1);
    TS_ASSERT_EQUALS(vertex.Y(), 10.9);
  }

  void test_Construction_Yields_Self_As_Next_And_Prev()
  {
    Vertex2D origin;
    TS_ASSERT_EQUALS(origin.next(), &origin);
    TS_ASSERT_EQUALS(origin.previous(), &origin);

    Vertex2D vertex(5.1, 10.9);
    TS_ASSERT_EQUALS(vertex.next(), &vertex);
    TS_ASSERT_EQUALS(vertex.previous(), &vertex);

    Vertex2D vertexPt(V2D(5.1, 10.9));
    TS_ASSERT_EQUALS(vertexPt.next(), &vertexPt);
    TS_ASSERT_EQUALS(vertexPt.previous(), &vertexPt);
  }

  void test_Insert_Yields_Next_As_Inserted_Vertex()
  {
    makeThreeVertexChain(true, false);
  }

   void test_Remove_Returns_An_Isolated_Vertex()
   {
     makeThreeVertexChain(false, true);
   }

private:
  void makeThreeVertexChain(const bool doInsertTests, const bool doRemoveTests)
  {
    Vertex2D origin;
    Vertex2D two(0.0,1.0);
    Vertex2D *vertexTwo = origin.insert(&two);
    if( doInsertTests )
    {
      TS_ASSERT_EQUALS(vertexTwo, &two);

      TS_ASSERT_EQUALS(origin.next(), &two);
      TS_ASSERT_EQUALS(origin.previous(), &two);
      TS_ASSERT_EQUALS(vertexTwo->previous(), &origin);
      TS_ASSERT_EQUALS(vertexTwo->next(), &origin);
    }
    //and a third
    Vertex2D third(1.0, 1.0);
    Vertex2D *vertexThree = two.insert(&third);
    if( doInsertTests )
    {
      TS_ASSERT_EQUALS(vertexThree, &third);

      TS_ASSERT_EQUALS(origin.next(), &two);
      TS_ASSERT_EQUALS(origin.previous(), &third);
      TS_ASSERT_EQUALS(vertexTwo->previous(), &origin);
      TS_ASSERT_EQUALS(vertexTwo->next(), &third);
      TS_ASSERT_EQUALS(vertexThree->previous(), &two);
      TS_ASSERT_EQUALS(vertexThree->next(), &origin);
    }

    if( doRemoveTests )
    {
      Vertex2D *removedOne = vertexThree->remove();
      TS_ASSERT_EQUALS(removedOne, vertexThree);
      TS_ASSERT_EQUALS(origin.next(), &two);
      TS_ASSERT_EQUALS(origin.previous(), &two);
      TS_ASSERT_EQUALS(vertexTwo->previous(), &origin);
      TS_ASSERT_EQUALS(vertexTwo->next(), &origin);

      Vertex2D *removedTwo = vertexTwo->remove();
      TS_ASSERT_EQUALS(origin.next(), &origin);
      TS_ASSERT_EQUALS(origin.previous(), &origin);
    }

  }

};


#endif /* MANTID_GEOMETRY_VERTEX2DTEST_H_ */

