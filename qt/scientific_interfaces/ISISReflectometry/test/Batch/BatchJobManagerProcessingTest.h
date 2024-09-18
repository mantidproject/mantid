// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/Reduction/RowExceptions.h"
#include "BatchJobManagerTest.h"
#include "test/Batch/MockReflAlgorithmFactory.h"

#include <memory>

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;

class BatchJobManagerProcessingTest : public CxxTest::TestSuite, public BatchJobManagerTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobManagerProcessingTest *createSuite() { return new BatchJobManagerProcessingTest(); }
  static void destroySuite(BatchJobManagerProcessingTest *suite) { delete suite; }

  void tearDown() override {
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void testInitialisedWithNonRunningState() {
    auto jobManager = makeJobManager();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), false);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
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
  }

  void testReductionPaused() {
    auto jobManager = makeJobManager();
    jobManager.notifyReductionPaused();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), false);
  }

  void testAutoreductionResumed() {
    auto jobManager = makeJobManager();
    jobManager.notifyAutoreductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), false);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), true);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
    TS_ASSERT_EQUALS(jobManager.m_processAll, true);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
  }

  void testAutoreductionPaused() {
    auto jobManager = makeJobManager();
    jobManager.notifyAutoreductionPaused();
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
  }

  void testSetReprocessFailedItems() {
    auto jobManager = makeJobManager();
    jobManager.setReprocessFailedItems(true);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, true);
  }

  void testReductionResumedWithNoSelection() {
    auto jobManager = makeJobManager(twoGroupsWithARowModel());
    jobManager.notifyReductionResumed();
    TS_ASSERT_EQUALS(jobManager.isProcessing(), true);
    TS_ASSERT_EQUALS(jobManager.isAutoreducing(), false);
    TS_ASSERT_EQUALS(jobManager.m_reprocessFailed, false);
    TS_ASSERT_EQUALS(jobManager.m_processAll, true);
    TS_ASSERT_EQUALS(jobManager.m_processPartial, false);
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
  }

  void testGetAlgorithmsWithMultipleMatchingRows() {
    auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
    EXPECT_CALL(*mockAlgFactory, makeRowProcessingAlgorithm(_))
        .Times(1)
        .WillOnce(Throw(MultipleRowsFoundException("")));

    // Create the job manager and ensure the group/row is selected for processing
    auto jobManager = makeJobManager(twoGroupsWithARowModel(), std::move(mockAlgFactory));
    selectGroup(jobManager, 0);
    selectRow(jobManager, 0, 0);

    // Execute the test
    auto const algorithms = jobManager.getAlgorithms();

    // Check the row was marked with an error
    auto const &row = m_runsTable.reductionJobs().groups()[0].rows()[0].get();
    TS_ASSERT_EQUALS(row.state(), State::ITEM_ERROR);
    TS_ASSERT_EQUALS(row.message(),
                     "The title and angle specified matches multiple rows in the Experiment Settings tab");
    // Check the row was not included in the results
    TS_ASSERT(algorithms.empty());

    auto const &unprocessedRow = m_runsTable.reductionJobs().groups()[1].rows()[0].get();
    TS_ASSERT_EQUALS(unprocessedRow.state(), State::ITEM_NOT_STARTED);
  }

  void testGetAlgorithmsWithInvalidOptions() {
    auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
    EXPECT_CALL(*mockAlgFactory, makeRowProcessingAlgorithm(_)).Times(1).WillOnce(Throw(std::invalid_argument("")));

    // Create the job manager and ensure the group/row is selected for processing
    auto jobManager = makeJobManager(twoGroupsWithARowModel(), std::move(mockAlgFactory));
    selectGroup(jobManager, 0);
    selectRow(jobManager, 0, 0);

    // Execute the test
    auto const algorithms = jobManager.getAlgorithms();

    // Check the row was marked with an error
    auto const &row = m_runsTable.reductionJobs().groups()[0].rows()[0].get();
    TS_ASSERT_EQUALS(row.state(), State::ITEM_ERROR);
    TS_ASSERT_EQUALS(row.message(), "Error while setting algorithm properties: ");
    // Check the row was not included in the results
    TS_ASSERT(algorithms.empty());

    auto const &unprocessedRow = m_runsTable.reductionJobs().groups()[1].rows()[0].get();
    TS_ASSERT_EQUALS(unprocessedRow.state(), State::ITEM_NOT_STARTED);
  }

  void testGetAlgorithmsWithEmptyModel() {
    auto jobManager = makeJobManager();
    auto const algorithms = jobManager.getAlgorithms();
    TS_ASSERT_EQUALS(algorithms, std::deque<IConfiguredAlgorithm_sptr>());
  }

  void testGetAlgorithmsWithMultiGroupModel() {
    // TODO add content to model
    auto jobManager = makeJobManager();
    auto const algorithms = jobManager.getAlgorithms();
    TS_ASSERT_EQUALS(algorithms, std::deque<IConfiguredAlgorithm_sptr>());
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
  }
};
