// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHMTEST_H_
#define MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHMTEST_H_
#include "../../../ISISReflectometry/GUI/Batch/RowProcessingAlgorithm.h"
#include "../../../ISISReflectometry/Reduction/Batch.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

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
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["AnalysisMode"], "MultiDetectorAnalysis");
    TS_ASSERT_EQUALS(result["ReductionType"], "NonFlatSample");
    TS_ASSERT_EQUALS(result["SummationType"], "SumInQ");
    TS_ASSERT_EQUALS(result["IncludePartialBins"], "1");
    TS_ASSERT_EQUALS(result["Debug"], "1");
    TS_ASSERT_EQUALS(result["PolarizationAnalysis"], "ParameterFile");
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
    TS_ASSERT_EQUALS(result["MomentumTransferMin"], "0.007000");
    TS_ASSERT_EQUALS(result["MomentumTransferStep"], "0.010000");
    TS_ASSERT_EQUALS(result["MomentumTransferMax"], "1.100000");
    TS_ASSERT_EQUALS(result["ScaleFactor"], "0.700000");
    TS_ASSERT_EQUALS(result["ProcessingInstructions"], "1");
  }

  void testInstrumentSettings() {
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

  void testSettingsForSlicingByTime() {
    auto slicing = Slicing(UniformSlicingByTime(123.4));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["TimeInterval"], "123.400000");
  }

  void testSettingsForSlicingByNumberOfSlices() {
    auto slicing = Slicing(UniformSlicingByNumberOfSlices(3));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["NumberOfSlices"], "3");
  }

  void testSettingsForSlicingByList() {
    auto slicing = Slicing(CustomSlicingByList({3.1, 10.2, 47.35}));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
    TS_ASSERT_EQUALS(result["TimeInterval"], "3.1, 10.2, 47.35");
  }

  void testSettingsForSlicingByLog() {
    auto slicing = Slicing(SlicingByEventLog({18.2}, "test_log_name"));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, slicing);
    auto row = makeEmptyRow();
    auto result = createAlgorithmRuntimeProps(model, row);
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

  Row makeEmptyRow() {
    return Row({}, 0.0, TransmissionRunPair(), RangeInQ(), boost::none,
               ReductionOptionsMap(),
               ReductionWorkspaces({}, TransmissionRunPair()));
  }

  Row makeRow(double theta = 0.5) {
    return Row({}, theta, TransmissionRunPair(), RangeInQ(), boost::none,
               ReductionOptionsMap(),
               ReductionWorkspaces({}, TransmissionRunPair()));
  }

  Row makeRowWithMainCellsFilled(double theta = 0.5) {
    return Row({"12345", "12346"}, theta, TransmissionRunPair("92345", "92346"),
               RangeInQ(0.1, 0.09, 0.91), 2.2, ReductionOptionsMap(),
               ReductionWorkspaces({"12345", "12346"},
                                   TransmissionRunPair("92345", "92346")));
  }

  Row makeRowWithOptionsCellFilled(double theta, ReductionOptionsMap options) {
    return Row({}, theta, TransmissionRunPair(), RangeInQ(), boost::none,
               std::move(options),
               ReductionWorkspaces({}, TransmissionRunPair()));
  }

  Experiment makeExperiment() {
    return Experiment(AnalysisMode::MultiDetector, ReductionType::NonFlatSample,
                      SummationType::SumInQ, true, true,
                      makePolarizationCorrections(), makeFloodCorrections(),
                      makeTransmissionRunRange(), makeStitchOptions(),
                      makePerThetaDefaultsWithTwoAnglesAndWildcard());
  }

  Instrument makeInstrument() {
    return Instrument(makeWavelengthRange(), makeMonitorCorrections(),
                      makeDetectorCorrections());
  }

  std::vector<PerThetaDefaults> makePerThetaDefaultsWithTwoAnglesAndWildcard() {
    return std::vector<PerThetaDefaults>{
        // wildcard row with no angle
        PerThetaDefaults(boost::none, TransmissionRunPair("22345", "22346"),
                         RangeInQ(0.007, 0.01, 1.1), 0.7,
                         ProcessingInstructions("1")),
        // two angle rows
        PerThetaDefaults(0.5, TransmissionRunPair("22347", ""),
                         RangeInQ(0.008, 0.02, 1.2), 0.8,
                         ProcessingInstructions("2-3")),
        PerThetaDefaults(
            2.3,
            TransmissionRunPair(std::vector<std::string>{"22348", "22349"},
                                std::vector<std::string>{"22358", "22359"}),
            RangeInQ(0.009, 0.03, 1.3), 0.9, ProcessingInstructions("4-6"))};
  }

  std::map<std::string, std::string> makeStitchOptions() {
    return std::map<std::string, std::string>{{"key1", "value1"},
                                              {"key2", "value2"}};
  }

  PolarizationCorrections makePolarizationCorrections() {
    return PolarizationCorrections(PolarizationCorrectionType::ParameterFile);
  }

  FloodCorrections makeFloodCorrections() {
    return FloodCorrections(FloodCorrectionType::Workspace,
                            boost::optional<std::string>("test_workspace"));
  }

  RangeInLambda makeTransmissionRunRange() { return RangeInLambda(7.5, 9.2); }

  RangeInLambda makeWavelengthRange() { return RangeInLambda(2.3, 14.4); }

  RangeInLambda makeMonitorBackgroundRange() {
    return RangeInLambda(1.1, 17.2);
  }

  RangeInLambda makeMonitorIntegralRange() { return RangeInLambda(3.4, 10.8); }

  MonitorCorrections makeMonitorCorrections() {
    return MonitorCorrections(2, true, makeMonitorBackgroundRange(),
                              makeMonitorIntegralRange());
  }

  DetectorCorrections makeDetectorCorrections() {
    return DetectorCorrections(true,
                               DetectorCorrectionType::RotateAroundSample);
  }
};
#endif // MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHMTEST_H_
