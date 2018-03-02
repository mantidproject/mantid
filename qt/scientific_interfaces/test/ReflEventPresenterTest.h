#ifndef MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflEventPresenter.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflEventPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflEventPresenterTest *createSuite() {
    return new ReflEventPresenterTest();
  }
  static void destroySuite(ReflEventPresenterTest *suite) { delete suite; }

  ReflEventPresenterTest() {}

  void test_get_slicing_values() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getTimeSlicingValues()).Times(Exactly(1));
    presenter.getTimeSlicingValues();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_get_slicing_type() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getTimeSlicingType()).Times(Exactly(1));
    presenter.getTimeSlicingType();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testDisablesControlsOnReductionResumed() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);
    EXPECT_CALL(mockView, disableAll()).Times(AtLeast(1));

    presenter.onReductionResumed();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testEnabledControlsOnReductionPaused() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);
    EXPECT_CALL(mockView, enableAll()).Times(AtLeast(1));

    presenter.onReductionPaused();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H */
