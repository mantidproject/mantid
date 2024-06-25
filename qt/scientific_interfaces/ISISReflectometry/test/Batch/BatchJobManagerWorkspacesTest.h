// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BatchJobManagerTest.h"

class BatchJobManagerWorkspacesTest : public CxxTest::TestSuite, public BatchJobManagerTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobManagerWorkspacesTest *createSuite() { return new BatchJobManagerWorkspacesTest(); }
  static void destroySuite(BatchJobManagerWorkspacesTest *suite) { delete suite; }

  void tearDown() override {
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void testGetWorkspacesToSaveForOnlyRowInGroup() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    auto *row = getRow(jobManager, 0, 0);
    row->setOutputNames({"", "IvsQ", "IvsQBin"});

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(row));

    // For a single row, we save the binned workspace for the row
    auto workspacesToSave = jobManager.algorithmOutputWorkspacesToSave(m_jobAlgorithm, false);
    TS_ASSERT_EQUALS(workspacesToSave, std::vector<std::string>{"IvsQBin"});
  }

  void testGetWorkspacesToSaveForRowInMultiRowGroup() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobManager, 0, 0);
    row->setOutputNames({"", "IvsQ", "IvsQBin"});

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(row));

    // For multiple rows, we don't save any workspaces
    auto workspacesToSave = jobManager.algorithmOutputWorkspacesToSave(m_jobAlgorithm, false);
    TS_ASSERT_EQUALS(workspacesToSave, std::vector<std::string>{});
  }

  void testGetWorkspacesToSaveForGroupWithoutIncludeRows() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto &group = getGroup(jobManager, 0);
    group.setOutputNames({
        "stitched_test",
    });

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(&group));

    auto workspacesToSave = jobManager.algorithmOutputWorkspacesToSave(m_jobAlgorithm, false);
    TS_ASSERT_EQUALS(workspacesToSave, std::vector<std::string>{"stitched_test"});
  }

  void testGetWorkspacesToSaveForGroupWithIncludeRows() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto &group = getGroup(jobManager, 0);
    group.setOutputNames({
        "stitched_test",
    });
    auto *row1 = getRow(jobManager, 0, 0);
    row1->setOutputNames({"", "test1", "row_bin_test01"});
    auto *row2 = getRow(jobManager, 0, 1);
    row2->setOutputNames({"", "test2", "row_bin_test02"});

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(&group));

    auto workspacesToSave = jobManager.algorithmOutputWorkspacesToSave(m_jobAlgorithm, true);
    // The order of items in the vector of workspace names is important - the stitched workspace name must be the first
    // one to ensure that it is picked up as the filename if saving all workspaces to a single file
    TS_ASSERT_EQUALS(workspacesToSave, std::vector<std::string>({"stitched_test", "row_bin_test01", "row_bin_test02"}));
  }

  void testGetWorkspacesToSaveForGroupHasNoRowsWithIncludeRows() {
    auto jobManager = makeJobManager(oneEmptyGroupModel());
    auto &group = getGroup(jobManager, 0);
    group.setOutputNames({
        "stitched_test",
    });

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(&group));

    auto workspacesToSave = jobManager.algorithmOutputWorkspacesToSave(m_jobAlgorithm, true);
    TS_ASSERT_EQUALS(workspacesToSave, std::vector<std::string>{"stitched_test"});
  }

  void testGetWorkspacesToSaveForGroupHasInvalidRowWithIncludeRows() {
    auto jobManager = makeJobManager(oneGroupWithAnInvalidRowModel());
    auto &group = getGroup(jobManager, 0);
    group.setOutputNames({
        "stitched_test",
    });

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(&group));

    auto workspacesToSave = jobManager.algorithmOutputWorkspacesToSave(m_jobAlgorithm, true);
    TS_ASSERT_EQUALS(workspacesToSave, std::vector<std::string>{"stitched_test"});
  }

  void testDeletedWorkspaceResetsStateForRow() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobManager, 0, 1);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobManager.notifyWorkspaceDeleted("IvsQBin_test");
    TS_ASSERT_EQUALS(row->state(), State::ITEM_NOT_STARTED);
  }

  void testDeletedWorkspaceResetsOutputNamesForRow() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobManager, 0, 1);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobManager.notifyWorkspaceDeleted("IvsQBin_test");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQ(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQBinned(), "");
  }

  void testDeleteWorkspaceResetsStateForGroup() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto &group = getGroup(jobManager, 0);
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobManager.notifyWorkspaceDeleted("stitched_test");
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);
  }

  void testDeleteWorkspaceResetsOutputNamesForGroup() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto &group = getGroup(jobManager, 0);
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobManager.notifyWorkspaceDeleted("stitched_test");
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "");
  }

  void testRenameWorkspaceDoesNotResetStateForRow() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobManager, 0, 1);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobManager.notifyWorkspaceRenamed("IvsQBin_test", "IvsQBin_new");
    TS_ASSERT_EQUALS(row->state(), State::ITEM_SUCCESS);
  }

  void testRenameWorkspaceDoesResetStateForRowWhenOldNameIsSameAsCurrent() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobManager, 0, 1);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobManager.notifyWorkspaceRenamed("IvsQBin_new", "IvsQBin_test");
    TS_ASSERT_DIFFERS(row->state(), State::ITEM_SUCCESS);
  }

  void testRenameWorkspaceUpdatesCorrectWorkspaceForRow() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobManager, 0, 1);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});

    jobManager.notifyWorkspaceRenamed("IvsQBin_test", "IvsQBin_new");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQ(), "IvsQ_test");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQBinned(), "IvsQBin_new");
  }

  void testRenameWorkspaceDoesNotResetStateForGroup() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto &group = getGroup(jobManager, 0);
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobManager.notifyWorkspaceRenamed("stitched_test", "stitched_new");
    TS_ASSERT_EQUALS(group.state(), State::ITEM_SUCCESS);
  }

  void testRenameWorkspaceUpdatesPostprocessedNameForGroup() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto &group = getGroup(jobManager, 0);
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobManager.notifyWorkspaceRenamed("stitched_test", "stitched_new");
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "stitched_new");
  }

  void testDeleteAllWorkspacesResetsStateForRowAndGroup() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobManager, 0, 1);
    auto &group = getGroup(jobManager, 0);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobManager.notifyAllWorkspacesDeleted();
    TS_ASSERT_EQUALS(row->state(), State::ITEM_NOT_STARTED);
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);
  }

  void testDeleteAllWorkspacesResetsOutputNamesForRowAndGroup() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    auto *row = getRow(jobManager, 0, 1);
    auto &group = getGroup(jobManager, 0);
    row->setSuccess();
    row->setOutputNames({"", "IvsQ_test", "IvsQBin_test"});
    group.setSuccess();
    group.setOutputNames({"stitched_test"});

    jobManager.notifyAllWorkspacesDeleted();
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQ(), "");
    TS_ASSERT_EQUALS(row->reducedWorkspaceNames().iVsQBinned(), "");
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "");
  }
};
