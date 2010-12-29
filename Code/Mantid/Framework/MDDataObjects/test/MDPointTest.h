#ifndef MDPointTEST_H
#define MDPointTEST_H

#include <cxxtest/TestSuite.h>

#include "MDDataObjects/Events/MDPoint.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDDataObjects;

class MDPointTest :    public CxxTest::TestSuite
{

  struct MyExtraData
  {
    uint detectorID;
    char instrument;
  };

public:
  void testConstructors()
  {
    MDPoint<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 1.0);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.0);

    MDPoint<4> b(2.5, 1.5);
    TS_ASSERT_EQUALS( b.getNumDims(), 4);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);

    TS_ASSERT_EQUALS( sizeof(a), sizeof(CoordType)*3+8);
    TS_ASSERT_EQUALS( sizeof(b), sizeof(CoordType)*4+8);
  }

  void testConstructors_MoreTemplateParameters()
  {
    MDPoint<3,3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( sizeof(a), sizeof(CoordType)*3*4+8);
  }

  void testConstructors_EvenMoreTemplateParameters()
  {
    MDPoint<3,3, MyExtraData> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( sizeof(a), sizeof(CoordType)*3*4 + 8 +sizeof(MyExtraData));
  }


  void testConstructorsWithCenters()
  {
    // Fixed-size array
    CoordType coords[3] = {0.123, 1.234, 2.345};
    MDPoint<3> a(2.5, 1.5, coords );
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.345);

    // Dynamic array
    CoordType * coords2 = new CoordType[5];
    coords2[0] = 1.0;
    coords2[1] = 2.0;
    coords2[2] = 3.0;
    // The array does not have to be the same size, it is safe if it is bigger (though why would you do that?)
    MDPoint<3> b(2.5, 1.5, coords2 );
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCenter(0), 1);
    TS_ASSERT_EQUALS( b.getCenter(1), 2);
    TS_ASSERT_EQUALS( b.getCenter(2), 3);

  }


  // =============================================================================================
  // =============================================================================================

  void testCenter()
  {
    MDPoint<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);

    a.setCenter(0, 0.123);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);

    a.setCenter(1, 1.234);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);

    a.setCenter(2, 2.345);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.345);
  }

  void testSetCenters()
  {
    MDPoint<3> a;
    CoordType coords[3] = {0.123, 1.234, 2.345};
    a.setCenters( coords );
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.345);
  }

  void testCopyConstructor()
  {
    CoordType coords[3] = {0.123, 1.234, 2.345};
    MDPoint<3> b(2.5, 1.5, coords );
    MDPoint<3> a(b);
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.345);
  }



  void test_getError()
  {
    MDPoint<3> a(2.0, 4.0);
    TS_ASSERT_EQUALS( a.getSignal(), 2.0);
    TS_ASSERT_EQUALS( a.getError(), 2.0);
  }


  // =============================================================================================
  // =============================================================================================

  void test_getExtra()
  {
    MDPoint<3,0,MyExtraData> a(2.0, 4.0);
    TS_ASSERT_EQUALS( a.getSignal(), 2.0);
    TS_ASSERT_EQUALS( a.getError(), 2.0);
    a.getExtra().detectorID = 12;
    a.getExtra().instrument = 'C';

    MyExtraData & e = a.getExtra();
    TS_ASSERT_EQUALS( e.detectorID, 12);
    TS_ASSERT_EQUALS( e.instrument, 'C');

    const MyExtraData & e2 = a.getExtra();
    TS_ASSERT_EQUALS( e2.detectorID, 12);
    TS_ASSERT_EQUALS( e2.instrument, 'C');
  }

  void test_setExtra()
  {
    MDPoint<3,0,MyExtraData> a(2.0, 4.0);
    MyExtraData e;
    e.detectorID = 34;
    e.instrument = 'D';
    a.setExtra(e);

    TS_ASSERT_EQUALS( a.getExtra().detectorID, 34);
    TS_ASSERT_EQUALS( a.getExtra().instrument, 'D');
    // setExtra() made a copy of the extra
    TS_ASSERT_DIFFERS( &a.getExtra(), &e);
  }



  // =============================================================================================
  // =============================================================================================

  void test_setCorner_individually()
  {
    // 2 dimensions, 3 vertices
    MDPoint<2,3> a;
    a.setCorner(0, 0, 0.123);
    TS_ASSERT_EQUALS( a.getCorner(0,0), 0.123);

    a.setCorner(0, 1, 1.234);
    a.setCorner(1, 0, 2);
    a.setCorner(1, 1, 3);
    a.setCorner(2, 0, 4);
    a.setCorner(2, 1, 5);

    TS_ASSERT_EQUALS( a.getCorner(0,0), 0.123);
    TS_ASSERT_EQUALS( a.getCorner(0,1), 1.234);
    TS_ASSERT_EQUALS( a.getCorner(1,0), 2);
    TS_ASSERT_EQUALS( a.getCorner(1,1), 3);
  }


  void test_setCorner_vertex()
  {
    // 2 dimensions, 3 vertices
    MDPoint<2,3> a;
    CoordType v0[2] = {1, 2};
    CoordType v1[2] = {4, 5};
    CoordType v2[2] = {9, 10};
    // Set the entire vertex at once
    a.setCorner(0, v0);
    a.setCorner(1, v1);
    a.setCorner(2, v2);

    // They match!
    for (size_t i=0; i<2; i++)
    {
      TS_ASSERT_EQUALS( a.getCorner(0)[i], v0[i]);
      TS_ASSERT_EQUALS( a.getCorner(1)[i], v1[i]);
      TS_ASSERT_EQUALS( a.getCorner(2)[i], v2[i]);
    }

    // But if you change the original, they no longer match
    v1[1] = 45;
    TS_ASSERT_DIFFERS( a.getCorner(1)[1], v1[1]);
  }

  //  void testCopyConstructor_with_Corners()
  //  {
  //    CoordType coords[2] = {0.123, 1.234};
  //    // One corner vertec
  //    CoordType corners[2][1] = {{5.691}, {7.2}};
  //    MDPoint<2,1> b(2.5, 1.5, coords );
  //    b.setCorners( corners);
  //    // Do the copy
  //    MDPoint<2,1> a(b);
  //    TS_ASSERT_EQUALS( a.getNumDims(), 2);
  //    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
  //    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
  //    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
  //    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
  //    TS_ASSERT_EQUALS( a.getCorner(0, 0), 5.691);
  //    TS_ASSERT_EQUALS( a.getCorner(0, 1), 7.2);
  //  }

};

#endif
