// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "../../../ISISReflectometry/Reduction/ReductionWorkspaces.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class RunsTablePresenterGroupDeletionTest : public CxxTest::TestSuite, RunsTablePresenterTest {
public:
  static RunsTablePresenterGroupDeletionTest *createSuite() { return new RunsTablePresenterGroupDeletionTest(); }

  static void destroySuite(RunsTablePresenterGroupDeletionTest *suite) { delete suite; }

  void testUpdatesViewWhenGroupDeletedFromDirectSelection() {
    auto reductionJobs = twoEmptyGroupsModel();
    selectedRowLocationsAre(m_jobs, {location(0)});

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    EXPECT_CALL(m_jobs, removeRowAt(location(0))).Times(2);
    presenter.notifyDeleteGroupRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenGroupDeletedFromDirectSelection() {
    selectedRowLocationsAre(m_jobs, {location(1)});

    auto presenter = makePresenter(m_view, twoGroupsWithARowModel());
    presenter.notifyDeleteGroupRequested();

    auto &groups = jobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(1, groups.size());
    TS_ASSERT_EQUALS("Test group 1", groups[0].name());

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenGroupDeletedFromMultiselection() {
    selectedRowLocationsAre(m_jobs, {location(0), location(1)});

    auto presenter = makePresenter(m_view, twoEmptyGroupsModel());
    presenter.notifyDeleteGroupRequested();

    auto &groups = jobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(1, groups.size());

    verifyAndClearExpectations();
  }

  void testUpdatesViewWhenGroupDeletedFromMultiSelection() {
    auto reductionJobs = twoGroupsWithARowModel();
    selectedRowLocationsAre(m_jobs, {location(0), location(1)});

    {
      testing::InSequence s;
      EXPECT_CALL(m_jobs, removeRowAt(location(1)));
      EXPECT_CALL(m_jobs, removeRowAt(location(0))).Times(2);
    }

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyDeleteGroupRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesViewWhenGroupDeletedFromChildRowSelection() {
    auto reductionJobs = twoGroupsWithARowModel();
    selectedRowLocationsAre(m_jobs, {location(0, 0)});

    EXPECT_CALL(m_jobs, removeRowAt(location(0)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyDeleteGroupRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesViewWhenGroupDeletedFromChildRowMultiSelection() {
    auto reductionJobs = twoGroupsWithARowModel();
    selectedRowLocationsAre(m_jobs, {location(0, 0), location(1, 0)});

    {
      testing::InSequence s;
      EXPECT_CALL(m_jobs, removeRowAt(location(1)));
      EXPECT_CALL(m_jobs, removeRowAt(location(0))).Times(2);
    }

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyDeleteGroupRequested();

    verifyAndClearExpectations();
  }

  void testGroupNotDeletedWhenProcessing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectIsProcessing();
    EXPECT_CALL(m_jobs, removeRowAt(_)).Times(0);
    presenter.notifyDeleteGroupRequested();
    verifyAndClearExpectations();
  }

  void testGroupNotDeletedWhenAutoreducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectIsAutoreducing();
    EXPECT_CALL(m_jobs, removeRowAt(_)).Times(0);
    presenter.notifyDeleteGroupRequested();
    verifyAndClearExpectations();
  }

  void testRemoveAllRowsAndGroupsUpdatesView() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_jobs, removeAllRows()).Times(1);
    presenter.notifyRemoveAllRowsAndGroupsRequested();
    verifyAndClearExpectations();
  }

  void testRemoveAllRowsAndGroupsPerformedIfProcessingOrAutoreducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, isProcessing()).Times(0);
    EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(0);
    presenter.notifyRemoveAllRowsAndGroupsRequested();
    verifyAndClearExpectations();
  }

  void testRemoveAllRowsAndGroupsLeavesAGroupAndRow() {
    auto presenter = makePresenter(m_view);
    presenter.notifyRemoveAllRowsAndGroupsRequested();
    auto &groups = jobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(1, groups.size());
    TS_ASSERT_EQUALS(1, groups[0].rows().size());
  }
};
