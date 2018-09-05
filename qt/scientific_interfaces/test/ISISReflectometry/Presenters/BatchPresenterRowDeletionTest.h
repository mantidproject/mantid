#ifndef MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERGROUPDELETETEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERGROUPDELETETEST_H_

#include "../../../ISISReflectometry/Presenters/BatchPresenter.h"
#include "../../../ISISReflectometry/Reduction/ReductionWorkspaces.h"
#include "BatchPresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class BatchPresenterRowDeletionTest : public CxxTest::TestSuite,
                                      BatchPresenterTest {
public:
  static BatchPresenterRowDeletionTest *createSuite() {
    return new BatchPresenterRowDeletionTest();
  }

  static void destroySuite(BatchPresenterRowDeletionTest *suite) {
    delete suite;
  }

  void testUpdatesViewWhenRowDeletedFromDirectSelection() {
    auto reductionJobs = twoGroupsWithARowModel();
    selectedRowLocationsAre(m_jobs, {location(0, 0)});

    EXPECT_CALL(
        m_jobs,
        removeRows(std::vector<MantidQt::MantidWidgets::Batch::RowLocation>(
            {location(0, 0)})));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyDeleteRowRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenRowDeletedFromDirectSelection() {
    selectedRowLocationsAre(m_jobs, {location(0, 0)});

    auto presenter = makePresenter(m_view, twoGroupsWithARowModel());
    presenter.notifyDeleteRowRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(0, groups[0].rows().size());

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenRowsDeletedFromMultiSelection() {
    selectedRowLocationsAre(m_jobs, {location(0, 0), location(1, 0)});

    auto presenter = makePresenter(m_view, twoGroupsWithARowModel());
    presenter.notifyDeleteRowRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(0, groups[0].rows().size());
    TS_ASSERT_EQUALS(0, groups[1].rows().size());

    verifyAndClearExpectations();
  }

  void testUpdatesViewWhenRowDeletedFromMultiSelection() {
    auto reductionJobs = oneGroupWithTwoRowsModel();
    selectedRowLocationsAre(m_jobs, {location(0, 0), location(0, 1)});

    EXPECT_CALL(
        m_jobs,
        removeRows(std::vector<MantidQt::MantidWidgets::Batch::RowLocation>(
            {location(0, 0), location(0, 1)})));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyDeleteRowRequested();

    verifyAndClearExpectations();
  }

  void testProducesErrorWhenOnlyGroupsSelected() {
    auto reductionJobs = twoGroupsWithARowModel();
    selectedRowLocationsAre(m_jobs, {location(0), location(1)});

    EXPECT_CALL(m_view, mustNotSelectGroup());

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyDeleteRowRequested();

    verifyAndClearExpectations();
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERGROUPDELETETEST_H_
