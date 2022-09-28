// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BatchJobManagerTest.h"

using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;

class BatchJobManagerProgressBarTest : public CxxTest::TestSuite, public BatchJobManagerTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobManagerProgressBarTest *createSuite() { return new BatchJobManagerProgressBarTest(); }
  static void destroySuite(BatchJobManagerProgressBarTest *suite) { delete suite; }

  void testProgressWithEmptyTable() {
    auto jobManager = makeJobManager(oneEmptyGroupModel());
    jobManager.m_processAll = true;
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressWithEmptyGroup() {
    auto jobManager = makeJobManager(oneEmptyGroupModel());
    jobManager.m_processAll = true;
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressWhenRowNotStarted() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    jobManager.m_processAll = true;
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressWhenRowStarting() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setStarting();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressWhenRowRunning() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setRunning();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressWhenRowComplete() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setSuccess();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressWhenRowFailed() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setError("error message");
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressWhenGroupNotStarted() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    jobManager.m_processAll = true;
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressWhenGroupStarting() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    jobManager.m_processAll = true;
    getGroup(jobManager, 0).setStarting();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressWhenGroupRunning() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    jobManager.m_processAll = true;
    getGroup(jobManager, 0).setRunning();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressWhenGroupComplete() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    jobManager.m_processAll = true;
    getGroup(jobManager, 0).setSuccess();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 33);
  }

  void testProgressWhenGroupError() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    jobManager.m_processAll = true;
    getGroup(jobManager, 0).setError("error message");
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 33);
  }

  void testProgressExcludesSingleRowGroup() {
    // Postprocessing is not applicable to a group if it only has one row, so
    // in this case the single row is the only item that needs processing and
    // so we expect 100% when that row is complete
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setSuccess();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressForTwoRowGroupWithOneRowComplete() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setSuccess();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 33);
  }

  void testProgressForTwoRowGroupWithTwoRowsComplete() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 66);
  }

  void testProgressForTwoRowGroupWithEverythingComplete() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    getGroup(jobManager, 0).setSuccess();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressForTwoGroupsWithOneGroupComplete() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    getGroup(jobManager, 0).setSuccess();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 50);
  }

  void testProgressForTwoGroupsWithBothGroupsComplete() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    getGroup(jobManager, 0).setSuccess();
    getRow(jobManager, 1, 0)->setSuccess();
    getRow(jobManager, 1, 1)->setSuccess();
    getGroup(jobManager, 1).setSuccess();
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionWithEmptyTable() {
    auto jobManager = makeJobManager(oneEmptyGroupModel());
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionWithEmptyGroup() {
    auto jobManager = makeJobManager(oneEmptyGroupModel());
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionWhenRowNotStarted() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    selectGroup(jobManager, 0);
    selectRow(jobManager, 0, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenRowStarting() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    getRow(jobManager, 0, 0)->setStarting();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenRowRunning() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    getRow(jobManager, 0, 0)->setRunning();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenRowComplete() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    getRow(jobManager, 0, 0)->setSuccess();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionWhenRowFailed() {
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    getRow(jobManager, 0, 0)->setError("error message");
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionWhenGroupNotStarted() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenGroupStarting() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getGroup(jobManager, 0).setStarting();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenGroupRunning() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getGroup(jobManager, 0).setRunning();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 0);
  }

  void testProgressOfSelectionWhenGroupComplete() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getGroup(jobManager, 0).setSuccess();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 33);
  }

  void testProgressOfSelectionWhenGroupError() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getGroup(jobManager, 0).setError("error message");
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 33);
  }

  void testProgressOfSelectionExcludesSingleRowGroup() {
    // Postprocessing is not applicable to a group if it only has one row, so
    // so we expect 100% when that row is complete
    auto jobManager = makeJobManager(oneGroupWithARowModel());
    jobManager.m_processAll = true;
    getRow(jobManager, 0, 0)->setSuccess();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionForTwoRowGroupWithOneRowComplete() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getRow(jobManager, 0, 0)->setSuccess();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 33);
  }

  void testProgressOfSelectionForTwoRowGroupWithTwoRowsComplete() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 66);
  }

  void testProgressOfSelectionForTwoRowGroupWithEverythingComplete() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    getGroup(jobManager, 0).setSuccess();
    selectGroup(jobManager, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionForTwoGroupsWithOneGroupComplete() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsModel());
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    getGroup(jobManager, 0).setSuccess();
    selectGroup(jobManager, 0);
    selectGroup(jobManager, 1);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 50);
  }

  void testProgressOfSelectionWithBothChildAndParentItemsSelected() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getRow(jobManager, 0, 1)->setSuccess();
    // The rows are implicitly selected when we select the group, but make sure
    // they rows are only counted once if we also select one of the rows
    selectGroup(jobManager, 0);
    selectRow(jobManager, 0, 0);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 33);
  }

  void testProgressOfSelectionWithOneRowOutOfTwoSelected() {
    auto jobManager = makeJobManager(oneGroupWithTwoRowsModel());
    getRow(jobManager, 0, 1)->setSuccess();
    selectRow(jobManager, 0, 1);
    // the selected row is complete
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionForTwoGroupsWithBothGroupsComplete() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsModel());
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    getGroup(jobManager, 0).setSuccess();
    getRow(jobManager, 1, 0)->setSuccess();
    getRow(jobManager, 1, 1)->setSuccess();
    getGroup(jobManager, 1).setSuccess();
    selectGroup(jobManager, 0);
    selectGroup(jobManager, 1);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 100);
  }

  void testProgressOfSelectionExcludesUnselectedGroups() {
    auto jobManager = makeJobManager(twoGroupsWithTwoRowsModel());
    // first group is 100% complete
    getGroup(jobManager, 0).setSuccess();
    getRow(jobManager, 0, 0)->setSuccess();
    getRow(jobManager, 0, 1)->setSuccess();
    // second group is 33% complete
    getRow(jobManager, 1, 0)->setSuccess();
    // select second group only
    selectGroup(jobManager, 1);
    TS_ASSERT_EQUALS(jobManager.percentComplete(), 33);
  }
};
