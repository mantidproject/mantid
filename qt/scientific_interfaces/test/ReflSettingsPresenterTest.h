#ifndef MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "../ISISReflectometry/ReflSettingsPresenter.h"
#include "ReflMockObjects.h"
#include <boost/algorithm/string.hpp>

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
class ReflSettingsPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflSettingsPresenterTest *createSuite() {
    return new ReflSettingsPresenterTest();
  }
  static void destroySuite(ReflSettingsPresenterTest *suite) { delete suite; }

  ReflSettingsPresenterTest() { FrameworkManager::Instance(); }

  void testGetTransmissionOptions() {
    MockSettingsView mockView;
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mockView, instrumentSettingsEnabled())
        .Times(1)
        .WillOnce(Return(true));
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
    EXPECT_CALL(mockView, getProcessingInstructions())
        .Times(Exactly(1))
        .WillOnce(Return("3,4"));
    EXPECT_CALL(mockView, getStartOverlap())
        .Times(Exactly(1))
        .WillOnce(Return("10"));
    EXPECT_CALL(mockView, getEndOverlap())
        .Times(Exactly(1))
        .WillOnce(Return("12"));
    EXPECT_CALL(mockView, getTransmissionRuns())
        .Times(Exactly(1))
        .WillOnce(Return("INTER00013463,INTER00013464"));

    auto options = presenter.getTransmissionOptions();
    TS_ASSERT_EQUALS(options.size(), 12);
    TS_ASSERT_EQUALS(options["AnalysisMode"].toStdString(),
                     "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(options["StartOverlap"].toStdString(), "10");
    TS_ASSERT_EQUALS(options["EndOverlap"].toStdString(), "12");
    TS_ASSERT_EQUALS(options["FirstTransmissionRun"].toStdString(),
                     "INTER00013463,INTER00013464");
    TS_ASSERT_EQUALS(options["MonitorIntegrationWavelengthMin"].toStdString(),
                     "4");
    TS_ASSERT_EQUALS(options["MonitorIntegrationWavelengthMax"].toStdString(),
                     "10");
    TS_ASSERT_EQUALS(options["MonitorBackgroundWavelengthMin"].toStdString(),
                     "12");
    TS_ASSERT_EQUALS(options["MonitorBackgroundWavelengthMax"].toStdString(),
                     "17");
    TS_ASSERT_EQUALS(options["WavelengthMin"].toStdString(), "1");
    TS_ASSERT_EQUALS(options["WavelengthMax"].toStdString(), "15");
    TS_ASSERT_EQUALS(options["I0MonitorIndex"].toStdString(), "2");
    TS_ASSERT_EQUALS(options["ProcessingInstructions"].toStdString(), "3,4");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetReductionOptions() {
    MockSettingsView mockView;
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mockView, instrumentSettingsEnabled())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mockView, getAnalysisMode())
        .Times(Exactly(1))
        .WillOnce(Return("MultiDetectorAnalysis"));
    EXPECT_CALL(mockView, getCRho())
        .Times(Exactly(1))
        .WillOnce(Return("2.5,0.4,1.1"));
    EXPECT_CALL(mockView, getCAlpha())
        .Times(Exactly(1))
        .WillOnce(Return("0.6,0.9,1.2"));
    EXPECT_CALL(mockView, getCAp())
        .Times(Exactly(1))
        .WillOnce(Return("100.0,17.0,44.0"));
    EXPECT_CALL(mockView, getCPp())
        .Times(Exactly(1))
        .WillOnce(Return("0.54,0.33,1.81"));
    EXPECT_CALL(mockView, getDirectBeam())
        .Times(Exactly(1))
        .WillOnce(Return("0,3"));
    EXPECT_CALL(mockView, getPolarisationCorrections())
        .Times(Exactly(1))
        .WillOnce(Return("PNR"));
    EXPECT_CALL(mockView, getIntMonCheck())
        .Times(Exactly(1))
        .WillOnce(Return("True"));
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
    EXPECT_CALL(mockView, getProcessingInstructions())
        .Times(Exactly(1))
        .WillOnce(Return("3,4"));
    EXPECT_CALL(mockView, getDetectorCorrectionType())
        .Times(Exactly(1))
        .WillOnce(Return("VerticalShift"));
    EXPECT_CALL(mockView, getTransmissionRuns())
        .Times(Exactly(1))
        .WillOnce(Return("INTER00013463,INTER00013464"));
    EXPECT_CALL(mockView, getStartOverlap())
        .Times(Exactly(1))
        .WillOnce(Return("10"));
    EXPECT_CALL(mockView, getEndOverlap())
        .Times(Exactly(1))
        .WillOnce(Return("12"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(options.size(), 22);
    TS_ASSERT_EQUALS(options["AnalysisMode"].toStdString(),
                     "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(options["CRho"].toStdString(), "2.5,0.4,1.1");
    TS_ASSERT_EQUALS(options["CAlpha"].toStdString(), "0.6,0.9,1.2");
    TS_ASSERT_EQUALS(options["CAp"].toStdString(), "100.0,17.0,44.0");
    TS_ASSERT_EQUALS(options["CPp"].toStdString(), "0.54,0.33,1.81");
    TS_ASSERT_EQUALS(options["RegionOfDirectBeam"].toStdString(), "0,3");
    TS_ASSERT_EQUALS(options["PolarizationAnalysis"].toStdString(), "PNR");
    TS_ASSERT_EQUALS(options["ScaleFactor"].toStdString(), "2");
    TS_ASSERT_EQUALS(options["MomentumTransferStep"].toStdString(), "-0.02");
    TS_ASSERT_EQUALS(options["StartOverlap"].toStdString(), "10");
    TS_ASSERT_EQUALS(options["EndOverlap"].toStdString(), "12");
    TS_ASSERT_EQUALS(options["FirstTransmissionRun"].toStdString(),
                     "INTER00013463,INTER00013464");
    TS_ASSERT_EQUALS(options["NormalizeByIntegratedMonitors"].toStdString(),
                     "True");
    TS_ASSERT_EQUALS(options["MonitorIntegrationWavelengthMin"].toStdString(),
                     "4");
    TS_ASSERT_EQUALS(options["MonitorIntegrationWavelengthMax"].toStdString(),
                     "10");
    TS_ASSERT_EQUALS(options["MonitorBackgroundWavelengthMin"].toStdString(),
                     "12");
    TS_ASSERT_EQUALS(options["MonitorBackgroundWavelengthMax"].toStdString(),
                     "17");
    TS_ASSERT_EQUALS(options["WavelengthMin"].toStdString(), "1");
    TS_ASSERT_EQUALS(options["WavelengthMax"].toStdString(), "15");
    TS_ASSERT_EQUALS(options["I0MonitorIndex"].toStdString(), "2");
    TS_ASSERT_EQUALS(options["ProcessingInstructions"].toStdString(), "3,4");
    TS_ASSERT_EQUALS(options["DetectorCorrectionType"].toStdString(),
                     "VerticalShift");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testStitchOptions() {
    MockSettingsView mockView;
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mockView, instrumentSettingsEnabled()).Times(0);
    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(1));
    presenter.getStitchOptions();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testPolarisationOptionsEnabled() {
    MockSettingsView mockView;
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, setIsPolCorrEnabled(false)).Times(Exactly(1));
    EXPECT_CALL(mockView, setPolarisationOptionsEnabled(false))
        .Times(Exactly(1));
    presenter.setInstrumentName("INTER");
    EXPECT_CALL(mockView, setIsPolCorrEnabled(true)).Times(Exactly(1));
    EXPECT_CALL(mockView, setPolarisationOptionsEnabled(true))
        .Times(Exactly(1));
    presenter.setInstrumentName("POLREF");
  }

  void testExperimentDefaults() {
    MockSettingsView mockView;
    ReflSettingsPresenter presenter(&mockView);
    MockMainWindowPresenter mainPresenter;

    // Set instrument to 'POLREF'
    EXPECT_CALL(mockView, setIsPolCorrEnabled(true)).Times(Exactly(1));
    EXPECT_CALL(mockView, setPolarisationOptionsEnabled(true))
        .Times(Exactly(1));
    presenter.setInstrumentName("POLREF");

    std::vector<std::string> defaults = {
        "PointDetectorAnalysis", "None",
        "1.006831,-0.011467,0.002244,-0.000095",
        "1.017526,-0.017183,0.003136,-0.000140",
        "0.917940,0.038265,-0.006645,0.000282",
        "0.972762,0.001828,-0.000261,0.0", "10", "12"};

    EXPECT_CALL(mockView, setExpDefaults(defaults)).Times(1);
    presenter.notify(IReflSettingsPresenter::ExpDefaultsFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testInstrumentDefaults() {
    MockSettingsView mockView;
    MockMainWindowPresenter mainPresenter;
    ReflSettingsPresenter presenter(&mockView);

    // Set instrument to 'INTER'
    EXPECT_CALL(mockView, setIsPolCorrEnabled(false)).Times(Exactly(1));
    EXPECT_CALL(mockView, setPolarisationOptionsEnabled(false))
        .Times(Exactly(1));
    presenter.setInstrumentName("INTER");

    std::vector<double> defaults_double = {1.,  4.0, 10., 17.,
                                           18., 1.5, 17., 2.0};
    std::vector<std::string> defaults_str = {"VerticalShift"};

    EXPECT_CALL(mockView, setInstDefaults(defaults_double, defaults_str))
        .Times(1);
    presenter.notify(IReflSettingsPresenter::InstDefaultsFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testExperimentSettingsDisabled() {

    MockSettingsView mockView;
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(3)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, instrumentSettingsEnabled())
        .Times(2)
        .WillRepeatedly(Return(true));

    // Experiment settings shouldn't be called
    EXPECT_CALL(mockView, getAnalysisMode()).Times(Exactly(0));
    EXPECT_CALL(mockView, getStartOverlap()).Times(Exactly(0));
    EXPECT_CALL(mockView, getEndOverlap()).Times(Exactly(0));
    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(0));

    // Instrument settings should be called
    EXPECT_CALL(mockView, getIntMonCheck()).Times(Exactly(1));
    EXPECT_CALL(mockView, getMonitorIntegralMin()).Times(Exactly(2));
    EXPECT_CALL(mockView, getMonitorIntegralMax()).Times(Exactly(2));
    EXPECT_CALL(mockView, getMonitorBackgroundMin()).Times(Exactly(2));
    EXPECT_CALL(mockView, getMonitorBackgroundMax()).Times(Exactly(2));
    EXPECT_CALL(mockView, getLambdaMin()).Times(Exactly(2));
    EXPECT_CALL(mockView, getLambdaMax()).Times(Exactly(2));
    EXPECT_CALL(mockView, getI0MonitorIndex()).Times(Exactly(2));
    EXPECT_CALL(mockView, getProcessingInstructions()).Times(Exactly(2));
    EXPECT_CALL(mockView, getDetectorCorrectionType()).Times(Exactly(1));

    auto transmissionOptions = presenter.getTransmissionOptions();
    auto reductionOptions = presenter.getReductionOptions();
    auto stitchOptions = presenter.getStitchOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testInstrumentSettingsDisabled() {

    MockSettingsView mockView;
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(3)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, instrumentSettingsEnabled())
        .Times(2)
        .WillRepeatedly(Return(false));

    // Instrument settings shouldn't be called
    EXPECT_CALL(mockView, getMonitorIntegralMin()).Times(Exactly(0));
    EXPECT_CALL(mockView, getMonitorIntegralMax()).Times(Exactly(0));
    EXPECT_CALL(mockView, getMonitorBackgroundMin()).Times(Exactly(0));
    EXPECT_CALL(mockView, getMonitorBackgroundMax()).Times(Exactly(0));
    EXPECT_CALL(mockView, getLambdaMin()).Times(Exactly(0));
    EXPECT_CALL(mockView, getLambdaMax()).Times(Exactly(0));
    EXPECT_CALL(mockView, getI0MonitorIndex()).Times(Exactly(0));
    EXPECT_CALL(mockView, getProcessingInstructions()).Times(Exactly(0));
    EXPECT_CALL(mockView, getIntMonCheck()).Times(Exactly(0));
    EXPECT_CALL(mockView, getDetectorCorrectionType()).Times(Exactly(0));

    // Experiment settings should be called
    EXPECT_CALL(mockView, getAnalysisMode()).Times(Exactly(2));
    EXPECT_CALL(mockView, getCRho()).Times(Exactly(1));
    EXPECT_CALL(mockView, getCAlpha()).Times(Exactly(1));
    EXPECT_CALL(mockView, getCAp()).Times(Exactly(1));
    EXPECT_CALL(mockView, getCPp()).Times(Exactly(1));
    EXPECT_CALL(mockView, getDirectBeam()).Times(Exactly(1));
    EXPECT_CALL(mockView, getPolarisationCorrections()).Times(Exactly(1));
    EXPECT_CALL(mockView, getScaleFactor()).Times(Exactly(1));
    EXPECT_CALL(mockView, getMomentumTransferStep()).Times(Exactly(1));
    EXPECT_CALL(mockView, getStartOverlap()).Times(Exactly(2));
    EXPECT_CALL(mockView, getEndOverlap()).Times(Exactly(2));
    EXPECT_CALL(mockView, getTransmissionRuns()).Times(Exactly(2));
    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(1));

    auto transmissionOptions = presenter.getTransmissionOptions();
    auto reductionOptions = presenter.getReductionOptions();
    auto stitchOptions = presenter.getStitchOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H */
