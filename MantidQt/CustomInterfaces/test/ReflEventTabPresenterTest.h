#ifndef MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflEventTabPresenter.h"
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

  void test_slicing_options() {
    MockEventPresenter presenter_1;
    MockEventPresenter presenter_2;
    std::vector<IReflEventPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflEventTabPresenter presenter(settingsPresenters);

    EXPECT_CALL(presenter_1, getTimeSlicingOptions()).Times(1);
    EXPECT_CALL(presenter_2, getTimeSlicingOptions()).Times(0);
    presenter.getTimeSlicingOptions(0);
    EXPECT_CALL(presenter_1, getTimeSlicingOptions()).Times(0);
    EXPECT_CALL(presenter_2, getTimeSlicingOptions()).Times(1);
    presenter.getTimeSlicingOptions(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H */
