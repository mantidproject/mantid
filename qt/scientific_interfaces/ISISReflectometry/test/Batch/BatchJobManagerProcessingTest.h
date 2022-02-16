// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BatchJobManagerTest.h"

class BatchJobManagerProcessingTest : public CxxTest::TestSuite, public BatchJobManagerTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobManagerProcessingTest *createSuite() { return new BatchJobManagerProcessingTest(); }
  static void destroySuite(BatchJobManagerProcessingTest *suite) { delete suite; }

  void testInitialisedWithNonRunningState() {
    auto jobManager = makeJobManager();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), false);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    verifyAndClear();
  }

  void testReductionResumed() {
    auto jobManager = makeJobManager();
    jobManager.notifyReductionResumed();
    auto const hasSelection = false;
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, hasSelection);
    TS_ASSERT_EQUALS(jobManager.m_processAll, !hasSelection);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, hasSelection);
    verifyAndClear();
  }

  void testReductionPaused() {
    auto jobManager = makeJobManager();
    jobManager.notifyReductionPaused();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), false);
    verifyAndClear();
  }

  void testAutoreductionResumed() {
    auto jobManager = makeJobManager();
    jobManager.notifyAutoreductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), false);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), true);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, true);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testAutoreductionPaused() {
    auto jobManager = makeJobManager();
    jobManager.notifyAutoreductionPaused();
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    verifyAndClear();
  }

  void testSetReprocessFailedItems() {
    auto jobManager = makeJobManager();
    jobManager.setReprocessFailedItems(true);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    verifyAndClear();
  }

  void testReductionResumedWithNoSelection() {
    auto jobManager = makeJobManager(twoGroupsWithARowModel());
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, false);
    TS_ASSERT_EQUALS(jobManager.m_processAll, true);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testReductionResumedWithBothGroupsSelected() {
    auto jobManager = makeJobManager(twoGroupsWithARowModel());
    selectGroup(jobManager, 0);
    selectGroup(jobManager, 1);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testReductionResumedWithBothGroupsSelectedAndEmptyGroupNotSelected() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsAndOneEmptyGroupModel());
    selectGroup(jobManager, 0);
    selectGroup(jobManager, 1);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testReductionResumedWithGroupAndRowSelected() {
    auto jobManager = makeJobManager(twoGroupsWithARowModel());
    selectGroup(jobManager, 0);
    selectRow(jobManager, 1, 0);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testReductionResumedWithGroupAndNonInvalidRowSelected() {
    auto jobManager = makeJobManager(oneGroupWithOneRowAndOneGroupWithOneRowAndOneInvalidRowModel());
    selectGroup(jobManager, 0);
    selectRow(jobManager, 1, 0);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testReductionResumedWithAllRowsSelected() {
    auto jobManager = makeJobManager(twoGroupsWithARowModel());
    selectRow(jobManager, 0, 0);
    selectRow(jobManager, 1, 0);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testReductionResumedWithAllNonInvalidRowsSelected() {
    auto jobManager = makeJobManager(twoGroupsWithOneRowAndOneInvalidRowModel());
    selectRow(jobManager, 0, 0);
    selectRow(jobManager, 1, 0);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testReductionResumedWithSomeRowsSelected() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsModel());
    selectRow(jobManager, 0, 1);
    selectRow(jobManager, 1, 0);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, true);
    verifyAndClear();
  }

  void testReductionResumedWithGroupAndSomeRowsSelected() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsModel());
    selectGroup(jobManager, 0);
    selectRow(jobManager, 1, 0);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, true);
    verifyAndClear();
  }

  void testReductionResumedWithGroupAndChildRowSelected() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsModel());
    selectGroup(jobManager, 0);
    selectRow(jobManager, 0, 0);
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, false);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
    verifyAndClear();
  }

  void testGetAlgorithmsWithEmptyModel() {
    auto jobManager = makeJobManager();
    auto const algorithms = jobManager.getAlgorithms();
    TS_ASSERT_EQUALS(algorithms, std::deque<IConfiguredAlgorithm_sptr>());
    verifyAndClear();
  }

  void testGetAlgorithmsWithMultiGroupModel() {
    // TODO add content to model
    auto jobManager = makeJobManager();
    auto const algorithms = jobManager.getAlgorithms();
    TS_ASSERT_EQUALS(algorithms, std::deque<IConfiguredAlgorithm_sptr>());
    verifyAndClear();
  }

  void testAlgorithmStarted() {
    auto row = makeRow("12345", 0.5);
    auto jobManager = makeJobManager();

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(&row));

    jobManager.algorithmStarted(m_jobAlgorithm);
    TS_ASSERT_EQUALS(row.state(), State::ITEM_RUNNING);
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQ(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQBinned(), "");
    verifyAndClear();
  }

  void testAlgorithmComplete() {
    auto row = makeRow("12345", 0.5);
    auto jobManager = makeJobManager();
    auto iVsQ = createWorkspace();
    auto iVsQBin = createWorkspace();

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(&row));
    EXPECT_CALL(*m_jobAlgorithm, updateItem()).Times(1);

    jobManager.algorithmComplete(m_jobAlgorithm);
    TS_ASSERT_EQUALS(row.state(), State::ITEM_SUCCESS);
    verifyAndClear();
  }

  void testAlgorithmError() {
    auto row = makeRow("12345", 0.5);
    auto jobManager = makeJobManager();
    auto message = std::string("test error message");

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(&row));

    jobManager.algorithmError(m_jobAlgorithm, message);
    TS_ASSERT_EQUALS(row.state(), State::ITEM_ERROR);
    TS_ASSERT_EQUALS(row.message(), message);
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsLambda(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQ(), "");
    TS_ASSERT_EQUALS(row.reducedWorkspaceNames().iVsQBinned(), "");
    verifyAndClear();
  }

  void testAlgorithmCompleteSetsParentsSingleRow() {
    auto group = makeGroupWithOneRow();
    Row *row = &group.mutableRows()[0].get();
    auto jobManager = makeJobManager();

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(AtLeast(1)).WillRepeatedly(Return(row));
    EXPECT_CALL(*m_jobAlgorithm, updateItem()).Times(1);

    jobManager.algorithmComplete(m_jobAlgorithm);

    TS_ASSERT_EQUALS(row->state(), State::ITEM_SUCCESS);
    TS_ASSERT_EQUALS(group.state(), State::ITEM_CHILDREN_SUCCESS);

    verifyAndClear();
  }

  void testAlgorithmCompleteSetsParentsMultipleRows() {
    auto group = makeGroupWithTwoRows();
    Row *row1 = &group.mutableRows()[0].get();
    Row *row2 = &group.mutableRows()[1].get();
    auto jobManager = makeJobManager();

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(2).WillOnce(Return(row1)).WillOnce(Return(row2));
    EXPECT_CALL(*m_jobAlgorithm, updateItem()).Times(2);

    jobManager.algorithmComplete(m_jobAlgorithm);

    TS_ASSERT_EQUALS(row1->state(), State::ITEM_SUCCESS);
    TS_ASSERT_EQUALS(row2->state(), State::ITEM_NOT_STARTED);
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);

    jobManager.algorithmComplete(m_jobAlgorithm);

    TS_ASSERT_EQUALS(row1->state(), State::ITEM_SUCCESS);
    TS_ASSERT_EQUALS(row2->state(), State::ITEM_SUCCESS);
    TS_ASSERT_EQUALS(group.state(), State::ITEM_CHILDREN_SUCCESS);

    verifyAndClear();
  }

  void testAlgorithmErrorSetsParentIncomplete() {
    auto group = makeGroupWithTwoRows();
    Row *row1 = &group.mutableRows()[0].get();
    Row *row2 = &group.mutableRows()[1].get();
    auto jobManager = makeJobManager();

    EXPECT_CALL(*m_jobAlgorithm, item()).Times(2).WillOnce(Return(row1)).WillOnce(Return(row2));
    EXPECT_CALL(*m_jobAlgorithm, updateItem()).Times(1);

    jobManager.algorithmError(m_jobAlgorithm, "row1 invalid");

    TS_ASSERT_EQUALS(row1->state(), State::ITEM_ERROR);
    TS_ASSERT_EQUALS(row2->state(), State::ITEM_NOT_STARTED);
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);

    jobManager.algorithmComplete(m_jobAlgorithm);

    TS_ASSERT_EQUALS(row1->state(), State::ITEM_ERROR);
    TS_ASSERT_EQUALS(row2->state(), State::ITEM_SUCCESS);
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);

    verifyAndClear();
  }
};
