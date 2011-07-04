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

  void testHasCalculated()
  {
    Mantid::VATES::UserDefinedThresholdRange userRangeCalculator(1, 2);
    TS_ASSERT(userRangeCalculator.hasCalculated()); //Should return true no matter what!
  }

  void testClone()
  {
    Mantid::VATES::UserDefinedThresholdRange original(1, 2);
    Mantid::VATES::UserDefinedThresholdRange* cloned = original.clone();

    TS_ASSERT_EQUALS(original.getMaximum(), cloned->getMaximum());
    TS_ASSERT_EQUALS(original.getMinimum(), cloned->getMinimum());

    delete cloned;
  }

};

#endif