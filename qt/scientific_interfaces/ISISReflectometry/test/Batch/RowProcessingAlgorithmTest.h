// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/GUI/Batch/RowProcessingAlgorithm.h"
#include "../../../ISISReflectometry/Reduction/Batch.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;

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
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result->getPropertyValue("AnalysisMode"), "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(result->getPropertyValue("ReductionType"), "NonFlatSample");
    TS_ASSERT_EQUALS(result->getPropertyValue("SummationType"), "SumInQ");
    TS_ASSERT_EQUALS(result->getPropertyValue("IncludePartialBins"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("Debug"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("SubtractBackground"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("BackgroundCalculationMethod"), "Polynomial");
    TS_ASSERT_EQUALS(result->getPropertyValue("DegreeOfPolynomial"), "3");
    TS_ASSERT_EQUALS(result->getPropertyValue("CostFunction"), "Unweighted least squares");
    TS_ASSERT_EQUALS(result->getPropertyValue("PolarizationAnalysis"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("FloodCorrection"), "Workspace");
    TS_ASSERT_EQUALS(result->getPropertyValue("FloodWorkspace"), "test_workspace");
    TS_ASSERT_EQUALS(result->getPropertyValue("StartOverlap"), "7.500000");
    TS_ASSERT_EQUALS(result->getPropertyValue("EndOverlap"), "9.200000");
    TS_ASSERT_EQUALS(result->getPropertyValue("Params"), "-0.02");
    TS_ASSERT_EQUALS(result->getPropertyValue("ScaleRHSWorkspace"), "1");
  }

  void testExperimentSettingsWithEmptyRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("AnalysisMode"), "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(result->getPropertyValue("ReductionType"), "NonFlatSample");
    TS_ASSERT_EQUALS(result->getPropertyValue("SummationType"), "SumInQ");
    TS_ASSERT_EQUALS(result->getPropertyValue("IncludePartialBins"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("Debug"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("SubtractBackground"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("BackgroundCalculationMethod"), "Polynomial");
    TS_ASSERT_EQUALS(result->getPropertyValue("DegreeOfPolynomial"), "3");
    TS_ASSERT_EQUALS(result->getPropertyValue("CostFunction"), "Unweighted least squares");
    TS_ASSERT_EQUALS(result->getPropertyValue("PolarizationAnalysis"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("FloodCorrection"), "Workspace");
    TS_ASSERT_EQUALS(result->getPropertyValue("FloodWorkspace"), "test_workspace");
    TS_ASSERT_EQUALS(result->getPropertyValue("StartOverlap"), "7.500000");
    TS_ASSERT_EQUALS(result->getPropertyValue("EndOverlap"), "9.200000");
  }

  void testLookupRowWithAngleLookup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // angle within tolerance of 2.3
    auto row = makeRow(2.29);
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("FirstTransmissionRunList"), "22348, 22349");
    TS_ASSERT_EQUALS(result->getPropertyValue("SecondTransmissionRunList"), "22358, 22359");
    TS_ASSERT_EQUALS(result->getPropertyValue("TransmissionProcessingInstructions"), "4");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferMin"), "0.009000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferStep"), "0.030000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferMax"), "1.300000");
    TS_ASSERT_EQUALS(result->getPropertyValue("ScaleFactor"), "0.900000");
    TS_ASSERT_EQUALS(result->getPropertyValue("ProcessingInstructions"), "4-6");
    TS_ASSERT_EQUALS(result->getPropertyValue("BackgroundProcessingInstructions"), "2-3,7-8");
  }

  void testLookupRowWithWildcardLookup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // angle outside tolerance of any angle matches wildcard row instead
    auto row = makeRow(2.28);
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("FirstTransmissionRunList"), "22345");
    TS_ASSERT_EQUALS(result->getPropertyValue("SecondTransmissionRunList"), "22346");
    TS_ASSERT_EQUALS(result->getPropertyValue("TransmissionProcessingInstructions"), "5-6");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferMin"), "0.007000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferStep"), "0.010000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferMax"), "1.100000");
    TS_ASSERT_EQUALS(result->getPropertyValue("ScaleFactor"), "0.700000");
    TS_ASSERT_EQUALS(result->getPropertyValue("ProcessingInstructions"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("BackgroundProcessingInstructions"), "3,7");
  }

  void testInstrumentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result->getPropertyValue("WavelengthMin"), "2.300000");
    TS_ASSERT_EQUALS(result->getPropertyValue("WavelengthMax"), "14.400000");
    TS_ASSERT_EQUALS(result->getPropertyValue("I0MonitorIndex"), "2");
    TS_ASSERT_EQUALS(result->getPropertyValue("NormalizeByIntegratedMonitors"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("MonitorBackgroundWavelengthMin"), "1.100000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MonitorBackgroundWavelengthMax"), "17.200000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MonitorIntegrationWavelengthMin"), "3.400000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MonitorIntegrationWavelengthMax"), "10.800000");
    TS_ASSERT_EQUALS(result->getPropertyValue("CorrectDetectors"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("DetectorCorrectionType"), "RotateAroundSample");
  }

  void testInstrumentSettingsWithEmptyRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);

    TS_ASSERT_EQUALS(result->getPropertyValue("WavelengthMin"), "2.300000");
    TS_ASSERT_EQUALS(result->getPropertyValue("WavelengthMax"), "14.400000");
    TS_ASSERT_EQUALS(result->getPropertyValue("I0MonitorIndex"), "2");
    TS_ASSERT_EQUALS(result->getPropertyValue("NormalizeByIntegratedMonitors"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("MonitorBackgroundWavelengthMin"), "1.100000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MonitorBackgroundWavelengthMax"), "17.200000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MonitorIntegrationWavelengthMin"), "3.400000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MonitorIntegrationWavelengthMax"), "10.800000");
    TS_ASSERT_EQUALS(result->getPropertyValue("CorrectDetectors"), "1");
    TS_ASSERT_EQUALS(result->getPropertyValue("DetectorCorrectionType"), "RotateAroundSample");
  }

  void testSettingsForSlicingWithEmptyRow() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("TimeInterval"), "123.400000");
  }

  void testSettingsForSlicingByTime() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result->getPropertyValue("TimeInterval"), "123.400000");
  }

  void testSettingsForSlicingByNumberOfSlices() {
    auto slicing = Slicing(UniformSlicingByNumberOfSlices(3));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result->getPropertyValue("NumberOfSlices"), "3");
  }

  void testSettingsForSlicingByList() {
    auto slicing = Slicing(CustomSlicingByList({3.1, 10.2, 47.35}));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result->getPropertyValue("TimeInterval"), "3.1, 10.2, 47.35");
  }

  void testSettingsForSlicingByLog() {
    auto slicing = Slicing(SlicingByEventLog({18.2}, "test_log_name"));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result->getPropertyValue("LogName"), "test_log_name");
    TS_ASSERT_EQUALS(result->getPropertyValue("LogValueInterval"), "18.200000");
  }

  void testSettingsForRowCellValues() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // Use an angle that will match per-theta defaults. They should be
    // overridden by the cell values
    auto row = makeRowWithMainCellsFilled(2.3);
    auto result = createAlgorithmRuntimeProps(model, row);

    TS_ASSERT_EQUALS(result->getPropertyValue("InputRunList"), "12345, 12346");
    TS_ASSERT_EQUALS(result->getPropertyValue("FirstTransmissionRunList"), "92345");
    TS_ASSERT_EQUALS(result->getPropertyValue("SecondTransmissionRunList"), "92346");
    TS_ASSERT_EQUALS(result->getPropertyValue("ThetaIn"), "2.300000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferMin"), "0.100000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferStep"), "0.090000");
    TS_ASSERT_EQUALS(result->getPropertyValue("MomentumTransferMax"), "0.910000");
    TS_ASSERT_EQUALS(result->getPropertyValue("ScaleFactor"), "2.200000");
  }

  void testAddingPropertyViaOptionsCell() {
    // This tests adding a property via the options cell on a row, for a
    // property that does not get set anywhere else on the GUI
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(2.3, ReductionOptionsMap{{"ThetaLogName", "theta_log_name"}});
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("ThetaLogName"), "theta_log_name");
  }

  void testOptionsCellOverridesExperimentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(
        2.3, ReductionOptionsMap{{"AnalysisMode", "PointDetectorAnalysis"}, {"ReductionType", "DivergentBeam"}});
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("AnalysisMode"), "PointDetectorAnalysis");
    TS_ASSERT_EQUALS(result->getPropertyValue("ReductionType"), "DivergentBeam");
  }

  void testOptionsCellOverridesLookupRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // Use an angle that will match per-theta defaults. They should be
    // overridden by the cell values
    auto row = makeRowWithOptionsCellFilled(2.3, ReductionOptionsMap{{"ProcessingInstructions", "390-410"}});
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("ProcessingInstructions"), "390-410");
  }

  void testOptionsCellOverridesInstrumentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(2.3, ReductionOptionsMap{{"WavelengthMin", "3.3"}});
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result->getPropertyValue("WavelengthMin"), "3.3");
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
    auto result = createAlgorithmRuntimeProps(model, row);

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
};
