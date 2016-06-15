#ifndef IGNORE_ZEROS_THRESHOLD_RANGE_TEST_H_
#define IGNORE_ZEROS_THRESHOLD_RANGE_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"

using namespace Mantid;
using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class IgnoreZerosThresholdRangeTest : public CxxTest::TestSuite {

public:
  void testIgnoreEmptyCells() {
    IgnoreZerosThresholdRange range;

    TS_ASSERT_EQUALS(true, range.inRange(0.001));
    TS_ASSERT_EQUALS(true, range.inRange(-0.001));
    TS_ASSERT_EQUALS(false, range.inRange(0));
  }

  void testGetMinMax() {
    IgnoreZerosThresholdRange range;

    range.inRange(1);
    range.inRange(3);
    range.inRange(-2);
    range.inRange(5);
    TSM_ASSERT_EQUALS("Wrong max found", 5, range.getMaximum());
    TSM_ASSERT_EQUALS("Wrong min found", -2, range.getMinimum());
  }

  void testMinIsNeverZero() {
    IgnoreZerosThresholdRange range;

    range.inRange(0);
    range.inRange(0.5);
    range.inRange(2);
    TSM_ASSERT_EQUALS("Wrong min found", 0.5, range.getMinimum());
  }
};

#endif