#ifndef MANTID_MDEVENTS_COORDTRANSFORMTEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMTEST_H_

#include "MantidAPI/CoordTransform.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;

class CoordTransformTest : public CxxTest::TestSuite {
public:
  void test_nothing() {
    /// Abstract class, nothing much to test.
  }
};

#endif /* MANTID_MDEVENTS_COORDTRANSFORMTEST_H_ */
