// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/GUI/Batch/RowProcessingAlgorithm.h"
#include "../../../ISISReflectometry/Reduction/Batch.h"
#include "../../../ISISReflectometry/Reduction/PreviewRow.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using MantidQt::API::IAlgorithmRuntimeProps;

namespace {
void assertProperty(IAlgorithmRuntimeProps const &props, std::string const &name, double expected) {
  TS_ASSERT_DELTA(static_cast<double>(props.getProperty(name)), expected, 1e-6);
}
} // namespace

class RowProcessingAlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RowProcessingAlgorithmTest *createSuite() { return new RowProcessingAlgorithmTest(); }
  static void destroySuite(RowProcessingAlgorithmTest *suite) { delete suite; }

  RowProcessingAlgorithmTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"}, m_thetaTolerance(0.01),
        m_experiment(makeExperiment()), m_instrument(makeInstrument()),
        m_runsTable(m_instruments, m_thetaTolerance, ReductionJobs()), m_slicing() {}

  void testExperimentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model);
    checkExperimentSettings(*result);
  }

  void testExperimentSettingsWithEmptyRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeEmptyRow();
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    checkExperimentSettings(*result);
  }

  void testExperimentSettingsWithPreviewRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto result = Reduction::createAlgorithmRuntimeProps(model, makePreviewRow());
    checkExperimentSettings(*result);
  }

  void testLookupRowWithAngleLookup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // angle within tolerance of 2.3
    auto row = makeRow(2.29);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    checkMatchesAngleRow(*result);
  }

  // TODO
  //  void testLookupPreviewRowWithAngleLookup() {
  //    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
  //    // angle within tolerance of 2.3
  //    auto row = makePreviewRow(2.29);
  //    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
  //    checkMatchesAngleRow(*result);
  //  }

  void testLookupRowWithWildcardLookup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // angle outside tolerance of any angle matches wildcard row instead
    auto row = makeRow(2.28);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    checkMatchesWildcardRow(*result);
  }

  // TODO
  //  void testLookupPreviewRowWithWildcardLookup() {
  //    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
  //    // angle outside tolerance of any angle matches wildcard row instead
  //    auto row = makePreviewRow(2.28);
  //    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
  //    checkMatchesWildcardRow(*result);
  //  }

  void testInstrumentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model);
    checkMatchesInstrument(*result);
  }

  void testInstrumentSettingsWithEmptyRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeEmptyRow();
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    checkMatchesInstrument(*result);
  }

  void testInstrumentSettingsWithPreviewRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto result = Reduction::createAlgorithmRuntimeProps(model, makePreviewRow());
    checkMatchesInstrument(*result);
  }

  void testSettingsForSlicingWithEmptyRow() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto row = makeEmptyRow();
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    checkMatchesSlicing(*result);
  }

  void testSettingsForSlicingWithPreviewRow() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = Reduction::createAlgorithmRuntimeProps(model, makePreviewRow());
    checkMatchesSlicing(*result);
  }

  void testSettingsForSlicingByTime() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model);
    checkMatchesSlicingByTime(*result);
  }

  void testSettingsForSlicingByTimeWithPreviewRow() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = Reduction::createAlgorithmRuntimeProps(model, makePreviewRow());
    checkMatchesSlicingByTime(*result);
  }

  void testSettingsForSlicingByNumberOfSlices() {
    auto slicing = Slicing(UniformSlicingByNumberOfSlices(3));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model);
    checkMatchesSlicingByNumber(*result);
  }

  void testSettingsForSlicingByNumberOfSlicesWithPreviewRow() {
    auto slicing = Slicing(UniformSlicingByNumberOfSlices(3));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = Reduction::createAlgorithmRuntimeProps(model, makePreviewRow());
    checkMatchesSlicingByNumber(*result);
  }

  void testSettingsForSlicingByList() {
    auto slicing = Slicing(CustomSlicingByList({3.1, 10.2, 47.35}));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model);
    checkMatchesSlicingByList(*result);
  }

  void testSettingsForSlicingByListWithPreviewRow() {
    auto slicing = Slicing(CustomSlicingByList({3.1, 10.2, 47.35}));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = Reduction::createAlgorithmRuntimeProps(model, makePreviewRow());
    checkMatchesSlicingByList(*result);
  }

  void testSettingsForSlicingByLog() {
    auto slicing = Slicing(SlicingByEventLog({18.2}, "test_log_name"));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model);
    checkMatchesSlicingByLog(*result);
  }

  void testSettingsForSlicingByLogWithPreviewRow() {
    auto slicing = Slicing(SlicingByEventLog({18.2}, "test_log_name"));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = Reduction::createAlgorithmRuntimeProps(model, makePreviewRow());
    checkMatchesSlicingByLog(*result);
  }

  void testSettingsForRowCellValues() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // Use an angle that will match per-theta defaults. They should be
    // overridden by the cell values
    auto row = makeRowWithMainCellsFilled(2.3);
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);

    TS_ASSERT_EQUALS(result->getPropertyValue("InputRunList"), "12345, 12346");
    TS_ASSERT_EQUALS(result->getPropertyValue("FirstTransmissionRunList"), "92345");
    TS_ASSERT_EQUALS(result->getPropertyValue("SecondTransmissionRunList"), "92346");
    assertProperty(*result, "ThetaIn", 2.3);
    assertProperty(*result, "MomentumTransferMin", 0.1);
    assertProperty(*result, "MomentumTransferStep", 0.09);
    assertProperty(*result, "MomentumTransferMax", 0.91);
    assertProperty(*result, "ScaleFactor", 2.2);
  }

  void testAddingPropertyViaOptionsCell() {
    // This tests adding a property via the options cell on a row, for a
    // property that does not get set anywhere else on the GUI
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(2.3, ReductionOptionsMap{{"ThetaLogName", "theta_log_name"}});
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("ThetaLogName"), "theta_log_name");
  }

  void testOptionsCellOverridesExperimentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(
        2.3, ReductionOptionsMap{{"AnalysisMode", "PointDetectorAnalysis"}, {"ReductionType", "DivergentBeam"}});
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("AnalysisMode"), "PointDetectorAnalysis");
    TS_ASSERT_EQUALS(result->getPropertyValue("ReductionType"), "DivergentBeam");
  }

  void testOptionsCellOverridesLookupRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // Use an angle that will match per-theta defaults. They should be
    // overridden by the cell values
    auto row = makeRowWithOptionsCellFilled(2.3, ReductionOptionsMap{{"ProcessingInstructions", "390-410"}});
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("ProcessingInstructions"), "390-410");
  }

  void testOptionsCellOverridesInstrumentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(2.3, ReductionOptionsMap{{"WavelengthMin", "3.3"}});
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);
    assertProperty(*result, "WavelengthMin", 3.3);
  }

  void testOptionsCellOverridesSubtractBackgroundAndStillPicksUpSettings() {
    auto experiment =
        Experiment(AnalysisMode::MultiDetector, ReductionType::NonFlatSample, SummationType::SumInQ, true, true,
                   BackgroundSubtraction(false, BackgroundSubtractionType::AveragePixelFit, 3,
                                         CostFunctionType::UnweightedLeastSquares),
                   makePolarizationCorrections(), makeFloodCorrections(), makeTransmissionStitchOptions(),
                   makeStitchOptions(), makeLookupTableWithTwoAnglesAndWildcard());
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(2.3, ReductionOptionsMap{{"SubtractBackground", "1"}});
    auto result = RowProcessing::createAlgorithmRuntimeProps(model, row);

    TS_ASSERT_EQUALS(result->getPropertyValue("SubtractBackground"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("BackgroundCalculationMethod"), "AveragePixelFit");
    TS_ASSERT_EQUALS(result->getPropertyValue("DegreeOfPolynomial"), "3");
    TS_ASSERT_EQUALS(result->getPropertyValue("CostFunction"), "Unweighted least squares");
  }

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;

  PreviewRow makePreviewRow() { return PreviewRow({"12345"}); }

  void checkExperimentSettings(IAlgorithmRuntimeProps const &result) {
    TS_ASSERT_EQUALS(result.getPropertyValue("AnalysisMode"), "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(result.getPropertyValue("ReductionType"), "NonFlatSample");
    TS_ASSERT_EQUALS(result.getPropertyValue("SummationType"), "SumInQ");
    TS_ASSERT_EQUALS(result.getPropertyValue("IncludePartialBins"), "1");
    TS_ASSERT_EQUALS(result.getPropertyValue("Debug"), "1");
    TS_ASSERT_EQUALS(result.getPropertyValue("SubtractBackground"), "1");
    TS_ASSERT_EQUALS(result.getPropertyValue("BackgroundCalculationMethod"), "Polynomial");
    TS_ASSERT_EQUALS(result.getPropertyValue("DegreeOfPolynomial"), "3");
    TS_ASSERT_EQUALS(result.getPropertyValue("CostFunction"), "Unweighted least squares");
    TS_ASSERT_EQUALS(result.getPropertyValue("PolarizationAnalysis"), "1");
    TS_ASSERT_EQUALS(result.getPropertyValue("FloodCorrection"), "Workspace");
    TS_ASSERT_EQUALS(result.getPropertyValue("FloodWorkspace"), "test_workspace");
    assertProperty(result, "StartOverlap", 7.5);
    assertProperty(result, "EndOverlap", 9.2);
    TS_ASSERT_EQUALS(result.getPropertyValue("Params"), "-0.02");
    TS_ASSERT_EQUALS(result.getPropertyValue("ScaleRHSWorkspace"), "1");
  }

  void checkMatchesAngleRow(IAlgorithmRuntimeProps const &result) {
    TS_ASSERT_EQUALS(result.getPropertyValue("FirstTransmissionRunList"), "22348, 22349");
    TS_ASSERT_EQUALS(result.getPropertyValue("SecondTransmissionRunList"), "22358, 22359");
    TS_ASSERT_EQUALS(result.getPropertyValue("TransmissionProcessingInstructions"), "4");
    assertProperty(result, "MomentumTransferMin", 0.009);
    assertProperty(result, "MomentumTransferStep", 0.03);
    assertProperty(result, "MomentumTransferMax", 1.3);
    assertProperty(result, "ScaleFactor", 0.9);
    TS_ASSERT_EQUALS(result.getPropertyValue("ProcessingInstructions"), "4-6");
    TS_ASSERT_EQUALS(result.getPropertyValue("BackgroundProcessingInstructions"), "2-3,7-8");
  }

  void checkMatchesWildcardRow(IAlgorithmRuntimeProps const &result) {
    TS_ASSERT_EQUALS(result.getPropertyValue("FirstTransmissionRunList"), "22345");
    TS_ASSERT_EQUALS(result.getPropertyValue("SecondTransmissionRunList"), "22346");
    TS_ASSERT_EQUALS(result.getPropertyValue("TransmissionProcessingInstructions"), "5-6");
    assertProperty(result, "MomentumTransferMin", 0.007);
    assertProperty(result, "MomentumTransferStep", 0.01);
    assertProperty(result, "MomentumTransferMax", 1.1);
    assertProperty(result, "ScaleFactor", 0.7);
    TS_ASSERT_EQUALS(result.getPropertyValue("ProcessingInstructions"), "1");
    TS_ASSERT_EQUALS(result.getPropertyValue("BackgroundProcessingInstructions"), "3,7");
  }

  void checkMatchesInstrument(IAlgorithmRuntimeProps const &result) {
    assertProperty(result, "WavelengthMin", 2.3);
    assertProperty(result, "WavelengthMax", 14.4);
    TS_ASSERT_EQUALS(result.getPropertyValue("I0MonitorIndex"), "2");
    TS_ASSERT_EQUALS(result.getPropertyValue("NormalizeByIntegratedMonitors"), "1");
    assertProperty(result, "MonitorBackgroundWavelengthMin", 1.1);
    assertProperty(result, "MonitorBackgroundWavelengthMax", 17.2);
    assertProperty(result, "MonitorIntegrationWavelengthMin", 3.4);
    assertProperty(result, "MonitorIntegrationWavelengthMax", 10.8);
    TS_ASSERT_EQUALS(result.getPropertyValue("CorrectDetectors"), "1");
    TS_ASSERT_EQUALS(result.getPropertyValue("DetectorCorrectionType"), "RotateAroundSample");
  }

  void checkMatchesSlicing(IAlgorithmRuntimeProps const &result) { assertProperty(result, "TimeInterval", 123.4); }

  void checkMatchesSlicingByTime(IAlgorithmRuntimeProps const &result) {
    assertProperty(result, "TimeInterval", 123.4);
  }

  void checkMatchesSlicingByNumber(IAlgorithmRuntimeProps const &result) {
    TS_ASSERT_EQUALS(result.getPropertyValue("NumberOfSlices"), "3");
  }

  void checkMatchesSlicingByList(IAlgorithmRuntimeProps const &result) {
    TS_ASSERT_EQUALS(result.getPropertyValue("TimeInterval"), "3.1, 10.2, 47.35");
  }

  void checkMatchesSlicingByLog(IAlgorithmRuntimeProps const &result) {
    TS_ASSERT_EQUALS(result.getPropertyValue("LogName"), "test_log_name");
    assertProperty(result, "LogValueInterval", 18.2);
  }
};
