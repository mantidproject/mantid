// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERPROGRESBARTEST_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERPROGRESBARTEST_H_

#include "BatchJobRunnerTest.h"

using namespace MantidQt::CustomInterfaces::ModelCreationHelper;

class BatchJobRunnerProgressBarTest : public CxxTest::TestSuite,
                                      public BatchJobRunnerTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobRunnerProgressBarTest *createSuite() {
    return new BatchJobRunnerProgressBarTest();
  }
  static void destroySuite(BatchJobRunnerProgressBarTest *suite) {
    delete suite;
  }

  void testProgressWithEmptyTable() {
    auto jobRunner = makeJobRunner(oneEmptyGroupModel());
    jobRunner.m_processAll = true;
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressWithEmptyGroup() {
    auto jobRunner = makeJobRunner(oneEmptyGroupModel());
    jobRunner.m_processAll = true;
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressWhenRowNotStarted() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    jobRunner.m_processAll = true;
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressWhenRowStarting() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    jobRunner.m_processAll = true;
    getRow(jobRunner, 0, 0)->setStarting();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressWhenRowRunning() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    jobRunner.m_processAll = true;
    getRow(jobRunner, 0, 0)->setRunning();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressWhenRowComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    jobRunner.m_processAll = true;
    getRow(jobRunner, 0, 0)->setSuccess();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressWhenRowFailed() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    jobRunner.m_processAll = true;
    getRow(jobRunner, 0, 0)->setError("error message");
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressWhenGroupNotStarted() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    jobRunner.m_processAll = true;
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressWhenGroupStarting() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getGroup(jobRunner, 0).setStarting();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressWhenGroupRunning() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getGroup(jobRunner, 0).setRunning();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressWhenGroupComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getGroup(jobRunner, 0).setSuccess();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 33);
  }

  void testProgressWhenGroupError() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getGroup(jobRunner, 0).setError("error message");
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 33);
  }

  void testProgressExcludesSingleRowGroup() {
    // Postprocessing is not applicable to a group if it only has one row, so
    // in this case the single row is the only item that needs processing and
    // so we expect 100% when that row is complete
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    jobRunner.m_processAll = true;
    getRow(jobRunner, 0, 0)->setSuccess();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressForTwoRowGroupWithOneRowComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getRow(jobRunner, 0, 0)->setSuccess();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 33);
  }

  void testProgressForTwoRowGroupWithTwoRowsComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 66);
  }

  void testProgressForTwoRowGroupWithEverythingComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getGroup(jobRunner, 0).setSuccess();
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressForTwoGroupsWithOneGroupComplete() {
    auto jobRunner = makeJobRunner(twoGroupsWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getGroup(jobRunner, 0).setSuccess();
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 50);
  }

  void testProgressForTwoGroupsWithBothGroupsComplete() {
    auto jobRunner = makeJobRunner(twoGroupsWithTwoRowsModel());
    jobRunner.m_processAll = true;
    getGroup(jobRunner, 0).setSuccess();
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    getGroup(jobRunner, 1).setSuccess();
    getRow(jobRunner, 1, 0)->setSuccess();
    getRow(jobRunner, 1, 1)->setSuccess();
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionWithEmptyTable() {
    auto jobRunner = makeJobRunner(oneEmptyGroupModel());
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionWithEmptyGroup() {
    auto jobRunner = makeJobRunner(oneEmptyGroupModel());
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionWhenRowNotStarted() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    selectGroup(jobRunner, 0);
    selectRow(jobRunner, 0, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenRowStarting() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    getRow(jobRunner, 0, 0)->setStarting();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenRowRunning() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    getRow(jobRunner, 0, 0)->setRunning();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenRowComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    getRow(jobRunner, 0, 0)->setSuccess();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionWhenRowFailed() {
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    getRow(jobRunner, 0, 0)->setError("error message");
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionWhenGroupNotStarted() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenGroupStarting() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getGroup(jobRunner, 0).setStarting();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenGroupRunning() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getGroup(jobRunner, 0).setRunning();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenGroupComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getGroup(jobRunner, 0).setSuccess();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 33);
  }

  void testProgressOfSelectionWhenGroupError() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getGroup(jobRunner, 0).setError("error message");
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 33);
  }

  void testProgressOfSelectionExcludesSingleRowGroup() {
    // Postprocessing is not applicable to a group if it only has one row, so
    // so we expect 100% when that row is complete
    auto jobRunner = makeJobRunner(oneGroupWithARowModel());
    jobRunner.m_processAll = true;
    getRow(jobRunner, 0, 0)->setSuccess();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionForTwoRowGroupWithOneRowComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getRow(jobRunner, 0, 0)->setSuccess();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 33);
  }

  void testProgressOfSelectionForTwoRowGroupWithTwoRowsComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 66);
  }

  void testProgressOfSelectionForTwoRowGroupWithEverythingComplete() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getGroup(jobRunner, 0).setSuccess();
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    selectGroup(jobRunner, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionForTwoGroupsWithOneGroupComplete() {
    auto jobRunner = makeJobRunner(twoGroupsWithTwoRowsModel());
    getGroup(jobRunner, 0).setSuccess();
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    selectGroup(jobRunner, 0);
    selectGroup(jobRunner, 1);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 50);
  }

  void testProgressOfSelectionWithBothChildAndParentItemsSelected() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getRow(jobRunner, 0, 1)->setSuccess();
    // The rows are implicitly selected when we select the group, but make sure
    // they rows are only counted once if we also select one of the rows
    selectGroup(jobRunner, 0);
    selectRow(jobRunner, 0, 0);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 33);
  }

  void testProgressOfSelectionWithOneRowOutOfTwoSelected() {
    auto jobRunner = makeJobRunner(oneGroupWithTwoRowsModel());
    getRow(jobRunner, 0, 1)->setSuccess();
    selectRow(jobRunner, 0, 1);
    // the selected row is complete
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionForTwoGroupsWithBothGroupsComplete() {
    auto jobRunner = makeJobRunner(twoGroupsWithTwoRowsModel());
    getGroup(jobRunner, 0).setSuccess();
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    getGroup(jobRunner, 1).setSuccess();
    getRow(jobRunner, 1, 0)->setSuccess();
    getRow(jobRunner, 1, 1)->setSuccess();
    selectGroup(jobRunner, 0);
    selectGroup(jobRunner, 1);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 100);
  }

  void testProgressOfSelectionExcludesUnselectedGroups() {
    auto jobRunner = makeJobRunner(twoGroupsWithTwoRowsModel());
    // first group is 100% complete
    getGroup(jobRunner, 0).setSuccess();
    getRow(jobRunner, 0, 0)->setSuccess();
    getRow(jobRunner, 0, 1)->setSuccess();
    // second group is 33% complete
    getRow(jobRunner, 1, 0)->setSuccess();
    // select second group only
    selectGroup(jobRunner, 1);
    TS_ASSERT_EQUALS(jobRunner.percentComplete(), 33);
  }
};

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERPROGRESSBARTEST_H_
