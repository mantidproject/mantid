#ifndef MD_REBINNING_VIEW_ADAPTER_TEST_H
#define MD_REBINNING_VIEW_ADAPTER_TEST_H

#include "MantidVatesAPI/MDRebinningViewAdapter.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h> 
#include "MockObjects.h"

using namespace testing;
using namespace Mantid::VATES;

class MDRebinningViewAdapterTest : public CxxTest::TestSuite
{

public:
  
  void testWireUp()
  {
    //Set expectations on adaptee
    MockMDRebinningView view;
    EXPECT_CALL(view, getImplicitFunction()).Times(1);
    EXPECT_CALL(view, getWidth()).Times(1);
    EXPECT_CALL(view, getMaxThreshold()).Times(1);
    EXPECT_CALL(view, getMinThreshold()).Times(1);
    EXPECT_CALL(view, getApplyClip()).Times(1);
    EXPECT_CALL(view, getTimeStep()).Times(1);
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(1);
    EXPECT_CALL(view, updateAlgorithmProgress(_)).Times(1);

    //Create adapter using adaptee
    MDRebinningViewAdapter<MockMDRebinningView> view_adapter(&view);

    //Use an alias to ensure that adapting to the right type.
    MDRebinningView& alias = view_adapter;
    
    //Test running adaptees invokes expecations and exits cleanly
    TS_ASSERT_THROWS_NOTHING(alias.getImplicitFunction());
    TS_ASSERT_THROWS_NOTHING(alias.getWidth());
    TS_ASSERT_THROWS_NOTHING(alias.getMaxThreshold());
    TS_ASSERT_THROWS_NOTHING(alias.getMinThreshold());
    TS_ASSERT_THROWS_NOTHING(alias.getApplyClip());
    TS_ASSERT_THROWS_NOTHING(alias.getTimeStep());
    TS_ASSERT_THROWS_NOTHING(alias.getAppliedGeometryXML());
    TS_ASSERT_THROWS_NOTHING(alias.updateAlgorithmProgress(0));

    //Check the expectations.
    TSM_ASSERT("Not wired-up correctly", Mock::VerifyAndClearExpectations(&view));
  }

};

#endif