// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflEventTabPresenter.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflEventTabPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflEventTabPresenterTest *createSuite() {
    return new ReflEventTabPresenterTest();
  }
  static void destroySuite(ReflEventTabPresenterTest *suite) { delete suite; }

  ReflEventTabPresenterTest() {}

  void test_get_slicing_values() {
    MockEventPresenter presenter_1;
    MockEventPresenter presenter_2;
    std::vector<IReflEventPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflEventTabPresenter presenter(settingsPresenters);

    EXPECT_CALL(presenter_1, getTimeSlicingValues()).Times(1);
    EXPECT_CALL(presenter_2, getTimeSlicingValues()).Times(0);
    presenter.getTimeSlicingValues(0);
    EXPECT_CALL(presenter_1, getTimeSlicingValues()).Times(0);
    EXPECT_CALL(presenter_2, getTimeSlicingValues()).Times(1);
    presenter.getTimeSlicingValues(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void testPassesMessageToCorrectChildPresenterWhenReductionPaused() {
    MockEventPresenter presenter_1;
    MockEventPresenter presenter_2;
    std::vector<IReflEventPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflEventTabPresenter presenter(settingsPresenters);
    EXPECT_CALL(presenter_2, onReductionPaused());

    presenter.onReductionPaused(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void testPassesMessageToCorrectChildPresenterWhenReductionResumed() {
    MockEventPresenter presenter_1;
    MockEventPresenter presenter_2;
    std::vector<IReflEventPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflEventTabPresenter presenter(settingsPresenters);
    EXPECT_CALL(presenter_1, onReductionResumed());

    presenter.onReductionResumed(0);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void testGetSlicingType() {
    MockEventPresenter presenter_1;
    MockEventPresenter presenter_2;
    std::vector<IReflEventPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflEventTabPresenter presenter(settingsPresenters);

    EXPECT_CALL(presenter_1, getTimeSlicingType()).Times(1);
    EXPECT_CALL(presenter_2, getTimeSlicingType()).Times(0);
    presenter.getTimeSlicingType(0);
    EXPECT_CALL(presenter_1, getTimeSlicingType()).Times(0);
    EXPECT_CALL(presenter_2, getTimeSlicingType()).Times(1);
    presenter.getTimeSlicingType(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H */
