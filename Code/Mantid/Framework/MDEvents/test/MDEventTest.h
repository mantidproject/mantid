#ifndef MDEVENTTEST_H
#define MDEVENTTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/MDEvent.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Timer.h"
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;

class MDEventTest :    public CxxTest::TestSuite
{

public:


  void test_Constructors()
  {
    MDEvent<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 1.0);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.0);

    MDEvent<4> b(2.5, 1.5);
    TS_ASSERT_EQUALS( b.getNumDims(), 4);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);

    TS_ASSERT_EQUALS( sizeof(a), sizeof(CoordType)*3+8);
    TS_ASSERT_EQUALS( sizeof(b), sizeof(CoordType)*4+8);
  }

  void test_ConstructorsWithCoords()
  {
    // Fixed-size array
    CoordType coords[3] = {0.123, 1.234, 2.345};
    MDEvent<3> a(2.5, 1.5, coords );
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

    MDEvent<3> b(2.5, 1.5, coords2 );
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCenter(0), 1);
    TS_ASSERT_EQUALS( b.getCenter(1), 2);
    TS_ASSERT_EQUALS( b.getCenter(2), 3);

  }


  void test_Coord()
  {
    MDEvent<3> a;
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
    TS_ASSERT_EQUALS( a.getCenter()[0], 0.123);
    TS_ASSERT_EQUALS( a.getCenter()[1], 1.234);
    TS_ASSERT_EQUALS( a.getCenter()[2], 2.345);
  }

  void test_setCenter_array()
  {
    MDEvent<3> a;
    CoordType coords[3] = {0.123, 1.234, 2.345};
    a.setCoords( coords );
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.345);
  }

  void test_CopyConstructor()
  {
    CoordType coords[3] = {0.123, 1.234, 2.345};
    MDEvent<3> b(2.5, 1.5, coords );
    MDEvent<3> a(b);
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.345);
  }

  void test_getError()
  {
    MDEvent<3> a(2.0, 4.0);
    TS_ASSERT_EQUALS( a.getSignal(), 2.0);
    TS_ASSERT_EQUALS( a.getError(), 2.0);
  }



};

#endif
