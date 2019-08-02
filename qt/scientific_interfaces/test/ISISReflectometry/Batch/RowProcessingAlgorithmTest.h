// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHMTEST_H_
#define MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHMTEST_H_
#include "../../../ISISReflectometry/Common/ModelCreationHelper.h"
#include "../../../ISISReflectometry/GUI/Batch/RowProcessingAlgorithm.h"
#include "../../../ISISReflectometry/Reduction/Batch.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::ModelCreationHelper;

class RowProcessingAlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RowProcessingAlgorithmTest *createSuite() {
    return new RowProcessingAlgorithmTest();
  }
  static void destroySuite(RowProcessingAlgorithmTest *suite) { delete suite; }

  RowProcessingAlgorithmTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"},
        m_thetaTolerance(0.01), m_experiment(makeExperiment()),
        m_instrument(makeInstrument()),
        m_runsTable(m_instruments, m_thetaTolerance, ReductionJobs()),
        m_slicing() {}

  void testExperimentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result["AnalysisMode"], "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(result["ReductionType"], "NonFlatSample");
    TS_ASSERT_EQUALS(result["SummationType"], "SumInQ");
    TS_ASSERT_EQUALS(result["IncludePartialBins"], "1");
    TS_ASSERT_EQUALS(result["Debug"], "1");
    TS_ASSERT_EQUALS(result["PolarizationAnalysis"], "1");
    TS_ASSERT_EQUALS(result["FloodCorrection"], "Workspace");
    TS_ASSERT_EQUALS(result["FloodWorkspace"], "test_workspace");
    TS_ASSERT_EQUALS(result["StartOverlap"], "7.500000");
    TS_ASSERT_EQUALS(result["EndOverlap"], "9.200000");
    TS_ASSERT_EQUALS(result["Params"], "-0.02");
    TS_ASSERT_EQUALS(result["ScaleRHSWorkspace"], "1");
  }

  void testExperimentSettingsWithEmptyRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["AnalysisMode"], "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(result["ReductionType"], "NonFlatSample");
    TS_ASSERT_EQUALS(result["SummationType"], "SumInQ");
    TS_ASSERT_EQUALS(result["IncludePartialBins"], "1");
    TS_ASSERT_EQUALS(result["Debug"], "1");
    TS_ASSERT_EQUALS(result["PolarizationAnalysis"], "1");
    TS_ASSERT_EQUALS(result["FloodCorrection"], "Workspace");
    TS_ASSERT_EQUALS(result["FloodWorkspace"], "test_workspace");
    TS_ASSERT_EQUALS(result["StartOverlap"], "7.500000");
    TS_ASSERT_EQUALS(result["EndOverlap"], "9.200000");
  }

  void testPerThetaDefaultsWithAngleLookup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // angle within tolerance of 2.3
    auto row = makeRow(2.29);
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["FirstTransmissionRunList"], "22348, 22349");
    TS_ASSERT_EQUALS(result["SecondTransmissionRunList"], "22358, 22359");
    TS_ASSERT_EQUALS(result["TransmissionProcessingInstructions"], "4");
    TS_ASSERT_EQUALS(result["MomentumTransferMin"], "0.009000");
    TS_ASSERT_EQUALS(result["MomentumTransferStep"], "0.030000");
    TS_ASSERT_EQUALS(result["MomentumTransferMax"], "1.300000");
    TS_ASSERT_EQUALS(result["ScaleFactor"], "0.900000");
    TS_ASSERT_EQUALS(result["ProcessingInstructions"], "4-6");
  }

  void testPerThetaDefaultsWithWildcardLookup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // angle outside tolerance of any angle matches wildcard row instead
    auto row = makeRow(2.28);
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["FirstTransmissionRunList"], "22345");
    TS_ASSERT_EQUALS(result["SecondTransmissionRunList"], "22346");
    TS_ASSERT_EQUALS(result["TransmissionProcessingInstructions"], "5-6");
    TS_ASSERT_EQUALS(result["MomentumTransferMin"], "0.007000");
    TS_ASSERT_EQUALS(result["MomentumTransferStep"], "0.010000");
    TS_ASSERT_EQUALS(result["MomentumTransferMax"], "1.100000");
    TS_ASSERT_EQUALS(result["ScaleFactor"], "0.700000");
    TS_ASSERT_EQUALS(result["ProcessingInstructions"], "1");
  }

  void testInstrumentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result["WavelengthMin"], "2.300000");
    TS_ASSERT_EQUALS(result["WavelengthMax"], "14.400000");
    TS_ASSERT_EQUALS(result["I0MonitorIndex"], "2");
    TS_ASSERT_EQUALS(result["NormalizeByIntegratedMonitors"], "1");
    TS_ASSERT_EQUALS(result["MonitorBackgroundWavelengthMin"], "1.100000");
    TS_ASSERT_EQUALS(result["MonitorBackgroundWavelengthMax"], "17.200000");
    TS_ASSERT_EQUALS(result["MonitorIntegrationWavelengthMin"], "3.400000");
    TS_ASSERT_EQUALS(result["MonitorIntegrationWavelengthMax"], "10.800000");
    TS_ASSERT_EQUALS(result["CorrectDetectors"], "1");
    TS_ASSERT_EQUALS(result["DetectorCorrectionType"], "RotateAroundSample");
  }

  void testInstrumentSettingsWithEmptyRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["WavelengthMin"], "2.300000");
    TS_ASSERT_EQUALS(result["WavelengthMax"], "14.400000");
    TS_ASSERT_EQUALS(result["I0MonitorIndex"], "2");
    TS_ASSERT_EQUALS(result["NormalizeByIntegratedMonitors"], "1");
    TS_ASSERT_EQUALS(result["MonitorBackgroundWavelengthMin"], "1.100000");
    TS_ASSERT_EQUALS(result["MonitorBackgroundWavelengthMax"], "17.200000");
    TS_ASSERT_EQUALS(result["MonitorIntegrationWavelengthMin"], "3.400000");
    TS_ASSERT_EQUALS(result["MonitorIntegrationWavelengthMax"], "10.800000");
    TS_ASSERT_EQUALS(result["CorrectDetectors"], "1");
    TS_ASSERT_EQUALS(result["DetectorCorrectionType"], "RotateAroundSample");
  }

  void testSettingsForSlicingWithEmptyRow() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["TimeInterval"], "123.400000");
  }

  void testSettingsForSlicingByTime() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result["TimeInterval"], "123.400000");
  }

  void testSettingsForSlicingByNumberOfSlices() {
    auto slicing = Slicing(UniformSlicingByNumberOfSlices(3));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result["NumberOfSlices"], "3");
  }

  void testSettingsForSlicingByList() {
    auto slicing = Slicing(CustomSlicingByList({3.1, 10.2, 47.35}));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result["TimeInterval"], "3.1, 10.2, 47.35");
  }

  void testSettingsForSlicingByLog() {
    auto slicing = Slicing(SlicingByEventLog({18.2}, "test_log_name"));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto result = createAlgorithmRuntimeProps(model);
    TS_ASSERT_EQUALS(result["LogName"], "test_log_name");
    TS_ASSERT_EQUALS(result["LogValueInterval"], "18.200000");
  }

  void testSettingsForRowCellValues() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // Use an angle that will match per-theta defaults. They should be
    // overridden by the cell values
    auto row = makeRowWithMainCellsFilled(2.3);
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["InputRunList"], "12345, 12346");
    TS_ASSERT_EQUALS(result["FirstTransmissionRunList"], "92345");
    TS_ASSERT_EQUALS(result["SecondTransmissionRunList"], "92346");
    TS_ASSERT_EQUALS(result["ThetaIn"], "2.300000");
    TS_ASSERT_EQUALS(result["MomentumTransferMin"], "0.100000");
    TS_ASSERT_EQUALS(result["MomentumTransferStep"], "0.090000");
    TS_ASSERT_EQUALS(result["MomentumTransferMax"], "0.910000");
    TS_ASSERT_EQUALS(result["ScaleFactor"], "2.200000");
  }

  void testAddingPropertyViaOptionsCell() {
    // This tests adding a property via the options cell on a row, for a
    // property that does not get set anywhere else on the GUI
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(
        2.3, ReductionOptionsMap{{"ThetaLogName", "theta_log_name"}});
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["ThetaLogName"], "theta_log_name");
  }

  void testOptionsCellOverridesExperimentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(
        2.3, ReductionOptionsMap{{"AnalysisMode", "PointDetectorAnalysis"},
                                 {"ReductionType", "DivergentBeam"}});
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["AnalysisMode"], "PointDetectorAnalysis");
    TS_ASSERT_EQUALS(result["ReductionType"], "DivergentBeam");
  }

  void testOptionsCellOverridesPerThetaDefaults() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    // Use an angle that will match per-theta defaults. They should be
    // overridden by the cell values
    auto row = makeRowWithOptionsCellFilled(
        2.3, ReductionOptionsMap{{"ProcessingInstructions", "390-410"}});
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["ProcessingInstructions"], "390-410");
  }

  void testOptionsCellOverridesInstrumentSettings() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto row = makeRowWithOptionsCellFilled(
        2.3, ReductionOptionsMap{{"WavelengthMin", "3.3"}});
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["WavelengthMin"], "3.3");
  }

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
};
#endif // MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHMTEST_H_
