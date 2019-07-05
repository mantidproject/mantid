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

class RunsTablePresenterGroupInsertionTest : public CxxTest::TestSuite,
                                             RunsTablePresenterTest {
public:
  static RunsTablePresenterGroupInsertionTest *createSuite() {
    return new RunsTablePresenterGroupInsertionTest();
  }

  static void destroySuite(RunsTablePresenterGroupInsertionTest *suite) {
    delete suite;
  }

  void testExpandsAllGroupsWhenRequested() {
    EXPECT_CALL(m_jobs, expandAll()).Times(2);

    auto presenter = makePresenter(m_view);
    presenter.notifyExpandAllRequested();

    verifyAndClearExpectations();
  }

  void testCollapsesAllGroupsWhenRequested() {
    EXPECT_CALL(m_jobs, collapseAll());

    auto presenter = makePresenter(m_view);
    presenter.notifyCollapseAllRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesViewWhenGroupInsertedAfterSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(0)});
    EXPECT_CALL(m_jobs, insertChildRowOf(location(), 1))
        .WillOnce(Return(location(1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenGroupInsertedAfterSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(0)});
    ON_CALL(m_jobs, insertChildRowOf(location(), 1))
        .WillByDefault(Return(location(1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = jobsFromPresenter(presenter).groups();

    TS_ASSERT_EQUALS(3, groups.size());
    TS_ASSERT_EQUALS("Test group 1", groups[0].name());
    TS_ASSERT_EQUALS("Group1",
                     groups[1].name()); // default name for inserted group
    TS_ASSERT_EQUALS("Test group 2", groups[2].name());

    verifyAndClearExpectations();
  }

  void testUpdatesViewWhenGroupAppendedBasedOnEmptySelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {});
    EXPECT_CALL(m_jobs, appendChildRowOf(location()))
        .WillOnce(Return(location(2)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenGroupAppendedBasedOnEmptySelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {});
    ON_CALL(m_jobs, insertChildRowOf(location(), 1))
        .WillByDefault(Return(location(1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = jobsFromPresenter(presenter).groups();

    TS_ASSERT_EQUALS(3, groups.size());
    TS_ASSERT_EQUALS("Group1", groups[2].name());
    verifyAndClearExpectations();
  }

  void testInsertsGroupAfterLastSelectedBasedOnMultiSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(1), location(0)});
    ON_CALL(m_jobs, insertChildRowOf(location(), 2))
        .WillByDefault(Return(location(2)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = jobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(3, groups.size());
    TS_ASSERT_EQUALS("Group1", groups[2].name());

    verifyAndClearExpectations();
  }

  void testGroupNotInsertedWhenProcessing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectIsProcessing();
    EXPECT_CALL(m_jobs, insertChildRowOf(_, _)).Times(0);
    presenter.notifyInsertGroupRequested();
    verifyAndClearExpectations();
  }

  void testGroupNotInsertedWhenAutoreducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectIsAutoreducing();
    EXPECT_CALL(m_jobs, insertChildRowOf(_, _)).Times(0);
    presenter.notifyInsertGroupRequested();
    verifyAndClearExpectations();
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPINSERTTEST_H_
