#ifndef MD_REBINNING_VIEW_ADAPTER_TEST_H
#define MD_REBINNING_VIEW_ADAPTER_TEST_H

#include "MantidKernel/V3D.h"
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
    Mantid::Kernel::V3D v;
    //Set expectations on adaptee
    MockMDRebinningView view;
    EXPECT_CALL(view, getMaxThreshold()).Times(1);
    EXPECT_CALL(view, getMinThreshold()).Times(1);
    EXPECT_CALL(view, getApplyClip()).Times(1);
    EXPECT_CALL(view, getTimeStep()).Times(1);
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(1);
    EXPECT_CALL(view, updateAlgorithmProgress(_)).Times(1);
    EXPECT_CALL(view, getOrigin()).Times(1).WillOnce(Return(v));
    EXPECT_CALL(view, getB1()).Times(1).WillOnce(Return(v));
    EXPECT_CALL(view, getB2()).Times(1).WillOnce(Return(v));
    EXPECT_CALL(view, getLengthB1()).Times(1);
    EXPECT_CALL(view, getLengthB2()).Times(1);
    EXPECT_CALL(view, getLengthB3()).Times(1);
    EXPECT_CALL(view, getForceOrthogonal()).Times(1);
    EXPECT_CALL(view, getOutputHistogramWS()).Times(1);

    //Create adapter using adaptee
    MDRebinningViewAdapter<MockMDRebinningView> view_adapter(&view);

    //Use an alias to ensure that adapting to the right type.
    MDRebinningView& alias = view_adapter;
    
    //Test running adaptees invokes expecations and exits cleanly
    TS_ASSERT_THROWS_NOTHING(alias.getMaxThreshold());
    TS_ASSERT_THROWS_NOTHING(alias.getMinThreshold());
    TS_ASSERT_THROWS_NOTHING(alias.getApplyClip());
    TS_ASSERT_THROWS_NOTHING(alias.getTimeStep());
    TS_ASSERT_THROWS_NOTHING(alias.getAppliedGeometryXML());
    TS_ASSERT_THROWS_NOTHING(alias.updateAlgorithmProgress(0));
    TS_ASSERT_THROWS_NOTHING(alias.getLengthB1());
    TS_ASSERT_THROWS_NOTHING(alias.getLengthB2());
    TS_ASSERT_THROWS_NOTHING(alias.getLengthB3());
    TS_ASSERT_THROWS_NOTHING(alias.getB1());
    TS_ASSERT_THROWS_NOTHING(alias.getB2());
    TS_ASSERT_THROWS_NOTHING(alias.getOrigin());
    TS_ASSERT_THROWS_NOTHING(alias.getForceOrthogonal());
    TS_ASSERT_THROWS_NOTHING(alias.getOutputHistogramWS());

    //Check the expectations.
    TSM_ASSERT("Not wired-up correctly", Mock::VerifyAndClearExpectations(&view));
  }

};

#endif