#ifndef MANTID_MDEVENTS_COORDTRANSFORMDISTANCETEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMDISTANCETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/CoordTransformDistance.h"

using namespace Mantid::MDEvents;

class CoordTransformDistanceTest : public CxxTest::TestSuite
{
public:

  /** Helper to compare two "vectors" (bare float arrays) */
  void compare(size_t numdims, CoordType * value, const CoordType * expected)
  {
    for (size_t i=0; i< numdims; i++)
      TS_ASSERT_DELTA( value[i], expected[i], 1e-5);
  }

  void test_constructor()
  {
    CoordType center[4] = {1,2,3,4};
    bool used[4] = {true, false, true, true};
    CoordTransformDistance ct(4, center, used);
    // A copy was made
    TS_ASSERT_DIFFERS(ct.getCenter(), center);
    TS_ASSERT_DIFFERS(ct.getDimensionsUsed(), used);

    // Contents are good
    compare(4, center, ct.getCenter());
    for (size_t i=0; i<4; i++)
      TS_ASSERT_EQUALS( used[i], ct.getDimensionsUsed()[i]);
  }


  /** Calculate the distance (squared)*/
  void test_distance_all_used()
  {
    // Build it
    CoordType center[2] = {1, 2};
    bool used[2] = {true, true};
    CoordTransformDistance ct(2,center,used);

    CoordType out = 0;

    CoordType in1[2] = {0, 3};
    TS_ASSERT_THROWS_NOTHING( ct.apply(in1, &out) );
    TS_ASSERT_DELTA( out, 2, 1e-5);

    CoordType in2[2] = {-1, 5};
    TS_ASSERT_THROWS_NOTHING( ct.apply(in2, &out) );
    TS_ASSERT_DELTA( out, 13, 1e-5);
  }

  /** Calculate the distance (squared)*/
  void test_distance_some_unused()
  {
    // Build it
    CoordType center[2] = {1, 2};
    bool used[2] = {true, false};
    CoordTransformDistance ct(2,center,used);

    CoordType out = 0;

    CoordType in1[2] = {0, 3};
    TS_ASSERT_THROWS_NOTHING( ct.apply(in1, &out) );
    TS_ASSERT_DELTA( out, 1, 1e-5);

    CoordType in2[2] = {-1, 5};
    TS_ASSERT_THROWS_NOTHING( ct.apply(in2, &out) );
    TS_ASSERT_DELTA( out, 4, 1e-5);
  }

};




class CoordTransformDistanceTestPerformance : public CxxTest::TestSuite
{
public:
  void test_apply_3D_performance()
  {
    CoordType center[3] = {2.0, 3.0, 4.0};
    bool used[3] = {true, true, true};
    CoordTransformDistance ct(3,center,used);
    CoordType in[3] = {1.5, 2.5, 3.5};
    CoordType out;

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, &out);
    }
    TS_ASSERT_DELTA( out, .25*3, 1e-5);
  }

  void test_apply_4D_performance()
  {
    CoordType center[4] = {2.0, 3.0, 4.0, 5.0};
    bool used[4] = {true, true, true, true};
    CoordTransformDistance ct(4,center,used);
    CoordType in[4] = {1.5, 2.5, 3.5, 4.5};
    CoordType out;

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, &out);
    }
    TS_ASSERT_DELTA( out, .25*4, 1e-5);
  }

  void test_apply_10D_with_3D_used_performance()
  {
    CoordType center[10] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    bool used[10] = {true, true, true, false, false, false, false, false, false, false};
    CoordTransformDistance ct(10, center,used);
    CoordType in[10] = {1.5, 2.5, 3.5, 4.5, 16, 17, 18, 19, 20, 21};
    CoordType out;

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, &out);
    }
    TS_ASSERT_DELTA( out, .25*3, 1e-5);
  }


};


#endif /* MANTID_MDEVENTS_COORDTRANSFORMDISTANCETEST_H_ */

