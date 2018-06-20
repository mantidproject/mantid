#ifndef MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_

#include "../../../ISISReflectometry/Presenters/BatchPresenter.h"
#include "../../../ISISReflectometry/Reduction/Slicing.h"
#include "MockBatchView.h"
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::Mock;
using testing::NiceMock;
using testing::_;

class BatchPresenterTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  void jobsViewIs(MantidQt::MantidWidgets::Batch::IJobTreeView &jobsView,
                  MockBatchView &view) {
    ON_CALL(view, jobs()).WillByDefault(::testing::ReturnRef(jobsView));
  }

  UnslicedRow basicRow() {
    return UnslicedRow(std::vector<std::string>({"101", "102"}), 1.2,
                       {"A", "B"}, boost::none, boost::none, {},
                       ReductionWorkspaces({}, {"", ""}, "", "", "", ""));
  }

  UnslicedReductionJobs twoEmptyGroupsModel() {
    auto reductionJobs = UnslicedReductionJobs();
    reductionJobs.appendGroup(UnslicedGroup("Group 1"));
    reductionJobs.appendGroup(UnslicedGroup("Group 2"));
    return reductionJobs;
  }

  UnslicedReductionJobs twoGroupsWithARowModel() {
    auto reductionJobs = UnslicedReductionJobs();
    auto group1 = UnslicedGroup("Group 1");
    group1.appendRow(basicRow());
    reductionJobs.appendGroup(std::move(group1));

    auto group2 = UnslicedGroup("Group 2");
    group2.appendRow(basicRow());
    reductionJobs.appendGroup(std::move(group2));

    return reductionJobs;
  }

  UnslicedReductionJobs oneGroupWithTwoRowsModel() {
    auto reductionJobs = UnslicedReductionJobs();
    auto group1 = UnslicedGroup("Group 1");
    group1.appendRow(basicRow());
    group1.appendRow(basicRow());
    reductionJobs.appendGroup(std::move(group1));
    return reductionJobs;
  }

  BatchPresenterTest() : m_jobs(), m_view() {
    jobsViewIs(m_jobs, m_view);
    ON_CALL(m_jobs, cellsAt(_))
        .WillByDefault(Return(std::vector<MantidQt::MantidWidgets::Batch::Cell>(
            8, MantidQt::MantidWidgets::Batch::Cell(""))));
  }

  bool verifyAndClearExpectations() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobs));
    return true;
  }

  void selectedRowLocationsAre(
      MantidQt::MantidWidgets::Batch::MockJobTreeView &mockJobs,
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          locations) {
    ON_CALL(mockJobs, selectedRowLocations()).WillByDefault(Return(locations));
  }

  UnslicedReductionJobs const &
  unslicedJobsFromPresenter(BatchPresenter &presenter) {
    return boost::get<UnslicedReductionJobs>(presenter.reductionJobs());
  }

  template <typename... Args>
  MantidQt::MantidWidgets::Batch::RowLocation location(Args... args) {
    return MantidQt::MantidWidgets::Batch::RowLocation(
        MantidQt::MantidWidgets::Batch::RowPath({args...}));
  }

  BatchPresenter makePresenter(IBatchView &view) {
    static auto slicing = Slicing();
    return BatchPresenter(&view, {}, 0.01, WorkspaceNamesFactory(slicing),
                          UnslicedReductionJobs());
  }

  BatchPresenter makePresenter(IBatchView &view, Jobs jobs) {
    static auto slicing = Slicing();
    return BatchPresenter(&view, {}, 0.01, WorkspaceNamesFactory(slicing),
                          std::move(jobs));
  }

protected:
  NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> m_jobs;
  NiceMock<MockBatchView> m_view;
};

#endif // MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
