#ifndef MDEVENTTEST_H
#define MDEVENTTEST_H

#include <cxxtest/TestSuite.h>

#include "MDDataObjects/Events/MDEvent.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDDataObjects;

class MDEventTest :    public CxxTest::TestSuite
{
public:
  void testConstructors()
  {
    MDEvent<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 1.0);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.0);

    MDEvent<4> b(2.5, 1.5);
    TS_ASSERT_EQUALS( b.getNumDims(), 4);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
  }

  void testConstructorsWithCoords()
  {
    // Fixed-size array
    CoordType coords[3] = {0.123, 1.234, 2.345};
    MDEvent<3> a(2.5, 1.5, coords );
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( a.getCoord(0), 0.123);
    TS_ASSERT_EQUALS( a.getCoord(1), 1.234);
    TS_ASSERT_EQUALS( a.getCoord(2), 2.345);

    // Dynamic array
    CoordType * coords2 = new CoordType[5];
    coords2[0] = 1.0;
    coords2[1] = 2.0;
    coords2[2] = 3.0;

    MDEvent<3> b(2.5, 1.5, coords2 );
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCoord(0), 1);
    TS_ASSERT_EQUALS( b.getCoord(1), 2);
    TS_ASSERT_EQUALS( b.getCoord(2), 3);

  }


  void testCooord()
  {
    MDEvent<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);

    a.setCoord(0, 0.123);
    TS_ASSERT_EQUALS( a.getCoord(0), 0.123);

    a.setCoord(1, 1.234);
    TS_ASSERT_EQUALS( a.getCoord(0), 0.123);
    TS_ASSERT_EQUALS( a.getCoord(1), 1.234);

    a.setCoord(2, 2.345);
    TS_ASSERT_EQUALS( a.getCoord(0), 0.123);
    TS_ASSERT_EQUALS( a.getCoord(1), 1.234);
    TS_ASSERT_EQUALS( a.getCoord(2), 2.345);
  }

  void testSetCooords()
  {
    MDEvent<3> a;
    CoordType coords[3] = {0.123, 1.234, 2.345};
    a.setCoords( coords );
    TS_ASSERT_EQUALS( a.getCoord(0), 0.123);
    TS_ASSERT_EQUALS( a.getCoord(1), 1.234);
    TS_ASSERT_EQUALS( a.getCoord(2), 2.345);
  }

  void testCopyConstructor()
  {
    CoordType coords[3] = {0.123, 1.234, 2.345};
    MDEvent<3> b(2.5, 1.5, coords );
    MDEvent<3> a(b);
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( a.getCoord(0), 0.123);
    TS_ASSERT_EQUALS( a.getCoord(1), 1.234);
    TS_ASSERT_EQUALS( a.getCoord(2), 2.345);
  }

  void test_getError()
  {
    MDEvent<3> a(2.0, 4.0);
    TS_ASSERT_EQUALS( a.getSignal(), 2.0);
    TS_ASSERT_EQUALS( a.getError(), 2.0);
  }


};

#endif
