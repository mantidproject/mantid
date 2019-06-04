// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPINSERTTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPINSERTTEST_H_

#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::ModelCreationHelper;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class RunsTablePresenterRowInsertionTest : public CxxTest::TestSuite,
                                           RunsTablePresenterTest {
public:
  static RunsTablePresenterRowInsertionTest *createSuite() {
    return new RunsTablePresenterRowInsertionTest();
  }

  static void destroySuite(RunsTablePresenterRowInsertionTest *suite) {
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

    auto &groups = jobsFromPresenter(presenter).groups();

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

    auto &groups = jobsFromPresenter(presenter).groups();
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

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPINSERTTEST_H_
