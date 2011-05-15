#ifndef MANTID_MDEVENTS_COORDTRANSFORMTEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/CoordTransform.h"
#include "MantidMDEvents/MDEventFactory.h"

using namespace Mantid::MDEvents;

class CoordTransformTest : public CxxTest::TestSuite
{
public:

  void test_initialization()
  {
    // Can't output more dimensions than the input
    TS_ASSERT_THROWS_ANYTHING(CoordTransform ct_cant(2,3))
        CoordTransform ct(2,1);
    TS_ASSERT_EQUALS( ct.getMatrix().numRows(), 2);
    TS_ASSERT_EQUALS( ct.getMatrix().numCols(), 3);
  }

  /** Helper to compare two "vectors" (bare float arrays) */
  void compare(size_t numdims, coord_t * value, const coord_t * expected)
  {
    for (size_t i=0; i< numdims; i++)
      TS_ASSERT_DELTA( value[i], expected[i], 1e-5);
  }

  /** Simple identity transform */
  void test_donothing()
  {
    coord_t in[2] = {1.5, 2.5};
    coord_t out[2];
    CoordTransform ct(2,2); // defaults to identity
    ct.apply(in, out);
    compare(2, out, in);
  }

  /** Translate in 2D */
  void test_translate()
  {
    coord_t in[2] = {1.5, 2.5};
    coord_t out[2];
    coord_t translation[2] = {2.0, 3.0};
    coord_t expected[2] = {3.5, 5.5};
    CoordTransform ct(2,2);
    ct.addTranslation(translation);
    ct.apply(in, out);
    compare(2, out, expected);
  }


};


class CoordTransformTestPerformance : public CxxTest::TestSuite
{
public:
  void test_apply_3D_performance()
  {
    // Do a simple 3-3 transform.
    CoordTransform ct(3,3);
    coord_t translation[3] = {2.0, 3.0, 4.0};
    coord_t in[3] = {1.5, 2.5, 3.5};
    coord_t out[3];
    ct.addTranslation(translation);

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, out);
    }
  }
  void test_apply_4D_performance()
  {
    CoordTransform ct(4,4);
    coord_t translation[4] = {2.0, 3.0, 4.0, 5.0};
    coord_t in[4] = {1.5, 2.5, 3.5, 4.5};
    coord_t out[4];
    ct.addTranslation(translation);

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, out);
    }
  }

};

#endif /* MANTID_MDEVENTS_COORDTRANSFORMTEST_H_ */

