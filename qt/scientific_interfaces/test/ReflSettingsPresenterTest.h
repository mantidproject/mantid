#ifndef MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "../ISISReflectometry/ReflSettingsPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
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

// Get a std::string from a QVariant which represents a QString
std::string variantToString(const QVariant &variant) {
  return variant.value<QString>().toStdString();
}
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
    EXPECT_CALL(mockView, getTransmissionRuns()).Times(Exactly(0));

    auto options = presenter.getTransmissionOptions();
    TS_ASSERT_EQUALS(options.size(), 11);
    TS_ASSERT_EQUALS(variantToString(options["AnalysisMode"]),
                     "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(variantToString(options["StartOverlap"]), "10");
    TS_ASSERT_EQUALS(variantToString(options["EndOverlap"]), "12");
    TS_ASSERT_EQUALS(
        variantToString(options["MonitorIntegrationWavelengthMin"]), "4");
    TS_ASSERT_EQUALS(
        variantToString(options["MonitorIntegrationWavelengthMax"]), "10");
    TS_ASSERT_EQUALS(variantToString(options["MonitorBackgroundWavelengthMin"]),
                     "12");
    TS_ASSERT_EQUALS(variantToString(options["MonitorBackgroundWavelengthMax"]),
                     "17");
    TS_ASSERT_EQUALS(variantToString(options["WavelengthMin"]), "1");
    TS_ASSERT_EQUALS(variantToString(options["WavelengthMax"]), "15");
    TS_ASSERT_EQUALS(variantToString(options["I0MonitorIndex"]), "2");
    TS_ASSERT_EQUALS(variantToString(options["ProcessingInstructions"]), "3,4");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void onCallReturnDefaultSettings(MockSettingsView &mockView) {
    ON_CALL(mockView, instrumentSettingsEnabled()).WillByDefault(Return(true));
    onCallReturnDefaultInstrumentSettings(mockView);
    onCallReturnDefaultExperimentSettings(mockView);
  }

  void onCallReturnDefaultExperimentSettings(MockSettingsView &mockView) {
    ON_CALL(mockView, experimentSettingsEnabled()).WillByDefault(Return(true));
    onCallReturnDefaultTransmissionRuns(mockView);
    onCallReturnDefaultAnalysisMode(mockView);
    onCallReturnDefaultOverlap(mockView);
    onCallReturnDefaultPolarisationCorrections(mockView);
    onCallReturnDefaultSummationSettings(mockView);
    onCallReturnDefaultMomentumTransferStep(mockView);
    onCallReturnDefaultScaleFactor(mockView);
  }

  void onCallReturnDefaultAnalysisMode(MockSettingsView &mockView) {
    ON_CALL(mockView, getAnalysisMode())
        .WillByDefault(Return("PointDetectorAnalysis"));
  }

  void onCallReturnDefaultTransmissionRuns(MockSettingsView &mockView) {
    ON_CALL(mockView, getTransmissionRuns()).WillByDefault(Return(""));
  }

  void onCallReturnDefaultScaleFactor(MockSettingsView &mockView) {
    ON_CALL(mockView, getScaleFactor()).WillByDefault(Return(""));
  }

  void onCallReturnDefaultMomentumTransferStep(MockSettingsView &mockView) {
    ON_CALL(mockView, getMomentumTransferStep()).WillByDefault(Return(""));
  }

  void onCallReturnDefaultOverlap(MockSettingsView &mockView) {
    ON_CALL(mockView, getStartOverlap()).WillByDefault(Return(""));
    ON_CALL(mockView, getEndOverlap()).WillByDefault(Return(""));
  }

  void onCallReturnDefaultSummationSettings(MockSettingsView &mockView) {
    ON_CALL(mockView, getSummationType()).WillByDefault(Return("SumInLambda"));
    ON_CALL(mockView, getReductionType()).WillByDefault(Return("Normal"));
  }

  void onCallReturnDefaultPolarisationCorrections(MockSettingsView &mockView) {
    ON_CALL(mockView, getPolarisationCorrections())
        .WillByDefault(Return("None"));
    ON_CALL(mockView, getCRho()).WillByDefault(Return(""));
    ON_CALL(mockView, getCAlpha()).WillByDefault(Return(""));
    ON_CALL(mockView, getCAp()).WillByDefault(Return(""));
    ON_CALL(mockView, getCPp()).WillByDefault(Return(""));
  }

  void onCallReturnDefaultInstrumentSettings(MockSettingsView &mockView) {
    ON_CALL(mockView, getIntMonCheck()).WillByDefault(Return("False"));
    onCallReturnDefaultMonitorIntegralRange(mockView);
    onCallReturnDefaultMonitorBackgroundRange(mockView);
    onCallReturnDefaultLambdaRange(mockView);
    ON_CALL(mockView, getI0MonitorIndex()).WillByDefault(Return(""));
    ON_CALL(mockView, getProcessingInstructions()).WillByDefault(Return(""));
    ON_CALL(mockView, getDetectorCorrectionType())
        .WillByDefault(Return("VerticalShift"));
  }

  void onCallReturnDefaultLambdaRange(MockSettingsView &mockView) {
    ON_CALL(mockView, getLambdaMin()).WillByDefault(Return(""));
    ON_CALL(mockView, getLambdaMax()).WillByDefault(Return(""));
  }

  void onCallReturnDefaultMonitorIntegralRange(MockSettingsView &mockView) {
    ON_CALL(mockView, getMonitorIntegralMin()).WillByDefault(Return(""));
    ON_CALL(mockView, getMonitorIntegralMax()).WillByDefault(Return(""));
  }

  void onCallReturnDefaultMonitorBackgroundRange(MockSettingsView &mockView) {
    ON_CALL(mockView, getMonitorBackgroundMin()).WillByDefault(Return(""));
    ON_CALL(mockView, getMonitorBackgroundMax()).WillByDefault(Return(""));
  }

  bool keyNotSet(
      QString const &key,
      MantidQt::MantidWidgets::DataProcessor::OptionsQMap const &options) {
    return !options.contains(key);
  }

  void testGetQSummationOptionsWhenSummingInLambda() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getSummationType())
        .Times(AtLeast(1))
        .WillOnce(Return("SumInLambda"));
    EXPECT_CALL(mockView, getReductionType())
        .Times(AnyNumber())
        .WillRepeatedly(Return("NonFlatSample"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["SummationType"]), "SumInLambda");
    TS_ASSERT(keyNotSet("ReductionType", options));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetQSummationOptionsWhenSummingInQ() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getSummationType())
        .Times(AtLeast(1))
        .WillOnce(Return("SumInQ"));
    EXPECT_CALL(mockView, getReductionType())
        .Times(AtLeast(1))
        .WillOnce(Return("DivergentBeam"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["SummationType"]), "SumInQ");
    TS_ASSERT_EQUALS(variantToString(options["ReductionType"]),
                     "DivergentBeam");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetAnalysisMode() {
    MockSettingsView mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getAnalysisMode())
        .Times(AtLeast(1))
        .WillOnce(Return("MultiDetectorAnalysis"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["AnalysisMode"]),
                     "MultiDetectorAnalysis");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetPolarisationCorrectionOptions() {
    MockSettingsView mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getPolarisationCorrections())
        .Times(AtLeast(1))
        .WillOnce(Return("PNR"));
    EXPECT_CALL(mockView, getCRho())
        .Times(AtLeast(1))
        .WillOnce(Return("2.5,0.4,1.1"));
    EXPECT_CALL(mockView, getCAlpha())
        .Times(AtLeast(1))
        .WillOnce(Return("0.6,0.9,1.2"));
    EXPECT_CALL(mockView, getCAp())
        .Times(AtLeast(1))
        .WillOnce(Return("100.0,17.0,44.0"));
    EXPECT_CALL(mockView, getCPp())
        .Times(AtLeast(1))
        .WillOnce(Return("0.54,0.33,1.81"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["PolarizationAnalysis"]), "PNR");
    TS_ASSERT_EQUALS(variantToString(options["CRho"]), "2.5,0.4,1.1");
    TS_ASSERT_EQUALS(variantToString(options["CAlpha"]), "0.6,0.9,1.2");
    TS_ASSERT_EQUALS(variantToString(options["CAp"]), "100.0,17.0,44.0");
    TS_ASSERT_EQUALS(variantToString(options["CPp"]), "0.54,0.33,1.81");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetIntMonCheck() {
    MockSettingsView mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getIntMonCheck())
        .Times(AtLeast(1))
        .WillOnce(Return("True"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["NormalizeByIntegratedMonitors"]),
                     "True");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetMonitorIntegralRangeOptions() {
    MockSettingsView mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getMonitorIntegralMin())
        .Times(AtLeast(1))
        .WillOnce(Return("4"));
    EXPECT_CALL(mockView, getMonitorIntegralMax())
        .Times(AtLeast(1))
        .WillOnce(Return("10"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(
        variantToString(options["MonitorIntegrationWavelengthMin"]), "4");
    TS_ASSERT_EQUALS(
        variantToString(options["MonitorIntegrationWavelengthMax"]), "10");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetMonitorBackgroundRangeOptions() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getMonitorBackgroundMin())
        .Times(AtLeast(1))
        .WillOnce(Return("12"));
    EXPECT_CALL(mockView, getMonitorBackgroundMax())
        .Times(AtLeast(1))
        .WillOnce(Return("17"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["MonitorBackgroundWavelengthMin"]),
                     "12");
    TS_ASSERT_EQUALS(variantToString(options["MonitorBackgroundWavelengthMax"]),
                     "17");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetLambdaRangeOptions() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getLambdaMin())
        .Times(AtLeast(1))
        .WillOnce(Return("1"));
    EXPECT_CALL(mockView, getLambdaMax())
        .Times(AtLeast(1))
        .WillOnce(Return("15"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["WavelengthMin"]), "1");
    TS_ASSERT_EQUALS(variantToString(options["WavelengthMax"]), "15");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetI0MonitorIndexOption() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getI0MonitorIndex())
        .Times(AtLeast(1))
        .WillOnce(Return("2"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["I0MonitorIndex"]), "2");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetScaleFactorOption() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getScaleFactor())
        .Times(AtLeast(1))
        .WillOnce(Return("2"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["ScaleFactor"]), "2");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetMomentumTransferStepOption() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getMomentumTransferStep())
        .Times(AtLeast(1))
        .WillOnce(Return("-0.02"));

    auto options = presenter.getReductionOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetProcessingInstructionsOption() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getProcessingInstructions())
        .Times(AtLeast(1))
        .WillOnce(Return("3,4"));

    auto options = presenter.getReductionOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetDetectorCorrectionTypeOption() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getDetectorCorrectionType())
        .Times(AtLeast(1))
        .WillOnce(Return("VerticalShift"));

    auto options = presenter.getReductionOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetTransmissionRunOptions() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getTransmissionRuns())
        .Times(AtLeast(1))
        .WillOnce(Return("INTER00013463,INTER00013464"));

    auto options = presenter.getReductionOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetOverlapRangeOptions() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    ReflSettingsPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getStartOverlap())
        .Times(AtLeast(1))
        .WillOnce(Return("10"));
    EXPECT_CALL(mockView, getEndOverlap())
        .Times(AtLeast(1))
        .WillOnce(Return("12"));

    auto options = presenter.getReductionOptions();

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
    EXPECT_CALL(mockView, getPolarisationCorrections()).Times(Exactly(1));
    EXPECT_CALL(mockView, getScaleFactor()).Times(Exactly(1));
    EXPECT_CALL(mockView, getMomentumTransferStep()).Times(Exactly(1));
    EXPECT_CALL(mockView, getStartOverlap()).Times(Exactly(2));
    EXPECT_CALL(mockView, getEndOverlap()).Times(Exactly(2));
    EXPECT_CALL(mockView, getTransmissionRuns()).Times(Exactly(1));
    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(1));

    auto transmissionOptions = presenter.getTransmissionOptions();
    auto reductionOptions = presenter.getReductionOptions();
    auto stitchOptions = presenter.getStitchOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H */
