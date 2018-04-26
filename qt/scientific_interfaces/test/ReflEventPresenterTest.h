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

  void testDefaultGetSlicingValues() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getUniformEvenTimeSlicingValues()).Times(Exactly(1));
    presenter.getTimeSlicingValues();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetSlicingType() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);
    presenter.notifySliceTypeChanged(SliceType::LogValue);
    TS_ASSERT_EQUALS("LogValue", presenter.getTimeSlicingType());
  }

  void testDisablesControlsOnReductionResumed() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);
    EXPECT_CALL(mockView, disableSliceType(_)).Times(AtLeast(1));
    EXPECT_CALL(mockView, disableSliceTypeSelection()).Times(AtLeast(1));

    presenter.onReductionResumed();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testDisablesCorrectControlsOnReductionResumed() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);
    presenter.notifySliceTypeChanged(SliceType::Custom);
    EXPECT_CALL(mockView, disableSliceType(SliceType::Custom))
        .Times(AtLeast(1));

    presenter.onReductionResumed();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testEnablesControlsOnReductionPaused() {
    MockEventView mockView;
    ReflEventPresenter presenter(&mockView);
    EXPECT_CALL(mockView, enableSliceType(SliceType::UniformEven))
        .Times(AtLeast(1));

    presenter.onReductionPaused();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H */
