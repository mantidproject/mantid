#ifndef MANTID_MDEVENTS_MDLEANEVENTTEST_H_
#define MANTID_MDEVENTS_MDLEANEVENTTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDLeanEventTest : public CxxTest::TestSuite
{
public:

  void test_Constructors()
  {
    MDLeanEvent<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 1.0);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.0);

    MDLeanEvent<4> b(2.5, 1.5);
    TS_ASSERT_EQUALS( b.getNumDims(), 4);
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);

    TS_ASSERT_EQUALS( sizeof(a), sizeof(coord_t)*3+8);
    TS_ASSERT_EQUALS( sizeof(b), sizeof(coord_t)*4+8);
  }

  void test_ConstructorsWithCoords()
  {
    // Fixed-size array
    coord_t coords[3] = {0.125, 1.25, 2.5};
    MDLeanEvent<3> a(2.5, 1.5, coords );
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.25);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.5);

    // Dynamic array
    coord_t * coords2 = new coord_t[5];
    coords2[0] = 1.0;
    coords2[1] = 2.0;
    coords2[2] = 3.0;

    MDLeanEvent<3> b(2.5, 1.5, coords2 );
    TS_ASSERT_EQUALS( b.getSignal(), 2.5);
    TS_ASSERT_EQUALS( b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( b.getCenter(0), 1);
    TS_ASSERT_EQUALS( b.getCenter(1), 2);
    TS_ASSERT_EQUALS( b.getCenter(2), 3);

  }


  void test_Coord()
  {
    MDLeanEvent<3> a;
    TS_ASSERT_EQUALS( a.getNumDims(), 3);

    a.setCenter(0, 0.125);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.125);

    a.setCenter(1, 1.25);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.25);

    a.setCenter(2, 2.5);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.25);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.5);
    TS_ASSERT_EQUALS( a.getCenter()[0], 0.125);
    TS_ASSERT_EQUALS( a.getCenter()[1], 1.25);
    TS_ASSERT_EQUALS( a.getCenter()[2], 2.5);
  }

  void test_setCenter_array()
  {
    MDLeanEvent<3> a;
    coord_t coords[3] = {0.125, 1.25, 2.5};
    a.setCoords( coords );
    TS_ASSERT_EQUALS( a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.25);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.5);
  }

  /** Note: the copy constructor is not explicitely written but rather is filled in by the compiler */
  void test_CopyConstructor()
  {
    coord_t coords[3] = {0.125, 1.25, 2.5};
    MDLeanEvent<3> b(2.5, 1.5, coords );
    MDLeanEvent<3> a(b);
    TS_ASSERT_EQUALS( a.getNumDims(), 3);
    TS_ASSERT_EQUALS( a.getSignal(), 2.5);
    TS_ASSERT_EQUALS( a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS( a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS( a.getCenter(1), 1.25);
    TS_ASSERT_EQUALS( a.getCenter(2), 2.5);
  }

  void test_getError()
  {
    MDLeanEvent<3> a(2.0, 4.0);
    TS_ASSERT_EQUALS( a.getSignal(), 2.0);
    TS_ASSERT_EQUALS( a.getError(), 2.0);
  }




};


#endif /* MANTID_MDEVENTS_MDLEANEVENTTEST_H_ */

