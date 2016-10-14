#ifndef MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsTabPresenter.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflSettingsTabPresenterTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflSettingsTabPresenterTest *createSuite() {
    return new ReflSettingsTabPresenterTest();
  }
  static void destroySuite(ReflSettingsTabPresenterTest *suite) {
    delete suite;
  }

  ReflSettingsTabPresenterTest() { FrameworkManager::Instance(); }

  void testGetPlusOptions() {
    MockSettingsTabView mockView;
    ReflSettingsTabPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getPlusOptions()).Times(Exactly(1));
    presenter.getPlusOptions();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetTransmissionOptions() {
    MockSettingsTabView mockView;
    ReflSettingsTabPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getTransmissionOptions()).Times(Exactly(1));
    presenter.getTransmissionOptions();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetReductionOptions() {
    MockSettingsTabView mockView;
    ReflSettingsTabPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getReductionOptions())
        .Times(Exactly(1))
        .WillOnce(Return(""));
    EXPECT_CALL(mockView, getAnalysisMode())
        .Times(Exactly(1))
        .WillOnce(Return("MultiDetectorAnalysis"));
    auto options = presenter.getReductionOptions();

    TS_ASSERT_EQUALS(options, "AnalysisMode=MultiDetectorAnalysis");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testStitchOptions() {
    MockSettingsTabView mockView;
    ReflSettingsTabPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(1));
    presenter.getStitchOptions();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTERTEST_H */
