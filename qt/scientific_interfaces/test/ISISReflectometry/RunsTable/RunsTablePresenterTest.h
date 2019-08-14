// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERTEST_H_

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
using testing::AtLeast;
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

  RunsTablePresenterTest() : m_jobs(), m_view() {
    jobsViewIs(m_jobs, m_view);
    ON_CALL(m_jobs, cellsAt(_))
        .WillByDefault(Return(std::vector<MantidQt::MantidWidgets::Batch::Cell>(
            8, MantidQt::MantidWidgets::Batch::Cell(""))));
  }

  bool verifyAndClearExpectations() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobs));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
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
    return makePresenter(view, ReductionJobs());
  }

  RunsTablePresenter makePresenter(IRunsTableView &view, ReductionJobs jobs) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    Plotter plotter(nullptr);
#else
    Plotter plotter;
#endif
    auto presenter =
        RunsTablePresenter(&view, {}, 0.01, std::move(jobs), plotter);
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void expectIsProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
  }

  void expectIsAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
  }

protected:
  NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> m_jobs;
  NiceMock<MockRunsTableView> m_view;
  NiceMock<MockRunsPresenter> m_mainPresenter;
};
#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERTEST_H_
