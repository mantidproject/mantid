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
    TS_ASSERT_EQUALS( a.getDetectorID(), 0);

    MDEvent<4> b(2.5, 1.5);
    TS_ASSERT_EQUALS( b.getNumDims(), 4);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getRunIndex(), 0);
    TS_ASSERT_EQUALS( b.getDetectorID(), 0);

    // NOTE: The pragma (pack,2) call has no effect on some platforms: RHEL5, Ubuntu 10.04 and MacOS as of now.
    // Therefore these tests fail and the events are somewhat too big on these platforms:
    // TS_ASSERT_EQUALS( sizeof(a), sizeof(coord_t)*3+8+6);
    // TS_ASSERT_EQUALS( sizeof(b), sizeof(coord_t)*4+8+6);
  }

  void test_constructor()
  {
    MDEvent<3> b(2.5, 1.5, 123, 456789);
    TS_ASSERT_EQUALS( b.getNumDims(), 3);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getRunIndex(), 123);
    TS_ASSERT_EQUALS( b.getDetectorID(), 456789);
  }

  void test_constructor_withCoords()
  {
    // Fixed-size array
    coord_t coords[3] = {0.125, 1.25, 2.5};
    MDEvent<3> b(2.5, 1.5, 123, 456789, coords );
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( b.getCenter(1), 1.25);
    TS_ASSERT_EQUALS( b.getCenter(2), 2.5);
    TS_ASSERT_EQUALS( b.getRunIndex(), 123);
    TS_ASSERT_EQUALS( b.getDetectorID(), 456789);
  }

  /** Note: the copy constructor is not explicitely written but rather is filled in by the compiler */
  void test_CopyConstructor()
  {
    coord_t coords[3] = {0.125, 1.25, 2.5};
    MDEvent<3> b(2.5, 1.5, 123, 456789, coords );
    MDEvent<3> a(b);
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( b.getCenter(1), 1.25);
    TS_ASSERT_EQUALS( b.getCenter(2), 2.5);
    TS_ASSERT_EQUALS( a.getRunIndex(), 123);
    TS_ASSERT_EQUALS( a.getDetectorID(), 456789);
  }


};


class MDEventTestPerformance :  public CxxTest::TestSuite
{
public:
  std::vector<MDEvent<3> > events3;
  std::vector<MDLeanEvent<3> > lean_events3;
  std::vector<MDEvent<4> > events4;
  std::vector<MDLeanEvent<4> > lean_events4;
  size_t num;

  void setUp()
  {
    num = 1000000;
    events3.clear(); events3.reserve(num);
    events4.clear(); events4.reserve(num);
    lean_events3.clear(); lean_events3.reserve(num);
    lean_events4.clear(); lean_events4.reserve(num);
  }

  void test_create_MDEvent3()
  {
    float signal(1.5);
    float error(2.5);
    uint16_t runIndex = 123;
    uint16_t detectorId = 45678;
    coord_t center[3] = {1.25, 2.5, 3.5};
    for (size_t i=0; i<num; i++)
      events3.push_back( MDEvent<3>(signal, error, runIndex, detectorId, center) );
  }

  void test_create_MDEvent4()
  {
    float signal(1.5);
    float error(2.5);
    uint16_t runIndex = 123;
    uint16_t detectorId = 45678;
    coord_t center[4] = {1.25, 2.5, 3.5, 4.75};
    for (size_t i=0; i<num; i++)
      events4.push_back( MDEvent<4>(signal, error, runIndex, detectorId, center) );
  }

  void test_create_MDLeanEvent3()
  {
    float signal(1.5);
    float error(2.5);
    coord_t center[3] = {1.25, 2.5, 3.5};
    for (size_t i=0; i<num; i++)
      lean_events3.push_back( MDLeanEvent<3>(signal, error, center) );
  }

  void test_create_MDLeanEvent4()
  {
    float signal(1.5);
    float error(2.5);
    coord_t center[4] = {1.25, 2.5, 3.5, 4.75};
    for (size_t i=0; i<num; i++)
      lean_events4.push_back( MDLeanEvent<4>(signal, error, center) );
  }

};

#endif
