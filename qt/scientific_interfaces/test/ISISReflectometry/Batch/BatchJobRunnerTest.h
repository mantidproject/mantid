// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_

#include "../../../ISISReflectometry/GUI/Batch/BatchJobRunner.h"
#include "../ModelCreationHelpers.h"
#include "../ReflMockObjects.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using Mantid::API::Workspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using MantidQt::API::IConfiguredAlgorithm;
using MantidQt::API::IConfiguredAlgorithm_sptr;
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
        m_tolerance(0.1), m_experiment(makeEmptyExperiment()),
        m_instrument(makeEmptyInstrument()),
        m_runsTable(m_instruments, m_tolerance, ReductionJobs()), m_slicing() {
    m_jobAlgorithm = boost::make_shared<MockBatchJobAlgorithm>();
  }

  void testInitialisedWithNonRunningState() {
    auto jobRunner = makeJobRunner();
    TS_ASSERT_EQUALS(jobRunner.isProcessing(), false);
    TS_ASSERT_EQUALS(jobRunner.isAutoreducing(), false);
    verifyAndClear();
  }

  void testReductionResumed() {
    auto jobRunner = makeJobRunner();
    jobRunner.reductionResumed();
    auto const hasSelection = false;
    TS_ASSERT_EQUALS(jobRunner.isProcessing(), true);
    TS_ASSERT_EQUALS(jobRunner.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobRunner.m_reprocessFailed, hasSelection);
    TS_ASSERT_EQUALS(jobRunner.m_processAll, !hasSelection);
    verifyAndClear();
  }

  void testReductionPaused() {
    auto jobRunner = makeJobRunner();
    jobRunner.reductionPaused();
    TS_ASSERT_EQUALS(jobRunner.isProcessing(), false);
    verifyAndClear();
  }

  void testAutoreductionResumed() {
    auto jobRunner = makeJobRunner();
    jobRunner.autoreductionResumed();
    TS_ASSERT_EQUALS(jobRunner.isProcessing(), true);
    TS_ASSERT_EQUALS(jobRunner.isAutoreducing(), true);
    TS_ASSERT_EQUALS(jobRunner.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobRunner.m_processAll, true);
    verifyAndClear();
  }

  void testAutoreductionPaused() {
    auto jobRunner = makeJobRunner();
    jobRunner.autoreductionPaused();
    TS_ASSERT_EQUALS(jobRunner.isAutoreducing(), false);
    verifyAndClear();
  }

  void testSetReprocessFailedItems() {
    auto jobRunner = makeJobRunner();
    jobRunner.setReprocessFailedItems(true);
    TS_ASSERT_EQUALS(jobRunner.m_reprocessFailed, true);
    verifyAndClear();
  }

  void testGetAlgorithmsWithEmptyModel() {
    auto jobRunner = makeJobRunner();
    auto const algorithms = jobRunner.getAlgorithms();
    TS_ASSERT_EQUALS(algorithms, std::deque<IConfiguredAlgorithm_sptr>());
    verifyAndClear();
  }

  void testGetAlgorithmsWithMultiGroupModel() {
    // TODO add content to model
    auto jobRunner = makeJobRunner();
    auto const algorithms = jobRunner.getAlgorithms();
    TS_ASSERT_EQUALS(algorithms, std::deque<IConfiguredAlgorithm_sptr>());
    verifyAndClear();
  }

  void testAlgorithmStarted() {
    auto row = makeRow("12345", 0.5);
    auto jobRunner = makeJobRunner();

    EXPECT_CALL(*m_jobAlgorithm, item())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(&row));

    jobRunner.algorithmStarted(m_jobAlgorithm);
    TS_ASSERT_EQUALS(row.state(), State::ITEM_RUNNING);
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQ(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQBinned(), "");
    verifyAndClear();
  }

  void testAlgorithmComplete() {
    auto row = makeRow("12345", 0.5);
    auto jobRunner = makeJobRunner();
    auto iVsQ = createWorkspace();
    auto iVsQBin = createWorkspace();

    EXPECT_CALL(*m_jobAlgorithm, item())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(&row));
    EXPECT_CALL(*m_jobAlgorithm, outputWorkspaceNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{"", "IvsQ", "IvsQBin"}));
    EXPECT_CALL(*m_jobAlgorithm, outputWorkspaceNameToWorkspace())
        .Times(1)
        .WillOnce(Return(std::map<std::string, Workspace_sptr>{
            {"OutputWorkspace", iVsQ}, {"OutputWorkspaceBinned", iVsQBin}}));

    jobRunner.algorithmComplete(m_jobAlgorithm);
    TS_ASSERT_EQUALS(row.state(), State::ITEM_COMPLETE);
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQ(), "IvsQ");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQBinned(), "IvsQBin");
    verifyAndClear();
  }

  void testAlgorithmError() {
    auto row = makeRow("12345", 0.5);
    auto jobRunner = makeJobRunner();
    auto message = std::string("test error message");

    EXPECT_CALL(*m_jobAlgorithm, item())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(&row));

    jobRunner.algorithmError(m_jobAlgorithm, message);
    TS_ASSERT_EQUALS(row.state(), State::ITEM_ERROR);
    TS_ASSERT_EQUALS(row.message(), message);
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQ(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQBinned(), "");
    verifyAndClear();
  }

  void testGetWorkspacesToSaveForOnlyRowInGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto *row = &reductionJobs.mutableGroups()[0].mutableRows()[0].get();
    row->setOutputNames({"", "IvsQ", "IvsQBin"});

    EXPECT_CALL(*m_jobAlgorithm, item())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(row));

    // For a single row, we save the binned workspace for the row
    auto workspacesToSave =
        jobRunner.algorithmOutputWorkspacesToSave(m_jobAlgorithm);
    TS_ASSERT_EQUALS(workspacesToSave, std::vector<std::string>{"IvsQBin"});

    verifyAndClear();
  }

  void testGetWorkspacesToSaveForRowInMultiRowGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto *row = &reductionJobs.mutableGroups()[0].mutableRows()[0].get();
    row->setOutputNames({"", "IvsQ", "IvsQBin"});

    EXPECT_CALL(*m_jobAlgorithm, item())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(row));

    // For multiple rows, we don't save any workspaces
    auto workspacesToSave =
        jobRunner.algorithmOutputWorkspacesToSave(m_jobAlgorithm);
    TS_ASSERT_EQUALS(workspacesToSave, std::vector<std::string>{});

    verifyAndClear();
  }

  void testGetWorkspacesToSaveForGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto *group = &reductionJobs.mutableGroups()[0];
    group->setOutputNames({
        "stitched_test",
    });

    EXPECT_CALL(*m_jobAlgorithm, item())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(group));

    auto workspacesToSave =
        jobRunner.algorithmOutputWorkspacesToSave(m_jobAlgorithm);
    TS_ASSERT_EQUALS(workspacesToSave,
                     std::vector<std::string>{"stitched_test"});

    verifyAndClear();
  }

  void testDeletedWorkspaceResetsStateForRow() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &row = reductionJobs.mutableGroups()[0].mutableRows()[1];
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobRunner.notifyWorkspaceDeleted("IvsQBin_test");
    TS_ASSERT_EQUALS(row->state(), State::ITEM_NOT_STARTED);
    verifyAndClear();
  }

  void testDeletedWorkspaceResetsOutputNamesForRow() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &row = reductionJobs.mutableGroups()[0].mutableRows()[1];
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobRunner.notifyWorkspaceDeleted("IvsQBin_test");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQ(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQBinned(), "");
    verifyAndClear();
  }

  void testDeleteWorkspaceResetsStateForGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &group = reductionJobs.mutableGroups()[0];
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyWorkspaceDeleted("stitched_test");
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);
    verifyAndClear();
  }

  void testDeleteWorkspaceResetsOutputNamesForGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &group = reductionJobs.mutableGroups()[0];
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyWorkspaceDeleted("stitched_test");
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "");
    verifyAndClear();
  }

  void testRenameWorkspaceDoesNotResetStateForRow() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &row = reductionJobs.mutableGroups()[0].mutableRows()[1];
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobRunner.notifyWorkspaceRenamed("IvsQBin_test", "IvsQBin_new");
    TS_ASSERT_EQUALS(row->state(), State::ITEM_COMPLETE);
    verifyAndClear();
  }

  void testRenameWorkspaceUpdatesCorrectWorkspaceForRow() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &row = reductionJobs.mutableGroups()[0].mutableRows()[1];
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobRunner.notifyWorkspaceRenamed("IvsQBin_test", "IvsQBin_new");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQ(), "IvsQ_test");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQBinned(), "IvsQBin_new");
    verifyAndClear();
  }

  void testRenameWorkspaceDoesNotResetStateForGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &group = reductionJobs.mutableGroups()[0];
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyWorkspaceRenamed("stitched_test", "stitched_new");
    TS_ASSERT_EQUALS(group.state(), State::ITEM_COMPLETE);
    verifyAndClear();
  }

  void testRenameWorkspaceUpdatesPostprocessedNameForGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &group = reductionJobs.mutableGroups()[0];
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyWorkspaceRenamed("stitched_test", "stitched_new");
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "stitched_new");
    verifyAndClear();
  }

  void testDeleteAllWorkspacesResetsStateForRowAndGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &row = reductionJobs.mutableGroups()[0].mutableRows()[1];
    auto &group = reductionJobs.mutableGroups()[0];
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyAllWorkspacesDeleted();
    TS_ASSERT_EQUALS(row->state(), State::ITEM_NOT_STARTED);
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);
    verifyAndClear();
  }

  void testDeleteAllWorkspacesResetsOutputNamesForRowAndGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &row = reductionJobs.mutableGroups()[0].mutableRows()[1];
    auto &group = reductionJobs.mutableGroups()[0];
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyAllWorkspacesDeleted();
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQ(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQBinned(), "");
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "");
    verifyAndClear();
  }

private:
  std::vector<std::string> m_instruments;
  double m_tolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
  boost::shared_ptr<MockBatchJobAlgorithm> m_jobAlgorithm;

  class BatchJobRunnerFriend : public BatchJobRunner {
    friend class BatchJobRunnerTest;

  public:
    BatchJobRunnerFriend(Batch batch) : BatchJobRunner(batch) {}
  };

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobAlgorithm));
  }

  RunsTable makeRunsTable(ReductionJobs reductionJobs) {
    return RunsTable(m_instruments, m_tolerance, std::move(reductionJobs));
  }

  BatchJobRunnerFriend
  makeJobRunner(ReductionJobs reductionJobs = ReductionJobs()) {
    m_experiment = makeEmptyExperiment();
    m_instrument = makeEmptyInstrument();
    m_runsTable = makeRunsTable(std::move(reductionJobs));
    m_slicing = Slicing();
    return BatchJobRunnerFriend(
        Batch(m_experiment, m_instrument, m_runsTable, m_slicing));
  }

  Workspace2D_sptr createWorkspace() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    return ws;
  }
};

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_
