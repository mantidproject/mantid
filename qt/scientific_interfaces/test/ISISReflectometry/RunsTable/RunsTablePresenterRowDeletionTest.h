// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPDELETETEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPDELETETEST_H_

#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "../../../ISISReflectometry/Reduction/ReductionWorkspaces.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::ModelCreationHelper;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class RunsTablePresenterRowDeletionTest : public CxxTest::TestSuite,
                                          RunsTablePresenterTest {
public:
  static RunsTablePresenterRowDeletionTest *createSuite() {
    return new RunsTablePresenterRowDeletionTest();
  }

  static void destroySuite(RunsTablePresenterRowDeletionTest *suite) {
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

    auto &groups = jobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(0, groups[0].rows().size());

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenRowsDeletedFromMultiSelection() {
    selectedRowLocationsAre(m_jobs, {location(0, 0), location(1, 0)});

    auto presenter = makePresenter(m_view, twoGroupsWithARowModel());
    presenter.notifyDeleteRowRequested();

    auto &groups = jobsFromPresenter(presenter).groups();
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

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPDELETETEST_H_
