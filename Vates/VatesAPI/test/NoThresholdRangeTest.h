#ifndef NO_THRESHOLD_RANGE_TEST_H_
#define NO_THRESHOLD_RANGE_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidVatesAPI/NoThresholdRange.h"

using namespace Mantid;
using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class NoThresholdRangeTest : public CxxTest::TestSuite {
public:
  void testEverythingWithinRange() {
    NoThresholdRange range;

    TS_ASSERT_EQUALS(true, range.inRange(-1e9));
    TS_ASSERT_EQUALS(true, range.inRange(0));
    TS_ASSERT_EQUALS(true, range.inRange(1e9));
  }

  void testGetMinMax() {
    NoThresholdRange range;

    range.inRange(1);
    range.inRange(3);
    range.inRange(-2);
    range.inRange(5);
    TSM_ASSERT_EQUALS("Wrong max found", 5, range.getMaximum());
    TSM_ASSERT_EQUALS("Wrong min found", -2, range.getMinimum());
  }
};

#endif