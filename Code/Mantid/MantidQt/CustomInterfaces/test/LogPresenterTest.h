#ifndef CUSTOM_INTERFACES_LOG_PRESENTER_TEST_H_
#define CUSTOM_INTERFACES_LOG_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtCustomInterfaces/LogView.h"
#include "MantidQtCustomInterfaces/LogPresenter.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"
#include "MantidDataObjects/MementoTableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <gmock/gmock.h>
#include <map>
#include "MantidQtCustomInterfaces/Approach.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class LogPresenterTest : public CxxTest::TestSuite
{

private:

  //Mock view.
  class MockLogView : public LogView 
  {
  public:
    MOCK_METHOD1(initalize,
      void(std::vector<AbstractMementoItem_sptr>));
    MOCK_CONST_METHOD0(getLogData,
      LogDataMap());
    MOCK_METHOD0(indicateModified, void());
    MOCK_METHOD0(indicateDefault, void());
    MOCK_METHOD0(show, void());
    MOCK_METHOD0(hide, void());
    MOCK_CONST_METHOD0(fetchStatus, LogViewStatus());
  };
  // Helper method to generate a workspace memento;
  static WorkspaceMemento* makeMemento()
  {
    using namespace Mantid::DataObjects;
    using namespace Mantid::API;

    TableWorkspace_sptr ws(new MementoTableWorkspace(1));
    TableRow row = ws->getRow(0);
    row << "TestWSRow" << "CNCS" << 1 << "SampleXML" << 1.0 << 1.0 << 1.0 << 90.0 << 90.0 << 90.0 << "Not Ready";
    int rowIndex = 0;

    WorkspaceMemento* memento = new WorkspaceMemento(ws, "TestWSRow", rowIndex);
    LoanedMemento managed(memento);
    WorkspaceMementoService<LoanedMemento> service(managed);
    service.addAllItems(ws, rowIndex);
   
    //Add some log values.
    std::vector<std::string> logNames;
    logNames.push_back("LogValueA");
    logNames.push_back("LogValueB");
    logNames.push_back("LogValueC");
    service.declareLogItems(ws, logNames, rowIndex);

    return memento;
  }

public:

//=====================================================================================
// Functional tests
//=====================================================================================
  
   void testInitalizationReadOnlyView()
   {
     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(1);
     EXPECT_CALL(view, show()).Times(1);

     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptReadOnlyView(&view);

     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

   //Should not be able to update the presenter without providing both views first.
   void testInitalizationEditableView()
   {
     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(AnyNumber());
     EXPECT_CALL(view, hide()).Times(AnyNumber());

     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptEditableView(&view);

     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

   //Should not be able to update the presenter without providing both views first.
   void testThrowsWithoutReadOnlyView()
   {
     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(AnyNumber());
     EXPECT_CALL(view, hide()).Times(AnyNumber());

     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptEditableView(&view);
     //presenter.acceptReadOnlyView(&view);

     TS_ASSERT_THROWS(presenter.update(), std::runtime_error);
     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

   //Should not be able to update the presenter without providing both views first.
   void testThrowsWithoutEditableView()
   {
     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(AnyNumber());
     EXPECT_CALL(view, show()).Times(AnyNumber());

     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     //presenter.acceptEditableView(&view);
     presenter.acceptReadOnlyView(&view);

     TS_ASSERT_THROWS(presenter.update(), std::runtime_error);
     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

   void testThrowsWithoutBothViews()
   {
     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(AnyNumber());

     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     //presenter.acceptEditableView(&view);
     //presenter.acceptReadOnlyView(&view);

     TS_ASSERT_THROWS(presenter.update(), std::runtime_error);
     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

   void testCancelledAfterEdit()
   {
      //Created some edited log values
     std::map<std::string, std::string> logs;
     logs.insert(std::make_pair("LogValueA", "A"));
     logs.insert(std::make_pair("LogValueB", "B"));
     logs.insert(std::make_pair("LogValueC", "C"));

     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(AtLeast(1));
     EXPECT_CALL(view, fetchStatus()).WillOnce(Return(LogViewStatus::cancelling)); //No change to status, but log value data is now different.
     EXPECT_CALL(view, show()).Times(AnyNumber());
     EXPECT_CALL(view, hide()).Times(AnyNumber());

     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptReadOnlyView(&view);
     presenter.acceptEditableView(&view);
     presenter.update(); // Updated should call getLogData on view

     ///Service acts as helper in picking out log values.
     WorkspaceMementoService<LoanedMemento> service(loanedMemento);

     std::string temp;

     service.getLogData()[0]->getValue(temp);
     TSM_ASSERT_EQUALS("Should have rolled-back to default", "", temp);

     service.getLogData()[1]->getValue(temp);
     TSM_ASSERT_EQUALS("Should have rolled-back to default", "", temp);

     service.getLogData()[2]->getValue(temp);
     TSM_ASSERT_EQUALS("Should have rolled-back to default", "", temp);

     //Check that mock object has been used as expected
     std::vector<AbstractMementoItem_sptr> persistedLogs = service.getLogData();
     TSM_ASSERT_EQUALS("Should have same number of logs as at start", 3, persistedLogs.size());
     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

   void testCancelledAfterInsertion()
   {
     //Created some edited log values + an additional log value
     std::map<std::string, std::string> logs;
     logs.insert(std::make_pair("LogValueA", "A"));
     logs.insert(std::make_pair("LogValueB", "B"));
     logs.insert(std::make_pair("LogValueC", "C"));
     logs.insert(std::make_pair("LogValueD", "D")); //Additional log value.

     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(AtLeast(1));
     EXPECT_CALL(view, fetchStatus()).WillOnce(Return(LogViewStatus::cancelling));
     EXPECT_CALL(view, show()).Times(AnyNumber());
     EXPECT_CALL(view, hide()).Times(AnyNumber());
     WorkspaceMemento* wsMemento = makeMemento();
     int originalColCount = wsMemento->getData()->columnCount();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptReadOnlyView(&view);
     presenter.acceptEditableView(&view);
     presenter.update(); // Updated should call getLogData on view

     ///Service acts as helper in picking out log values.
     WorkspaceMementoService<LoanedMemento> service(loanedMemento);

     std::vector<AbstractMementoItem_sptr> persistedLogs = service.getLogData();
     TSM_ASSERT_EQUALS("Aborted insertion operation, should have same number of log values as start", 3, persistedLogs.size());
     TSM_ASSERT_EQUALS("Aborted insertion operation, should have same number of columns as at start", originalColCount, wsMemento->getData()->columnCount());

     std::string temp; //Temp variable for templated member functions

     persistedLogs[0]->getValue(temp);
     TS_ASSERT_EQUALS("", temp);

     persistedLogs[1]->getValue(temp);
     TS_ASSERT_EQUALS("", temp);

     persistedLogs[2]->getValue(temp);
     TS_ASSERT_EQUALS("", temp);

     //Check that mock object has been used as expected
     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));

   }

   /// Assume that the view is already an Editable one.
   void testEdited()
   {
     //Created some edited log values
     std::map<std::string, std::string> logs;
     logs.insert(std::make_pair("LogValueA", "A"));
     logs.insert(std::make_pair("LogValueB", "B"));
     logs.insert(std::make_pair("LogValueC", "C"));

     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(AtLeast(1));
     EXPECT_CALL(view, getLogData()).WillOnce(Return(logs));
     EXPECT_CALL(view, fetchStatus()).WillOnce(Return(LogViewStatus::saving)); //No change to status, but log value data is now different.
     EXPECT_CALL(view, show()).Times(AnyNumber());
     EXPECT_CALL(view, hide()).Times(AnyNumber());

     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptReadOnlyView(&view);
     presenter.acceptEditableView(&view);
     presenter.update(); // Updated should call getLogData on view

     /*
     Check that the edited log values persist. We need to call commit here in order to overwrite the table workspace
     with the newely edited values. Note that commit on the workspace would only be called once user was satisfied with the edits made to the
     entire workspace metadata, including logs.
     */
     wsMemento->commit();

     // Instantiate a service to help pickout log values.
     WorkspaceMementoService<LoanedMemento> service(loanedMemento);

     std::string temp;

     std::vector<AbstractMementoItem_sptr> persistedLogs = service.getLogData();
     TSM_ASSERT_EQUALS("Edit operation, should have same number of log values as start", 3, persistedLogs.size());

     persistedLogs[0]->getValue(temp);
     TS_ASSERT_EQUALS("A", temp);

     persistedLogs[1]->getValue(temp);
     TS_ASSERT_EQUALS("B", temp);

     persistedLogs[2]->getValue(temp);
     TS_ASSERT_EQUALS("C", temp);

     //Check that mock object has been used as expected
     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));

   }

   void testCreated()
   {
     //Created some edited log values + an additional log value
     std::map<std::string, std::string> logs;
     logs.insert(std::make_pair("LogValueA", "A"));
     logs.insert(std::make_pair("LogValueB", "B"));
     logs.insert(std::make_pair("LogValueC", "C"));
     logs.insert(std::make_pair("LogValueD", "D")); //Additional log value.

     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(AtLeast(1));
     EXPECT_CALL(view, getLogData()).WillOnce(Return(logs));
     EXPECT_CALL(view, fetchStatus()).WillOnce(Return(LogViewStatus::saving));
     EXPECT_CALL(view, show()).Times(AnyNumber());
     EXPECT_CALL(view, hide()).Times(AnyNumber());
     
     WorkspaceMemento* wsMemento = makeMemento();
     int originalColCount = wsMemento->getData()->columnCount();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptReadOnlyView(&view);
     presenter.acceptEditableView(&view);
     presenter.update(); // Updated should call getLogData on view

     /*
     Check that the edited log values persist. We need to call commit here in order to overwrite the table workspace
     with the newely edited values. Note that commit on the workspace would only be called once user was satisfied with the edits made to the
     entire workspace metadata, including logs.
     */
     wsMemento->commit();

     //Instantiate a service to help pick out log values.
     WorkspaceMementoService<LoanedMemento> service(loanedMemento);

     std::string temp; //Temp variable for templated member functions

     std::vector<AbstractMementoItem_sptr> persistedLogs = service.getLogData();
     TSM_ASSERT_EQUALS("Should have one additional log value in WSMemento", 4, persistedLogs.size());
     TSM_ASSERT_EQUALS("Should have one additional column in Table WS", originalColCount + 1, wsMemento->getData()->columnCount());

     persistedLogs[0]->getValue(temp);
     TS_ASSERT_EQUALS("A", temp);

     persistedLogs[1]->getValue(temp);
     TS_ASSERT_EQUALS("B", temp);

     persistedLogs[2]->getValue(temp);
     TS_ASSERT_EQUALS("C", temp);

     persistedLogs[3]->getValue(temp);
     TS_ASSERT_EQUALS("D", temp);

     //Check that mock object has been used as expected
     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

};

#endif