// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROCESSINGTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROCESSINGTEST_H_

#include "../../../ISISReflectometry/Common/ModelCreationHelper.h"
#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class RunsTablePresenterProcessingTest : public CxxTest::TestSuite,
                                         RunsTablePresenterTest {
public:
  static RunsTablePresenterProcessingTest *createSuite() {
    return new RunsTablePresenterProcessingTest();
  }

  static void destroySuite(RunsTablePresenterProcessingTest *suite) {
    delete suite;
  }

  void testNotifyReductionResumed() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, notifyReductionResumed()).Times(1);
    presenter.notifyReductionResumed();
    verifyAndClearExpectations();
  }

  void testNotifyReductionPaused() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, notifyReductionPaused()).Times(1);
    presenter.notifyReductionPaused();
    verifyAndClearExpectations();
  }

  void testNotifyInstrumentChanged() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    auto const instrument = std::string("test_instrument");
    EXPECT_CALL(m_view, getInstrumentName())
        .Times(1)
        .WillOnce(Return(instrument));
    EXPECT_CALL(m_mainPresenter, notifyInstrumentChanged(instrument)).Times(1);
    presenter.notifyInstrumentChanged();
    verifyAndClearExpectations();
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

private:
  void expectUpdateProgressBar() {
    auto progress = 33;
    EXPECT_CALL(m_mainPresenter, percentComplete())
        .Times(1)
        .WillOnce(Return(progress));
    EXPECT_CALL(m_view, setProgress(progress)).Times(1);
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROCESSINGTEST_H_
