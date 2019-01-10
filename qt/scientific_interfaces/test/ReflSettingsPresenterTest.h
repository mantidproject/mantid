// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflSettingsPresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
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
} // namespace

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

  void onCallReturnDefaultSettings(MockSettingsView &mockView) {
    ON_CALL(mockView, instrumentSettingsEnabled()).WillByDefault(Return(true));
    onCallReturnDefaultInstrumentSettings(mockView);
    onCallReturnDefaultExperimentSettings(mockView);
  }

  void onCallReturnDefaultExperimentSettings(MockSettingsView &mockView) {
    ON_CALL(mockView, experimentSettingsEnabled()).WillByDefault(Return(true));
    onCallReturnDefaultOptions(mockView);
    onCallReturnDefaultAnalysisMode(mockView);
    onCallReturnDefaultOverlap(mockView);
    onCallReturnDefaultPolarisationCorrections(mockView);
    onCallReturnDefaultSummationSettings(mockView);
  }

  void onCallReturnDefaultAnalysisMode(MockSettingsView &mockView) {
    ON_CALL(mockView, getAnalysisMode())
        .WillByDefault(Return("PointDetectorAnalysis"));
  }

  void onCallReturnDefaultOptions(MockSettingsView &mockView) {
    ON_CALL(mockView, getPerAngleOptions())
        .WillByDefault(Return(std::map<std::string, OptionsQMap>()));
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

  ReflSettingsPresenter makeReflSettingsPresenter(IReflSettingsView *view) {
    return ReflSettingsPresenter(view, 1);
  }

  void testGetQSummationOptionsWhenSummingInLambda() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    auto presenter = makeReflSettingsPresenter(&mockView);

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
    auto presenter = makeReflSettingsPresenter(&mockView);

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
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, getAnalysisMode())
        .Times(AtLeast(1))
        .WillOnce(Return("MultiDetectorAnalysis"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["AnalysisMode"]),
                     "MultiDetectorAnalysis");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetPolarisationCorrectionOptions() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, getPolarisationCorrections())
        .Times(AtLeast(1))
        .WillOnce(Return("PNR"));
    EXPECT_CALL(mockView, getCAp())
        .Times(AtLeast(1))
        .WillOnce(Return("100.0,17.0,44.0"));
    EXPECT_CALL(mockView, getCPp())
        .Times(AtLeast(1))
        .WillOnce(Return("0.54,0.33,1.81"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["PolarizationAnalysis"]), "PNR");
    TS_ASSERT_EQUALS(variantToString(options["CRho"]), "");
    TS_ASSERT_EQUALS(variantToString(options["CAlpha"]), "");
    TS_ASSERT_EQUALS(variantToString(options["CAp"]), "100.0,17.0,44.0");
    TS_ASSERT_EQUALS(variantToString(options["CPp"]), "0.54,0.33,1.81");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetIntMonCheck() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, getIntMonCheck())
        .Times(AtLeast(1))
        .WillOnce(Return("True"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["NormalizeByIntegratedMonitors"]),
                     "True");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetMonitorIntegralRangeOptions() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    auto presenter = makeReflSettingsPresenter(&mockView);

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
    auto presenter = makeReflSettingsPresenter(&mockView);

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
    auto presenter = makeReflSettingsPresenter(&mockView);

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
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, getI0MonitorIndex())
        .Times(AtLeast(1))
        .WillOnce(Return("2"));

    auto options = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(variantToString(options["I0MonitorIndex"]), "2");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetDetectorCorrectionTypeOptions() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, detectorCorrectionEnabled())
        .Times(Exactly(1))
        .WillOnce(Return(true));
    EXPECT_CALL(mockView, getDetectorCorrectionType())
        .Times(AtLeast(1))
        .WillOnce(Return("VerticalShift"));

    auto options = presenter.getReductionOptions();

    TS_ASSERT_EQUALS(variantToString(options["CorrectDetectors"]), "1");
    TS_ASSERT_EQUALS(variantToString(options["DetectorCorrectionType"]),
                     "VerticalShift");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testReductionOptionsIncludePerAngleDefaults() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    auto presenter = makeReflSettingsPresenter(&mockView);

    // The reduction options should include any defaults specified in the
    // per-angle options table. These are specified via a single row in the
    // table which has an empty angle as the key
    OptionsQMap options;
    options["FirstTransmissionRun"] = "INTER00013463,INTER00013464";
    std::map<std::string, OptionsQMap> perAngleOptions = {{"", options}};

    EXPECT_CALL(mockView, getPerAngleOptions())
        .Times(AtLeast(1))
        .WillOnce(Return(perAngleOptions));

    auto result = presenter.getReductionOptions();
    TS_ASSERT_EQUALS(result["FirstTransmissionRun"],
                     perAngleOptions[""]["FirstTransmissionRun"]);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetOverlapRangeOptions() {
    NiceMock<MockSettingsView> mockView;
    onCallReturnDefaultSettings(mockView);
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, getStartOverlap())
        .Times(AtLeast(1))
        .WillOnce(Return("10"));
    EXPECT_CALL(mockView, getEndOverlap())
        .Times(AtLeast(1))
        .WillOnce(Return("12"));

    auto options = presenter.getReductionOptions();

    TS_ASSERT_EQUALS(variantToString(options["StartOverlap"]), "10");
    TS_ASSERT_EQUALS(variantToString(options["EndOverlap"]), "12");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testStitchOptions() {
    NiceMock<MockSettingsView> mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mockView, instrumentSettingsEnabled()).Times(0);
    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(1));
    presenter.getStitchOptions();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testPolarisationOptionsEnabled() {
    NiceMock<MockSettingsView> mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, setIsPolCorrEnabled(false)).Times(Exactly(1));
    EXPECT_CALL(mockView, setPolarisationOptionsEnabled(false))
        .Times(Exactly(1));
    presenter.setInstrumentName("INTER");
    EXPECT_CALL(mockView, setIsPolCorrEnabled(true)).Times(Exactly(1));
    EXPECT_CALL(mockView, setPolarisationOptionsEnabled(true))
        .Times(Exactly(1));
    presenter.setInstrumentName("POLREF");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testDisablesControlsWhenReductionResumed() {
    NiceMock<MockSettingsView> mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, disableAll());
    presenter.onReductionResumed();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testEnablesControlsWhenReductionPaused() {
    NiceMock<MockSettingsView> mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);

    EXPECT_CALL(mockView, enableAll());
    presenter.onReductionPaused();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testExperimentDefaults() {
    NiceMock<MockSettingsView> mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);
    expectNoOptionLoadErrors(mockView);
    MockMainWindowPresenter mainPresenter;

    // Set instrument to 'POLREF'
    EXPECT_CALL(mockView, setIsPolCorrEnabled(true)).Times(Exactly(1));
    EXPECT_CALL(mockView, setPolarisationOptionsEnabled(true))
        .Times(Exactly(1));
    presenter.setInstrumentName("POLREF");

    auto fromIDFOrReductionAlg = ExperimentOptionDefaults();
    fromIDFOrReductionAlg.AnalysisMode = "PointDetectorAnalysis";
    fromIDFOrReductionAlg.PolarizationAnalysis = "None";
    fromIDFOrReductionAlg.CRho = "1.006831,-0.011467,0.002244,-0.000095";
    fromIDFOrReductionAlg.CAlpha = "1.017526,-0.017183,0.003136,-0.000140";
    fromIDFOrReductionAlg.CAp = "0.917940,0.038265,-0.006645,0.000282";
    fromIDFOrReductionAlg.CPp = "0.972762,0.001828,-0.000261,0.0";
    fromIDFOrReductionAlg.TransRunStartOverlap = 10.0;
    fromIDFOrReductionAlg.TransRunEndOverlap = 12.0;
    fromIDFOrReductionAlg.SummationType = "SumInLambda";
    fromIDFOrReductionAlg.ReductionType = "Normal";

    EXPECT_CALL(mockView, setExpDefaults(fromIDFOrReductionAlg)).Times(1);
    presenter.notify(IReflSettingsPresenter::ExpDefaultsFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void expectNoOptionLoadErrors(MockSettingsView &mockView) {
    EXPECT_CALL(mockView, showOptionLoadErrors(_, _)).Times(0);
  }

  void expectOptionLoadErrors(MockSettingsView &mockView) {
    EXPECT_CALL(mockView, showOptionLoadErrors(_, _)).Times(AtLeast(1));
  }

  void testInstrumentDefaults() {
    NiceMock<MockSettingsView> mockView;
    MockMainWindowPresenter mainPresenter;
    auto presenter = makeReflSettingsPresenter(&mockView);

    // Set instrument to 'INTER'
    EXPECT_CALL(mockView, setIsPolCorrEnabled(false)).Times(Exactly(1));
    EXPECT_CALL(mockView, setPolarisationOptionsEnabled(false))
        .Times(Exactly(1));
    expectNoOptionLoadErrors(mockView);

    presenter.setInstrumentName("INTER");
    auto fromIDFOrReductionAlg = InstrumentOptionDefaults();

    fromIDFOrReductionAlg.NormalizeByIntegratedMonitors = true;
    fromIDFOrReductionAlg.MonitorIntegralMin = 4.0;
    fromIDFOrReductionAlg.MonitorIntegralMax = 10.0;
    fromIDFOrReductionAlg.MonitorBackgroundMin = 17.0;
    fromIDFOrReductionAlg.MonitorBackgroundMax = 18.0;
    fromIDFOrReductionAlg.LambdaMin = 1.5;
    fromIDFOrReductionAlg.LambdaMax = 17.0;
    fromIDFOrReductionAlg.I0MonitorIndex = 2.0;
    fromIDFOrReductionAlg.DetectorCorrectionType = "VerticalShift";
    fromIDFOrReductionAlg.CorrectDetectors = true;

    EXPECT_CALL(mockView, setInstDefaults(fromIDFOrReductionAlg)).Times(1);
    presenter.notify(IReflSettingsPresenter::InstDefaultsFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testExperimentSettingsDisabled() {
    NiceMock<MockSettingsView> mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);

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
    EXPECT_CALL(mockView, getIntMonCheck()).Times(Exactly(2));
    EXPECT_CALL(mockView, getMonitorIntegralMin()).Times(Exactly(2));
    EXPECT_CALL(mockView, getMonitorIntegralMax()).Times(Exactly(2));
    EXPECT_CALL(mockView, getMonitorBackgroundMin()).Times(Exactly(2));
    EXPECT_CALL(mockView, getMonitorBackgroundMax()).Times(Exactly(2));
    EXPECT_CALL(mockView, getLambdaMin()).Times(Exactly(2));
    EXPECT_CALL(mockView, getLambdaMax()).Times(Exactly(2));
    EXPECT_CALL(mockView, getI0MonitorIndex()).Times(Exactly(2));
    EXPECT_CALL(mockView, getDetectorCorrectionType()).Times(Exactly(1));

    auto transmissionOptions = presenter.getTransmissionOptions();
    auto reductionOptions = presenter.getReductionOptions();
    auto stitchOptions = presenter.getStitchOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testInstrumentSettingsDisabled() {
    NiceMock<MockSettingsView> mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);
    onCallReturnDefaultOptions(mockView);

    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(5)
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
    EXPECT_CALL(mockView, getIntMonCheck()).Times(Exactly(0));
    EXPECT_CALL(mockView, getDetectorCorrectionType()).Times(Exactly(0));
    EXPECT_CALL(mockView, detectorCorrectionEnabled()).Times(Exactly(0));

    // Experiment settings should be called
    EXPECT_CALL(mockView, getAnalysisMode()).Times(Exactly(2));
    EXPECT_CALL(mockView, getCRho()).Times(Exactly(0));
    EXPECT_CALL(mockView, getCAlpha()).Times(Exactly(0));
    EXPECT_CALL(mockView, getCAp()).Times(Exactly(0));
    EXPECT_CALL(mockView, getCPp()).Times(Exactly(0));
    EXPECT_CALL(mockView, getPolarisationCorrections()).Times(Exactly(1));
    EXPECT_CALL(mockView, getFloodCorrection()).Times(Exactly(1));
    EXPECT_CALL(mockView, getStartOverlap()).Times(Exactly(2));
    EXPECT_CALL(mockView, getEndOverlap()).Times(Exactly(2));
    EXPECT_CALL(mockView, getPerAngleOptions()).Times(Exactly(2));
    EXPECT_CALL(mockView, getStitchOptions()).Times(Exactly(1));

    auto transmissionOptions = presenter.getTransmissionOptions();
    auto reductionOptions = presenter.getReductionOptions();
    auto stitchOptions = presenter.getStitchOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetDefaultOptions() {
    MockSettingsView mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);

    // Default options are specified with a single entry with an empty angle as
    // the key
    OptionsQMap options;
    options["FirstTransmissionRun"] = "INTER00013463,INTER00013464";
    std::map<std::string, OptionsQMap> perAngleOptions = {{"", options}};

    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(1)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getPerAngleOptions())
        .Times(1)
        .WillOnce(Return(perAngleOptions));

    auto result = presenter.getDefaultOptions();
    TS_ASSERT_EQUALS(result, perAngleOptions[""]);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGetOptionsForAngle() {
    MockSettingsView mockView;
    auto presenter = makeReflSettingsPresenter(&mockView);

    // Set up a table with transmission runs for 2 different angles
    OptionsQMap options1;
    options1["FirstTransmissionRun"] = "INTER00013463,INTER00013464";
    OptionsQMap options2;
    options2["FirstTransmissionRun"] = "INTER00013463R00013464";
    std::map<std::string, OptionsQMap> perAngleOptions = {{"0.7", options1},
                                                          {"2.33", options2}};

    // Test looking up transmission runs based on the angle. It has
    // quite a generous tolerance so the angle does not have to be
    // exact
    EXPECT_CALL(mockView, experimentSettingsEnabled())
        .Times(4)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getPerAngleOptions())
        .Times(6)
        .WillRepeatedly(Return(perAngleOptions));

    auto result = presenter.getOptionsForAngle(0.69);
    TS_ASSERT_EQUALS(result, perAngleOptions["0.7"]);
    result = presenter.getOptionsForAngle(2.34);
    TS_ASSERT_EQUALS(result, perAngleOptions["2.33"]);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTERTEST_H */
