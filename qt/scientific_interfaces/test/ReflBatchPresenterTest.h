#ifndef MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_

#include "../ISISReflectometry/Presenters/BatchPresenter.h"
#include "MockBatchView.h"
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::Mock;
using testing::NiceMock;

class ReflBatchPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflBatchPresenterTest *createSuite() {
    return new ReflBatchPresenterTest();
  }

  static void destroySuite(ReflBatchPresenterTest *suite) { delete suite; }

  void selectedRowLocationsAre(
      MantidQt::MantidWidgets::Batch::MockJobTreeView &mockJobs,
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          locations) {
    ON_CALL(mockJobs, selectedRowLocations()).WillByDefault(Return(locations));
  }

  void jobsViewIs(MantidQt::MantidWidgets::Batch::IJobTreeView &jobsView,
                  MockBatchView &view) {
    ON_CALL(view, jobs()).WillByDefault(::testing::ReturnRef(jobsView));
  }

  UnslicedReductionJobs const& unslicedJobsFromPresenter(BatchPresenter &presenter) {
    return boost::get<UnslicedReductionJobs>(presenter.reductionJobs());
  }

  template <typename... Args>
  MantidQt::MantidWidgets::Batch::RowLocation location(Args... args) {
    return MantidQt::MantidWidgets::Batch::RowLocation(
        MantidQt::MantidWidgets::Batch::RowPath({args...}));
  }

  void testExpandsAllGroupsWhenRequested() {
    MantidQt::MantidWidgets::Batch::MockJobTreeView jobs;
    MockBatchView view;
    jobsViewIs(jobs, view);
    EXPECT_CALL(jobs, expandAll());

    BatchPresenter presenter(&view, {}, 0.01, UnslicedReductionJobs());
    presenter.notifyExpandAllRequested();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&jobs));
  }

  void testCollapsesAllGroupsWhenRequested() {
    MantidQt::MantidWidgets::Batch::MockJobTreeView jobs;
    MockBatchView view;
    jobsViewIs(jobs, view);
    EXPECT_CALL(jobs, collapseAll());

    BatchPresenter presenter(&view, {}, 0.01, UnslicedReductionJobs());
    presenter.notifyCollapseAllRequested();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&jobs));
  }

  UnslicedReductionJobs twoEmptyGroupsModel() {
    auto reductionJobs = UnslicedReductionJobs();
    reductionJobs.appendGroup(UnslicedGroup("Group 1"));
    reductionJobs.appendGroup(UnslicedGroup("Group 2"));
    return reductionJobs;
  }

  void testUpdatesViewWhenGroupInsertedAfterSelection() {
    NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> jobs;
    NiceMock<MockBatchView> view;
    jobsViewIs(jobs, view);

    auto reductionJobs = twoEmptyGroupsModel();
    selectedRowLocationsAre(jobs, {location(0)});
    EXPECT_CALL(jobs, insertChildRowOf(location(), 1))
        .WillOnce(Return(location(1)));

    BatchPresenter presenter(&view, {}, 0.01, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&jobs));
  }

  void testUpdatesModelWhenGroupInsertedAfterSelection() {
    NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> jobs;
    NiceMock<MockBatchView> view;
    jobsViewIs(jobs, view);

    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(jobs, {location(0)});
    ON_CALL(jobs, insertChildRowOf(location(), 1))
        .WillByDefault(Return(location(1)));

    BatchPresenter presenter(&view, {}, 0.01, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();

    TS_ASSERT(groups.size() == 3);
    TS_ASSERT_EQUALS(groups[0].name(), "Group 1");
    TS_ASSERT_EQUALS(groups[1].name(), "");
    TS_ASSERT_EQUALS(groups[2].name(), "Group 2");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&jobs));
  }

  void testUpdatesViewWhenGroupAppendedBasedOnEmptySelection() {
    NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> jobs;
    NiceMock<MockBatchView> view;
    jobsViewIs(jobs, view);

    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(jobs, {});
    EXPECT_CALL(jobs, appendChildRowOf(location()))
        .WillOnce(Return(location(2)));

    BatchPresenter presenter(&view, {}, 0.01, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&jobs));
  }

  void testUpdatesModelWhenGroupAppendedBasedOnEmptySelection() {
    NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> jobs;
    NiceMock<MockBatchView> view;
    jobsViewIs(jobs, view);

    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(jobs, {});
    ON_CALL(jobs, insertChildRowOf(location(), 1))
        .WillByDefault(Return(location(1)));

    BatchPresenter presenter(&view, {}, 0.01, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();

    TS_ASSERT(groups.size() == 3);
    TS_ASSERT_EQUALS(groups[0].name(), "Group 1");
    TS_ASSERT_EQUALS(groups[1].name(), "Group 2");
    TS_ASSERT_EQUALS(groups[2].name(), "");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&jobs));
  }

  void testInsertsAfterLastSelectedBasedOnMultiSelection() {
    NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> jobs;
    NiceMock<MockBatchView> view;
    jobsViewIs(jobs, view);

    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(jobs, {location(1), location(0)});
    ON_CALL(jobs, insertChildRowOf(location(), 2))
        .WillByDefault(Return(location(2)));

    BatchPresenter presenter(&view, {}, 0.01, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();

    TS_ASSERT(groups.size() == 3);
    TS_ASSERT_EQUALS(groups[0].name(), "Group 1");
    TS_ASSERT_EQUALS(groups[1].name(), "Group 2");
    TS_ASSERT_EQUALS(groups[2].name(), "");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&jobs));
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
