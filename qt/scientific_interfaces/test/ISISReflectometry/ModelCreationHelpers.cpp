// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../../ISISReflectometry/Reduction/Batch.h"

namespace MantidQt {
namespace CustomInterfaces {

namespace { // unnamed
Row makeRowWithOutputNames(std::vector<std::string> const &outputNames) {
  auto row = Row({}, 0.0, TransmissionRunPair(), RangeInQ(), boost::none,
                 ReductionOptionsMap(),
                 ReductionWorkspaces({}, TransmissionRunPair()));
  row.setOutputNames(outputNames);
  return row;
}
} // unnamed

/* Rows */

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

Row makeRow(std::string const &run, double theta) {
  return Row({run}, theta, TransmissionRunPair(), RangeInQ(), boost::none,
             ReductionOptionsMap(),
             ReductionWorkspaces({"IvsLam", "IvsQ", "IvsQBin"},
                                 TransmissionRunPair()));
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

/* Groups */

Group makeEmptyGroup() { return Group("test_group"); }

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

/* Reduction Jobs */

ReductionJobs makeReductionJobsWithSingleRowGroup() {
  auto groups = std::vector<Group>();
  // Create some rows for the first group
  auto group1Rows = std::vector<boost::optional<Row>>();
  group1Rows.emplace_back(makeRow("12345", 0.5));
  groups.emplace_back(Group("Test group 1", group1Rows));
  // Create the reduction jobs
  return ReductionJobs(groups);
}

ReductionJobs makeReductionJobsWithTwoRowGroup() {
  auto groups = std::vector<Group>();
  // Create some rows for the first group
  auto group1Rows = std::vector<boost::optional<Row>>();
  group1Rows.emplace_back(makeRow("12345", 0.5));
  group1Rows.emplace_back(makeRow("12346", 0.8));
  groups.emplace_back(Group("Test group 1", group1Rows));
  // Create the reduction jobs
  return ReductionJobs(groups);
}

ReductionJobs makeReductionJobsWithTwoGroups() {
  auto groups = std::vector<Group>();
  // Create some rows for the first group
  auto group1Rows = std::vector<boost::optional<Row>>();
  group1Rows.emplace_back(makeRow("12345", 0.5));
  group1Rows.emplace_back(boost::none); // indicates invalid row
  group1Rows.emplace_back(makeRow("12346", 0.8));
  groups.emplace_back(Group("Test group 1", group1Rows));
  // Create some rows for the second group
  auto group2Rows = std::vector<boost::optional<Row>>();
  group2Rows.emplace_back(makeRow("22345", 0.5));
  group2Rows.emplace_back(makeRow("22346", 0.9));
  groups.emplace_back(Group("Second Group", group2Rows));
  // Create the reduction jobs
  return ReductionJobs(groups);
}

/* Experiment */

std::vector<PerThetaDefaults> makePerThetaDefaults() {
  auto perThetaDefaults =
      PerThetaDefaults(boost::none, TransmissionRunPair(),
                       RangeInQ(boost::none, boost::none, boost::none),
                       boost::none, boost::none);
  return std::vector<PerThetaDefaults>{std::move(perThetaDefaults)};
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
          2.3, TransmissionRunPair(std::vector<std::string>{"22348", "22349"},
                                   std::vector<std::string>{"22358", "22359"}),
          RangeInQ(0.009, 0.03, 1.3), 0.9, ProcessingInstructions("4-6"))};
}

std::map<std::string, std::string> makeStitchOptions() {
  return std::map<std::string, std::string>{{"key1", "value1"},
                                            {"key2", "value2"}};
}

std::map<std::string, std::string> makeEmptyStitchOptions() {
  return std::map<std::string, std::string>();
}

PolarizationCorrections makePolarizationCorrections() {
  return PolarizationCorrections(PolarizationCorrectionType::ParameterFile);
}

PolarizationCorrections makeEmptyPolarizationCorrections() {
  return PolarizationCorrections(PolarizationCorrectionType::None);
}

FloodCorrections makeFloodCorrections() {
  return FloodCorrections(FloodCorrectionType::Workspace,
                          boost::optional<std::string>("test_workspace"));
}

RangeInLambda makeTransmissionRunRange() { return RangeInLambda(7.5, 9.2); }

RangeInLambda makeEmptyTransmissionRunRange() { return RangeInLambda{0, 0}; }

Experiment makeExperiment() {
  return Experiment(AnalysisMode::MultiDetector, ReductionType::NonFlatSample,
                    SummationType::SumInQ, true, true,
                    makePolarizationCorrections(), makeFloodCorrections(),
                    makeTransmissionRunRange(), makeStitchOptions(),
                    makePerThetaDefaultsWithTwoAnglesAndWildcard());
}

Experiment makeEmptyExperiment() {
  return Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                    SummationType::SumInLambda, false, false,
                    PolarizationCorrections(PolarizationCorrectionType::None),
                    FloodCorrections(FloodCorrectionType::Workspace),
                    boost::none, std::map<std::string, std::string>(),
                    std::vector<PerThetaDefaults>());
}

/* Instrument */

RangeInLambda makeWavelengthRange() { return RangeInLambda(2.3, 14.4); }

RangeInLambda makeMonitorBackgroundRange() { return RangeInLambda(1.1, 17.2); }

RangeInLambda makeMonitorIntegralRange() { return RangeInLambda(3.4, 10.8); }

MonitorCorrections makeMonitorCorrections() {
  return MonitorCorrections(2, true, makeMonitorBackgroundRange(),
                            makeMonitorIntegralRange());
}

DetectorCorrections makeDetectorCorrections() {
  return DetectorCorrections(true, DetectorCorrectionType::RotateAroundSample);
}

Instrument makeInstrument() {
  return Instrument(makeWavelengthRange(), makeMonitorCorrections(),
                    makeDetectorCorrections());
}

Instrument makeEmptyInstrument() {
  return Instrument(
      RangeInLambda(0.0, 0.0),
      MonitorCorrections(0, true, RangeInLambda(0.0, 0.0),
                         RangeInLambda(0.0, 0.0)),
      DetectorCorrections(false, DetectorCorrectionType::VerticalShift));
}
} // CustomInterfaces
} // MantidQt
