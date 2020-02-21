// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ModelCreationHelper.h"
#include "../../ISISReflectometry/Reduction/Batch.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
namespace ModelCreationHelper {

namespace { // unnamed
Row makeRowWithOutputNames(std::vector<std::string> const &outputNames) {
  auto row = Row({}, 0.0, TransmissionRunPair(), RangeInQ(), boost::none,
                 ReductionOptionsMap(),
                 ReductionWorkspaces({}, TransmissionRunPair()));
  row.setOutputNames(outputNames);
  return row;
}
} // namespace

/* Rows */

Row makeEmptyRow() {
  return Row({}, 0.0, TransmissionRunPair(), RangeInQ(), boost::none,
             ReductionOptionsMap(),
             ReductionWorkspaces({}, TransmissionRunPair()));
}

Row makeRow(double theta) {
  return Row({}, theta, TransmissionRunPair({"22348", "22349"}), RangeInQ(),
             boost::none, ReductionOptionsMap(),
             ReductionWorkspaces({}, TransmissionRunPair()));
}

Row makeRow(std::string const &run, double theta) {
  return Row(
      {run}, theta, TransmissionRunPair({"Trans A", "Trans B"}), RangeInQ(),
      boost::none, ReductionOptionsMap(),
      ReductionWorkspaces({run}, TransmissionRunPair({"Trans A", "Trans B"})));
}

Row makeSimpleRow(std::string const &run, double theta) {
  return Row({run}, theta, TransmissionRunPair(), RangeInQ(), boost::none,
             ReductionOptionsMap(),
             ReductionWorkspaces({run}, TransmissionRunPair()));
}

Row makeRow(std::string const &run, double theta, std::string const &trans1,
            std::string const &trans2, boost::optional<double> qMin,
            boost::optional<double> qMax, boost::optional<double> qStep,
            boost::optional<double> scale,
            ReductionOptionsMap const &optionsMap) {
  return Row({run}, theta, TransmissionRunPair({trans1, trans2}),
             RangeInQ(qMin, qMax, qStep), scale, optionsMap,
             ReductionWorkspaces({run}, TransmissionRunPair({trans1, trans2})));
}

Row makeRow(std::vector<std::string> const &runs, double theta) {
  return Row(
      runs, theta, TransmissionRunPair({"Trans A", "Trans B"}), RangeInQ(),
      boost::none, ReductionOptionsMap(),
      ReductionWorkspaces(runs, TransmissionRunPair({"Trans A", "Trans B"})));
}

Row makeCompletedRow() {
  auto row = Row({}, 0.0, TransmissionRunPair(), RangeInQ(), boost::none,
                 ReductionOptionsMap(),
                 ReductionWorkspaces({}, TransmissionRunPair()));
  row.setSuccess();
  return row;
}

Row makeRowWithMainCellsFilled(double theta) {
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

Group makeEmptyGroup() { return Group("Test group 1"); }

Group makeGroupWithOneRow() {
  return Group("single_row_group",
               std::vector<boost::optional<Row>>{
                   makeRowWithOutputNames({"IvsLam", "IvsQ", "IvsQBin"})});
}

Group makeGroupWithOneRowAndOneEmptyRow() {
  return Group("multi_row_group",
               std::vector<boost::optional<Row>>{
                   makeRowWithOutputNames({"IvsLam", "IvsQ", "IvsQBin"}),
                   makeEmptyRow()});
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

Group makeGroupWithTwoRowsWithMixedQResolutions() {
  auto group1 = Group("Test group 1");
  group1.appendRow(boost::none);
  group1.appendRow(makeRow());
  group1.appendRow(
      Row({"22222"}, 0.5, TransmissionRunPair({}), RangeInQ(0.5, 0.015, 0.9),
          boost::none, ReductionOptionsMap(),
          ReductionWorkspaces({"22222"}, TransmissionRunPair({}))));
  group1.appendRow(
      Row({"33333"}, 0.5, TransmissionRunPair({}), RangeInQ(0.5, 0.016, 0.9),
          boost::none, ReductionOptionsMap(),
          ReductionWorkspaces({"33333"}, TransmissionRunPair({}))));
  return group1;
}

Group makeGroupWithTwoRowsWithOutputQResolutions() {
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow());
  group1.appendRow(
      Row({"22222"}, 0.5, TransmissionRunPair({}), RangeInQ(), boost::none,
          ReductionOptionsMap(),
          ReductionWorkspaces({"22222"}, TransmissionRunPair({}))));
  auto row = Row({"33333"}, 0.5, TransmissionRunPair({}), RangeInQ(),
                 boost::none, ReductionOptionsMap(),
                 ReductionWorkspaces({"33333"}, TransmissionRunPair({})));
  row.setOutputQRange(RangeInQ(0.5, 0.016, 0.9));
  group1.appendRow(row);
  return group1;
}

/* Reduction Jobs */

ReductionJobs oneEmptyGroupModel() {
  auto reductionJobs = ReductionJobs();
  reductionJobs.appendGroup(Group("Test group 1"));
  return reductionJobs;
}

ReductionJobs twoEmptyGroupsModel() {
  auto reductionJobs = ReductionJobs();
  reductionJobs.appendGroup(Group("Test group 1"));
  reductionJobs.appendGroup(Group("Test group 2"));
  return reductionJobs;
}

ReductionJobs oneGroupWithAnInvalidRowModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(boost::none);
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs oneGroupWithARowModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow("12345", 0.5));
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs oneGroupWithARowWithInputQRangeModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  auto row = Row({"12345"}, 0.5, TransmissionRunPair({"Trans A", "Trans B"}),
                 RangeInQ(0.5, 0.01, 0.9), boost::none, ReductionOptionsMap(),
                 ReductionWorkspaces(
                     {"12345"}, TransmissionRunPair({"Trans A", "Trans B"})));
  group1.appendRow(row);
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs oneGroupWithARowWithOutputQRangeModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  auto row = makeRow("12345", 0.5);
  row.setOutputQRange(RangeInQ(0.5, 0.01, 0.9));
  group1.appendRow(row);
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs oneGroupWithARowWithInputQRangeModelMixedPrecision() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  auto row =
      Row({"12345"}, 0.555555, TransmissionRunPair({"Trans A", "Trans B"}),
          RangeInQ(0.55567, 0.012, 0.9), boost::none, ReductionOptionsMap(),
          ReductionWorkspaces({"12345"},
                              TransmissionRunPair({"Trans A", "Trans B"})));
  group1.appendRow(row);
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs oneGroupWithAnotherRowModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow("12346", 0.8));
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs oneGroupWithAnotherRunWithSameAngleModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow("12346", 0.5));
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs oneGroupWithTwoRunsInARowModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow(std::vector<std::string>{"12345", "12346"}, 0.5));
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs oneGroupWithTwoRowsModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow("12345", 0.5));
  group1.appendRow(makeRow("12346", 0.8));
  reductionJobs.appendGroup(std::move(group1));
  return reductionJobs;
}

