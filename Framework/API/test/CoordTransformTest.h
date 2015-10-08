#ifndef MANTID_MDEVENTS_COORDTRANSFORMTEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/CoordTransform.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;

class CoordTransformTest : public CxxTest::TestSuite {
public:
  void test_nothing() {
    /// Abstract class, nothing much to test.
  }
};

#endif /* MANTID_MDEVENTS_COORDTRANSFORMTEST_H_ */
