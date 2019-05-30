// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROGRESSBARTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROGRESSBARTEST_H_

#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "../ModelCreationHelpers.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class RunsTablePresenterProgressBarTest : public CxxTest::TestSuite,
                                          RunsTablePresenterTest {
public:
  static RunsTablePresenterProgressBarTest *createSuite() {
    return new RunsTablePresenterProgressBarTest();
  }

  static void destroySuite(RunsTablePresenterProgressBarTest *suite) {
    delete suite;
  }

  void testMergeJobsUpdatesProgressBar() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectUpdateProgressBar();
    presenter.mergeAdditionalJobs(ReductionJobs());
    verifyAndClearExpectations();
  }

  void testRowStateChangedUpdatesProgressBar() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectUpdateProgressBar();
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testAllCompleteWithEmptyTable() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectUpdateProgressBar(100);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testAllCompleteWithEmptyGroup() {
    auto reductionJobs = oneEmptyGroupModel();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(100);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowNotStarted() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    expectUpdateProgressBar(0);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStarting() {
    auto reductionJobs = oneGroupWithARowModel();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setStarting();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(0);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowRunning() {
    auto reductionJobs = oneGroupWithARowModel();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setRunning();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(0);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowSuccecss() {
    auto reductionJobs = oneGroupWithARowModel();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setSuccess();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(100);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowError() {
    auto reductionJobs = oneGroupWithARowModel();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setError(
        "error message");
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(100);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testGroupNotStarted() {
    auto presenter = makePresenter(m_view, oneGroupWithTwoRowsModel());
    expectUpdateProgressBar(0);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testGroupStarting() {
    auto reductionJobs = oneGroupWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].setStarting();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(0);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testGroupRunning() {
    auto reductionJobs = oneGroupWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].setRunning();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(0);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testGroupSuccecss() {
    auto reductionJobs = oneGroupWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].setSuccess();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(33);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testGroupError() {
    auto reductionJobs = oneGroupWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].setError("error message");
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(33);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testSingleRowGroupIsExcludedFromCount() {
    // Postprocessing is not applicable to a group if it only has one row, so
    // in this case the single row is the only item that needs processing and
    // so we expect 100% when that row is complete
    auto reductionJobs = oneGroupWithARowModel();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setSuccess();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(100);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testTwoRowGroupWithOneRowComplete() {
    auto reductionJobs = oneGroupWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setSuccess();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(33);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testTwoRowGroupWithTwoRowsComplete() {
    auto reductionJobs = oneGroupWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setSuccess();
    reductionJobs.mutableGroups()[0].mutableRows()[1]->setSuccess();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(66);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testTwoRowGroupWithEverythingComplete() {
    auto reductionJobs = oneGroupWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].setSuccess();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setSuccess();
    reductionJobs.mutableGroups()[0].mutableRows()[1]->setSuccess();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(100);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testTwoGroupsWithOneGroupComplete() {
    auto reductionJobs = twoGroupsWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].setSuccess();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setSuccess();
    reductionJobs.mutableGroups()[0].mutableRows()[1]->setSuccess();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(50);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testTwoGroupsWithBothGroupsComplete() {
    auto reductionJobs = twoGroupsWithTwoRowsModel();
    reductionJobs.mutableGroups()[0].setSuccess();
    reductionJobs.mutableGroups()[0].mutableRows()[0]->setSuccess();
    reductionJobs.mutableGroups()[0].mutableRows()[1]->setSuccess();
    reductionJobs.mutableGroups()[1].setSuccess();
    reductionJobs.mutableGroups()[1].mutableRows()[0]->setSuccess();
    reductionJobs.mutableGroups()[1].mutableRows()[1]->setSuccess();
    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    expectUpdateProgressBar(100);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

private:
  void expectUpdateProgressBar() {
    EXPECT_CALL(m_view, setProgress(_)).Times(1);
  }

  void expectUpdateProgressBar(int percentComplete) {
    EXPECT_CALL(m_view, setProgress(percentComplete)).Times(1);
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROGRESSBARTEST_H_
