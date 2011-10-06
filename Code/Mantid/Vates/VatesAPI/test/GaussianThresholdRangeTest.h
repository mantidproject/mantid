#ifndef GAUSSIAN_THRESHOLD_RANGE_TEST_H_
#define GAUSSIAN_THRESHOLD_RANGE_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidVatesAPI/GaussianThresholdRange.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class GaussianThresholdRangeTest: public CxxTest::TestSuite
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
    sptrWs->setSignalAt(0,2.0);
    sptrWs->setSignalAt(1,4);
    sptrWs->setSignalAt(2,4);
    sptrWs->setSignalAt(3,4);
    sptrWs->setSignalAt(4,5);
    sptrWs->setSignalAt(5,5);
    sptrWs->setSignalAt(6,7);
    sptrWs->setSignalAt(7,9);
  }

  void testWithDefaultSamplingApplied()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 1, 0); //1stdDev, 0 skips
    gaussianCalculator.calculate();
    TS_ASSERT_DELTA( sptrWs->getSignalNormalizedAt(0), 2.0, 1e-5);

    TS_ASSERT(gaussianCalculator.hasCalculated());
    TSM_ASSERT_EQUALS("Should be 1*sigma standard deviations from central value.", 3.5 + 2 + 2, gaussianCalculator.getMaximum());
    TSM_ASSERT_EQUALS("Should be 1*sigma standard deviations from central value.", 3.5 + 2 - 2, gaussianCalculator.getMinimum());

    //Boundary Value Analysis
    signal_t just_above_upper_boundary = 7.5001;
    signal_t just_below_lower_boundary = 3.4999;
    signal_t on_lower_boundary = 3.5;
    signal_t on_upper_boundary = 7.5;
    signal_t just_below_upper_boundary = 7.4999;
    signal_t just_above_lower_boundary = 3.5001;
    TS_ASSERT_EQUALS(false, gaussianCalculator.inRange(just_above_upper_boundary));
    TS_ASSERT_EQUALS(false, gaussianCalculator.inRange(just_below_lower_boundary));
    TS_ASSERT_EQUALS(true, gaussianCalculator.inRange(on_lower_boundary));
    TS_ASSERT_EQUALS(true, gaussianCalculator.inRange(on_upper_boundary));
    TS_ASSERT_EQUALS(true, gaussianCalculator.inRange(just_below_upper_boundary));
    TS_ASSERT_EQUALS(true, gaussianCalculator.inRange(just_above_lower_boundary));
  }

  void testWithHalfStdDev()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 0.5, 0); //Half stdDev, 0 skips
    gaussianCalculator.calculate();

    TSM_ASSERT_EQUALS("Should be 0.5*sigma standard deviations from central value.", 3.5 + 2 + 1, gaussianCalculator.getMaximum());
    TSM_ASSERT_EQUALS("Should be 0.5*sigma standard deviations from central value.", 3.5 + 2 - 1, gaussianCalculator.getMinimum());
  }

  void testWithEveryOtherCellSampled()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 1, 4); //1stdDev, skip every other cell.
    gaussianCalculator.calculate();
  }

  void testGetMaxWithoutCalculatingThrows()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 1, 1);
    TSM_ASSERT("Should indicate not calculated.", !gaussianCalculator.hasCalculated());
    TSM_ASSERT_THROWS("Should throw if :getMaximum() is requested without first calculating.", gaussianCalculator.getMaximum(), std::runtime_error);
  }

  void testGetMinWithoutCalculatingThrows()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 1, 1);
    TSM_ASSERT("Should indicate not calculated.", !gaussianCalculator.hasCalculated());
    TSM_ASSERT_THROWS("Should throw if :getMaximum() is requested without first calculating.", gaussianCalculator.getMinimum(), std::runtime_error);
  }

  void testSaturateIfTooManyStdevs()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 10, 0); //Ten stdDevs, 0 skips
    gaussianCalculator.calculate();
    TSM_ASSERT_EQUALS("Should have saturated to min signal.", 9, gaussianCalculator.getMaximum());
    TSM_ASSERT_EQUALS("Should have saturated to max signal.", 2, gaussianCalculator.getMinimum());
  }

  void testSettWorkspaceOnObject()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(1, 0); //1stdDev, 0 skips
    gaussianCalculator.setWorkspace(sptrWs);
    gaussianCalculator.calculate();
    TS_ASSERT(gaussianCalculator.hasCalculated());
    TSM_ASSERT_EQUALS("Should be 1*sigma standard deviations from central value.", 3.5 + 2 + 2, gaussianCalculator.getMaximum());
    TSM_ASSERT_EQUALS("Should be 1*sigma standard deviations from central value.", 3.5 + 2 - 2, gaussianCalculator.getMinimum());
  }

  void testCalculateWithoutWorkspaceThrows()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator; //No workspace provided!
    TSM_ASSERT_THROWS("Calling calculate without a workspace should throw", gaussianCalculator.calculate(), std::logic_error);
  }

  void testSetWorkspaceResetsCalculation()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 1, 0); //1stdDev, 0 skips
    gaussianCalculator.calculate();
    gaussianCalculator.setWorkspace(sptrWs);
    TSM_ASSERT("Setting a workspace should mark object as uncalculated.", !gaussianCalculator.hasCalculated());
  }

  void testIgnoreEmptyCells()
  {
    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs);
    gaussianCalculator.calculate();
    TSM_ASSERT_EQUALS("Should be 1*sigma standard deviations from central value.", 3.5 + 2 + 2, gaussianCalculator.getMaximum());
    TSM_ASSERT_EQUALS("Should be 1*sigma standard deviations from central value.", 3.5 + 2 - 2, gaussianCalculator.getMinimum());
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class GaussianThresholdRangeTestPerformance : public CxxTest::TestSuite
{
public:

  void testAnalyseLargeWorkspaceSampleEveryTen()
  {
    const size_t workspaceSize = 10000000; //Ten million cells
    Mantid::API::IMDWorkspace_sptr sptrWs = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, workspaceSize);

    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 10, 10); //Ten stdDevs, 10 skips
    gaussianCalculator.calculate();

    gaussianCalculator.getMaximum();
    gaussianCalculator.getMinimum();
  }

  void testAnalyseLargeWorkspaceSampleEveryTenThousand()
  {
    const size_t workspaceSize = 10000000; //Ten million cells
    Mantid::API::IMDWorkspace_sptr sptrWs = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, workspaceSize);

    Mantid::VATES::GaussianThresholdRange gaussianCalculator(sptrWs, 10, 10000); //Ten stdDevs, 10'000 skips
    gaussianCalculator.calculate();

    gaussianCalculator.getMaximum();
    gaussianCalculator.getMinimum();
  }

};

#endif
