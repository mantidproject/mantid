// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Instrument/InstrumentPresenter.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../ReflMockObjects.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/ReflectometryHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MockInstrumentOptionDefaults.h"
#include "MockInstrumentView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class InstrumentPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentPresenterTest *createSuite() { return new InstrumentPresenterTest(); }
  static void destroySuite(InstrumentPresenterTest *suite) { delete suite; }

  InstrumentPresenterTest() : m_view() { Mantid::API::FrameworkManager::Instance(); }

  void tearDown() override {
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
  }

  void testSetValidWavelengthRange() {
    auto const range = RangeInLambda(1.5, 14);
    runTestForValidWavelengthRange(range, range);
  }

  void testWavelengthRangeIsInvalidIfStartGreaterThanEnd() {
    auto const range = RangeInLambda(7.5, 2);
    runTestForInvalidWavelengthRange(range);
  }

  void testWavelengthRangeIsInvalidIfZeroLength() {
    auto const range = RangeInLambda(7.5, 7.5);
    runTestForInvalidWavelengthRange(range);
  }

  void testWavelengthRangeIsValidIfStartUnset() {
    auto const range = RangeInLambda(0.0, 7.5);
    runTestForValidWavelengthRange(range, range);
  }

  void testWavelengthRangeIsValidIfEndUnset() {
    auto const range = RangeInLambda(7.5, 0.0);
    runTestForValidWavelengthRange(range, range);
  }

  void testWavelengthRangeIsValidButNotUpdatedIfUnset() {
    auto const range = RangeInLambda(0.0, 0.0);
    runTestForValidWavelengthRange(range, boost::none);
  }

  void testIntegratedMonitorsToggled() {
    auto presenter = makePresenter();
    auto const integrate = !presenter.instrument().integratedMonitors();

    EXPECT_CALL(m_view, getIntegrateMonitors()).WillOnce(Return(integrate));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().integratedMonitors(), integrate);
  }

  void testSetMonitorIndex() {
    auto presenter = makePresenter();
    auto const monitorIndex = 3;

    EXPECT_CALL(m_view, getMonitorIndex()).WillOnce(Return(monitorIndex));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().monitorIndex(), monitorIndex);
  }

  void testSetValidMonitorIntegralRange() {
    auto const range = RangeInLambda(3.4, 12.2);
    runTestForValidMonitorIntegralRange(range, range);
  }

  void testMonitorIntegralRangeIsInvalidIfStartGreaterThanEnd() {
    auto const range = RangeInLambda(7.5, 4);
    runTestForInvalidMonitorIntegralRange(range);
  }

  void testMonitorIntegralRangeIsInvalidIfZeroLength() {
    auto const range = RangeInLambda(7.5, 7.5);
    runTestForInvalidMonitorIntegralRange(range);
  }

  void testMonitorIntegralRangeIsValidIfStartUnset() {
    auto const range = RangeInLambda(0.0, 4.5);
    runTestForValidMonitorIntegralRange(range, range);
  }

  void testMonitorIntegralRangeIsValidIfEndUnset() {
    auto const range = RangeInLambda(4.5, 0.0);
    runTestForValidMonitorIntegralRange(range, range);
  }

  void testMonitorIntegralRangeIsValidButNotUpdatedIfUnset() {
    auto const range = RangeInLambda(0.0, 0.0);
    runTestForValidMonitorIntegralRange(range, boost::none);
  }

  void testSetValidMonitorBackgroundRange() {
    auto const range = RangeInLambda(2.0, 13);
    runTestForValidMonitorBackgroundRange(range, range);
  }

  void testMonitorBackgroundRangeIsInvalidIfStartGreaterThanEnd() {
    auto const range = RangeInLambda(3.5, 3.4);
    runTestForInvalidMonitorBackgroundRange(range);
  }

  void testMonitorBackgroundRangeIsInvalidIfZeroLength() {
    auto const range = RangeInLambda(2.0, 2.0);
    runTestForInvalidMonitorBackgroundRange(range);
  }

  void testMonitorBackgroundRangeIsInvalidIfOnlyStartSet() {
    auto const range = RangeInLambda(2.001, 0.0);
    runTestForInvalidMonitorBackgroundRange(range);
  }

  void testMonitorBackgroundRangeIsInvalidIfOnlyEndSet() {
    auto const range = RangeInLambda(0.0, 7.8);
    runTestForInvalidMonitorBackgroundRange(range);
  }

  void testMonitorBackgroundRangeIsValidButNotUpdatedIfUnset() {
    auto const range = RangeInLambda(0.0, 0.0);
    runTestForValidMonitorBackgroundRange(range, boost::none);
  }

  void testCorrectDetectorsToggledUpdatesModel() {
    auto presenter = makePresenter();
    auto const correctDetectors = !presenter.instrument().correctDetectors();

    EXPECT_CALL(m_view, getCorrectDetectors()).WillOnce(Return(correctDetectors));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().correctDetectors(), correctDetectors);
  }

  void testEnablingCorrectDetectorsEnablesCorrectionType() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getCorrectDetectors()).WillOnce(Return(true));
    EXPECT_CALL(m_view, enableDetectorCorrectionType()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testDiablingCorrectDetectorsDisablesCorrectionType() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getCorrectDetectors()).WillOnce(Return(false));
    EXPECT_CALL(m_view, disableDetectorCorrectionType()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testSetDetectorCorrectionTypeUpdatesModel() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getDetectorCorrectionType()).WillOnce(Return("RotateAroundSample"));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().detectorCorrectionType(), DetectorCorrectionType::RotateAroundSample);
  }

  void testSetCalibrationFileUpdatesModel() {
    auto calibrationFilePath = "/path/to/calibration_file.dat";
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getCalibrationFilePath()).WillOnce(Return(calibrationFilePath));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().calibrationFilePath(), calibrationFilePath);
  }

  void testAllWidgetsAreEnabledWhenReductionPaused() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, enableAll()).Times(1);
    expectNotProcessingOrAutoreducing();
    presenter.notifyReductionPaused();
  }

  void testAllWidgetsAreDisabledWhenReductionResumed() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableAll()).Times(1);
    expectProcessing();
    presenter.notifyReductionResumed();
  }

  void testAllWidgetsAreEnabledWhenAutoreductionPaused() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, enableAll()).Times(1);
    expectNotProcessingOrAutoreducing();
    presenter.notifyAutoreductionPaused();
  }

  void testAllWidgetsAreDisabledWhenAutoreductionResumed() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableAll()).Times(1);
    expectAutoreducing();
    presenter.notifyAutoreductionResumed();
  }

  void testSettingsChangedNotifiesMainPresenter() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifySettingsChanged();
  }

  void testRestoreDefaultsUpdatesInstrument() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyUpdateInstrumentRequested()).Times(1);
    presenter.notifyRestoreDefaultsRequested();
  }

  void testInstrumentChangedUpdatesMonitorOptionsInView() {
    auto model =
        makeModelWithMonitorOptions(MonitorCorrections(2, true, RangeInLambda(17.0, 18.0), RangeInLambda(4.0, 10.0)));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setMonitorIndex(2)).Times(1);
    EXPECT_CALL(m_view, setIntegrateMonitors(true)).Times(1);
    EXPECT_CALL(m_view, setMonitorBackgroundMin(17.0)).Times(1);
    EXPECT_CALL(m_view, setMonitorBackgroundMax(18.0)).Times(1);
    EXPECT_CALL(m_view, setMonitorIntegralMin(4.0)).Times(1);
    EXPECT_CALL(m_view, setMonitorIntegralMax(10.0)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesMonitorOptionsInModel() {
    auto model =
        makeModelWithMonitorOptions(MonitorCorrections(2, true, RangeInLambda(17.0, 18.0), RangeInLambda(4.0, 10.0)));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    TS_ASSERT_EQUALS(presenter.instrument().monitorIndex(), 2);
    TS_ASSERT_EQUALS(presenter.instrument().integratedMonitors(), true);
    TS_ASSERT_EQUALS(presenter.instrument().monitorBackgroundRange(), RangeInLambda(17.0, 18.0));
    TS_ASSERT_EQUALS(presenter.instrument().monitorIntegralRange(), RangeInLambda(4.0, 10.0));
  }

  void testInstrumentChangedUpdatesWavelengthRangeInView() {
    auto model = makeModelWithWavelengthRange(RangeInLambda(1.5, 17.0));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setLambdaMin(1.5)).Times(1);
    EXPECT_CALL(m_view, setLambdaMax(17.0)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesWavelengthRangeInModel() {
    auto model = makeModelWithWavelengthRange(RangeInLambda(1.5, 17.0));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    TS_ASSERT_EQUALS(presenter.instrument().wavelengthRange(), RangeInLambda(1.5, 17.0));
  }

  void testInstrumentChangedUpdatesUpdatesDetectorOptionsInView() {
    auto model =
        makeModelWithDetectorCorrections(DetectorCorrections(true, DetectorCorrectionType::RotateAroundSample));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setCorrectDetectors(true)).Times(1);
    EXPECT_CALL(m_view, setDetectorCorrectionType("RotateAroundSample")).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesUpdatesDetectorOptionsInModel() {
    auto model =
        makeModelWithDetectorCorrections(DetectorCorrections(true, DetectorCorrectionType::RotateAroundSample));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    auto const expected = DetectorCorrections(true, DetectorCorrectionType::RotateAroundSample);
    TS_ASSERT_EQUALS(presenter.instrument().detectorCorrections(), expected);
  }

  void testInstrumentChangedUpdatesCalibrationFilePathInView() {
    auto defaultFilepath = "default/path.dat";
    auto model = makeModelWithCalibrationFilePath(defaultFilepath);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setCalibrationFilePath(defaultFilepath)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testEnteringInvalidCalibrationFilePathTriggersError() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getCalibrationFilePath()).WillOnce(Return("test"));
    EXPECT_CALL(m_fileHandler, fileExists("test")).WillOnce(Return(false));
    EXPECT_CALL(m_view, showCalibrationFilePathInvalid()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testEnteringEmptyCalibrationFilePathDoesNotTriggerError() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getCalibrationFilePath()).WillOnce(Return(""));
    EXPECT_CALL(m_fileHandler, fileExists("")).Times(0);
    EXPECT_CALL(m_view, showCalibrationFilePathValid()).Times(1);
    presenter.notifySettingsChanged();
  }

private:
  NiceMock<MockInstrumentView> m_view;
  NiceMock<MockBatchPresenter> m_mainPresenter;
  NiceMock<MockFileHandler> m_fileHandler;
  NiceMock<MockMessageHandler> m_messageHandler;

  Instrument makeModelWithMonitorOptions(MonitorCorrections monitorCorrections) {
    auto wavelengthRange = RangeInLambda(0.0, 0.0);
    auto detectorCorrections = DetectorCorrections(false, DetectorCorrectionType::VerticalShift);
    return Instrument(std::move(wavelengthRange), std::move(monitorCorrections), std::move(detectorCorrections), "");
  }

  Instrument makeModelWithWavelengthRange(RangeInLambda wavelengthRange) {
    auto monitorCorrections = MonitorCorrections(0, false, RangeInLambda(0.0, 0.0), RangeInLambda(0.0, 0.0));
    auto detectorCorrections = DetectorCorrections(false, DetectorCorrectionType::VerticalShift);
    return Instrument(std::move(wavelengthRange), std::move(monitorCorrections), std::move(detectorCorrections), "");
  }

  Instrument makeModelWithDetectorCorrections(DetectorCorrections detectorCorrections) {
    auto wavelengthRange = RangeInLambda(0.0, 0.0);
    auto monitorCorrections = MonitorCorrections(0, false, RangeInLambda(0.0, 0.0), RangeInLambda(0.0, 0.0));
    return Instrument(std::move(wavelengthRange), std::move(monitorCorrections), std::move(detectorCorrections), "");
  }

  Instrument makeModelWithCalibrationFilePath(const std::string &filepath) {
    auto wavelengthRange = RangeInLambda(0.0, 0.0);
    auto monitorCorrections = MonitorCorrections(0, false, RangeInLambda(0.0, 0.0), RangeInLambda(0.0, 0.0));
    auto detectorCorrections = DetectorCorrections(false, DetectorCorrectionType::VerticalShift);
    return Instrument(std::move(wavelengthRange), std::move(monitorCorrections), std::move(detectorCorrections),
                      filepath);
  }

  InstrumentPresenter makePresenter(
      std::unique_ptr<IInstrumentOptionDefaults> defaultOptions = std::make_unique<MockInstrumentOptionDefaults>()) {
    auto presenter = InstrumentPresenter(&m_view, ModelCreationHelper::makeEmptyInstrument(), &m_fileHandler,
                                         &m_messageHandler, std::move(defaultOptions));
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
  }

  std::unique_ptr<MockInstrumentOptionDefaults> expectDefaults(Instrument const &model) {
    // Create a defaults object, set expectations on it, and return it so
    // that it can be passed to the presenter
    auto defaultOptions = std::make_unique<MockInstrumentOptionDefaults>();
    EXPECT_CALL(*defaultOptions, get(_)).Times(1).WillOnce(Return(model));
    return defaultOptions;
  }

  void expectProcessing() { EXPECT_CALL(m_mainPresenter, isProcessing()).Times(1).WillOnce(Return(true)); }

  void expectAutoreducing() { EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(1).WillOnce(Return(true)); }

  void expectNotProcessingOrAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isProcessing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(1).WillOnce(Return(false));
  }

  void runTestForValidWavelengthRange(RangeInLambda const &range, boost::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getLambdaMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getLambdaMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showLambdaRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().wavelengthRange(), result);
  }

  void runTestForInvalidWavelengthRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getLambdaMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getLambdaMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showLambdaRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().wavelengthRange(), boost::none);
  }

  void runTestForValidMonitorIntegralRange(RangeInLambda const &range, boost::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getMonitorIntegralMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorIntegralMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorIntegralRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().monitorIntegralRange(), result);
  }

  void runTestForInvalidMonitorIntegralRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getMonitorIntegralMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorIntegralMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorIntegralRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().monitorIntegralRange(), boost::none);
  }

  void runTestForValidMonitorBackgroundRange(RangeInLambda const &range, boost::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getMonitorBackgroundMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorBackgroundMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorBackgroundRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().monitorBackgroundRange(), result);
  }

  void runTestForInvalidMonitorBackgroundRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getMonitorBackgroundMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorBackgroundMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorBackgroundRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().monitorBackgroundRange(), boost::none);
  }
};
