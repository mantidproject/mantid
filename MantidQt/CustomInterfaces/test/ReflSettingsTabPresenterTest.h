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

namespace {
  class split_q {
  private:
    mutable bool in_q;
  public:
    split_q() : in_q(false) {}
    bool operator() (char c) const
    {
      if (c == '\"') in_q = !in_q;
      return !in_q && c == ',';
    }
  };
};

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflSettingsTabPresenterTest : public CxxTest::TestSuite {

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

    EXPECT_CALL(mockView, getTransmissionOptions())
        .Times(Exactly(1))
        .WillOnce(Return(""));
    EXPECT_CALL(mockView, getAnalysisMode())
        .Times(Exactly(1))
        .WillOnce(Return("MultiDetectorAnalysis"));
    auto options = presenter.getTransmissionOptions();

    std::vector<std::string> optionsVec;
    boost::split(optionsVec, options, split_q());
    TS_ASSERT_EQUALS(optionsVec[0], "AnalysisMode=MultiDetectorAnalysis");

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
    EXPECT_CALL(mockView, getCRho()).Times(Exactly(1)).WillOnce(Return("2.5"));
    EXPECT_CALL(mockView, getCAlpha())
        .Times(Exactly(1))
        .WillOnce(Return("0.6"));
    EXPECT_CALL(mockView, getCAp()).Times(Exactly(1)).WillOnce(Return("100.0"));
    EXPECT_CALL(mockView, getCPp()).Times(Exactly(1)).WillOnce(Return("0.54"));
    EXPECT_CALL(mockView, getDirectBeam())
        .Times(Exactly(1))
        .WillOnce(Return("\"0,3\""));
    auto options = presenter.getReductionOptions();

    std::vector<std::string> optionsVec;
    boost::split(optionsVec, options, split_q());
    TS_ASSERT_EQUALS(optionsVec[0], "AnalysisMode=MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(optionsVec[1], "CRho=2.5");
    TS_ASSERT_EQUALS(optionsVec[2], "CAlpha=0.6");
    TS_ASSERT_EQUALS(optionsVec[3], "CAp=100.0");
    TS_ASSERT_EQUALS(optionsVec[4], "CPp=0.54");
    TS_ASSERT_EQUALS(optionsVec[5], "RegionOfDirectBeam=\"0,3\"");

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
