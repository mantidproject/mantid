#ifndef MDEVENTTEST_H
#define MDEVENTTEST_H

#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEvent.h"
#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;

class MDEventTest :    public CxxTest::TestSuite
{
public:


  void test_simple_constructors()
  {
    MDEvent<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 1.0);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.0);
    TS_ASSERT_EQUALS( a.getRunIndex(), 0);
    TS_ASSERT_EQUALS( a.getDetectorId(), 0);

    MDEvent<4> b(2.5, 1.5);
    TS_ASSERT_EQUALS( b.getNumDims(), 4);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getRunIndex(), 0);
    TS_ASSERT_EQUALS( b.getDetectorId(), 0);

    TS_ASSERT_EQUALS( sizeof(a), sizeof(coord_t)*3+8+6);
    TS_ASSERT_EQUALS( sizeof(b), sizeof(coord_t)*4+8+6);
  }

  void test_constructor()
  {
    MDEvent<3> b(2.5, 1.5, 123, 456789);
    TS_ASSERT_EQUALS( b.getNumDims(), 3);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getRunIndex(), 123);
    TS_ASSERT_EQUALS( b.getDetectorId(), 456789);
  }

  void test_constructor_withCoords()
  {
    // Fixed-size array
    coord_t coords[3] = {0.123, 1.234, 2.345};
    MDEvent<3> b(2.5, 1.5, 123, 456789, coords );
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( b.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( b.getCenter(2), 2.345);
    TS_ASSERT_EQUALS( b.getRunIndex(), 123);
    TS_ASSERT_EQUALS( b.getDetectorId(), 456789);
  }

  /** Note: the copy constructor is not explicitely written but rather is filled in by the compiler */
  void test_CopyConstructor()
  {
    coord_t coords[3] = {0.123, 1.234, 2.345};
    MDEvent<3> b(2.5, 1.5, 123, 456789, coords );
    MDEvent<3> a(b);
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.123);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.234);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.345);
    TS_ASSERT_EQUALS( a.getRunIndex(), 123);
    TS_ASSERT_EQUALS( a.getDetectorId(), 456789);
  }


};

#endif
