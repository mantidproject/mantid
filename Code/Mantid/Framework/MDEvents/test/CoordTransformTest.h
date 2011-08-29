#ifndef MANTID_MDEVENTS_COORDTRANSFORMTEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/CoordTransform.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::MDEvents;

class CoordTransformTest : public CxxTest::TestSuite
{
public:
  void test_nothing()
  {
    /// Abstract class, nothing much to test.
  }
};

#endif /* MANTID_MDEVENTS_COORDTRANSFORMTEST_H_ */

