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
    MOCK_CONST_METHOD0(getRequestEdit, bool());
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
  
   void testInitalization()
   {
     MockLogView view;
     EXPECT_CALL(view, initalize(_)).Times(1);
     //Test that it initalizes the view on accept
     
     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptView(&view);

     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

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
     EXPECT_CALL(view, getRequestEdit()).Times(1);
     
     WorkspaceMemento* wsMemento = makeMemento();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptView(&view);
     presenter.update(); // Updated should call getLogData on view

     //Check that the edited log values persist
     wsMemento->commit();
     WorkspaceMementoService<LoanedMemento> service(loanedMemento);

     std::string temp;

     service.getLogData()[0]->getValue(temp);
     TS_ASSERT_EQUALS("A", temp);

     service.getLogData()[1]->getValue(temp);
     TS_ASSERT_EQUALS("B", temp);

     service.getLogData()[2]->getValue(temp);
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
     EXPECT_CALL(view, getRequestEdit()).Times(1);
     
     WorkspaceMemento* wsMemento = makeMemento();
     int originalColCount = wsMemento->getData()->columnCount();
     LoanedMemento loanedMemento(wsMemento);

     LogPresenter presenter(loanedMemento);
     presenter.acceptView(&view);
     presenter.update(); // Updated should call getLogData on view

     //Check that the edited log values persist
     wsMemento->commit();
     WorkspaceMementoService<LoanedMemento> service(loanedMemento);

     std::string temp; //Temp variable for templated member functions

     service.getLogData()[0]->getValue(temp);
     TS_ASSERT_EQUALS("A", temp);

     service.getLogData()[1]->getValue(temp);
     TS_ASSERT_EQUALS("B", temp);

     service.getLogData()[2]->getValue(temp);
     TS_ASSERT_EQUALS("C", temp);

     service.getLogData()[3]->getValue(temp);
     TS_ASSERT_EQUALS("D", temp);

     TS_ASSERT_EQUALS(originalColCount + 1, wsMemento->getData()->columnCount());

     //Check that mock object has been used as expected
     TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
   }

};

#endif