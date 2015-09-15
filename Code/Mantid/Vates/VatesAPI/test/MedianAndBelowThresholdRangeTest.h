#ifndef MEDIAN_AND_BELOW_THRESHOLD_RANGE_TEST_H_
#define MEDIAN_AND_BELOW_THRESHOLD_RANGE_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidVatesAPI/MedianAndBelowThresholdRange.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidDataObjects/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================

class MedianAndBelowThresholdRangeTest: public CxxTest::TestSuite
{
private:

  // Fake workspace
  MDHistoWorkspace_sptr sptrWs;

public :

  void setUp()
  {
    // Fake workspace with 8 cells
    sptrWs = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, 8, 8.0);
    //Set up a standard set of values for subsequent tests. Note that the following set gives a standard deviation of +/-2
    sptrWs->setSignalAt(0,-1.0);
    sptrWs->setSignalAt(1,2);
    sptrWs->setSignalAt(2,2);
    sptrWs->setSignalAt(3,3);
    sptrWs->setSignalAt(4,4);
    sptrWs->setSignalAt(5,5);
    sptrWs->setSignalAt(6,6);
    sptrWs->setSignalAt(7,7);
  }

  void testMedianCalculation()
  {
    Mantid::VATES::MedianAndBelowThresholdRange medianCalculator;
    medianCalculator.setWorkspace(sptrWs);
    medianCalculator.calculate();
    //-1 + 2 + 2 + 3 + 4 + 5 + 6 + 7 / 8 = 3.5

    TSM_ASSERT_EQUALS("Wrong maximum value.", 3.5, medianCalculator.getMaximum());
    TSM_ASSERT_EQUALS("Wrong minimum value.", -1, medianCalculator.getMinimum());
  }

  void testInRange()
  {
    Mantid::VATES::MedianAndBelowThresholdRange medianCalculator;
    medianCalculator.setWorkspace(sptrWs);
    medianCalculator.calculate();
    //-1 + 2 + 2 + 3 + 4 + 5 + 6 + 7 / 8 = 3.5

    TS_ASSERT_EQUALS(true, medianCalculator.inRange(3.499));
    TS_ASSERT_EQUALS(false, medianCalculator.inRange(3.501));
  }

};

#endif
