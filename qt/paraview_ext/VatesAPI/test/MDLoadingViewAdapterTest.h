#ifndef MD_LOADING_VIEW_ADAPTER_TEST_H
#define MD_LOADING_VIEW_ADAPTER_TEST_H

#include "MantidVatesAPI/MDLoadingViewAdapter.h"

#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::VATES;
using namespace testing;

class MDLoadingViewAdapterTest : public CxxTest::TestSuite {

public:
  void testWireUp() {
    // Set expectations on adaptee
    MockMDLoadingView view;
    EXPECT_CALL(view, getTime()).Times(1);
    EXPECT_CALL(view, getRecursionDepth()).Times(1);
    EXPECT_CALL(view, getLoadInMemory()).Times(1);

    // Create adapter using adaptee
    MDLoadingViewAdapter<MockMDLoadingView> view_adapter(&view);

    // Use an alias to ensure that adapting to the right type.
    MDLoadingView &alias = view_adapter;

    // Test running adaptees invokes expecations and exits cleanly
    TS_ASSERT_THROWS_NOTHING(alias.getTime());
    TS_ASSERT_THROWS_NOTHING(alias.getRecursionDepth());
    TS_ASSERT_THROWS_NOTHING(alias.getLoadInMemory());

    // Check the expectations.
    TSM_ASSERT("Not wired-up correctly",
               Mock::VerifyAndClearExpectations(&view));
  }
};

#endif