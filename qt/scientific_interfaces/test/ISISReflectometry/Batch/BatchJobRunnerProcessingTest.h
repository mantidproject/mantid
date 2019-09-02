// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERPROCESSINGTEST_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERPROCESSINGTEST_H_

#include "BatchJobRunnerTest.h"

class BatchJobRunnerProcessingTest : public CxxTest::TestSuite,
                                     public BatchJobRunnerTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobRunnerProcessingTest *createSuite() {
    return new BatchJobRunnerProcessingTest();
  }
  static void destroySuite(BatchJobRunnerProcessingTest *suite) {
    delete suite;
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
    EXPECT_CALL(*m_jobAlgorithm, updateItem()).Times(1);

    jobRunner.algorithmComplete(m_jobAlgorithm);
    TS_ASSERT_EQUALS(row.state(), State::ITEM_COMPLETE);
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
};

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERPROCESSINGTEST_H_
