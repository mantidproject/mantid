#ifndef MANTID_MDEVENTS_COORDTRANSFORMALIGNEDTEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMALIGNEDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/CoordTransformAligned.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class CoordTransformAlignedTest : public CxxTest::TestSuite
{
public:

  void test_constructor_throws()
  {
    TSM_ASSERT_THROWS_ANYTHING( "Bad number of dimensions", CoordTransformAligned ct(0,0, NULL, NULL, NULL); );
    TSM_ASSERT_THROWS_ANYTHING( "Too many output dimensions", CoordTransformAligned ct(3,4, NULL, NULL, NULL); );
    TSM_ASSERT_THROWS_ANYTHING( "Null input", CoordTransformAligned ct(1,1, NULL, NULL, NULL); );
    size_t dimToBinFrom[3] = {4, 1, 0};
    coord_t origin[3] = {5, 10, 15};
    coord_t scaling[3] = {1, 2, 3};
    TSM_ASSERT_THROWS_ANYTHING( "DimtoBinFrom has too high an index", CoordTransformAligned ct(4,3, dimToBinFrom, origin, scaling); );
  }

  void test_constructor_and_apply()
  {
    size_t dimToBinFrom[3] = {3, 1, 0};
    coord_t origin[3] = {5, 10, 15};
    coord_t scaling[3] = {1, 2, 3};
    CoordTransformAligned ct(4,3, dimToBinFrom, origin, scaling);

    coord_t input[4] = {16, 11, 11111111 /*ignored*/, 6};
    coord_t output[3] = {0,0,0};
    ct.apply(input, output);
    TS_ASSERT_DELTA( output[0], 1.0, 1e-6 );
    TS_ASSERT_DELTA( output[1], 2.0, 1e-6 );
    TS_ASSERT_DELTA( output[2], 3.0, 1e-6 );
  }


};


class CoordTransformAlignedTestPerformance : public CxxTest::TestSuite
{
public:
  void test_apply_3D_performance()
  {
    // Do a simple 3-3 transform.
    size_t dimToBinFrom[3] = {0, 1, 2};
    coord_t origin[3] = {5, 10, 15};
    coord_t scaling[3] = {1, 2, 3};
    CoordTransformAligned ct(3,3, dimToBinFrom, origin, scaling);

    coord_t in[3] = {1.5, 2.5, 3.5};
    coord_t out[3];

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, out);
    }
  }
  void test_apply_4D_performance()
  {
    // Do a simple 4-4 transform.
    size_t dimToBinFrom[4] = {0, 1, 2, 3};
    coord_t origin[4] = {5, 10, 15, 20};
    coord_t scaling[4] = {1, 2, 3, 4};
    CoordTransformAligned ct(4,4, dimToBinFrom, origin, scaling);

    coord_t in[4] = {1.5, 2.5, 3.5, 4.5};
    coord_t out[4];

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, out);
    }
  }

};
#endif /* MANTID_MDEVENTS_COORDTRANSFORMALIGNEDTEST_H_ */

