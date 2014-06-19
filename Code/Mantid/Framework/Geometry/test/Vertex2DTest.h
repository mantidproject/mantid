#ifndef MANTID_GEOMETRY_VERTEX2DTEST_H_
#define MANTID_GEOMETRY_VERTEX2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Math/Vertex2D.h"

using Mantid::Geometry::Vertex2D;
using Mantid::Geometry::Vertex2DIterator;
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

  void test_Copy_Gives_Correct_Values_In_Constructed_Object()
  {
    Vertex2D vertex(5.1, 10.9);
    Vertex2D copy(vertex);
    TS_ASSERT_EQUALS(copy.X(), 5.1);
    TS_ASSERT_EQUALS(copy.Y(), 10.9);
    TS_ASSERT_EQUALS(copy.next(), &copy);
    TS_ASSERT_EQUALS(copy.previous(), &copy);   
  }

  void test_Assign_Gives_Correct_Values_On_LHS()
  {
    Vertex2D vertex(5.1, 10.9);
    Vertex2D assigned;
    assigned = vertex;
    TS_ASSERT_EQUALS(assigned.X(), 5.1);
    TS_ASSERT_EQUALS(assigned.Y(), 10.9);
    TS_ASSERT_EQUALS(assigned.next(), &assigned);
    TS_ASSERT_EQUALS(assigned.previous(), &assigned);   
  }

  void test_Vertex_As_Pt_Returns_Correct_Value()
  {
    Vertex2D vertex(5.1, 10.9);
    TS_ASSERT_EQUALS(vertex.point(), V2D(5.1,10.9));
    Vertex2D *vertex2 = new Vertex2D(5.1, 10.9);
    TS_ASSERT_EQUALS(vertex2->point(), V2D(5.1,10.9));
    delete vertex2;
  }

  void test_Insert_Yields_Next_As_Inserted_Vertex()
  {
    auto v2d = makeThreeVertexChain(true, false);
    delete v2d->next()->next();
    delete v2d->next();
    delete v2d;
  }

   void test_Remove_Returns_An_Isolated_Vertex()
   {
     makeThreeVertexChain(false, true);
   }

   void test_Iteration_Advances_Correctly()
   {
     Vertex2D * start = makeThreeVertexChain(false,false);
     Vertex2DIterator pIter(start);
     TS_ASSERT_EQUALS(pIter.point(), V2D());
     pIter.advance();
     TS_ASSERT_EQUALS(pIter.point(), V2D(0.0,1.0));
     pIter.advance();
     TS_ASSERT_EQUALS(pIter.point(), V2D(1.0,1.0));
     pIter.advance(); //Back to the start
     TS_ASSERT_EQUALS(pIter.point(), V2D());
     delete start->next()->next();
     delete start->next();
     delete start;
   }

private:
  Vertex2D * makeThreeVertexChain(const bool doInsertTests, const bool doRemoveTests)
  {
    Vertex2D *origin = new Vertex2D;
    Vertex2D *two = new Vertex2D(0.0,1.0);
    Vertex2D *vertexTwo = origin->insert(two);
    if( doInsertTests )
    {
      TS_ASSERT_EQUALS(vertexTwo, two);

      TS_ASSERT_EQUALS(origin->next(), two);
      TS_ASSERT_EQUALS(origin->previous(), two);
      TS_ASSERT_EQUALS(vertexTwo->previous(), origin);
      TS_ASSERT_EQUALS(vertexTwo->next(), origin);
    }
    //and a third
    Vertex2D *third = new Vertex2D(1.0, 1.0);
    Vertex2D *vertexThree = two->insert(third);
    if( doInsertTests )
    {
      TS_ASSERT_EQUALS(vertexThree, third);

      TS_ASSERT_EQUALS(origin->next(), two);
      TS_ASSERT_EQUALS(origin->previous(), third);
      TS_ASSERT_EQUALS(vertexTwo->previous(), origin);
      TS_ASSERT_EQUALS(vertexTwo->next(), third);
      TS_ASSERT_EQUALS(vertexThree->previous(), two);
      TS_ASSERT_EQUALS(vertexThree->next(), origin);
    }

    if( doRemoveTests )
    {
      Vertex2D *removedOne = vertexThree->remove();
      TS_ASSERT_EQUALS(removedOne, vertexThree);
      TS_ASSERT_EQUALS(origin->next(), two);
      TS_ASSERT_EQUALS(origin->previous(), two);
      TS_ASSERT_EQUALS(vertexTwo->previous(), origin);
      TS_ASSERT_EQUALS(vertexTwo->next(), origin);

      Vertex2D *removedTwo = vertexTwo->remove();
      UNUSED_ARG(removedTwo);
      TS_ASSERT_EQUALS(origin->next(), origin);
      TS_ASSERT_EQUALS(origin->previous(), origin);
      delete origin;
      delete third;
      delete two;
      return NULL;
    }
    else
    {
      return origin;
    }

  }

};


#endif /* MANTID_GEOMETRY_VERTEX2DTEST_H_ */

