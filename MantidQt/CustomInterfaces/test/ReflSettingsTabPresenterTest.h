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
  bool operator()(char c) const {
    if (c == '\"')
      in_q = !in_q;
    return !in_q && c == ',';
  }
};
}

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
    EXPECT_CALL(mockView, getMonitorIntegralMin())
        .Times(Exactly(1))
        .WillOnce(Return("4"));
    EXPECT_CALL(mockView, getMonitorIntegralMax())
        .Times(Exactly(1))
        .WillOnce(Return("10"));
    EXPECT_CALL(mockView, getMonitorBackgroundMin())
        .Times(Exactly(1))
        .WillOnce(Return("12"));
    EXPECT_CALL(mockView, getMonitorBackgroundMax())
        .Times(Exactly(1))
        .WillOnce(Return("17"));
    EXPECT_CALL(mockView, getLambdaMin())
        .Times(Exactly(1))
        .WillOnce(Return("1"));
    EXPECT_CALL(mockView, getLambdaMax())
        .Times(Exactly(1))
        .WillOnce(Return("15"));
    EXPECT_CALL(mockView, getI0MonitorIndex())
        .Times(Exactly(1))
        .WillOnce(Return("2"));
    EXPECT_CALL(mockView, getDetectorLimits())
        .Times(Exactly(1))
        .WillOnce(Return("\"3,4\""));
    auto options = presenter.getTransmissionOptions();

    std::vector<std::string> optionsVec;
    boost::split(optionsVec, options, split_q());
    TS_ASSERT_EQUALS(optionsVec[0], "AnalysisMode=MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(optionsVec[1], "MonitorIntegrationWavelengthMin=4");
    TS_ASSERT_EQUALS(optionsVec[2], "MonitorIntegrationWavelengthMax=10");
    TS_ASSERT_EQUALS(optionsVec[3], "MonitorBackgroundWavelengthMin=12");
    TS_ASSERT_EQUALS(optionsVec[4], "MonitorBackgroundWavelengthMax=17");
    TS_ASSERT_EQUALS(optionsVec[5], "WavelengthMin=1");
    TS_ASSERT_EQUALS(optionsVec[6], "WavelengthMax=15");
    TS_ASSERT_EQUALS(optionsVec[7], "I0MonitorIndex=2");
    TS_ASSERT_EQUALS(optionsVec[8], "ProcessingInstructions=\"3,4\"");

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
    EXPECT_CALL(mockView, getPolarisationCorrections())
        .Times(Exactly(1))
        .WillOnce(Return("PNR"));
    EXPECT_CALL(mockView, getMonitorIntegralMin())
        .Times(Exactly(1))
        .WillOnce(Return("4"));
    EXPECT_CALL(mockView, getMonitorIntegralMax())
        .Times(Exactly(1))
        .WillOnce(Return("10"));
    EXPECT_CALL(mockView, getMonitorBackgroundMin())
        .Times(Exactly(1))
        .WillOnce(Return("12"));
    EXPECT_CALL(mockView, getMonitorBackgroundMax())
        .Times(Exactly(1))
        .WillOnce(Return("17"));
    EXPECT_CALL(mockView, getLambdaMin())
        .Times(Exactly(1))
        .WillOnce(Return("1"));
    EXPECT_CALL(mockView, getLambdaMax())
        .Times(Exactly(1))
        .WillOnce(Return("15"));
    EXPECT_CALL(mockView, getI0MonitorIndex())
        .Times(Exactly(1))
        .WillOnce(Return("2"));
    EXPECT_CALL(mockView, getScaleFactor())
        .Times(Exactly(1))
        .WillOnce(Return("2"));
    EXPECT_CALL(mockView, getMomentumTransferStep())
        .Times(Exactly(1))
        .WillOnce(Return("-0.02"));
    EXPECT_CALL(mockView, getDetectorLimits())
        .Times(Exactly(1))
        .WillOnce(Return("\"3,4\""));
    auto options = presenter.getReductionOptions();

    std::vector<std::string> optionsVec;
    boost::split(optionsVec, options, split_q());
    TS_ASSERT_EQUALS(optionsVec[0], "AnalysisMode=MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(optionsVec[1], "CRho=2.5");
    TS_ASSERT_EQUALS(optionsVec[2], "CAlpha=0.6");
    TS_ASSERT_EQUALS(optionsVec[3], "CAp=100.0");
    TS_ASSERT_EQUALS(optionsVec[4], "CPp=0.54");
    TS_ASSERT_EQUALS(optionsVec[5], "RegionOfDirectBeam=\"0,3\"");
    TS_ASSERT_EQUALS(optionsVec[6], "PolarizationAnalysis=PNR");
    TS_ASSERT_EQUALS(optionsVec[7], "MonitorIntegrationWavelengthMin=4");
    TS_ASSERT_EQUALS(optionsVec[8], "MonitorIntegrationWavelengthMax=10");
    TS_ASSERT_EQUALS(optionsVec[9], "MonitorBackgroundWavelengthMin=12");
    TS_ASSERT_EQUALS(optionsVec[10], "MonitorBackgroundWavelengthMax=17");
    TS_ASSERT_EQUALS(optionsVec[11], "WavelengthMin=1");
    TS_ASSERT_EQUALS(optionsVec[12], "WavelengthMax=15");
    TS_ASSERT_EQUALS(optionsVec[13], "I0MonitorIndex=2");
    TS_ASSERT_EQUALS(optionsVec[14], "ScaleFactor=2");
    TS_ASSERT_EQUALS(optionsVec[15], "MomentumTransferStep=-0.02");
    TS_ASSERT_EQUALS(optionsVec[16], "MomentumTransferMaximum=0.2");
    TS_ASSERT_EQUALS(optionsVec[17], "ProcessingInstructions=\"3,4\"");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testStitchOptions() {
    MockSettingsTabView mockView;
    ReflSettingsTabPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(1));
    presenter.getStitchOptions();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testExperimentDefaults() {
    MockSettingsTabView mockView;
    ReflSettingsTabPresenter presenter(&mockView);

    std::vector<std::string> defaults = {"PointDetectorAnalysis", "None", "1"};

    EXPECT_CALL(mockView, setExpDefaults(defaults)).Times(1);
    presenter.notify(IReflSettingsTabPresenter::ExpDefaultsFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testInstrumentDefaults() {
    MockSettingsTabView mockView;
    MockMainWindowPresenter mainPresenter;
    ReflSettingsTabPresenter presenter(&mockView);

    // This presenter accepts the main presenter
    presenter.acceptMainPresenter(&mainPresenter);

    std::vector<double> defaults = {4.0, 10., 15., 17., 1.0, 17., 2.0};

    EXPECT_CALL(mainPresenter, getInstrumentName())
        .Times(1)
        .WillOnce(Return("INTER"));
    EXPECT_CALL(mockView, setInstDefaults(defaults)).Times(1);
    presenter.notify(IReflSettingsTabPresenter::InstDefaultsFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTERTEST_H */
