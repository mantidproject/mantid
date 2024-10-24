// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "ManualProgressReporter.h"
#include "MockAlgorithmProgressDialogWidget.h"

#include <QApplication>
#include <QProgressBar>
#include <QTreeWidgetItem>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

using MockViewT = MockAlgorithmProgressDialogWidget;

namespace {

struct AlgorithmProgressDialogMockedTypes {
  AlgorithmProgressDialogMockedTypes(std::unique_ptr<MockViewT> mockedView) : m_mockedView(std::move(mockedView)) {
    // This is an extremely complex dependency chain to mock, but unfortunately
    // legacy code is legacy code....
    initParentPresenter();
    m_presenter =
        std::make_unique<AlgorithmProgressDialogPresenter>(nullptr, m_mockedView.get(), _parentPresenter->model());
  }

  std::unique_ptr<MockViewT> m_mockedView;
  std::unique_ptr<AlgorithmProgressDialogPresenter> m_presenter;

private:
  // If you need to attach mocks to this please move your test to
  // AlgorithmProgressPresenterTest.h instead, as you're testing an impl detail
  std::unique_ptr<NiceMock<MockAlgorithmProgressWidget>> _mockedParentView;
  std::unique_ptr<AlgorithmProgressPresenter> _parentPresenter;

  void initParentPresenter() {
    _mockedParentView = std::make_unique<NiceMock<MockAlgorithmProgressWidget>>();
    _parentPresenter = std::make_unique<AlgorithmProgressPresenter>(nullptr, _mockedParentView.get());
  }
};

} // namespace

class AlgorithmProgressDialogPresenterTest : public CxxTest::TestSuite {
public:
  static AlgorithmProgressDialogPresenterTest *createSuite() {
    AlgorithmFactory::Instance().subscribe<Mantid::Algorithms::ManualProgressReporter>();
    return new AlgorithmProgressDialogPresenterTest();
  }
  static void destroySuite(AlgorithmProgressDialogPresenterTest *suite) {
    AlgorithmFactory::Instance().unsubscribe(NAME_MANUALRPOGRESSREPORTER, 1);
    delete suite;
  }

  /** This test runs the dev algorithm and sees if it was
   *  currectly tracked during start/updates/end
   */
  void testAlgorithmIsTrackedCorrectly() {
    auto mockedView = createMockView();

    auto widget = new QTreeWidgetItem();
    auto progressBar = new QProgressBar();
    auto returnPair = std::make_pair(widget, progressBar);

    auto alg = AlgorithmManager::Instance().create("ManualProgressReporter");
    ON_CALL(*mockedView, addAlgorithm(alg)).WillByDefault(Return(returnPair));
    EXPECT_CALL(*mockedView, addAlgorithm(alg)).Times(Exactly(1));

    auto mockedTypes = createPresenter(std::move(mockedView));

    QString emptyQString;

    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("NumberOfProgressReports", 10);
    alg->setRethrows(true);
    alg->execute();
    QCoreApplication::processEvents();
    TS_ASSERT_EQUALS(size_t{0}, mockedTypes.m_presenter->getNumberTrackedAlgorithms());
  }
  /** This tests running algorithms from inside an algorithm
   * and that they are all properly tracked for their lifetime
   * in the dialog
   */
  void testAlgorithmThatRunsOtherAlgorithmsIsTrackedCorrectly() {
    // This is the empty QString that will be
    // passed to update progress from the call
    QString emptyQString;

    // changing this will cause the test to fail the assertions
    // on the mainProgressBar, as the expected progress
    // number will be wrong
    constexpr int numReports = 10;
    // the number of widgets is 1 higher, as the algorithm made in this
    // test also needs a widget created for it
    constexpr int numWidgets = numReports + 1;

    // Vector to keep the pointers for all widgets
    // so they can be destroyed at the end of the test.
    // There was an attempt to use unique_ptr, but it kept
    // deleting the pointers on the make_pair call, resulting in segfault
    auto widgetPairs = std::vector<std::pair<QTreeWidgetItem *, QProgressBar *>>();
    widgetPairs.reserve(numWidgets);

    auto mockedView = createMockView();
    auto &expectedCallObject = EXPECT_CALL(*mockedView, addAlgorithm(_));
    // The loop is done numWidgets times, to account for the algorithm
    // that is initialised in this test, as this is the 11th call
    for (int i = 0; i < numWidgets; ++i) {
      widgetPairs.emplace_back(std::make_pair(new QTreeWidgetItem(), new QProgressBar()));
      // it's OK to use the reference, due to the lifetime of the vector
      // the objects will be alive until the end of the test
      const auto &pair = widgetPairs[i];
      // Appends expected calls to the testing object
      // each one gets a new pair of widget/progress bar.
      // This is done to avoid segfaulting whenever an
      // algorithm ends as it deletes the widget
      // by providing different objects for each algorithm
      expectedCallObject.WillOnce(Return(pair));
    }

    auto mockedTypes = createPresenter(std::move(mockedView));

    auto alg = AlgorithmManager::Instance().create("ManualProgressReporter");
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("NumberOfProgressReports", numReports);
    // this will start another alg as much times as there are reports
    alg->setProperty("StartAnotherAlgorithm", true);
    alg->execute();
    QCoreApplication::processEvents();
    TS_ASSERT_EQUALS(size_t{0}, mockedTypes.m_presenter->getNumberTrackedAlgorithms());

    // free the pointers for the widgets
    for (const auto &pair : widgetPairs) {
      // the first widget is deleted whenever the algorithm ends
      // manually delete the second one, otherwise it leaks memory
      delete pair.second;
    }
  }

private:
  std::unique_ptr<MockViewT> createMockView() { return std::make_unique<MockViewT>(); }

  AlgorithmProgressDialogMockedTypes createPresenter(std::unique_ptr<MockViewT> mockedView) {
    // Use ownership to force mocks to be set before, avoiding undef behaviour
    return AlgorithmProgressDialogMockedTypes(std::move(mockedView));
  }
};
