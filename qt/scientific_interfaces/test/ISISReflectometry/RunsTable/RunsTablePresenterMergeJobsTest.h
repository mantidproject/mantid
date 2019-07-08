// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERMERGEJOBSTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERMERGEJOBSTEST_H_

#include "../../../ISISReflectometry/Common/ModelCreationHelper.h"
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

class RunsTablePresenterMergeJobsTest : public CxxTest::TestSuite,
                                        RunsTablePresenterTest {
public:
  static RunsTablePresenterMergeJobsTest *createSuite() {
    return new RunsTablePresenterMergeJobsTest();
  }

  static void destroySuite(RunsTablePresenterMergeJobsTest *suite) {
    delete suite;
  }

  void testMergeEmptyTableDoesNothing() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    presenter.mergeAdditionalJobs(ReductionJobs());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, oneGroupWithARowModel());
  }

  void testMergeDuplicateGroupDoesNothing() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    presenter.mergeAdditionalJobs(oneGroupWithARowModel());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, oneGroupWithARowModel());
  }

  void testMergeNewGroup() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    presenter.mergeAdditionalJobs(anotherGroupWithARowModel());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, twoGroupsWithARowModel());
  }

  void testMergeDuplicateRowDoesNothing() {
    auto presenter = makePresenter(m_view, oneGroupWithTwoRowsModel());
    presenter.mergeAdditionalJobs(oneGroupWithARowModel());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, oneGroupWithTwoRowsModel());
  }

  void testMergeInvalidRowDoesNothing() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    presenter.mergeAdditionalJobs(oneGroupWithAnInvalidRowModel());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, oneGroupWithARowModel());
  }

  void testMergeNewRowIntoExistingGroup() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    presenter.mergeAdditionalJobs(oneGroupWithAnotherRowModel());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, oneGroupWithTwoRowsModel());
  }

  void testMergeNewRowIntoExistingGroupIsSortedByAngle() {
    auto presenter = makePresenter(m_view, oneGroupWithAnotherRowModel());
    presenter.mergeAdditionalJobs(oneGroupWithARowModel());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, oneGroupWithTwoRowsModel());
  }

  void testMergeNewRunIntoExistingRow() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    presenter.mergeAdditionalJobs(oneGroupWithAnotherRunWithSameAngleModel());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, oneGroupWithTwoRunsInARowModel());
  }

  void testMergeNewRunIntoExistingRowIsSortedByRunNumber() {
    auto presenter =
        makePresenter(m_view, oneGroupWithAnotherRunWithSameAngleModel());
    presenter.mergeAdditionalJobs(oneGroupWithARowModel());
    auto &result = jobsFromPresenter(presenter);
    TS_ASSERT_EQUALS(result, oneGroupWithTwoRunsInARowModel());
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERMERGEJOBSTEST_H_
