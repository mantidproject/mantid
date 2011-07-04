#ifndef USER_DEFINED_THRESHOLD_RANGE_TEST_H_
#define USER_DEFINED_THRESHOLD_RANGE_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/UserDefinedThresholdRange.h"

using namespace Mantid::VATES;

class UserDefinedThresholdRangeTest: public CxxTest::TestSuite
{
public :

  void testConstructMaxLessThanMinThrows()
  {
    TSM_ASSERT_THROWS("Should not be able to construct with max less than min.", UserDefinedThresholdRange(2, 1), std::invalid_argument);
  }
 
  void testGetMaximum()
  {
    Mantid::VATES::UserDefinedThresholdRange userRangeCalculator(1, 2);
    TSM_ASSERT_EQUALS("::getMaximum not wired-up correctly.", 2, userRangeCalculator.getMaximum());
  }

  void testGetMinimum()
  {
    Mantid::VATES::UserDefinedThresholdRange userRangeCalculator(1, 2);
    TSM_ASSERT_EQUALS("::getMinimum not wired-up correctly.", 1, userRangeCalculator.getMinimum());
  }

};

#endif