ReductionJobs oneGroupWithTwoSimpleRowsModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeSimpleRow("12345", 0.5));
  group1.appendRow(makeSimpleRow("12346", 0.8));
  reductionJobs.appendGroup(std::move(group1));
  return reductionJobs;
}

ReductionJobs anotherGroupWithARowModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 2");
  group1.appendRow(makeRow("12346", 0.8));
  reductionJobs.appendGroup(group1);
  return reductionJobs;
}

ReductionJobs twoGroupsWithARowModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow("12345", 0.5));
  reductionJobs.appendGroup(std::move(group1));

  auto group2 = Group("Test group 2");
  group2.appendRow(makeRow("12346", 0.8));
  reductionJobs.appendGroup(std::move(group2));

  return reductionJobs;
}

ReductionJobs twoGroupsWithTwoRowsModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow("12345", 0.5));
  group1.appendRow(makeRow("12346", 0.8));
  reductionJobs.appendGroup(std::move(group1));

  auto group2 = Group("Test group 2");
  group2.appendRow(makeRow("22345", 0.5));
  group2.appendRow(makeRow("22346", 0.8));
  reductionJobs.appendGroup(std::move(group2));

  return reductionJobs;
}

ReductionJobs twoGroupsWithMixedRowsModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Test group 1");
  group1.appendRow(makeRow("12345", 0.5));
  group1.appendRow(boost::none);
  group1.appendRow(makeRow("12346", 0.8));
  reductionJobs.appendGroup(std::move(group1));

  auto group2 = Group("Second Group");
  group2.appendRow(makeRow("22345", 0.5));
  group2.appendRow(makeRow("22346", 0.9));
  reductionJobs.appendGroup(std::move(group2));

  return reductionJobs;
}

