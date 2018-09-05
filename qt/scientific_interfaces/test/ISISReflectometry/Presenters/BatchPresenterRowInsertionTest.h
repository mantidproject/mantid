#ifndef MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERGROUPINSERTTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERGROUPINSERTTEST_H_

#include "../../../ISISReflectometry/Presenters/BatchPresenter.h"
#include "BatchPresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class BatchPresenterRowInsertionTest : public CxxTest::TestSuite,
                                       BatchPresenterTest {
public:
  static BatchPresenterRowInsertionTest *createSuite() {
    return new BatchPresenterRowInsertionTest();
  }

  static void destroySuite(BatchPresenterRowInsertionTest *suite) {
    delete suite;
  }

  void testUpdatesViewWhenRowInsertedAfterSelection() {
    auto reductionJobs = twoGroupsWithARowModel();

    selectedRowLocationsAre(m_jobs, {location(0, 0)});
    EXPECT_CALL(m_jobs, appendChildRowOf(location(0)))
        .WillOnce(Return(location(0, 1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertRowRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenRowInsertedAfterSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(0, 0)});
    ON_CALL(m_jobs, appendChildRowOf(location(0)))
        .WillByDefault(Return(location(0, 1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertRowRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();

    TS_ASSERT_EQUALS(1, groups[0].rows().size());
    verifyAndClearExpectations();
  }

  void testProducesErrorWhenNothingSelected() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {});
    EXPECT_CALL(m_view, mustSelectGroupOrRow());

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertRowRequested();

    verifyAndClearExpectations();
  }

  void testInsertsRowsInModelForEachSelectedBasedOnMultiSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(1), location(0)});
    ON_CALL(m_jobs, appendChildRowOf(location(0)))
        .WillByDefault(Return(location(0, 1)));
    ON_CALL(m_jobs, appendChildRowOf(location(1)))
        .WillByDefault(Return(location(1, 1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertRowRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(1, groups[0].rows().size());
    TS_ASSERT_EQUALS(1, groups[1].rows().size());

    verifyAndClearExpectations();
  }

  void testInsertsRowsInViewForEachSelectedBasedOnMultiSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(0), location(1)});
    EXPECT_CALL(m_jobs, appendChildRowOf(location(0)))
        .WillOnce(Return(location(0, 1)));
    EXPECT_CALL(m_jobs, appendChildRowOf(location(1)))
        .WillOnce(Return(location(1, 1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertRowRequested();

    verifyAndClearExpectations();
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERGROUPINSERTTEST_H_
