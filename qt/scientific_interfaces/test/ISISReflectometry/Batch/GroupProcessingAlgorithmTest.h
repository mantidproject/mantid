// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_GROUPPROCESSINGALGORITHMTEST_H_
#define MANTID_CUSTOMINTERFACES_GROUPPROCESSINGALGORITHMTEST_H_
#include "../../../ISISReflectometry/GUI/Batch/GroupProcessingAlgorithm.h"
#include "../../../ISISReflectometry/Reduction/Batch.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class GroupProcessingAlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupProcessingAlgorithmTest *createSuite() {
    return new GroupProcessingAlgorithmTest();
  }
  static void destroySuite(GroupProcessingAlgorithmTest *suite) {
    delete suite;
  }

  GroupProcessingAlgorithmTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"},
        m_thetaTolerance(0.01), m_experiment(makeExperiment()),
        m_instrument(makeInstrument()),
        m_runsTable(m_instruments, m_thetaTolerance, ReductionJobs()),
        m_slicing() {}

  void testThrowsIfInputWorkspaceGroupHasSingleRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithOneRow();
    TS_ASSERT_THROWS_ANYTHING(createAlgorithmRuntimeProps(model, group));
  }

  void testInputWorkspaceListForTwoRowGroup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result["InputWorkspaces"], "IvsQ_1, IvsQ_2");
  }

  void testInputWorkspaceListForRowsWithNonStandardNames() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithNonstandardNames();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result["InputWorkspaces"], "testQ1, testQ2");
  }

  void testOutputNameForTwoRowGroup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    // The standard IvsQ_ prefix is removed from the individual names so it
    // only appears once at the beginning.
    TS_ASSERT_EQUALS(result["OutputWorkspace"], "IvsQ_1_2");
  }

  void testOutputNameForRowsWithNonStandardNames() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithNonstandardNames();
    auto result = createAlgorithmRuntimeProps(model, group);
    // The output is constructed from an IvsQ_ prefix and the original
    // output workspace names
    TS_ASSERT_EQUALS(result["OutputWorkspace"], "IvsQ_testQ1_testQ2");
  }

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;

  Group makeEmptyGroup() { return Group("test_group"); }

  Row makeRowWithOutputNames(std::vector<std::string> const &outputNames) {
    auto row = Row({}, 0.0, TransmissionRunPair(), RangeInQ(), boost::none,
                   ReductionOptionsMap(),
                   ReductionWorkspaces({}, TransmissionRunPair()));
    row.setOutputNames(outputNames);
    return row;
  }

  Group makeGroupWithOneRow() {
    return Group("single_row_group",
                 std::vector<boost::optional<Row>>{
                     makeRowWithOutputNames({"IvsLam", "IvsQ", "IvsQBin"})});
  }

  Group makeGroupWithTwoRows() {
    return Group(
        "multi_row_group",
        std::vector<boost::optional<Row>>{
            makeRowWithOutputNames({"IvsLam_1", "IvsQ_1", "IvsQ_binned_1"}),
            makeRowWithOutputNames({"IvsLam_2", "IvsQ_2", "IvsQ_binned_2"})});
  }

  Group makeGroupWithTwoRowsWithNonstandardNames() {
    return Group(
        "multi_row_group",
        std::vector<boost::optional<Row>>{
            makeRowWithOutputNames({"testLam1", "testQ1", "testQBin1"}),
            makeRowWithOutputNames({"testLam2", "testQ2", "testQBin2"})});
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
#endif // MANTID_CUSTOMINTERFACES_GROUPPROCESSINGALGORITHMTEST_H_
