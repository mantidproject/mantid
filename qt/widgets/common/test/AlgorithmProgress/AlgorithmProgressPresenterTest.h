// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "ManualProgressReporter.h"
#include "MockAlgorithmProgressWidget.h"

#include <QCoreApplication>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

using namespace testing;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
void pauseForTimer() {
  // Algorithm will only write an update after 0.1 seconds, so suspend for 0.2
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}
} // namespace

class AlgorithmProgressPresenterTest : public CxxTest::TestSuite {
public:
  static AlgorithmProgressPresenterTest *createSuite() {
    AlgorithmFactory::Instance().subscribe<Mantid::Algorithms::ManualProgressReporter>();
    return new AlgorithmProgressPresenterTest();
  }
  static void destroySuite(AlgorithmProgressPresenterTest *suite) {
    AlgorithmFactory::Instance().unsubscribe(NAME_MANUALRPOGRESSREPORTER, 1);
    delete suite;
  }

  using MockViewT = MockAlgorithmProgressWidget;

  void testAlgorithmStart() {
    // Sets up a fake AlgorithmID - it is just a `void *`
    // so the address of an int is taken to act as one
    // what it actually points to does not matter
    int testInt = 123;
    void *algorithmIDpretender = &testInt;
    auto mockView = createMockView();
    EXPECT_CALL(*mockView, algorithmStarted()).Times(1);

    auto pres = createPresenter(mockView.get());
    pres->algorithmStartedSlot(algorithmIDpretender);
  }

  void testAlgorithmStart_SecondAlgorithmStartDoesntReplaceFirst() {
    int testInt = 123;
    void *algorithmIDpretender = &testInt;
    int testInt2 = 666;
    void *secondAlgorithmID = &testInt2;
    auto mockView = createMockView();
    EXPECT_CALL(*mockView, algorithmStarted()).Times(1);

    auto pres = createPresenter(mockView.get());
    pres->algorithmStartedSlot(algorithmIDpretender);
    // second call should not increment the algorithm started calls
    pres->algorithmStartedSlot(secondAlgorithmID);
  }
  void testAlgorithmEnd() {
    int testInt = 123;
    void *algorithmIDpretender = &testInt;
    auto mockView = createMockView();
    EXPECT_CALL(*mockView, algorithmStarted()).Times(1);
    EXPECT_CALL(*mockView, algorithmEnded()).Times(1);

    auto pres = createPresenter(mockView.get());
    pres->algorithmStartedSlot(algorithmIDpretender);
    // Alg ended is from another algorithm ID,
    // it should not cancel the first one
    pres->algorithmEndedSlot(algorithmIDpretender);
  }
  void testAlgorithmEnd_NotTrackedAlgorithmEnds() {
    int testInt = 123;
    void *algorithmIDpretender = &testInt;
    int testInt2 = 666;
    void *secondAlgorithmID = &testInt2;
    auto mockView = createMockView();
    EXPECT_CALL(*mockView, algorithmStarted()).Times(1);
    EXPECT_CALL(*mockView, algorithmEnded()).Times(0);

    auto pres = createPresenter(mockView.get());
    pres->algorithmStartedSlot(algorithmIDpretender);
    // Alg ended is from another algorithm ID,
    // it should not cancel the first one
    pres->algorithmEndedSlot(secondAlgorithmID);
  }

  void testUpdateProgressBar() {
    int testInt = 123;
    void *algorithmIDpretender = &testInt;
    auto mockView = createMockView();
    EXPECT_CALL(*mockView, algorithmStarted()).Times(1);
    QString emptyQString;
    EXPECT_CALL(*mockView, updateProgress(DoubleEq(3.0), emptyQString, 0., 0)).Times(1);

    auto pres = createPresenter(mockView.get());
    pres->algorithmStartedSlot(algorithmIDpretender);
    pauseForTimer();
    pres->updateProgressBarSlot(algorithmIDpretender, 3.0, "", 0., 0);
  }
  void testUpdateProgressBar_NotUpdatedIfAlgorithmNotBeingTracked() {
    int testInt = 123;
    void *algorithmIDpretender = &testInt;
    int testInt2 = 666;
    void *secondAlgorithmID = &testInt2;
    auto mockView = createMockView();
    EXPECT_CALL(*mockView, algorithmStarted()).Times(1);
    EXPECT_CALL(*mockView, updateProgress(3.0, QString(""), 0., 0)).Times(0);

    auto pres = createPresenter(mockView.get());
    pres->algorithmStartedSlot(algorithmIDpretender);
    pauseForTimer();
    pres->updateProgressBarSlot(secondAlgorithmID, 3.0, "", 0., 0);
  }

private:
  std::unique_ptr<MockViewT> createMockView() { return std::make_unique<MockViewT>(); }

  std::unique_ptr<AlgorithmProgressPresenter> createPresenter(MockViewT *mockView) {
    return std::make_unique<AlgorithmProgressPresenter>(nullptr, mockView);
  }
};
