#ifndef MANTID_GEOMETRY_VERTEX2DLISTTEST_H_
#define MANTID_GEOMETRY_VERTEX2DLISTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Math/Vertex2DList.h"
#include "MantidKernel/Exception.h"

using Mantid::Geometry::Vertex2DList;
using Mantid::Kernel::V2D;

class Vertex2DListTest : public CxxTest::TestSuite
{
public:

  void test_Constructing_Does_Not_Throw()
  {
    TS_ASSERT_THROWS_NOTHING(Vertex2DList());
  }

  void test_Constructing_With_Size_Gives_N_Points_At_Origin()
  {
    Vertex2DList vertices(3);
    for(size_t i = 0; i < 3; ++i)
    {
      TS_ASSERT_EQUALS(vertices[i], V2D());
    }
  }

  void test_A_Non_Existing_Point_Can_Be_Added_To_The_List()
  {
    Vertex2DList vertices;
    TS_ASSERT_THROWS_NOTHING(vertices.insert(V2D(1.0,0.0)));
    TS_ASSERT_EQUALS(vertices.size(), 1);
    TS_ASSERT_THROWS_NOTHING(vertices.insert(V2D(1.0,2.0)));
    TS_ASSERT_EQUALS(vertices.size(), 2);
  }
  
  void test_Adding_Existing_Point_Returns_Index_Of_Existing_Point()
  {
    Vertex2DList vertices;
    TS_ASSERT_THROWS_NOTHING(vertices.insert(V2D(1.0,0.0)));
    TS_ASSERT_THROWS_NOTHING(vertices.insert(V2D(1.0,1.0)));
    
    TS_ASSERT_EQUALS(vertices.insert(V2D(1.0,0.0)), 0);
  }

  void test_Operator_Access_For_Invalid_Throws()
  {
    Vertex2DList vertices;
    TS_ASSERT_THROWS(vertices[0], Mantid::Kernel::Exception::IndexError);
    vertices.insert(V2D(1.0,0.0));
    TS_ASSERT_THROWS(vertices[1], Mantid::Kernel::Exception::IndexError);
  }

  void test_Operator_Access_Returns_Correct_Value_When_In_Range()
  {
    Vertex2DList vertices;
    vertices.insert(V2D(1.0,0.0));
    TS_ASSERT_EQUALS(vertices[0], V2D(1.0,0.0));
    vertices.insert(V2D(2.0,3.0));
    TS_ASSERT_EQUALS(vertices[1], V2D(2.0,3.0));
  }
    
  void test_Operator_Can_Mutate_Given_Index()
  {
    Vertex2DList vertices(2);
    vertices[1] = V2D(2.0,3.0);
    TS_ASSERT_EQUALS(vertices[0], V2D());
    TS_ASSERT_EQUALS(vertices[1], V2D(2.0,3.0));
  }

};


#endif /* MANTID_GEOMETRY_VERTEX2DLISTTEST_H_ */

