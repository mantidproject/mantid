#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainViewPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableViewPresenter.h"

#include "ProgressableViewMockObject.h"
#include "ReflMainViewMockObjects.h"
#include "ReflTableViewMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflMainViewPresenterTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflMainViewPresenterTest *createSuite() {
    return new ReflMainViewPresenterTest();
  }
  static void destroySuite(ReflMainViewPresenterTest *suite) { delete suite; }

  ReflMainViewPresenterTest() { FrameworkManager::Instance(); }

  void test_constructor_sets_possible_transfer_methods() {
    NiceMock<MockView> mockView;
    NiceMock<MockTableView> mockTableView;
    MockProgressableView mockProgress;
    ReflTableViewPresenter tablePresenter(&mockTableView, &mockProgress);

    // Expect that the transfer methods get initialized on the view
    EXPECT_CALL(mockView, setTransferMethods(_)).Times(Exactly(1));
    // Expect that the view is populated with the instrument list
    EXPECT_CALL(mockTableView, setInstrumentList(_, _)).Times(Exactly(1));
    // Expect that the view clears the list of commands
    EXPECT_CALL(mockView, clearCommands()).Times(Exactly(1));
    // Expect that the view is populated with the list of table commands
    EXPECT_CALL(mockView, setTableCommandsProxy()).Times(Exactly(1));
    // Expect that the view is populated with the list of row commands
    EXPECT_CALL(mockView, setRowCommandsProxy()).Times(Exactly(1));

    // Constructor
    ReflMainViewPresenter presenter(&mockView, &tablePresenter, &mockProgress);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTableView));
  }

  void test_presenter_sets_commands_when_notified() {
    NiceMock<MockView> mockView;
    NiceMock<MockTableView> mockTableView;
    MockProgressableView mockProgress;
    ReflTableViewPresenter tablePresenter(&mockTableView, &mockProgress);

    ReflMainViewPresenter presenter(&mockView, &tablePresenter, &mockProgress);

    // Expect that the view clears the list of commands
    EXPECT_CALL(mockView, clearCommands()).Times(Exactly(1));
    // Expect that the view is populated with the list of table commands
    EXPECT_CALL(mockView, setTableCommandsProxy()).Times(Exactly(1));
    // Expect that the view is populated with the list of row commands
    EXPECT_CALL(mockView, setRowCommandsProxy()).Times(Exactly(1));
    // The presenter is notified that something changed in the ADS
    presenter.notify(WorkspaceReceiver::ADSChangedFlag);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};
#endif /* MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H */
