// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Plotting/Plotter.h"
#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "../../../ISISReflectometry/Reduction/Slicing.h"
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"
#include "MockRunsTableView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class RunsTablePresenterTest {
public:
  // The boilerplate methods are not included because this base class does not
  // include any tests itself

  void jobsViewIs(MantidQt::MantidWidgets::Batch::IJobTreeView &jobsView,
                  MockRunsTableView &view) {
    ON_CALL(view, jobs()).WillByDefault(::testing::ReturnRef(jobsView));
  }

  Row basicRow() {
    return Row(std::vector<std::string>({"101", "102"}), 1.2,
               TransmissionRunPair{"A", "B"}, RangeInQ(), boost::none, {},
               ReductionWorkspaces({}, TransmissionRunPair()));
  }

  ReductionJobs twoEmptyGroupsModel() {
    auto reductionJobs = ReductionJobs();
    reductionJobs.appendGroup(Group("Group 1"));
    reductionJobs.appendGroup(Group("Group 2"));
    return reductionJobs;
  }

  ReductionJobs twoGroupsWithARowModel() {
    auto reductionJobs = ReductionJobs();
    auto group1 = Group("Group 1");
    group1.appendRow(basicRow());
    reductionJobs.appendGroup(std::move(group1));

    auto group2 = Group("Group 2");
    group2.appendRow(basicRow());
    reductionJobs.appendGroup(std::move(group2));

    return reductionJobs;
  }

  ReductionJobs oneGroupWithTwoRowsModel() {
    auto reductionJobs = ReductionJobs();
    auto group1 = Group("Group 1");
    group1.appendRow(basicRow());
    group1.appendRow(basicRow());
    reductionJobs.appendGroup(std::move(group1));
    return reductionJobs;
  }

  RunsTablePresenterTest() : m_jobs(), m_view() {
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
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const
          &locations) {
    ON_CALL(mockJobs, selectedRowLocations()).WillByDefault(Return(locations));
  }

  ReductionJobs const &jobsFromPresenter(RunsTablePresenter &presenter) {
    return presenter.runsTable().reductionJobs();
  }

  template <typename... Args>
  MantidQt::MantidWidgets::Batch::RowLocation location(Args... args) {
    return MantidQt::MantidWidgets::Batch::RowLocation(
        MantidQt::MantidWidgets::Batch::RowPath({args...}));
  }

  RunsTablePresenter makePresenter(IRunsTableView &view) {
    Plotter plotter(nullptr);
    return RunsTablePresenter(&view, {}, 0.01, ReductionJobs(), plotter);
  }

  RunsTablePresenter makePresenter(IRunsTableView &view, ReductionJobs jobs) {
    Plotter plotter(nullptr);
    return RunsTablePresenter(&view, {}, 0.01, std::move(jobs), plotter);
  }

protected:
  NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> m_jobs;
  NiceMock<MockRunsTableView> m_view;
};
#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERTEST_H_
