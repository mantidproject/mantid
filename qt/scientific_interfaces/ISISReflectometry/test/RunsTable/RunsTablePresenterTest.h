// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Common/Plotter.h"
#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "../../../ISISReflectometry/Reduction/Slicing.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../ReflMockObjects.h"
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"
#include "MockRunsTableView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using MantidQt::MantidWidgets::Batch::Cell;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class RunsTablePresenterTest {
public:
  // The boilerplate methods are not included because this base class does not
  // include any tests itself

  void jobsViewIs(MantidQt::MantidWidgets::Batch::IJobTreeView &jobsView, MockRunsTableView &view) {
    ON_CALL(view, jobs()).WillByDefault(::testing::ReturnRef(jobsView));
  }

  RunsTablePresenterTest() : m_jobs(), m_view() {
    jobsViewIs(m_jobs, m_view);
    ON_CALL(m_jobs, cellsAt(_)).WillByDefault(Return(emptyCellsArray()));
  }

  bool verifyAndClearExpectations() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobs));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
    return true;
  }

  void selectedRowLocationsAre(MantidQt::MantidWidgets::Batch::MockJobTreeView &mockJobs,
                               std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations) {
    ON_CALL(mockJobs, selectedRowLocations()).WillByDefault(Return(locations));
  }

  void selectedColumnIs(MantidQt::MantidWidgets::Batch::MockJobTreeView &mockJobs, int columnIndex) {
    ON_CALL(mockJobs, currentColumn()).WillByDefault(Return(columnIndex));
  }

  ReductionJobs const &jobsFromPresenter(RunsTablePresenter &presenter) {
    return presenter.runsTable().reductionJobs();
  }

  template <typename... Args> MantidQt::MantidWidgets::Batch::RowLocation location(Args... args) {
    return MantidQt::MantidWidgets::Batch::RowLocation(MantidQt::MantidWidgets::Batch::RowPath({args...}));
  }

  RunsTablePresenter makePresenter(IRunsTableView &view) { return makePresenter(view, ReductionJobs()); }

  RunsTablePresenter makePresenter(IRunsTableView &view, ReductionJobs jobs) {
    auto presenter = RunsTablePresenter(&view, {}, 0.01, std::move(jobs), m_plotter);
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  Group &getGroup(RunsTablePresenter &presenter, int groupIndex) {
    auto &reductionJobs = presenter.mutableRunsTable().mutableReductionJobs();
    auto &group = reductionJobs.mutableGroups()[groupIndex];
    return group;
  }

  Row *getRow(RunsTablePresenter &presenter, int groupIndex, int rowIndex) {
    auto &reductionJobs = presenter.mutableRunsTable().mutableReductionJobs();
    auto *row = &reductionJobs.mutableGroups()[groupIndex].mutableRows()[rowIndex].get();
    return row;
  }

  Row *getRow(RunsTablePresenter &presenter, MantidQt::MantidWidgets::Batch::RowLocation const &location) {
    return getRow(presenter, location.path()[0], location.path()[1]);
  }

  std::vector<Cell> emptyCellsArray() {
    return std::vector<MantidQt::MantidWidgets::Batch::Cell>(10, MantidQt::MantidWidgets::Batch::Cell(""));
  }

  std::vector<Cell> cellsArray(std::string const &run = "12345", std::string const &theta = "0.5",
                               std::string const &trans1 = "", std::string const &trans2 = "") {
    return std::vector<Cell>{Cell(run), Cell(theta), Cell(trans1), Cell(trans2), Cell(""),
                             Cell(""),  Cell(""),    Cell(""),     Cell(""),     Cell("")};
  }

  void expectIsProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
  }

  void expectIsAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
  }

protected:
  NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> m_jobs;
  NiceMock<MockRunsTableView> m_view;
  NiceMock<MockRunsPresenter> m_mainPresenter;
  NiceMock<MockPlotter> m_plotter;
};
