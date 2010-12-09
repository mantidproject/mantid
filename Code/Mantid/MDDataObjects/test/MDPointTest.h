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


  struct MyExtraData
  {
    uint detectorID;
    char instrument;
  };

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



  void testCorners()
  {
    MDPoint<2,2> a;
    a.setCorner(0, 0, 0.123);
    TS_ASSERT_EQUALS( a.getCorner(0,0), 0.123);

    a.setCorner(0, 1, 1.234);
    TS_ASSERT_EQUALS( a.getCorner(0,0), 0.123);
    TS_ASSERT_EQUALS( a.getCorner(0,1), 1.234);

    a.setCenter(2, 2.345);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.345);
  }
};

#endif
