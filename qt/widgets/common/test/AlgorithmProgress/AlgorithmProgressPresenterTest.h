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
#include <memory>

using namespace testing;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

class AlgorithmProgressPresenterTest : public CxxTest::TestSuite {
public:
  static AlgorithmProgressPresenterTest *createSuite() {
    AlgorithmFactory::Instance().subscribe<Mantid::Algorithms::ManualProgressReporter>();
    return new AlgorithmProgressPresenterTest();
  }
  static void destroySuite(AlgorithmProgressPresenterTest *suite) { delete suite; }

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
    EXPECT_CALL(*mockView, updateProgress(3.0, _, 0, 0)).Times(1);

    auto pres = createPresenter(mockView.get());
    pres->algorithmStartedSlot(algorithmIDpretender);
    // Algorithm reports a progress update
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
    pres->updateProgressBarSlot(secondAlgorithmID, 3.0, "", 0., 0);
  }
  void testRealAlgorithmRunning() {
    auto mockView = createMockView();
    EXPECT_CALL(*mockView, algorithmStarted()).Times(1);
    int reports = 10;
    QString emptyQString;
    // This is the only way the comparison worked,
    // incrementing a bool had too high error (in 1e-1 range)),
    // but manually writing the values works.
    // This way testing::DoubleNear doesn't seem to be necessary
    // Another thing to note: 0.0 progress is NOT reported
    for (const auto prog : {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}) {
      EXPECT_CALL(*mockView, updateProgress(DoubleEq(prog), emptyQString, 0., 0));
    }
    EXPECT_CALL(*mockView, algorithmEnded()).Times(1);

    auto alg = AlgorithmManager::Instance().create("ManualProgressReporter");
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("NumberOfProgressReports", reports);
    alg->setRethrows(true);
    alg->execute();
    QCoreApplication::processEvents();
  }

private:
  std::unique_ptr<MockViewT> createMockView() { return std::make_unique<MockViewT>(); }

  std::unique_ptr<AlgorithmProgressPresenter> createPresenter(MockViewT *mockView) {
    return std::make_unique<AlgorithmProgressPresenter>(nullptr, mockView);
  }
};