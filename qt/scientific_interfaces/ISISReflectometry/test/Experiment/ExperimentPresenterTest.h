// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Experiment/ExperimentPresenter.h"
#include "../../../ISISReflectometry/Reduction/RowExceptions.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../ReflMockObjects.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/ReflectometryHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MockExperimentOptionDefaults.h"
#include "MockExperimentView.h"

#include <string>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::string_literals;
using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")

class ExperimentPresenterTest : public CxxTest::TestSuite {
  using OptionsRow = LookupRow::ValueArray;
  using OptionsTable = std::vector<OptionsRow>;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentPresenterTest *createSuite() { return new ExperimentPresenterTest(); }
  static void destroySuite(ExperimentPresenterTest *suite) { delete suite; }

  ExperimentPresenterTest() : m_view() { Mantid::API::FrameworkManager::Instance(); }

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

  void testModelUpdatedWhenAnalysisModeChanged() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getAnalysisMode()).WillOnce(Return(std::string("MultiDetectorAnalysis")));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().analysisMode(), AnalysisMode::MultiDetector);
  }

  void testModelUpdatedWhenSummationTypeChanged() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    presenter.notifySummationTypeChanged();

    TS_ASSERT_EQUALS(presenter.experiment().summationType(), SummationType::SumInQ);
  }

  void testSumInQWidgetsDisabledWhenChangeToSumInLambda() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableReductionType()).Times(1);
    EXPECT_CALL(m_view, disableIncludePartialBins()).Times(1);
    presenter.notifySummationTypeChanged();
  }

  void testSumInQWidgetsEnabledWhenChangeToSumInQ() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, enableReductionType()).Times(1);
    EXPECT_CALL(m_view, enableIncludePartialBins()).Times(1);
    presenter.notifySummationTypeChanged();
  }

  void testChangingIncludePartialBinsUpdatesModel() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, getIncludePartialBins()).WillOnce(Return(true));
    presenter.notifySettingsChanged();

    TS_ASSERT(presenter.experiment().includePartialBins());
  }

  void testChangingDebugOptionUpdatesModel() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, getDebugOption()).WillOnce(Return(true));
    presenter.notifySettingsChanged();

    TS_ASSERT(presenter.experiment().debug());
  }

  void testSetBackgroundSubtractionUpdatesModel() {
    auto presenter = makePresenter();
    expectSubtractBackground();
    presenter.notifySettingsChanged();
    assertBackgroundSubtractionOptionsSet(presenter);
  }

  void testBackgroundSubtractionMethodIsEnabledWhenSubtractBackgroundIsChecked() {
    auto presenter = makePresenter();
    expectSubtractBackground(true);
    EXPECT_CALL(m_view, enableBackgroundSubtractionMethod()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testPolynomialInputsEnabledWhenSubtractingPolynomialBackground() {
    auto presenter = makePresenter();
    expectSubtractBackground(true, "Polynomial");
    EXPECT_CALL(m_view, enablePolynomialDegree()).Times(1);
    EXPECT_CALL(m_view, enableCostFunction()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testPolynomialInputsDisabledWhenSubtractingPerDetectorAverage() {
    auto presenter = makePresenter();
    expectSubtractBackground(true, "PerDetectorAverage");
    EXPECT_CALL(m_view, disablePolynomialDegree()).Times(1);
    EXPECT_CALL(m_view, disableCostFunction()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testPolynomialInputsDisabledWhenSubtractingAveragePixelFit() {
    auto presenter = makePresenter();
    expectSubtractBackground(true, "AveragePixelFit");
    EXPECT_CALL(m_view, disablePolynomialDegree()).Times(1);
    EXPECT_CALL(m_view, disableCostFunction()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testBackgroundSubtractionInputsDisabledWhenOptionTurnedOff() {
    auto presenter = makePresenter();
    expectSubtractBackground(false);
    EXPECT_CALL(m_view, disableBackgroundSubtractionMethod()).Times(1);
    EXPECT_CALL(m_view, disablePolynomialDegree()).Times(1);
    EXPECT_CALL(m_view, disableCostFunction()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testTogglePolarizationCorrectionOptionUpdatesModel() {
    auto presenter = makePresenter();
    expectPolarizationAnalysisOn();
    presenter.notifySettingsChanged();
    assertPolarizationAnalysisWorkspace(presenter);
  }

  void testPolarizationCorrectionOptionDisablesWorkspaceInput() { runTestThatPolarizationCorrectionsDisabled(); }

  void testTogglePolarizationCorrectionOptionEnablesWorkspaceInput() {
    runTestThatPolarizationCorrectionsUsesParameterFile();
  }

  void testSettingPolarizationCorrectionWorkspaceUpdatesModel() { runTestThatPolarizationCorrectionsUsesWorkspace(); }

  void testSettingPolarizationCorrectionFilePathUpdatesModel() { runTestThatPolarizationCorrectionsUsesFilePath(); }

  void testValidPolarizationPathShowsAsValid() {
    auto const &testPath = "test/path.nxs";
    auto const &fullTestPath = "/full/pol/test/path.nxs";
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPolarizationCorrectionOption()).Times(2).WillRepeatedly(Return("FilePath"));
    EXPECT_CALL(m_view, getPolarizationEfficienciesFilePath()).WillRepeatedly(Return(testPath));
    EXPECT_CALL(m_fileHandler, getFullFilePath(testPath)).WillOnce(Return(fullTestPath));
    EXPECT_CALL(m_fileHandler, fileExists(fullTestPath)).WillOnce(Return(true));
    EXPECT_CALL(m_view, showPolCorrFilePathValid()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testInvalidPolarizationPathShowsAsInvalid() {
    auto const testPath = "test/path.nxs";
    auto const &fullTestPath = "/full/test/path.nxs";
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPolarizationCorrectionOption()).Times(2).WillRepeatedly(Return("FilePath"));
    EXPECT_CALL(m_view, getPolarizationEfficienciesFilePath()).WillRepeatedly(Return(testPath));
    EXPECT_CALL(m_fileHandler, getFullFilePath(testPath)).WillOnce(Return(fullTestPath));
    EXPECT_CALL(m_fileHandler, fileExists(fullTestPath)).WillOnce(Return(false));
    EXPECT_CALL(m_view, showPolCorrFilePathInvalid()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testSetFloodCorrectionsUpdatesModel() {
    auto presenter = makePresenter();
    FloodCorrections floodCorr(FloodCorrectionType::Workspace, std::string{"testWS"});
    EXPECT_CALL(m_view, getFloodCorrectionType()).Times(2).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(m_view, getFloodWorkspace()).WillOnce(Return(floodCorr.workspace().value()));
    EXPECT_CALL(m_view, setFloodCorrectionWorkspaceMode()).Times(1);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().floodCorrections(), floodCorr);
  }

  void testSetFloodCorrectionsUpdatesModelForFilePath() {
    auto presenter = makePresenter();
    FloodCorrections floodCorr(FloodCorrectionType::Workspace, std::string{"path/to/testWS"});

    EXPECT_CALL(m_view, getFloodCorrectionType()).Times(2).WillRepeatedly(Return("FilePath"));
    EXPECT_CALL(m_fileHandler, getFullFilePath(floodCorr.workspace().value()))
        .WillOnce(Return(floodCorr.workspace().value()));
    EXPECT_CALL(m_fileHandler, fileExists(floodCorr.workspace().value())).WillOnce(Return(true));
    EXPECT_CALL(m_view, getFloodFilePath()).WillOnce(Return(floodCorr.workspace().value()));
    EXPECT_CALL(m_view, setFloodCorrectionFilePathMode()).Times(1);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().floodCorrections(), floodCorr);
  }

  void testSetFloodCorrectionsUpdatesModelForNoCorrections() {
    auto presenter = makePresenter();
    FloodCorrections floodCorr(FloodCorrectionType::None);

    EXPECT_CALL(m_view, getFloodCorrectionType()).Times(2).WillRepeatedly(Return("None"));
    EXPECT_CALL(m_view, getFloodWorkspace()).Times(0);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().floodCorrections(), floodCorr);
  }

  void testSetFloodCorrectionsToWorkspaceEnablesInputs() {
    EXPECT_CALL(m_view, getFloodWorkspace()).Times(1);
    runWithFloodCorrectionInputsEnabled("Workspace");
  }

  void testSetFloodCorrectionsToFilePathEnablesInputs() {
    EXPECT_CALL(m_view, getFloodFilePath()).WillOnce(Return(""));
    EXPECT_CALL(m_fileHandler, getFullFilePath("")).WillOnce(Return(""));
    EXPECT_CALL(m_fileHandler, fileExists("")).Times(1);
    runWithFloodCorrectionInputsEnabled("FilePath");
  }

  void testSetFloodCorrectionsToParameterFileDisablesInputs() { runWithFloodCorrectionInputsDisabled("ParameterFile"); }

  void testSetFloodCorrectionsToNoneDisablesInputs() { runWithFloodCorrectionInputsDisabled("None"); }

  void testValidFloodPathShowsAsValid() {
    auto const &testPath = "test/flood/path.nxs";
    auto const &fullTestPath = "/full/test/flood/path.nxs";

    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getFloodCorrectionType()).Times(2).WillRepeatedly(Return("FilePath"));
    EXPECT_CALL(m_view, getFloodFilePath()).WillRepeatedly(Return(testPath));
    EXPECT_CALL(m_fileHandler, getFullFilePath(testPath)).WillOnce(Return(fullTestPath));
    EXPECT_CALL(m_fileHandler, fileExists(fullTestPath)).WillOnce(Return(true));
    EXPECT_CALL(m_view, showFloodCorrFilePathValid()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testInvalidFloodPathShowsAsInvalid() {
    auto const &testPath = "test/flood/path.nxs";
    auto const &fullTestPath = "/full/test/flood/path.nxs";
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getFloodCorrectionType()).Times(2).WillRepeatedly(Return("FilePath"));
    EXPECT_CALL(m_view, getFloodFilePath()).WillRepeatedly(Return(testPath));
    EXPECT_CALL(m_fileHandler, getFullFilePath(testPath)).WillOnce(Return(fullTestPath));
    EXPECT_CALL(m_fileHandler, fileExists(fullTestPath)).WillOnce(Return(false));
    EXPECT_CALL(m_view, showFloodCorrFilePathInvalid()).Times(1);
    presenter.notifySettingsChanged();
  }

  void testSetValidTransmissionRunRange() {
    RangeInLambda range(7.2, 10);
    runTestForValidTransmissionRunRange(range, range);
  }

  void testTransmissionRunRangeIsInvalidIfStartGreaterThanEnd() {
    RangeInLambda range(10.2, 7.1);
    runTestForInvalidTransmissionRunRange(range);
  }

  void testTransmissionRunRangeIsInvalidIfZeroLength() {
    RangeInLambda range(7.1, 7.1);
    runTestForInvalidTransmissionRunRange(range);
  }

  void testTransmissionRunRangeIsValidIfStartUnset() {
    RangeInLambda range(0.0, 7.1);
    runTestForValidTransmissionRunRange(range, range);
  }

  void testTransmissionRunRangeIsValidIfEndUnset() {
    RangeInLambda range(5, 0.0);
    runTestForValidTransmissionRunRange(range, range);
  }

  void testTransmissionRunRangeIsValidButNotUpdatedIfUnset() {
    RangeInLambda range(0.0, 0.0);
    runTestForValidTransmissionRunRange(range, std::nullopt);
  }

  void testTransmissionParamsAreValidWithPositiveValue() { runTestForValidTransmissionParams("0.02"); }

  void testTransmissionParamsAreValidWithNoValues() { runTestForValidTransmissionParams(""); }

  void testTransmissionParamsAreValidWithNegativeValue() { runTestForValidTransmissionParams("-0.02"); }

  void testTransmissionParamsAreValidWithThreeValues() { runTestForValidTransmissionParams("0.1, -0.02, 5"); }

  void testTransmissionParamsAreValidWithFiveValues() { runTestForValidTransmissionParams("0.1, -0.02, 5, 6, 7.9"); }

  void testTransmissionParamsIgnoresWhitespace() { runTestForValidTransmissionParams("    0.1  , -0.02 , 5   "); }

  void testTransmissionParamsAreInvalidWithTwoValues() { runTestForInvalidTransmissionParams("1, 2"); }

  void testTransmissionParamsAreInvalidWithFourValues() { runTestForInvalidTransmissionParams("1, 2, 3, 4"); }

  void testSetTransmissionScaleRHSProperty() {
    auto presenter = makePresenter();
    auto const scaleRHS = false;

    EXPECT_CALL(m_view, getTransmissionScaleRHSWorkspace()).WillOnce(Return(scaleRHS));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().transmissionStitchOptions().scaleRHS(), scaleRHS);
  }

  void testSetTransmissionParamsAreInvalidIfContainNonNumericValue() {
    auto presenter = makePresenter();
    auto const params = "1,bad";

    EXPECT_CALL(m_view, getTransmissionStitchParams()).WillOnce(Return(params));
    EXPECT_CALL(m_view, showTransmissionStitchParamsInvalid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().transmissionStitchOptions().rebinParameters(), "");
  }

  void testSetStitchOptions() {
    auto presenter = makePresenter();
    auto const optionsString = "Params=0.02";
    std::map<std::string, std::string> optionsMap = {{"Params", "0.02"}};

    EXPECT_CALL(m_view, getStitchOptions()).WillOnce(Return(optionsString));
    EXPECT_CALL(m_view, showStitchParametersValid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().stitchParameters(), optionsMap);
  }

  void testSetStitchOptionsInvalid() {
    auto presenter = makePresenter();
    auto const optionsString = "0.02";
    std::map<std::string, std::string> emptyOptionsMap;

    EXPECT_CALL(m_view, getStitchOptions()).WillOnce(Return(optionsString));
    EXPECT_CALL(m_view, showStitchParametersInvalid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().stitchParameters(), emptyOptionsMap);
  }

  void testSetStitchOptionsTrueTextReplacedWithValue() {
    auto presenter = makePresenter();
    auto const optionsString = "TestParam=True";
    std::map<std::string, std::string> optionsMap = {{"TestParam", "1"}};

    EXPECT_CALL(m_view, getStitchOptions()).WillOnce(Return(optionsString));
    EXPECT_CALL(m_view, showStitchParametersValid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().stitchParameters(), optionsMap);
  }

  void testSetStitchOptionsFalseTextReplacedWithValue() {
    auto presenter = makePresenter();
    auto const optionsString = "TestParam=False";
    std::map<std::string, std::string> optionsMap = {{"TestParam", "0"}};

    EXPECT_CALL(m_view, getStitchOptions()).WillOnce(Return(optionsString));
    EXPECT_CALL(m_view, showStitchParametersValid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().stitchParameters(), optionsMap);
  }

  void testNewLookupRowRequested() {
    auto presenter = makePresenter();

    // row should be added to view
    EXPECT_CALL(m_view, addLookupRow());
    // new value should be requested from view to update model
    EXPECT_CALL(m_view, getLookupTable()).Times(1);
    presenter.notifyNewLookupRowRequested();
  }

  void testRemoveLookupRowRequested() {
    auto presenter = makePresenter();

    int const indexToRemove = 0;
    // row should be removed from view
    EXPECT_CALL(m_view, removeLookupRow(indexToRemove)).Times(1);
    // new value should be requested from view to update model
    EXPECT_CALL(m_view, getLookupTable()).Times(1);
    presenter.notifyRemoveLookupRowRequested(indexToRemove);
  }

  void testChangingLookupRowUpdatesModel() {
    auto presenter = makePresenter();

    auto const row = 1;
    auto const column = 0;
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(), optionsRowWithSecondAngle()};
    EXPECT_CALL(m_view, getLookupTable()).WillOnce(Return(optionsTable));
    presenter.notifyLookupRowChanged(row, column);

    // Check the model contains the per-theta defaults returned by the view
    auto const lookupRows = presenter.experiment().lookupTableRows();
    TS_ASSERT_EQUALS(lookupRows.size(), 2);
    if (lookupRows.size() == 2) {
      TS_ASSERT_EQUALS(lookupRows[0].thetaOrWildcard(), defaultsWithFirstAngle().thetaOrWildcard());
      TS_ASSERT_EQUALS(lookupRows[1].thetaOrWildcard(), defaultsWithSecondAngle().thetaOrWildcard());
    }
  }

  void testMultipleUniqueAnglesAreValid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(), optionsRowWithSecondAngle()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testMultipleNonUniqueAnglesAreInvalid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(), optionsRowWithFirstAngle()};
    runTestForNonUniqueAngles(optionsTable);
  }

  void testSingleWildcardRowIsValid() {
    OptionsTable const optionsTable = {optionsRowWithWildcard()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testAngleAndWildcardRowAreValid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(), optionsRowWithWildcard()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testMultipleWildcardRowsAreInvalid() {
    OptionsTable const optionsTable = {optionsRowWithWildcard(), optionsRowWithWildcard()};
    for (auto row = 0; row < 2; ++row) {
      for (auto col = 0; col < 2; ++col) {
        EXPECT_CALL(
            m_view,
            setTooltip(
                row, col,
                "Error: Multiple wildcard rows. Only a single row in the table may have a blank angle and title cell."))
            .Times(1);
      }
    }
    runTestForInvalidOptionsTable(optionsTable, {0, 1}, {LookupRow::Column::THETA, LookupRow::Column::TITLE});
  }

  void testSetFirstTransmissionRun() {
    OptionsTable const optionsTable = {optionsRowWithFirstTransmissionRun()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetSecondTransmissionRun() {
    OptionsTable const optionsTable = {optionsRowWithSecondTransmissionRun()};
    runTestForInvalidOptionsTable(optionsTable, 0, {LookupRow::Column::FIRST_TRANS});
  }

  void testSetBothTransmissionRuns() {
    OptionsTable const optionsTable = {optionsRowWithBothTransmissionRuns()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetTransmissionProcessingInstructionsValid() {
    OptionsTable const optionsTable = {optionsRowWithTransProcessingInstructions()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetTransmissionProcessingInstructionsInvalid() {
    OptionsTable const optionsTable = {optionsRowWithTransProcessingInstructionsInvalid()};
    runTestForInvalidOptionsTable(optionsTable, 0, {LookupRow::Column::TRANS_SPECTRA});
  }

  void testSetQMin() {
    OptionsTable const optionsTable = {optionsRowWithQMin()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetQMinInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQMinInvalid()};
    runTestForInvalidOptionsTable(optionsTable, 0, {LookupRow::Column::QMIN});
  }

  void testSetQMax() {
    OptionsTable const optionsTable = {optionsRowWithQMax()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetQMaxInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQMaxInvalid()};
    runTestForInvalidOptionsTable(optionsTable, 0, {LookupRow::Column::QMAX});
  }

  void testSetQStep() {
    OptionsTable const optionsTable = {optionsRowWithQStep()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetQStepInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQStepInvalid()};
    runTestForInvalidOptionsTable(optionsTable, 0, {LookupRow::Column::QSTEP});
  }

  void testSetScale() {
    OptionsTable const optionsTable = {optionsRowWithScale()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetScaleInvalid() {
    OptionsTable const optionsTable = {optionsRowWithScaleInvalid()};
    runTestForInvalidOptionsTable(optionsTable, 0, {LookupRow::Column::SCALE});
  }

  void testSetProcessingInstructions() {
    OptionsTable const optionsTable = {optionsRowWithProcessingInstructions()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetProcessingInstructionsInvalid() {
    OptionsTable const optionsTable = {optionsRowWithProcessingInstructionsInvalid()};
    runTestForInvalidOptionsTable(optionsTable, 0, {LookupRow::Column::RUN_SPECTRA});
  }

  void testSetBackgroundProcessingInstructionsValid() {
    OptionsTable const optionsTable = {optionsRowWithBackgroundProcessingInstructions()};
    runTestForValidOptionsTable(optionsTable);
  }

  void testSetBackgroundProcessingInstructionsInvalid() {
    OptionsTable const optionsTable = {optionsRowWithBackgroundProcessingInstructionsInvalid()};
    runTestForInvalidOptionsTable(optionsTable, 0, {LookupRow::Column::BACKGROUND_SPECTRA});
  }

  void testChangingSettingsNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifySettingsChanged();
  }

  void testChangingLookupRowNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifyLookupRowChanged(0, 0);
  }

  void testRestoreDefaultsUpdatesInstrument() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyUpdateInstrumentRequested()).Times(1);
    presenter.notifyRestoreDefaultsRequested();
  }

  void testInstrumentChangedUpdatesAnalysisModeInView() {
    auto model = makeModelWithAnalysisMode(AnalysisMode::MultiDetector);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setAnalysisMode("MultiDetectorAnalysis")).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesAnalysisModeInModel() {
    auto model = makeModelWithAnalysisMode(AnalysisMode::MultiDetector);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    TS_ASSERT_EQUALS(presenter.experiment().analysisMode(), AnalysisMode::MultiDetector);
  }

  void testInstrumentChangedUpdatesReductionOptionsInView() {
    auto model = makeModelWithReduction(SummationType::SumInQ, ReductionType::NonFlatSample, true);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setSummationType("SumInQ")).Times(1);
    EXPECT_CALL(m_view, setReductionType("NonFlatSample")).Times(1);
    EXPECT_CALL(m_view, setIncludePartialBins(true)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesReductionOptionsInModel() {
    auto model = makeModelWithReduction(SummationType::SumInQ, ReductionType::NonFlatSample, true);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    TS_ASSERT_EQUALS(presenter.experiment().summationType(), SummationType::SumInQ);
    TS_ASSERT_EQUALS(presenter.experiment().reductionType(), ReductionType::NonFlatSample);
    TS_ASSERT_EQUALS(presenter.experiment().includePartialBins(), true);
  }

  void testInstrumentChangedUpdatesDebugOptionsInView() {
    auto model = makeModelWithDebug(true);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setDebugOption(true)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesDebugOptionsInModel() {
    auto model = makeModelWithDebug(true);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    TS_ASSERT_EQUALS(presenter.experiment().debug(), true);
  }

  void testInstrumentChangedUpdatesLookupRowInView() {
    auto lookupRow = LookupRow(boost::none, boost::none, TransmissionRunPair(), boost::none, RangeInQ(0.01, 0.03, 0.2),
                               0.7, std::string("390-415"), std::string("370-389,416-430"), boost::none);
    auto model = makeModelWithLookupRow(std::move(lookupRow));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    auto const expected = std::vector<LookupRow::ValueArray>{
        {"", "", "", "", "", "0.010000", "0.200000", "0.030000", "0.700000", "390-415", "370-389,416-430", ""}};
    EXPECT_CALL(m_view, setLookupTable(expected)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesLookupRowInModel() {
    auto model = makeModelWithLookupRow(LookupRow(boost::none, boost::none, TransmissionRunPair(), boost::none,
                                                  RangeInQ(0.01, 0.03, 0.2), 0.7, std::string("390-415"),
                                                  std::string("370-389,416-430"), boost::none));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    auto expected = LookupRow(boost::none, boost::none, TransmissionRunPair(), boost::none, RangeInQ(0.01, 0.03, 0.2),
                              0.7, std::string("390-415"), std::string("370-389,416-430"), boost::none);
    auto lookupRows = presenter.experiment().lookupTableRows();
    TS_ASSERT_EQUALS(lookupRows.size(), 1);
    if (lookupRows.size() == 1) {
      TS_ASSERT_EQUALS(lookupRows.front(), expected);
    }
  }

  void testInstrumentChangedUpdatesTransmissionRunRangeInView() {
    auto model = makeModelWithTransmissionRunRange(RangeInLambda{10.0, 12.0});
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setTransmissionStartOverlap(10.0)).Times(1);
    EXPECT_CALL(m_view, setTransmissionEndOverlap(12.0)).Times(1);
    EXPECT_CALL(m_view, showTransmissionRangeValid()).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesTransmissionRunRangeInModel() {
    auto model = makeModelWithTransmissionRunRange(RangeInLambda{10.0, 12.0});
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    auto const expected = RangeInLambda{10.0, 12.0};
    TS_ASSERT_EQUALS(presenter.experiment().transmissionStitchOptions().overlapRange(), expected);
  }

  void testInstrumentChangedUpdatesCorrectionInView() {
    auto model =
        makeModelWithCorrections(PolarizationCorrections(PolarizationCorrectionType::ParameterFile),
                                 FloodCorrections(FloodCorrectionType::ParameterFile), makeBackgroundSubtraction());
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setPolarizationCorrectionOption("ParameterFile")).Times(1);
    EXPECT_CALL(m_view, setFloodCorrectionType("ParameterFile")).Times(1);
    EXPECT_CALL(m_view, setSubtractBackground(true));
    EXPECT_CALL(m_view, setBackgroundSubtractionMethod("Polynomial"));
    EXPECT_CALL(m_view, setPolynomialDegree(3));
    EXPECT_CALL(m_view, setCostFunction("Unweighted least squares"));
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testInstrumentChangedUpdatesCorrectionInModel() {
    auto model =
        makeModelWithCorrections(PolarizationCorrections(PolarizationCorrectionType::ParameterFile),
                                 FloodCorrections(FloodCorrectionType::ParameterFile), makeBackgroundSubtraction());
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    assertBackgroundSubtractionOptionsSet(presenter);
    assertPolarizationAnalysisParameterFile(presenter);
    assertFloodCorrectionUsesParameterFile(presenter);
  }

  void testInstrumentChangedDisconnectsNotificationsBackFromView() {
    auto defaultOptions = expectDefaults(makeEmptyExperiment());
    EXPECT_CALL(m_view, disconnectExperimentSettingsWidgets()).Times(1);
    EXPECT_CALL(m_view, connectExperimentSettingsWidgets()).Times(1);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
  }

  void testPolarizationCorrectionsDisabledForINTER() {
    runTestThatPolarizationCorrectionsAreDisabledForInstrument("INTER");
  }

  void testPolarizationCorrectionsDisabledForSURF() {
    runTestThatPolarizationCorrectionsAreDisabledForInstrument("SURF");
  }

  void testPolarizationCorrectionsEnabledForOFFSPEC() {
    runTestThatPolarizationCorrectionsAreEnabledForInstrument("OFFSPEC");
  }

  void testPolarizationCorrectionsEnabledForPOLREF() {
    runTestThatPolarizationCorrectionsAreEnabledForInstrument("POLREF");
  }

  void testPolarizationCorrectionsEnabledForCRISP() {
    runTestThatPolarizationCorrectionsAreEnabledForInstrument("CRISP");
  }

  void testNotifyPreviewApplyRequestedUpdatesProcessingInstructions() {
    // makeExperiment will create a model Experiment with two lookup rows and a wildcard row
    auto presenter = makePresenter(makeDefaults(), makeExperiment());
    auto previewRow = PreviewRow({"1234"});
    previewRow.setSelectedBanks("9"s);
    previewRow.setProcessingInstructions(ROIType::Signal, "10"s);
    previewRow.setProcessingInstructions(ROIType::Background, "11"s);
    previewRow.setProcessingInstructions(ROIType::Transmission, "12"s);
    previewRow.setTheta(2.3);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(1);

    presenter.notifyPreviewApplyRequested(previewRow);
    // Row with angle 2.3 is the last row in the look-up table
    auto row = presenter.experiment().lookupTableRows().back();
    TS_ASSERT_EQUALS(row.roiDetectorIDs().get(), "9");
    TS_ASSERT_EQUALS(row.processingInstructions().get(), "10");
    TS_ASSERT_EQUALS(row.backgroundProcessingInstructions().get(), "11");
    TS_ASSERT_EQUALS(row.transmissionProcessingInstructions().get(), "12");
  }

  void testNotifyPreviewApplyRequestedClearsProcessingInstructionsWhenMissing() {
    // makeExperiment will create a model Experiment with two lookup rows and a wildcard row
    auto presenter = makePresenter(makeDefaults(), makeExperiment());
    auto previewRow = PreviewRow({"1234"});
    previewRow.setTheta(2.3);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(1);

    presenter.notifyPreviewApplyRequested(previewRow);
    // Row with angle 2.3 is the last row in the look-up table
    auto row = presenter.experiment().lookupTableRows().back();
    TS_ASSERT(!row.roiDetectorIDs());
    TS_ASSERT(!row.processingInstructions());
    TS_ASSERT(!row.backgroundProcessingInstructions());
    TS_ASSERT(!row.transmissionProcessingInstructions());
  }

  void testNotifyPreviewApplyRequestedDoesNotResetRowStateIfNoSettingsChanged() {
    // makeExperiment will create a model Experiment with two lookup rows and a wildcard row
    auto presenter = makePresenter(makeDefaults(), makeExperiment());
    auto previewRow = PreviewRow({"1234"});
    previewRow.setSelectedBanks("3-22"s);
    previewRow.setProcessingInstructions(ROIType::Signal, "4-6"s);
    previewRow.setProcessingInstructions(ROIType::Background, "2-3,7-8"s);
    previewRow.setProcessingInstructions(ROIType::Transmission, "4"s);
    previewRow.setTheta(2.3);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(0);

    presenter.notifyPreviewApplyRequested(previewRow);
    // Row with angle 2.3 is the last row in the look-up table
    auto row = presenter.experiment().lookupTableRows().back();
    TS_ASSERT_EQUALS(row.roiDetectorIDs().get(), "3-22");
    TS_ASSERT_EQUALS(row.processingInstructions().get(), "4-6");
    TS_ASSERT_EQUALS(row.backgroundProcessingInstructions().get(), "2-3,7-8");
    TS_ASSERT_EQUALS(row.transmissionProcessingInstructions().get(), "4");
  }

  void testNotifyPreviewApplyRequestedResetsRowStateIfOnlyDetROIChanged() {
    // makeExperiment will create a model Experiment with two lookup rows and a wildcard row
    auto presenter = makePresenter(makeDefaults(), makeExperiment());
    auto previewRow = PreviewRow({"1234"});
    previewRow.setSelectedBanks("10-20"s);
    previewRow.setProcessingInstructions(ROIType::Signal, "4-6"s);
    previewRow.setProcessingInstructions(ROIType::Background, "2-3,7-8"s);
    previewRow.setProcessingInstructions(ROIType::Transmission, "4"s);
    previewRow.setTheta(2.3);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(1);

    presenter.notifyPreviewApplyRequested(previewRow);
  }

  void testNotifyPreviewApplyRequestedResetsRowStateIfOnlySignalROIChanged() {
    // makeExperiment will create a model Experiment with two lookup rows and a wildcard row
    auto presenter = makePresenter(makeDefaults(), makeExperiment());
    auto previewRow = PreviewRow({"1234"});
    previewRow.setSelectedBanks("3-22"s);
    previewRow.setProcessingInstructions(ROIType::Signal, "4-10"s);
    previewRow.setProcessingInstructions(ROIType::Background, "2-3,7-8"s);
    previewRow.setProcessingInstructions(ROIType::Transmission, "4"s);
    previewRow.setTheta(2.3);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(1);

    presenter.notifyPreviewApplyRequested(previewRow);
  }

  void testNotifyPreviewApplyRequestedResetsRowStateIfOnlyBackgroundROIChanged() {
    // makeExperiment will create a model Experiment with two lookup rows and a wildcard row
    auto presenter = makePresenter(makeDefaults(), makeExperiment());
    auto previewRow = PreviewRow({"1234"});
    previewRow.setSelectedBanks("3-22"s);
    previewRow.setProcessingInstructions(ROIType::Signal, "4-6"s);
    previewRow.setProcessingInstructions(ROIType::Background, "7-8"s);
    previewRow.setProcessingInstructions(ROIType::Transmission, "4"s);
    previewRow.setTheta(2.3);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(1);

    presenter.notifyPreviewApplyRequested(previewRow);
  }

  void testNotifyPreviewApplyRequestedResetsRowStateIfOnlyTransmissionROIChanged() {
    // makeExperiment will create a model Experiment with two lookup rows and a wildcard row
    auto presenter = makePresenter(makeDefaults(), makeExperiment());
    auto previewRow = PreviewRow({"1234"});
    previewRow.setSelectedBanks("3-22"s);
    previewRow.setProcessingInstructions(ROIType::Signal, "4-6"s);
    previewRow.setProcessingInstructions(ROIType::Background, "2-3,7-8"s);
    previewRow.setProcessingInstructions(ROIType::Transmission, boost::none);
    previewRow.setTheta(2.3);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(1);

    presenter.notifyPreviewApplyRequested(previewRow);
  }

  void testNotifyPreviewApplyRequestedMatchingRowNotFound() {
    // makeExperimentWithValidDuplicateCriteria will create a model Experiment with two lookup rows and no wildcard
    auto presenter = makePresenter(makeDefaults(), makeExperimentWithValidDuplicateCriteria());
    auto previewRow = PreviewRow({"1234"});
    // This angle doesn't match any in the experiment lookup table
    previewRow.setTheta(10);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(0);

    TS_ASSERT_THROWS(presenter.notifyPreviewApplyRequested(previewRow), RowNotFoundException const &);
  }

  void testNotifyPreviewApplyRequestedInvalidTable() {
    // GIVEN an invalid table
    OptionsTable const optionsTable = {optionsRowWithWildcard(), optionsRowWithWildcard()};
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getLookupTable()).WillOnce(Return(optionsTable));
    presenter.notifyLookupRowChanged(1, 1);

    // WHEN we look for any row in the table
    auto previewRow = PreviewRow({""});

    // THEN an InvalidTableException is thrown.
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(0);
    TS_ASSERT_THROWS(presenter.notifyPreviewApplyRequested(previewRow), InvalidTableException const &);
  }

private:
  NiceMock<MockExperimentView> m_view;
  NiceMock<MockBatchPresenter> m_mainPresenter;
  NiceMock<MockFileHandler> m_fileHandler;
  double m_thetaTolerance{0.01};

  Experiment makeModelWithAnalysisMode(AnalysisMode analysisMode) {
    return Experiment(analysisMode, ReductionType::Normal, SummationType::SumInLambda, false, false,
                      BackgroundSubtraction(), makeEmptyPolarizationCorrections(), makeFloodCorrections(),
                      makeEmptyTransmissionStitchOptions(), makeEmptyStitchOptions(), makeLookupTable());
  }

  Experiment makeModelWithReduction(SummationType summationType, ReductionType reductionType, bool includePartialBins) {
    return Experiment(AnalysisMode::PointDetector, reductionType, summationType, includePartialBins, false,
                      BackgroundSubtraction(), makeEmptyPolarizationCorrections(), makeFloodCorrections(),
                      makeEmptyTransmissionStitchOptions(), makeEmptyStitchOptions(), makeLookupTable());
  }

  Experiment makeModelWithDebug(bool debug) {
    return Experiment(AnalysisMode::PointDetector, ReductionType::Normal, SummationType::SumInLambda, false, debug,
                      BackgroundSubtraction(), makeEmptyPolarizationCorrections(), makeFloodCorrections(),
                      makeEmptyTransmissionStitchOptions(), makeEmptyStitchOptions(), makeLookupTable());
  }

  Experiment makeModelWithLookupRow(LookupRow lookupRow) {
    auto lookupTable = LookupTable({std::move(lookupRow)});
    return Experiment(AnalysisMode::PointDetector, ReductionType::Normal, SummationType::SumInLambda, false, false,
                      BackgroundSubtraction(), makeEmptyPolarizationCorrections(), makeFloodCorrections(),
                      makeEmptyTransmissionStitchOptions(), makeEmptyStitchOptions(), std::move(lookupTable));
  }

  Experiment makeModelWithTransmissionRunRange(RangeInLambda range) {
    return Experiment(AnalysisMode::PointDetector, ReductionType::Normal, SummationType::SumInLambda, false, false,
                      BackgroundSubtraction(), makeEmptyPolarizationCorrections(), makeFloodCorrections(),
                      TransmissionStitchOptions(std::move(range), std::string(), false), makeEmptyStitchOptions(),
                      makeLookupTable());
  }

  Experiment makeModelWithCorrections(PolarizationCorrections polarizationCorrections,
                                      FloodCorrections floodCorrections, BackgroundSubtraction backgroundSubtraction) {
    return Experiment(AnalysisMode::PointDetector, ReductionType::Normal, SummationType::SumInLambda, false, false,
                      std::move(backgroundSubtraction), std::move(polarizationCorrections), std::move(floodCorrections),
                      makeEmptyTransmissionStitchOptions(), makeEmptyStitchOptions(), makeLookupTable());
  }

  std::unique_ptr<IExperimentOptionDefaults> makeDefaults() { return std::make_unique<MockExperimentOptionDefaults>(); }

  ExperimentPresenter makePresenter(
      std::unique_ptr<IExperimentOptionDefaults> defaultOptions = std::make_unique<MockExperimentOptionDefaults>(),
      Experiment experiment = makeEmptyExperiment()) {
    // The presenter gets values from the view on construction so the view must
    // return something sensible
    auto presenter = ExperimentPresenter(&m_view, std::move(experiment), m_thetaTolerance, &m_fileHandler,
                                         std::move(defaultOptions));
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
  }

  void expectProcessing() { EXPECT_CALL(m_mainPresenter, isProcessing()).Times(1).WillOnce(Return(true)); }

  void expectAutoreducing() { EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(1).WillOnce(Return(true)); }

  void expectNotProcessingOrAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isProcessing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(1).WillOnce(Return(false));
  }

  void expectViewReturnsSumInQDefaults() {
    EXPECT_CALL(m_view, getSummationType()).WillOnce(Return(std::string("SumInQ")));
    EXPECT_CALL(m_view, getReductionType()).WillOnce(Return(std::string("DivergentBeam")));
  }

  void expectSubtractBackground(bool subtractBackground = true,
                                std::string const &subtractionType = std::string("Polynomial"),
                                int degreeOfPolynomial = 3,
                                std::string const &costFunction = std::string("Unweighted least squares")) {
    EXPECT_CALL(m_view, getSubtractBackground()).Times(AtLeast(1)).WillRepeatedly(Return(subtractBackground));
    EXPECT_CALL(m_view, getBackgroundSubtractionMethod()).Times(AtLeast(1)).WillRepeatedly(Return(subtractionType));
    EXPECT_CALL(m_view, getPolynomialDegree()).Times(1).WillRepeatedly(Return(degreeOfPolynomial));
    EXPECT_CALL(m_view, getCostFunction()).Times(1).WillRepeatedly(Return(costFunction));
  }

  void assertBackgroundSubtractionOptionsSet(
      ExperimentPresenter const &presenter, bool subtractBackground = true,
      BackgroundSubtractionType subtractionType = BackgroundSubtractionType::Polynomial, int degreeOfPolynomial = 3,
      CostFunctionType costFunction = CostFunctionType::UnweightedLeastSquares) {
    TS_ASSERT_EQUALS(presenter.experiment().backgroundSubtraction().subtractBackground(), subtractBackground);
    TS_ASSERT_EQUALS(presenter.experiment().backgroundSubtraction().subtractionType(), subtractionType);
    TS_ASSERT_EQUALS(presenter.experiment().backgroundSubtraction().degreeOfPolynomial(), degreeOfPolynomial);
    TS_ASSERT_EQUALS(presenter.experiment().backgroundSubtraction().costFunction(), costFunction);
  }

  void expectPolarizationAnalysisOn() {
    EXPECT_CALL(m_view, getPolarizationCorrectionOption()).Times(AtLeast(1)).WillRepeatedly(Return("Workspace"));
  }

  void assertPolarizationAnalysisNone(ExperimentPresenter const &presenter) {
    TS_ASSERT_EQUALS(presenter.experiment().polarizationCorrections().correctionType(),
                     PolarizationCorrectionType::None);
  }

  void assertPolarizationAnalysisParameterFile(ExperimentPresenter const &presenter) {
    TS_ASSERT_EQUALS(presenter.experiment().polarizationCorrections().correctionType(),
                     PolarizationCorrectionType::ParameterFile);
  }

  void assertPolarizationAnalysisWorkspace(ExperimentPresenter const &presenter) {
    TS_ASSERT_EQUALS(presenter.experiment().polarizationCorrections().correctionType(),
                     PolarizationCorrectionType::Workspace);
  }

  void assertFloodCorrectionUsesParameterFile(ExperimentPresenter const &presenter) {
    TS_ASSERT_EQUALS(presenter.experiment().floodCorrections().correctionType(), FloodCorrectionType::ParameterFile);
  }

  std::unique_ptr<MockExperimentOptionDefaults> expectDefaults(Experiment const &model) {
    // Create a defaults object, set expectations on it, and return it so
    // that it can be passed to the presenter
    auto defaultOptions = std::make_unique<MockExperimentOptionDefaults>();
    EXPECT_CALL(*defaultOptions, get(_)).Times(1).WillOnce(Return(model));
    return defaultOptions;
  }

  void runTestThatPolarizationCorrectionsAreEnabledForInstrument(std::string const &instrument) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_mainPresenter, instrumentName()).Times(1).WillOnce(Return(instrument));
    EXPECT_CALL(m_view, enablePolarizationCorrections()).Times(1);
    presenter.notifySettingsChanged();
  }

  void runTestThatPolarizationCorrectionsAreDisabledForInstrument(std::string const &instrument) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_mainPresenter, instrumentName()).Times(1).WillOnce(Return(instrument));
    EXPECT_CALL(m_view, setPolarizationCorrectionOption("None")).Times(1);
    EXPECT_CALL(m_view, disablePolarizationCorrections()).Times(1);
    presenter.notifySettingsChanged();
  }

  void runTestThatPolarizationCorrectionsDisabled() {
    auto presenter = makePresenter();

    // Called thrice, once for getting it for the model, twice for choosing the efficiencies selector.
    EXPECT_CALL(m_view, getPolarizationCorrectionOption()).Times(2).WillRepeatedly(Return("None"));
    EXPECT_CALL(m_view, getPolarizationEfficienciesWorkspace()).Times(0);
    EXPECT_CALL(m_view, disablePolarizationEfficiencies()).Times(1);

    presenter.notifySettingsChanged();

    assertPolarizationAnalysisNone(presenter);
  }

  void runTestThatPolarizationCorrectionsUsesParameterFile() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getPolarizationCorrectionOption()).Times(2).WillRepeatedly(Return("ParameterFile"));
    EXPECT_CALL(m_view, getPolarizationEfficienciesWorkspace()).Times(0);
    EXPECT_CALL(m_view, disablePolarizationEfficiencies()).Times(1);

    presenter.notifySettingsChanged();

    assertPolarizationAnalysisParameterFile(presenter);
  }

  void runTestThatPolarizationCorrectionsUsesWorkspace() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getPolarizationCorrectionOption()).Times(2).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(m_view, getPolarizationEfficienciesWorkspace()).Times(1).WillOnce(Return("test_ws"));
    EXPECT_CALL(m_view, setPolarizationEfficienciesWorkspaceMode()).Times(1);
    EXPECT_CALL(m_view, enablePolarizationEfficiencies()).Times(1);

    presenter.notifySettingsChanged();

    assertPolarizationAnalysisWorkspace(presenter);
  }

  void runTestThatPolarizationCorrectionsUsesFilePath() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getPolarizationCorrectionOption()).Times(2).WillRepeatedly(Return("FilePath"));
    EXPECT_CALL(m_view, getPolarizationEfficienciesFilePath()).Times(1).WillOnce(Return("path/to/test_ws.nxs"));
    EXPECT_CALL(m_view, setPolarizationEfficienciesFilePathMode()).Times(1);
    EXPECT_CALL(m_view, enablePolarizationEfficiencies()).Times(1);

    presenter.notifySettingsChanged();

    assertPolarizationAnalysisWorkspace(presenter);
  }

  void runWithFloodCorrectionInputsDisabled(std::string const &type) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getFloodCorrectionType()).Times(2).WillRepeatedly(Return(type));
    EXPECT_CALL(m_view, disableFloodCorrectionInputs()).Times(1);
    EXPECT_CALL(m_view, getFloodWorkspace()).Times(0);
    presenter.notifySettingsChanged();
  }

  void runWithFloodCorrectionInputsEnabled(std::string const &type) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getFloodCorrectionType()).Times(2).WillRepeatedly(Return(type));
    EXPECT_CALL(m_view, enableFloodCorrectionInputs()).Times(1);
    presenter.notifySettingsChanged();
  }

  void runTestForValidTransmissionRunRange(RangeInLambda const &range, std::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStartOverlap()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getTransmissionEndOverlap()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showTransmissionRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.experiment().transmissionStitchOptions().overlapRange(), result);
  }

  void runTestForInvalidTransmissionRunRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStartOverlap()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getTransmissionEndOverlap()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showTransmissionRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.experiment().transmissionStitchOptions().overlapRange(), std::nullopt);
  }

  // These functions create various rows in the per-theta defaults tables,
  // either as an input array of strings or an output model
  OptionsRow optionsRowWithFirstAngle() { return {"0.5", "", "13463", ""}; }
  LookupRow defaultsWithFirstAngle() {
    return LookupRow(0.5, boost::none, TransmissionRunPair("13463", ""), boost::none, RangeInQ(), boost::none,
                     boost::none, boost::none, boost::none);
  }

  OptionsRow optionsRowWithSecondAngle() { return {"2.3", "", "13463", "13464"}; }
  LookupRow defaultsWithSecondAngle() {
    return LookupRow(2.3, boost::none, TransmissionRunPair("13463", "13464"), boost::none, RangeInQ(), boost::none,
                     boost::none, boost::none, boost::none);
  }
  OptionsRow optionsRowWithWildcard() { return {"", "", "13463", "13464"}; }
  OptionsRow optionsRowWithFirstTransmissionRun() { return {"", "", "13463"}; }
  OptionsRow optionsRowWithSecondTransmissionRun() { return {"", "", "", "13464"}; }
  OptionsRow optionsRowWithBothTransmissionRuns() { return {"", "", "13463", "13464"}; }
  OptionsRow optionsRowWithTransProcessingInstructions() { return {"", "", "", "", "1-4"}; }
  OptionsRow optionsRowWithTransProcessingInstructionsInvalid() { return {"", "", "", "", "bad"}; }
  OptionsRow optionsRowWithQMin() { return {"", "", "", "", "", "0.008"}; }
  OptionsRow optionsRowWithQMinInvalid() { return {"", "", "", "", "", "bad"}; }
  OptionsRow optionsRowWithQMax() { return {"", "", "", "", "", "", "0.1"}; }
  OptionsRow optionsRowWithQMaxInvalid() { return {"", "", "", "", "", "", "bad"}; }
  OptionsRow optionsRowWithQStep() { return {"", "", "", "", "", "", "", "0.02"}; }
  OptionsRow optionsRowWithQStepInvalid() { return {"", "", "", "", "", "", "", "bad"}; }
  OptionsRow optionsRowWithScale() { return {"", "", "", "", "", "", "", "", "1.4"}; }
  OptionsRow optionsRowWithScaleInvalid() { return {"", "", "", "", "", "", "", "", "bad"}; }
  OptionsRow optionsRowWithProcessingInstructions() { return {"", "", "", "", "", "", "", "", "", "1-4"}; }
  OptionsRow optionsRowWithProcessingInstructionsInvalid() { return {"", "", "", "", "", "", "", "", "", "bad"}; }
  OptionsRow optionsRowWithBackgroundProcessingInstructions() {
    return {"", "", "", "", "", "", "", "", "", "", "1-4"};
  }
  OptionsRow optionsRowWithBackgroundProcessingInstructionsInvalid() {
    return {"", "", "", "", "", "", "", "", "", "", "bad"};
  }

  void runTestForValidOptionsTable(OptionsTable const &optionsTable) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getLookupTable()).WillOnce(Return(optionsTable));
    EXPECT_CALL(m_view, showAllLookupRowsAsValid()).Times(1);
    presenter.notifyLookupRowChanged(1, 1);
  }

  void runTestForInvalidOptionsTable(OptionsTable const &optionsTable, const std::vector<int> &rows,
                                     std::vector<int> columns) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getLookupTable()).WillOnce(Return(optionsTable));
    for (auto row : rows) {
      for (auto col : columns) {
        EXPECT_CALL(m_view, showLookupRowAsInvalid(row, col)).Times(1);
      }
    }
    presenter.notifyLookupRowChanged(1, 1);
    TS_ASSERT(!presenter.hasValidSettings());
  }

  void runTestForInvalidOptionsTable(OptionsTable const &optionsTable, int row, std::vector<int> columns) {
    runTestForInvalidOptionsTable(optionsTable, std::vector<int>{row}, columns);
  }

  void runTestForNonUniqueAngles(OptionsTable const &optionsTable) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getLookupTable()).WillOnce(Return(optionsTable));
    for (auto row = 0; row < 2; ++row) {
      for (auto col = 0; col < 2; ++col) {
        EXPECT_CALL(m_view, showLookupRowAsInvalid(row, col)).Times(1);
        EXPECT_CALL(
            m_view,
            setTooltip(row, col,
                       "Error: Duplicated search criteria. No more than one row may have the same angle and title."))
            .Times(1);
      }
    }
    presenter.notifyLookupRowChanged(0, 0);
  }

  void runTestForValidTransmissionParams(std::string const &params) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStitchParams()).WillOnce(Return(params));
    EXPECT_CALL(m_view, showTransmissionStitchParamsValid());
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.experiment().transmissionStitchOptions().rebinParameters(), params);
  }

  void runTestForInvalidTransmissionParams(std::string const &params) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStitchParams()).WillOnce(Return(params));
    EXPECT_CALL(m_view, showTransmissionStitchParamsInvalid());
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.experiment().transmissionStitchOptions().rebinParameters(), "");
  }
};

GNU_DIAG_ON("missing-braces")
