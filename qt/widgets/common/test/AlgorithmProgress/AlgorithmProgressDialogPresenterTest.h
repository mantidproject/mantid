#ifndef MANTIDQT_MANTIDWIDGETS_ALGORITHMPROGRESSPRESENTERTEST_H_
#define MANTIDQT_MANTIDWIDGETS_ALGORITHMPROGRESSPRESENTERTEST_H_

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "ManualProgressReporter.h"
#include "MockAlgorithmProgressDialogWidget.h"

#include <QApplication>
#include <QProgressBar>
#include <QTreeWidgetItem>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

class AlgorithmProgressDialogPresenterTest : public CxxTest::TestSuite {
public:
  static AlgorithmProgressDialogPresenterTest *createSuite() {
    AlgorithmFactory::Instance()
        .subscribe<Mantid::Algorithms::ManualProgressReporter>();
    return new AlgorithmProgressDialogPresenterTest();
  }
  static void destroySuite(AlgorithmProgressDialogPresenterTest *suite) {
    AlgorithmFactory::Instance().unsubscribe(NAME_MANUALRPOGRESSREPORTER, 1);
    delete suite;
  }

  void setUp() override {
    mockDialogView.reset();
    // The mock view also creates the presenter, because
    // so that is passes the correct type into the constructor
    mockDialogView =
        std::make_unique<NiceMock<MockAlgorithmProgressDialogWidget>>();
  }

  /** This test runs the dev algorithm and sees if it was
   *  currectly tracked during start/updates/end
   */
  void testAlgorithmIsTrackedCorrectly() {
    auto mainProgressBar = mockDialogView->mainProgressBar;
    QString emptyQString;

    //----------
    // These assertions are actually for the main progress bar
    // but the algorithm will trigger them too
    // adding the expected calls prevents a bunch of GMock warnings
    EXPECT_CALL(*mainProgressBar.get(), algorithmStarted()).Times(1);
    for (const auto prog : {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}) {
      EXPECT_CALL(*mainProgressBar.get(),
                  updateProgress(DoubleEq(prog), emptyQString));
    }
    EXPECT_CALL(*mainProgressBar.get(), algorithmEnded()).Times(1);
    // End of assertions for the main progress bar
    //----------

    auto widget = new QTreeWidgetItem();
    auto progressBar = new QProgressBar();
    auto returnPair = std::make_pair(widget, progressBar);

    auto alg = AlgorithmManager::Instance().create("ManualProgressReporter");
    ON_CALL(*mockDialogView.get(), addAlgorithm(alg))
        .WillByDefault(Return(returnPair));
    EXPECT_CALL(*mockDialogView.get(), addAlgorithm(alg)).Times(Exactly(1));

    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("NumberOfProgressReports", 10);
    alg->setRethrows(true);
    alg->execute();
    QCoreApplication::processEvents();
    TS_ASSERT_EQUALS(size_t{0},
                     mockDialogView->m_presenter->getNumberTrackedAlgorithms());

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDialogView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mainProgressBar));
  }
  /** This tests running algorithms from inside an algorithm
   * and that they are all properly tracked for their lifetime
   * in the dialog
   */
  void testAlgorithmThatRunsOtherAlgorithmsIsTrackedCorrectly() {
    auto mainProgressBar = mockDialogView->mainProgressBar;

    // This is the empty QString that will be
    // passed to update progress from the call
    QString emptyQString;

    //----------
    // These assertions are actually for the main progress bar
    // but the algorithm will trigger them too
    // adding the expected calls prevents a bunch of GMock warnings
    EXPECT_CALL(*mainProgressBar.get(), algorithmStarted()).Times(1);
    for (const auto prog : {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}) {
      EXPECT_CALL(*mainProgressBar.get(),
                  updateProgress(DoubleEq(prog), emptyQString));
    }
    EXPECT_CALL(*mainProgressBar.get(), algorithmEnded()).Times(1);
    // End of assertions for the main progress bar
    //----------

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
    auto widgetPairs =
        std::vector<std::pair<QTreeWidgetItem *, QProgressBar *>>();
    widgetPairs.reserve(numWidgets);

    auto &expectedCallObject =
        EXPECT_CALL(*mockDialogView.get(), addAlgorithm(_));
    // The loop is done numWidgets times, to account for the algorithm
    // that is initialised in this test, as this is the 11th call
    for (int i = 0; i < numWidgets; ++i) {
      widgetPairs.emplace_back(
          std::make_pair(new QTreeWidgetItem(), new QProgressBar()));
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

    auto alg = AlgorithmManager::Instance().create("ManualProgressReporter");
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("NumberOfProgressReports", numReports);
    // this will start another alg as much times as there are reports
    alg->setProperty("StartAnotherAlgorithm", true);
    alg->execute();
    QCoreApplication::processEvents();
    TS_ASSERT_EQUALS(size_t{0},
                     mockDialogView->m_presenter->getNumberTrackedAlgorithms());

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDialogView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mainProgressBar));

    // free the pointers for the widgets
    for (const auto &pair : widgetPairs) {
      // the first widget is deleted whenever the algorithm ends
      // manually delete the second one, otherwise it leaks memory
      delete pair.second;
    }
  }

private:
  std::unique_ptr<NiceMock<MockAlgorithmProgressDialogWidget>> mockDialogView;
};
#endif // MANTIDQT_MANTIDWIDGETS_ALGORITHMPROGRESSTEST_H_