ReductionJobs emptyReductionJobs() {
  auto reductionJobs = ReductionJobs();
  auto group1 = Group("Group1");
  group1.appendRow(makeRow());
  reductionJobs.appendGroup(std::move(group1));

  return reductionJobs;
}

ReductionJobs oneGroupWithTwoRowsWithOutputNamesModel() {
  auto reductionJobs = ReductionJobs();
  auto group1 = makeGroupWithTwoRows();
  reductionJobs.appendGroup(std::move(group1));

  return reductionJobs;
}

/* Experiment */

std::vector<PerThetaDefaults> makePerThetaDefaults() {
  auto perThetaDefaults =
      PerThetaDefaults(boost::none, TransmissionRunPair(), boost::none,
                       RangeInQ(boost::none, boost::none, boost::none),
                       boost::none, boost::none);
  return std::vector<PerThetaDefaults>{std::move(perThetaDefaults)};
}

std::vector<PerThetaDefaults> makePerThetaDefaultsWithTwoAnglesAndWildcard() {
  return std::vector<PerThetaDefaults>{
      // wildcard row with no angle
      PerThetaDefaults(boost::none, TransmissionRunPair("22345", "22346"),
                       ProcessingInstructions("5-6"),
                       RangeInQ(0.007, 0.01, 1.1), 0.7,
                       ProcessingInstructions("1")),
      // two angle rows
      PerThetaDefaults(0.5, TransmissionRunPair("22347", ""), boost::none,
                       RangeInQ(0.008, 0.02, 1.2), 0.8,
                       ProcessingInstructions("2-3")),
      PerThetaDefaults(
          2.3,
          TransmissionRunPair(std::vector<std::string>{"22348", "22349"},
                              std::vector<std::string>{"22358", "22359"}),
          ProcessingInstructions("4"), RangeInQ(0.009, 0.03, 1.3), 0.9,
          ProcessingInstructions("4-6"))};
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

TransmissionStitchOptions makeTransmissionStitchOptions() {
  return TransmissionStitchOptions(RangeInLambda{7.5, 9.2},
                                   RebinParameters("-0.02"), true);
}

TransmissionStitchOptions makeEmptyTransmissionStitchOptions() {
  return TransmissionStitchOptions(RangeInLambda{0, 0}, std::string(), false);
}

Experiment makeExperiment() {
  return Experiment(AnalysisMode::MultiDetector, ReductionType::NonFlatSample,
                    SummationType::SumInQ, true, true,
                    makePolarizationCorrections(), makeFloodCorrections(),
                    makeTransmissionStitchOptions(), makeStitchOptions(),
                    makePerThetaDefaultsWithTwoAnglesAndWildcard());
}

Experiment makeEmptyExperiment() {
  return Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                    SummationType::SumInLambda, false, false,
                    PolarizationCorrections(PolarizationCorrectionType::None),
                    FloodCorrections(FloodCorrectionType::Workspace),
                    TransmissionStitchOptions(),
                    std::map<std::string, std::string>(),
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
} // namespace ModelCreationHelper
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
