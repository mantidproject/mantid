// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERWORKSPACESTEST_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERWORKSPACESTEST_H_

#include "BatchJobRunnerTest.h"

class BatchJobRunnerWorkspacesTest : public CxxTest::TestSuite,
                                     public BatchJobRunnerTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobRunnerWorkspacesTest *createSuite() {
    return new BatchJobRunnerWorkspacesTest();
  }
  static void destroySuite(BatchJobRunnerWorkspacesTest *suite) {
    delete suite;
  }

  void testGetWorkspacesToSaveForOnlyRowInGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    auto *row = getRow(jobRunner, 0, 0);
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
    auto *row = getRow(jobRunner, 0, 0);
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
    auto &group = getGroup(jobRunner, 0);
    group.setOutputNames({
        "stitched_test",
    });

    EXPECT_CALL(*m_jobAlgorithm, item())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(&group));

    auto workspacesToSave =
        jobRunner.algorithmOutputWorkspacesToSave(m_jobAlgorithm);
    TS_ASSERT_EQUALS(workspacesToSave,
                     std::vector<std::string>{"stitched_test"});

    verifyAndClear();
  }

  void testDeletedWorkspaceResetsStateForRow() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobRunner, 0, 1);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobRunner.notifyWorkspaceDeleted("IvsQBin_test");
    TS_ASSERT_EQUALS(row->state(), State::ITEM_NOT_STARTED);
    verifyAndClear();
  }

  void testDeletedWorkspaceResetsOutputNamesForRow() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobRunner, 0, 1);
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
    auto &group = getGroup(jobRunner, 0);
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyWorkspaceDeleted("stitched_test");
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);
    verifyAndClear();
  }

  void testDeleteWorkspaceResetsOutputNamesForGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &group = getGroup(jobRunner, 0);
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyWorkspaceDeleted("stitched_test");
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "");
    verifyAndClear();
  }

  void testRenameWorkspaceDoesNotResetStateForRow() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobRunner, 0, 1);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobRunner.notifyWorkspaceRenamed("IvsQBin_test", "IvsQBin_new");
    TS_ASSERT_EQUALS(row->state(), State::ITEM_COMPLETE);
    verifyAndClear();
  }

  void testRenameWorkspaceUpdatesCorrectWorkspaceForRow() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobRunner, 0, 1);
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
    auto &group = getGroup(jobRunner, 0);
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyWorkspaceRenamed("stitched_test", "stitched_new");
    TS_ASSERT_EQUALS(group.state(), State::ITEM_COMPLETE);
    verifyAndClear();
  }

  void testRenameWorkspaceUpdatesPostprocessedNameForGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto &group = getGroup(jobRunner, 0);
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobRunner.notifyWorkspaceRenamed("stitched_test", "stitched_new");
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "stitched_new");
    verifyAndClear();
  }

  void testDeleteAllWorkspacesResetsStateForRowAndGroup() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobRunner, 0, 1);
    auto &group = getGroup(jobRunner, 0);
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
    auto *row = getRow(jobRunner, 0, 1);
    auto &group = getGroup(jobRunner, 0);
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
};

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERWORKSPACESTEST_H_
