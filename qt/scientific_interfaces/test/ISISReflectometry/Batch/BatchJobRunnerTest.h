// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_

#include "../../../ISISReflectometry/GUI/Batch/BatchJobRunner.h"
#include "../ReflMockObjects.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using MantidQt::API::ConfiguredAlgorithm;
using MantidQt::API::ConfiguredAlgorithm_sptr;
using WorkspaceCreationHelper::MockAlgorithm;

using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class BatchJobRunnerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobRunnerTest *createSuite() { return new BatchJobRunnerTest(); }
  static void destroySuite(BatchJobRunnerTest *suite) { delete suite; }

  BatchJobRunnerTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"},
        m_tolerance(0.1),
        m_experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                     SummationType::SumInLambda, false, false,
                     PolarizationCorrections(PolarizationCorrectionType::None),
                     FloodCorrections(FloodCorrectionType::Workspace),
                     boost::none, std::map<std::string, std::string>(),
                     std::vector<PerThetaDefaults>()),
        m_instrument(
            RangeInLambda(0.0, 0.0),
            MonitorCorrections(0, true, RangeInLambda(0.0, 0.0),
                               RangeInLambda(0.0, 0.0)),
            DetectorCorrections(false, DetectorCorrectionType::VerticalShift)),
        m_runsTable(m_instruments, m_tolerance, ReductionJobs()), m_slicing() {}

  void testInitialisedWithNonRunningState() {
    auto jobRunner = makeJobRunner();
    TS_ASSERT_EQUALS(jobRunner.isProcessing(), false);
    TS_ASSERT_EQUALS(jobRunner.isAutoreducing(), false);
  }

  void testResumeReduction() {
    auto jobRunner = makeJobRunner();
    jobRunner.resumeReduction();
    auto const hasSelection = false;
    TS_ASSERT_EQUALS(jobRunner.isProcessing(), true);
    TS_ASSERT_EQUALS(jobRunner.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobRunner.m_reprocessFailed, hasSelection);
    TS_ASSERT_EQUALS(jobRunner.m_processAll, !hasSelection);
  }

  void testReductionPaused() {
    auto jobRunner = makeJobRunner();
    jobRunner.reductionPaused();
    TS_ASSERT_EQUALS(jobRunner.isProcessing(), false);
  }

  void testResumeAutoreduction() {
    auto jobRunner = makeJobRunner();
    jobRunner.resumeAutoreduction();
    TS_ASSERT_EQUALS(jobRunner.isProcessing(), true);
    TS_ASSERT_EQUALS(jobRunner.isAutoreducing(), true);
    TS_ASSERT_EQUALS(jobRunner.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobRunner.m_processAll, true);
  }

  void testAutoreductionPaused() {
    auto jobRunner = makeJobRunner();
    jobRunner.autoreductionPaused();
    TS_ASSERT_EQUALS(jobRunner.isAutoreducing(), false);
  }

  void testSetReprocessFailedItems() {
    auto jobRunner = makeJobRunner();
    jobRunner.setReprocessFailedItems(true);
    TS_ASSERT_EQUALS(jobRunner.m_reprocessFailed, true);
  }

  void testGetAlgorithmsWithEmptyModel() {
    auto jobRunner = makeJobRunner();
    auto const algorithms = jobRunner.getAlgorithms();
    TS_ASSERT_EQUALS(algorithms, std::deque<ConfiguredAlgorithm_sptr>());
  }

  void testGetAlgorithmsWithMultiGroupModel() {
    // TODO add content to model
    auto jobRunner = makeJobRunner();
    auto const algorithms = jobRunner.getAlgorithms();
    TS_ASSERT_EQUALS(algorithms, std::deque<ConfiguredAlgorithm_sptr>());
  }

  void testAlgorithmStarted() {
    auto reductionJobs = makeReductionJobsWithMultiGroups();
    auto &row = reductionJobs.mutableGroups()[0].mutableRows()[2];
    auto jobRunner = makeJobRunner(reductionJobs);
    auto algorithm = makeAlgorithm(*row);
    jobRunner.algorithmStarted(algorithm);
    TS_ASSERT_EQUALS(row->state(), State::ITEM_RUNNING);
  }

  void testAlgorithmComplete() {
    auto reductionJobs = makeReductionJobsWithMultiGroups();
    auto &row = reductionJobs.mutableGroups()[0].mutableRows()[2];
    auto jobRunner = makeJobRunner(reductionJobs);
    auto algorithm = makeAlgorithm(*row);
    jobRunner.algorithmComplete(algorithm);
    TS_ASSERT_EQUALS(row->state(), State::ITEM_COMPLETE);
  }

private:
  std::vector<std::string> m_instruments;
  double m_tolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;

  class BatchJobRunnerFriend : public BatchJobRunner {
    friend class BatchJobRunnerTest;

  public:
    BatchJobRunnerFriend(Batch batch) : BatchJobRunner(batch) {}
  };

  Experiment makeExperiment() {
    return Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                      SummationType::SumInLambda, false, false,
                      PolarizationCorrections(PolarizationCorrectionType::None),
                      FloodCorrections(FloodCorrectionType::Workspace),
                      boost::none, std::map<std::string, std::string>(),
                      std::vector<PerThetaDefaults>());
  }

  Instrument makeInstrument() {
    return Instrument(
        RangeInLambda(0.0, 0.0),
        MonitorCorrections(0, true, RangeInLambda(0.0, 0.0),
                           RangeInLambda(0.0, 0.0)),
        DetectorCorrections(false, DetectorCorrectionType::VerticalShift));
  }

  RunsTable makeRunsTable(ReductionJobs reductionJobs) {
    return RunsTable(m_instruments, m_tolerance, std::move(reductionJobs));
  }

  Row makeRow(std::string const &run, double theta) {
    return Row({run}, theta, TransmissionRunPair(), RangeInQ(), boost::none,
               ReductionOptionsMap(),
               ReductionWorkspaces({run}, TransmissionRunPair()));
  }

  ReductionJobs makeReductionJobsWithMultiGroups() {
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

  BatchJobRunnerFriend
  makeJobRunner(ReductionJobs reductionJobs = ReductionJobs()) {
    m_experiment = makeExperiment();
    m_instrument = makeInstrument();
    m_runsTable = makeRunsTable(std::move(reductionJobs));
    m_slicing = Slicing();
    return BatchJobRunnerFriend(
        Batch(m_experiment, m_instrument, m_runsTable, m_slicing));
  }

  ConfiguredAlgorithm_sptr makeAlgorithm(Item &item) {
    auto alg = boost::make_shared<MockAlgorithm>();
    auto properties =
        ConfiguredAlgorithm::AlgorithmRuntimeProps{{"InputRunList", "12345"}};
    // TODO: this ctor not recognised: BatchJobAlgorithm(alg, properties,
    // &item);
    auto jobAlgorithm =
        boost::make_shared<BatchJobAlgorithm>(alg, properties, &item);
    auto algorithm =
        boost::dynamic_pointer_cast<ConfiguredAlgorithm>(jobAlgorithm);
    return algorithm;
  }
};

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_
