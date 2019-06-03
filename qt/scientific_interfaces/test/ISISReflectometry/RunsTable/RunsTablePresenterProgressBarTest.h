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

private:
  void expectUpdateProgressBar() {
    auto progress = 33;
    EXPECT_CALL(m_mainPresenter, percentComplete())
        .Times(1)
        .WillOnce(Return(progress));
    EXPECT_CALL(m_view, setProgress(progress)).Times(1);
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROGRESSBARTEST_H_
