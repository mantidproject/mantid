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
    EXPECT_CALL(mockView, getTransmissionLambdaMin())
        .Times(Exactly(1))
        .WillOnce(Return("1.0"));
    EXPECT_CALL(mockView, getTransmissionLambdaMax())
        .Times(Exactly(1))
        .WillOnce(Return("15.0"));
    EXPECT_CALL(mockView, getBinningParameters())
        .Times(Exactly(1))
        .WillOnce(Return("\"1.5,0.02,17\""));
    auto options = presenter.getTransmissionOptions();

    auto optionsVec = split_comma_no_quotes(options);
    for (int i = 0; i < optionsVec.size(); i++) {
      std::cout << optionsVec[i] << "\n";
    }

    TS_ASSERT_EQUALS(optionsVec[0], "AnalysisMode=MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(optionsVec[1], "WavelengthMin=1.0");
    TS_ASSERT_EQUALS(optionsVec[2], "WavelengthMax=15.0");
    TS_ASSERT_EQUALS(optionsVec[3], "Params=1.5,0.02,17");

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
    EXPECT_CALL(mockView, getBinningParameters())
        .Times(Exactly(1))
        .WillOnce(Return("\"1.5,0.02,17\""));
    auto options = presenter.getReductionOptions();

    auto optionsVec = split_comma_no_quotes(options);
    TS_ASSERT_EQUALS(optionsVec[0], "AnalysisMode=MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(optionsVec[1], "CRho=2.5");
    TS_ASSERT_EQUALS(optionsVec[2], "CAlpha=0.6");
    TS_ASSERT_EQUALS(optionsVec[3], "CAp=100.0");
    TS_ASSERT_EQUALS(optionsVec[4], "CPp=0.54");
    TS_ASSERT_EQUALS(optionsVec[5], "Params=1.5,0.02,17");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testStitchOptions() {
    MockSettingsTabView mockView;
    ReflSettingsTabPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(1));
    presenter.getStitchOptions();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

private:
  /**
   * Splits a string by comma except in quotes
   */
  std::vector<std::string> split_comma_no_quotes(const std::string &str)
  {
    std::vector<std::string> tokens;
    boost::tokenizer<boost::escaped_list_separator<char>> 
        tok(str, boost::escaped_list_separator<char>());

    for (auto it = tok.begin(); it != tok.end(); ++it) {
      tokens.push_back(*it);
    }

    return tokens;
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTERTEST_H */
