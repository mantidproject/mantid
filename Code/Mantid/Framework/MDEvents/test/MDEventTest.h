#ifndef MDEVENTTEST_H
#define MDEVENTTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/MDEvent.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Timer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;

class MDEventTest :    public CxxTest::TestSuite
{

public:


//
//  /** Test that shows that OPENMP does not use a thread pool idea to optimally allocate threads */
//  void testOpenMP()
//  {
//    Timer overall;
//    int num = 32;
//    PARALLEL_FOR_NO_WSP_CHECK()
//    //PRAGMA_OMP(for ordered)
//    for (int i=0; i<num; i++)
//    {
//      double delay = num - i;
//      PARALLEL_CRITICAL(test1)
//      std::cout << std::setw(5) << i << ": Thread " << PARALLEL_THREAD_NUMBER << " will delay for " << delay << " seconds.\n";
//
//      // Waste time, but use up the CPU!
//      Timer time;
//      while (time.elapsed_no_reset() < delay)
//      {
//        double x = 1.1;
//        for (int j=0; j < 100000; j++)
//        {
//          x = x * x;
//          x = x / 1.1;
//        }
//      }
//      //sleep(delay);
//
//      PARALLEL_CRITICAL(test1)
//      std::cout << std::setw(5) << i << ": is done and took " << time.elapsed() << " secs (was supposed to be " << delay << " sec).\n";
//    }
//    std::cout << overall.elapsed() << " secs total.\n";
//  }

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

    TS_ASSERT_EQUALS( sizeof(a), sizeof(CoordType)*3+8);
    TS_ASSERT_EQUALS( sizeof(b), sizeof(CoordType)*4+8);
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
