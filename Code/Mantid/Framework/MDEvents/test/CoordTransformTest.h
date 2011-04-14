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
  void compare(size_t numdims, CoordType * value, const CoordType * expected)
  {
    for (size_t i=0; i< numdims; i++)
      TS_ASSERT_DELTA( value[i], expected[i], 1e-5);
  }

  /** Simple identity transform */
  void test_donothing()
  {
    CoordType in[2] = {1.5, 2.5};
    CoordType out[2];
    CoordTransform ct(2,2); // defaults to identity
    ct.apply(in, out);
    compare(2, out, in);
  }

  /** Translate in 2D */
  void test_translate()
  {
    CoordType in[2] = {1.5, 2.5};
    CoordType out[2];
    CoordType translation[2] = {2.0, 3.0};
    CoordType expected[2] = {3.5, 5.5};
    CoordTransform ct(2,2);
    ct.addTranslation(translation);
    ct.apply(in, out);
    compare(2, out, expected);
  }


};


#endif /* MANTID_MDEVENTS_COORDTRANSFORMTEST_H_ */

