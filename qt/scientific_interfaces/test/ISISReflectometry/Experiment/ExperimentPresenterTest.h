// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Experiment/ExperimentPresenter.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../ReflMockObjects.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/ReflectometryHelper.h"
#include "MockExperimentOptionDefaults.h"
#include "MockExperimentView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::
    ModelCreationHelper;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")

class ExperimentPresenterTest : public CxxTest::TestSuite {
  using OptionsRow = PerThetaDefaults::ValueArray;
  using OptionsTable = std::vector<OptionsRow>;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentPresenterTest *createSuite() {
    return new ExperimentPresenterTest();
  }
  static void destroySuite(ExperimentPresenterTest *suite) { delete suite; }

  ExperimentPresenterTest() : m_view() {
    Mantid::API::FrameworkManager::Instance();
  }

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testAllWidgetsAreEnabledWhenReductionPaused() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, enableAll()).Times(1);
    expectNotProcessingOrAutoreducing();
    presenter.notifyReductionPaused();

    verifyAndClear();
  }

  void testAllWidgetsAreDisabledWhenReductionResumed() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableAll()).Times(1);
    expectProcessing();
    presenter.notifyReductionResumed();

    verifyAndClear();
  }

  void testAllWidgetsAreEnabledWhenAutoreductionPaused() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, enableAll()).Times(1);
    expectNotProcessingOrAutoreducing();
    presenter.notifyAutoreductionPaused();

    verifyAndClear();
  }

  void testAllWidgetsAreDisabledWhenAutoreductionResumed() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableAll()).Times(1);
    expectAutoreducing();
    presenter.notifyAutoreductionResumed();

    verifyAndClear();
  }

  void testModelUpdatedWhenAnalysisModeChanged() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getAnalysisMode())
        .WillOnce(Return(std::string("MultiDetectorAnalysis")));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().analysisMode(),
                     AnalysisMode::MultiDetector);
    verifyAndClear();
  }

  void testModelUpdatedWhenSummationTypeChanged() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    presenter.notifySummationTypeChanged();

    TS_ASSERT_EQUALS(presenter.experiment().summationType(),
                     SummationType::SumInQ);
    verifyAndClear();
  }

  void testSumInQWidgetsDisabledWhenChangeToSumInLambda() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableReductionType()).Times(1);
    EXPECT_CALL(m_view, disableIncludePartialBins()).Times(1);
    presenter.notifySummationTypeChanged();

    verifyAndClear();
  }

  void testSumInQWidgetsEnabledWhenChangeToSumInQ() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, enableReductionType()).Times(1);
    EXPECT_CALL(m_view, enableIncludePartialBins()).Times(1);
    presenter.notifySummationTypeChanged();

    verifyAndClear();
  }

  void testChangingIncludePartialBinsUpdatesModel() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, getIncludePartialBins()).WillOnce(Return(true));
    presenter.notifySettingsChanged();

    TS_ASSERT(presenter.experiment().includePartialBins());
    verifyAndClear();
  }

  void testChangingDebugOptionUpdatesModel() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, getDebugOption()).WillOnce(Return(true));
    presenter.notifySettingsChanged();

    TS_ASSERT(presenter.experiment().debug());
    verifyAndClear();
  }

  void testSetBackgroundSubtractionUpdatesModel() {
    auto presenter = makePresenter();
    expectSubtractBackground();
    presenter.notifySettingsChanged();
    assertBackgroundSubtractionOptionsSet(presenter);
    verifyAndClear();
  }

  void
  testBackgroundSubtractionMethodIsEnabledWhenSubtractBackgroundIsChecked() {
    auto presenter = makePresenter();
    expectSubtractBackground(true);
    EXPECT_CALL(m_view, enableBackgroundSubtractionMethod()).Times(1);
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

  void testPolynomialInputsEnabledWhenSubtractingPolynomialBackground() {
    auto presenter = makePresenter();
    expectSubtractBackground(true, "Polynomial");
    EXPECT_CALL(m_view, enablePolynomialDegree()).Times(1);
    EXPECT_CALL(m_view, enableCostFunction()).Times(1);
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

  void testPolynomialInputsDisabledWhenSubtractingPerDetectorAverage() {
    auto presenter = makePresenter();
    expectSubtractBackground(true, "PerDetectorAverage");
    EXPECT_CALL(m_view, disablePolynomialDegree()).Times(1);
    EXPECT_CALL(m_view, disableCostFunction()).Times(1);
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

  void testPolynomialInputsDisabledWhenSubtractingAveragePixelFit() {
    auto presenter = makePresenter();
    expectSubtractBackground(true, "AveragePixelFit");
    EXPECT_CALL(m_view, disablePolynomialDegree()).Times(1);
    EXPECT_CALL(m_view, disableCostFunction()).Times(1);
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

  void testBackgroundSubtractionInputsDisabledWhenOptionTurnedOff() {
    auto presenter = makePresenter();
    expectSubtractBackground(false);
    EXPECT_CALL(m_view, disableBackgroundSubtractionMethod()).Times(1);
    EXPECT_CALL(m_view, disablePolynomialDegree()).Times(1);
    EXPECT_CALL(m_view, disableCostFunction()).Times(1);
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

  void testTogglePolarizationCorrectionOptionUpdatesModel() {
    auto presenter = makePresenter();
    expectPolarizationAnalysisOn();
    presenter.notifySettingsChanged();
    assertPolarizationAnalysisOn(presenter);
    verifyAndClear();
  }

  void testSetFloodCorrectionsUpdatesModel() {
    auto presenter = makePresenter();
    FloodCorrections floodCorr(FloodCorrectionType::Workspace,
                               std::string{"testWS"});

    EXPECT_CALL(m_view, getFloodCorrectionType()).WillOnce(Return("Workspace"));
    EXPECT_CALL(m_view, getFloodWorkspace())
        .WillOnce(Return(floodCorr.workspace().get()));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().floodCorrections(), floodCorr);
    verifyAndClear();
  }

  void testSetFloodCorrectionsToWorkspaceEnablesInputs() {
    runWithFloodCorrectionInputsEnabled("Workspace");
  }

  void testSetFloodCorrectionsToParameterFileDisablesInputs() {
    runWithFloodCorrectionInputsDisabled("ParameterFile");
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
    runTestForValidTransmissionRunRange(range, boost::none);
  }

  void testTransmissionParamsAreValidWithPositiveValue() {
    runTestForValidTransmissionParams("0.02");
  }

  void testTransmissionParamsAreValidWithNoValues() {
    runTestForValidTransmissionParams("");
  }

  void testTransmissionParamsAreValidWithNegativeValue() {
    runTestForValidTransmissionParams("-0.02");
  }

  void testTransmissionParamsAreValidWithThreeValues() {
    runTestForValidTransmissionParams("0.1, -0.02, 5");
  }

  void testTransmissionParamsAreValidWithFiveValues() {
    runTestForValidTransmissionParams("0.1, -0.02, 5, 6, 7.9");
  }

  void testTransmissionParamsIgnoresWhitespace() {
    runTestForValidTransmissionParams("    0.1  , -0.02 , 5   ");
  }

  void testTransmissionParamsAreInvalidWithTwoValues() {
    runTestForInvalidTransmissionParams("1, 2");
  }

  void testTransmissionParamsAreInvalidWithFourValues() {
    runTestForInvalidTransmissionParams("1, 2, 3, 4");
  }

  void testSetTransmissionScaleRHSProperty() {
    auto presenter = makePresenter();
    auto const scaleRHS = false;

    EXPECT_CALL(m_view, getTransmissionScaleRHSWorkspace())
        .WillOnce(Return(scaleRHS));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(
        presenter.experiment().transmissionStitchOptions().scaleRHS(),
        scaleRHS);
    verifyAndClear();
  }

  void testSetTransmissionParamsAreInvalidIfContainNonNumericValue() {
    auto presenter = makePresenter();
    auto const params = "1,bad";

    EXPECT_CALL(m_view, getTransmissionStitchParams()).WillOnce(Return(params));
    EXPECT_CALL(m_view, showTransmissionStitchParamsInvalid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(
        presenter.experiment().transmissionStitchOptions().rebinParameters(),
        "");
    verifyAndClear();
  }

  void testSetStitchOptions() {
    auto presenter = makePresenter();
    auto const optionsString = "Params=0.02";
    std::map<std::string, std::string> optionsMap = {{"Params", "0.02"}};

    EXPECT_CALL(m_view, getStitchOptions()).WillOnce(Return(optionsString));
    EXPECT_CALL(m_view, showStitchParametersValid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().stitchParameters(), optionsMap);
    verifyAndClear();
  }

  void testSetStitchOptionsInvalid() {
    auto presenter = makePresenter();
    auto const optionsString = "0.02";
    std::map<std::string, std::string> emptyOptionsMap;

    EXPECT_CALL(m_view, getStitchOptions()).WillOnce(Return(optionsString));
    EXPECT_CALL(m_view, showStitchParametersInvalid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().stitchParameters(),
                     emptyOptionsMap);
    verifyAndClear();
  }

  void testNewPerAngleDefaultsRequested() {
    auto presenter = makePresenter();

    // row should be added to view
    EXPECT_CALL(m_view, addPerThetaDefaultsRow());
    // new value should be requested from view to update model
    EXPECT_CALL(m_view, getPerAngleOptions()).Times(1);
    presenter.notifyNewPerAngleDefaultsRequested();

    verifyAndClear();
  }

  void testRemovePerAngleDefaultsRequested() {
    auto presenter = makePresenter();

    int const indexToRemove = 0;
    // row should be removed from view
    EXPECT_CALL(m_view, removePerThetaDefaultsRow(indexToRemove)).Times(1);
    // new value should be requested from view to update model
    EXPECT_CALL(m_view, getPerAngleOptions()).Times(1);
    presenter.notifyRemovePerAngleDefaultsRequested(indexToRemove);

    verifyAndClear();
  }

  void testChangingPerAngleDefaultsUpdatesModel() {
    auto presenter = makePresenter();

    auto const row = 1;
    auto const column = 0;
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(),
                                       optionsRowWithSecondAngle()};
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    presenter.notifyPerAngleDefaultsChanged(row, column);

    // Check the model contains the per-theta defaults returned by the view
    auto const perThetaDefaults = presenter.experiment().perThetaDefaults();
    TS_ASSERT_EQUALS(perThetaDefaults.size(), 2);
    TS_ASSERT_EQUALS(perThetaDefaults[0], defaultsWithFirstAngle());
    TS_ASSERT_EQUALS(perThetaDefaults[1], defaultsWithSecondAngle());
    verifyAndClear();
  }

  void testMultipleUniqueAnglesAreValid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(),
                                       optionsRowWithSecondAngle()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testMultipleNonUniqueAnglesAreInvalid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(),
                                       optionsRowWithFirstAngle()};
    runTestForNonUniqueAngles(optionsTable);
  }

  void testSingleWildcardRowIsValid() {
    OptionsTable const optionsTable = {optionsRowWithWildcard()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testAngleAndWildcardRowAreValid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(),
                                       optionsRowWithWildcard()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testMultipleWildcardRowsAreInvalid() {
    OptionsTable const optionsTable = {optionsRowWithWildcard(),
                                       optionsRowWithWildcard()};
    runTestForInvalidPerAngleOptions(optionsTable, {0, 1},
                                     PerThetaDefaults::Column::THETA);
  }

  void testSetFirstTransmissionRun() {
    OptionsTable const optionsTable = {optionsRowWithFirstTransmissionRun()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetSecondTransmissionRun() {
    OptionsTable const optionsTable = {optionsRowWithSecondTransmissionRun()};
    runTestForInvalidPerAngleOptions(optionsTable, 0,
                                     PerThetaDefaults::Column::FIRST_TRANS);
  }

  void testSetBothTransmissionRuns() {
    OptionsTable const optionsTable = {optionsRowWithBothTransmissionRuns()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetTransmissionProcessingInstructionsValid() {
    OptionsTable const optionsTable = {
        optionsRowWithTransProcessingInstructions()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetTransmissionProcessingInstructionsInvalid() {
    OptionsTable const optionsTable = {
        optionsRowWithTransProcessingInstructionsInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0,
                                     PerThetaDefaults::Column::TRANS_SPECTRA);
  }

  void testSetQMin() {
    OptionsTable const optionsTable = {optionsRowWithQMin()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetQMinInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQMinInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0,
                                     PerThetaDefaults::Column::QMIN);
  }

  void testSetQMax() {
    OptionsTable const optionsTable = {optionsRowWithQMax()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetQMaxInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQMaxInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0,
                                     PerThetaDefaults::Column::QMAX);
  }

  void testSetQStep() {
    OptionsTable const optionsTable = {optionsRowWithQStep()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetQStepInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQStepInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0,
                                     PerThetaDefaults::Column::QSTEP);
  }

  void testSetScale() {
    OptionsTable const optionsTable = {optionsRowWithScale()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetScaleInvalid() {
    OptionsTable const optionsTable = {optionsRowWithScaleInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0,
                                     PerThetaDefaults::Column::SCALE);
  }

  void testSetProcessingInstructions() {
    OptionsTable const optionsTable = {optionsRowWithProcessingInstructions()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetProcessingInstructionsInvalid() {
    OptionsTable const optionsTable = {
        optionsRowWithProcessingInstructionsInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0,
                                     PerThetaDefaults::Column::RUN_SPECTRA);
  }

  void testSetBackgroundProcessingInstructionsValid() {
    OptionsTable const optionsTable = {
        optionsRowWithBackgroundProcessingInstructions()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetBackgroundProcessingInstructionsInvalid() {
    OptionsTable const optionsTable = {
        optionsRowWithBackgroundProcessingInstructionsInvalid()};
    runTestForInvalidPerAngleOptions(
        optionsTable, 0, PerThetaDefaults::Column::BACKGROUND_SPECTRA);
  }

  void testChangingSettingsNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

  void testChangingPerAngleDefaultsNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifyPerAngleDefaultsChanged(0, 0);
    verifyAndClear();
  }

  void testRestoreDefaultsUpdatesInstrument() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyUpdateInstrumentRequested()).Times(1);
    presenter.notifyRestoreDefaultsRequested();
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesAnalysisModeInView() {
    auto model = makeModelWithAnalysisMode(AnalysisMode::MultiDetector);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setAnalysisMode("MultiDetectorAnalysis")).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesAnalysisModeInModel() {
    auto model = makeModelWithAnalysisMode(AnalysisMode::MultiDetector);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    TS_ASSERT_EQUALS(presenter.experiment().analysisMode(),
                     AnalysisMode::MultiDetector);
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesReductionOptionsInView() {
    auto model = makeModelWithReduction(SummationType::SumInQ,
                                        ReductionType::NonFlatSample, true);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setSummationType("SumInQ")).Times(1);
    EXPECT_CALL(m_view, setReductionType("NonFlatSample")).Times(1);
    EXPECT_CALL(m_view, setIncludePartialBins(true)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesReductionOptionsInModel() {
    auto model = makeModelWithReduction(SummationType::SumInQ,
                                        ReductionType::NonFlatSample, true);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    TS_ASSERT_EQUALS(presenter.experiment().summationType(),
                     SummationType::SumInQ);
    TS_ASSERT_EQUALS(presenter.experiment().reductionType(),
                     ReductionType::NonFlatSample);
    TS_ASSERT_EQUALS(presenter.experiment().includePartialBins(), true);
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesDebugOptionsInView() {
    auto model = makeModelWithDebug(true);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setDebugOption(true)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesDebugOptionsInModel() {
    auto model = makeModelWithDebug(true);
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    TS_ASSERT_EQUALS(presenter.experiment().debug(), true);
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesPerThetaInView() {
    auto perThetaDefaults =
        PerThetaDefaults(boost::none, TransmissionRunPair(), boost::none,
                         RangeInQ(0.01, 0.03, 0.2), 0.7, std::string("390-415"),
                         std::string("370-389,416-430"));
    auto model = makeModelWithPerThetaDefaults(std::move(perThetaDefaults));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    auto const expected = std::vector<PerThetaDefaults::ValueArray>{
        {"", "", "", "", "0.010000", "0.200000", "0.030000", "0.700000",
         "390-415", "370-389,416-430"}};
    EXPECT_CALL(m_view, setPerAngleOptions(expected)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesPerThetaInModel() {
    auto model = makeModelWithPerThetaDefaults(
        PerThetaDefaults(boost::none, TransmissionRunPair(), boost::none,
                         RangeInQ(0.01, 0.03, 0.2), 0.7, std::string("390-415"),
                         std::string("370-389,416-430")));
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    auto expected =
        PerThetaDefaults(boost::none, TransmissionRunPair(), boost::none,
                         RangeInQ(0.01, 0.03, 0.2), 0.7, std::string("390-415"),
                         std::string("370-389,416-430"));
    TS_ASSERT_EQUALS(presenter.experiment().perThetaDefaults().size(), 1);
    TS_ASSERT_EQUALS(presenter.experiment().perThetaDefaults().front(),
                     expected);
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesTransmissionRunRangeInView() {
    auto model = makeModelWithTransmissionRunRange(RangeInLambda{10.0, 12.0});
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setTransmissionStartOverlap(10.0)).Times(1);
    EXPECT_CALL(m_view, setTransmissionEndOverlap(12.0)).Times(1);
    EXPECT_CALL(m_view, showTransmissionRangeValid()).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesTransmissionRunRangeInModel() {
    auto model = makeModelWithTransmissionRunRange(RangeInLambda{10.0, 12.0});
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    auto const expected = RangeInLambda{10.0, 12.0};
    TS_ASSERT_EQUALS(
        presenter.experiment().transmissionStitchOptions().overlapRange(),
        expected);
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesCorrectionInView() {
    auto model = makeModelWithCorrections(
        PolarizationCorrections(PolarizationCorrectionType::ParameterFile),
        FloodCorrections(FloodCorrectionType::ParameterFile),
        makeBackgroundSubtraction());
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    EXPECT_CALL(m_view, setPolarizationCorrectionOption(true)).Times(1);
    EXPECT_CALL(m_view, setFloodCorrectionType("ParameterFile")).Times(1);
    EXPECT_CALL(m_view, setSubtractBackground(true));
    EXPECT_CALL(m_view, setBackgroundSubtractionMethod("Polynomial"));
    EXPECT_CALL(m_view, setPolynomialDegree(3));
    EXPECT_CALL(m_view, setCostFunction("Unweighted least squares"));
    presenter.notifyInstrumentChanged("POLREF");
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesCorrectionInModel() {
    auto model = makeModelWithCorrections(
        PolarizationCorrections(PolarizationCorrectionType::ParameterFile),
        FloodCorrections(FloodCorrectionType::ParameterFile),
        makeBackgroundSubtraction());
    auto defaultOptions = expectDefaults(model);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    assertBackgroundSubtractionOptionsSet(presenter);
    assertPolarizationAnalysisOn(presenter);
    assertFloodCorrectionUsesParameterFile(presenter);
    verifyAndClear();
  }

  void testInstrumentChangedDisconnectsNotificationsBackFromView() {
    auto defaultOptions = expectDefaults(makeEmptyExperiment());
    EXPECT_CALL(m_view, disconnectExperimentSettingsWidgets()).Times(1);
    EXPECT_CALL(m_view, connectExperimentSettingsWidgets()).Times(1);
    auto presenter = makePresenter(std::move(defaultOptions));
    presenter.notifyInstrumentChanged("POLREF");
    verifyAndClear();
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

private:
  NiceMock<MockExperimentView> m_view;
  NiceMock<MockBatchPresenter> m_mainPresenter;
  double m_thetaTolerance{0.01};

  Experiment makeModelWithAnalysisMode(AnalysisMode analysisMode) {
    return Experiment(
        analysisMode, ReductionType::Normal, SummationType::SumInLambda, false,
        false, BackgroundSubtraction(), makeEmptyPolarizationCorrections(),
        makeFloodCorrections(), makeEmptyTransmissionStitchOptions(),
        makeEmptyStitchOptions(), makePerThetaDefaults());
  }

  Experiment makeModelWithReduction(SummationType summationType,
                                    ReductionType reductionType,
                                    bool includePartialBins) {
    return Experiment(AnalysisMode::PointDetector, reductionType, summationType,
                      includePartialBins, false, BackgroundSubtraction(),
                      makeEmptyPolarizationCorrections(),
                      makeFloodCorrections(),
                      makeEmptyTransmissionStitchOptions(),
                      makeEmptyStitchOptions(), makePerThetaDefaults());
  }

  Experiment makeModelWithDebug(bool debug) {
    return Experiment(
        AnalysisMode::PointDetector, ReductionType::Normal,
        SummationType::SumInLambda, false, debug, BackgroundSubtraction(),
        makeEmptyPolarizationCorrections(), makeFloodCorrections(),
        makeEmptyTransmissionStitchOptions(), makeEmptyStitchOptions(),
        makePerThetaDefaults());
  }

  Experiment makeModelWithPerThetaDefaults(PerThetaDefaults perThetaDefaults) {
    auto perThetaList = std::vector<PerThetaDefaults>();
    perThetaList.emplace_back(std::move(perThetaDefaults));
    return Experiment(
        AnalysisMode::PointDetector, ReductionType::Normal,
        SummationType::SumInLambda, false, false, BackgroundSubtraction(),
        makeEmptyPolarizationCorrections(), makeFloodCorrections(),
        makeEmptyTransmissionStitchOptions(), makeEmptyStitchOptions(),
        std::move(perThetaList));
  }

  Experiment makeModelWithTransmissionRunRange(RangeInLambda range) {
    return Experiment(
        AnalysisMode::PointDetector, ReductionType::Normal,
        SummationType::SumInLambda, false, false, BackgroundSubtraction(),
        makeEmptyPolarizationCorrections(), makeFloodCorrections(),
        TransmissionStitchOptions(std::move(range), std::string(), false),
        makeEmptyStitchOptions(), makePerThetaDefaults());
  }

  Experiment
  makeModelWithCorrections(PolarizationCorrections polarizationCorrections,
                           FloodCorrections floodCorrections,
                           BackgroundSubtraction backgroundSubtraction) {
    return Experiment(
        AnalysisMode::PointDetector, ReductionType::Normal,
        SummationType::SumInLambda, false, false,
        std::move(backgroundSubtraction), std::move(polarizationCorrections),
        std::move(floodCorrections), makeEmptyTransmissionStitchOptions(),
        makeEmptyStitchOptions(), makePerThetaDefaults());
  }

  ExperimentPresenter
  makePresenter(std::unique_ptr<IExperimentOptionDefaults> defaultOptions =
                    std::make_unique<MockExperimentOptionDefaults>()) {
    // The presenter gets values from the view on construction so the view must
    // return something sensible
    auto presenter =
        ExperimentPresenter(&m_view, makeEmptyExperiment(), m_thetaTolerance,
                            std::move(defaultOptions));
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
  }

  void expectProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(1)
        .WillOnce(Return(true));
  }

  void expectAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(1)
        .WillOnce(Return(true));
  }

  void expectNotProcessingOrAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(1)
        .WillOnce(Return(false));
  }

  void expectViewReturnsSumInQDefaults() {
    EXPECT_CALL(m_view, getSummationType())
        .WillOnce(Return(std::string("SumInQ")));
    EXPECT_CALL(m_view, getReductionType())
        .WillOnce(Return(std::string("DivergentBeam")));
  }

  void expectSubtractBackground(
      bool subtractBackground = true,
      std::string const &subtractionType = std::string("Polynomial"),
      int degreeOfPolynomial = 3,
      std::string const &costFunction =
          std::string("Unweighted least squares")) {
    EXPECT_CALL(m_view, getSubtractBackground())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(subtractBackground));
    EXPECT_CALL(m_view, getBackgroundSubtractionMethod())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(subtractionType));
    EXPECT_CALL(m_view, getPolynomialDegree())
        .Times(1)
        .WillRepeatedly(Return(degreeOfPolynomial));
    EXPECT_CALL(m_view, getCostFunction())
        .Times(1)
        .WillRepeatedly(Return(costFunction));
  }

  void assertBackgroundSubtractionOptionsSet(
      ExperimentPresenter const &presenter, bool subtractBackground = true,
      BackgroundSubtractionType subtractionType =
          BackgroundSubtractionType::Polynomial,
      int degreeOfPolynomial = 3,
      CostFunctionType costFunction =
          CostFunctionType::UnweightedLeastSquares) {
    TS_ASSERT_EQUALS(
        presenter.experiment().backgroundSubtraction().subtractBackground(),
        subtractBackground);
    TS_ASSERT_EQUALS(
        presenter.experiment().backgroundSubtraction().subtractionType(),
        subtractionType);
    TS_ASSERT_EQUALS(
        presenter.experiment().backgroundSubtraction().degreeOfPolynomial(),
        degreeOfPolynomial);
    TS_ASSERT_EQUALS(
        presenter.experiment().backgroundSubtraction().costFunction(),
        costFunction);
  }

  void expectPolarizationAnalysisOn() {
    EXPECT_CALL(m_view, getPolarizationCorrectionOption())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
  }

  void assertPolarizationAnalysisOn(ExperimentPresenter const &presenter) {
    TS_ASSERT_EQUALS(
        presenter.experiment().polarizationCorrections().correctionType(),
        PolarizationCorrectionType::ParameterFile);
  }

  void
  assertFloodCorrectionUsesParameterFile(ExperimentPresenter const &presenter) {
    TS_ASSERT_EQUALS(presenter.experiment().floodCorrections().correctionType(),
                     FloodCorrectionType::ParameterFile);
  }

  std::unique_ptr<MockExperimentOptionDefaults>
  expectDefaults(Experiment const &model) {
    // Create a defaults object, set expectations on it, and return it so
    // that it can be passed to the presenter
    auto defaultOptions = std::make_unique<MockExperimentOptionDefaults>();
    EXPECT_CALL(*defaultOptions, get(_)).Times(1).WillOnce(Return(model));
    return defaultOptions;
  }

  void runTestThatPolarizationCorrectionsAreEnabledForInstrument(
      std::string const &instrument) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_mainPresenter, instrumentName())
        .Times(1)
        .WillOnce(Return(instrument));
    EXPECT_CALL(m_view, enablePolarizationCorrections()).Times(1);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void runTestThatPolarizationCorrectionsAreDisabledForInstrument(
      std::string const &instrument) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_mainPresenter, instrumentName())
        .Times(1)
        .WillOnce(Return(instrument));
    EXPECT_CALL(m_view, setPolarizationCorrectionOption(false)).Times(1);
    EXPECT_CALL(m_view, disablePolarizationCorrections()).Times(1);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void runWithFloodCorrectionInputsDisabled(std::string const &type) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getFloodCorrectionType()).WillOnce(Return(type));
    EXPECT_CALL(m_view, disableFloodCorrectionInputs()).Times(1);
    EXPECT_CALL(m_view, getFloodWorkspace()).Times(0);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void runWithFloodCorrectionInputsEnabled(std::string const &type) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getFloodCorrectionType()).WillOnce(Return(type));
    EXPECT_CALL(m_view, enableFloodCorrectionInputs()).Times(1);
    EXPECT_CALL(m_view, getFloodWorkspace()).Times(1);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void runTestForValidTransmissionRunRange(
      RangeInLambda const &range,
      boost::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStartOverlap())
        .WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getTransmissionEndOverlap())
        .WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showTransmissionRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(
        presenter.experiment().transmissionStitchOptions().overlapRange(),
        result);
    verifyAndClear();
  }

  void runTestForInvalidTransmissionRunRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStartOverlap())
        .WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getTransmissionEndOverlap())
        .WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showTransmissionRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(
        presenter.experiment().transmissionStitchOptions().overlapRange(),
        boost::none);
    verifyAndClear();
  }

  // These functions create various rows in the per-theta defaults tables,
  // either as an input array of strings or an output model
  OptionsRow optionsRowWithFirstAngle() { return {"0.5", "13463", ""}; }
  PerThetaDefaults defaultsWithFirstAngle() {
    return PerThetaDefaults(0.5, TransmissionRunPair("13463", ""), boost::none,
                            RangeInQ(), boost::none, boost::none, boost::none);
  }

  OptionsRow optionsRowWithSecondAngle() { return {"2.3", "13463", "13464"}; }
  PerThetaDefaults defaultsWithSecondAngle() {
    return PerThetaDefaults(2.3, TransmissionRunPair("13463", "13464"),
                            boost::none, RangeInQ(), boost::none, boost::none,
                            boost::none);
  }
  OptionsRow optionsRowWithWildcard() { return {"", "13463", "13464"}; }
  OptionsRow optionsRowWithFirstTransmissionRun() { return {"", "13463"}; }
  OptionsRow optionsRowWithSecondTransmissionRun() { return {"", "", "13464"}; }
  OptionsRow optionsRowWithBothTransmissionRuns() {
    return {"", "13463", "13464"};
  }
  OptionsRow optionsRowWithTransProcessingInstructions() {
    return {"", "", "", "1-4"};
  }
  OptionsRow optionsRowWithTransProcessingInstructionsInvalid() {
    return {"", "", "", "bad"};
  }
  OptionsRow optionsRowWithQMin() { return {"", "", "", "", "0.008"}; }
  OptionsRow optionsRowWithQMinInvalid() { return {"", "", "", "", "bad"}; }
  OptionsRow optionsRowWithQMax() { return {"", "", "", "", "", "0.1"}; }
  OptionsRow optionsRowWithQMaxInvalid() { return {"", "", "", "", "", "bad"}; }
  OptionsRow optionsRowWithQStep() { return {"", "", "", "", "", "", "0.02"}; }
  OptionsRow optionsRowWithQStepInvalid() {
    return {"", "", "", "", "", "", "bad"};
  }
  OptionsRow optionsRowWithScale() {
    return {"", "", "", "", "", "", "", "1.4"};
  }
  OptionsRow optionsRowWithScaleInvalid() {
    return {"", "", "", "", "", "", "", "bad"};
  }
  OptionsRow optionsRowWithProcessingInstructions() {
    return {"", "", "", "", "", "", "", "", "1-4"};
  }
  OptionsRow optionsRowWithProcessingInstructionsInvalid() {
    return {"", "", "", "", "", "", "", "", "bad"};
  }
  OptionsRow optionsRowWithBackgroundProcessingInstructions() {
    return {"", "", "", "", "", "", "", "", "", "1-4"};
  }
  OptionsRow optionsRowWithBackgroundProcessingInstructionsInvalid() {
    return {"", "", "", "", "", "", "", "", "", "bad"};
  }

  void runTestForValidPerAngleOptions(OptionsTable const &optionsTable) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    EXPECT_CALL(m_view, showAllPerAngleOptionsAsValid()).Times(1);
    presenter.notifyPerAngleDefaultsChanged(1, 1);
    verifyAndClear();
  }

  void runTestForInvalidPerAngleOptions(OptionsTable const &optionsTable,
                                        const std::vector<int> &rows,
                                        int column) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    for (auto row : rows)
      EXPECT_CALL(m_view, showPerAngleOptionsAsInvalid(row, column)).Times(1);
    presenter.notifyPerAngleDefaultsChanged(1, 1);
    verifyAndClear();
  }

  void runTestForInvalidPerAngleOptions(OptionsTable const &optionsTable,
                                        int row, int column) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    EXPECT_CALL(m_view, showPerAngleOptionsAsInvalid(row, column)).Times(1);
    presenter.notifyPerAngleDefaultsChanged(1, 1);
    verifyAndClear();
  }

  void runTestForNonUniqueAngles(OptionsTable const &optionsTable) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    EXPECT_CALL(m_view, showPerAngleThetasNonUnique(m_thetaTolerance)).Times(1);
    presenter.notifyPerAngleDefaultsChanged(0, 0);
    verifyAndClear();
  }

  void runTestForValidTransmissionParams(std::string const &params) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStitchParams()).WillOnce(Return(params));
    EXPECT_CALL(m_view, showTransmissionStitchParamsValid());
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(
        presenter.experiment().transmissionStitchOptions().rebinParameters(),
        params);
    verifyAndClear();
  }

  void runTestForInvalidTransmissionParams(std::string const &params) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStitchParams()).WillOnce(Return(params));
    EXPECT_CALL(m_view, showTransmissionStitchParamsInvalid());
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(
        presenter.experiment().transmissionStitchOptions().rebinParameters(),
        "");
    verifyAndClear();
  }
};

GNU_DIAG_ON("missing-braces")
