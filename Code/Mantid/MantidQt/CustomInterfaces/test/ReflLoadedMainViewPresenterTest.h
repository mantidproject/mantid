#ifndef MANTID_CUSTOMINTERFACES_REFLLOADEDMAINVIEWPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLLOADEDMAINVIEWPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/make_shared.hpp>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtCustomInterfaces/ReflLoadedMainViewPresenter.h"

#include "ReflMainViewMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflLoadedMainViewPresenterTest : public CxxTest::TestSuite
{

private:

  ITableWorkspace_sptr createWorkspace(const std::string& wsName = "")
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    auto colRuns = ws->addColumn("str","Run(s)");
    auto colTheta = ws->addColumn("str","ThetaIn");
    auto colTrans = ws->addColumn("str","TransRun(s)");
    auto colQmin = ws->addColumn("str","Qmin");
    auto colQmax = ws->addColumn("str","Qmax");
    auto colDqq = ws->addColumn("str","dq/q");
    auto colScale = ws->addColumn("str","Scale");
    auto colStitch = ws->addColumn("int","StitchGroup");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);
    colStitch->setPlotType(0);

    if(wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  ITableWorkspace_sptr createPrefilledWorkspace(const std::string& wsName = "")
  {
    auto ws = createWorkspace(wsName);

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 3;
    row = ws->appendRow();
    row << "13462" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 3;
    row = ws->appendRow();
    row << "13469" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 1;
    row = ws->appendRow();
    row << "13470" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 1;
    return ws;
  }

  ITableWorkspace_sptr createBadTypedWorkspace()
  {
    ITableWorkspace_sptr ws = createWorkspace();

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << "2" << "1";

    return ws;
  }

  ITableWorkspace_sptr createBadLengthWorkspace(bool longer)
  {
    ITableWorkspace_sptr ws = createWorkspace();

    if(longer)
      ws->addColumn("str","extracolumn");
    else
      ws->removeColumn("StitchGroup");

    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflLoadedMainViewPresenterTest *createSuite() { return new ReflLoadedMainViewPresenterTest(); }
  static void destroySuite( ReflLoadedMainViewPresenterTest *suite ) { delete suite; }
  
  ReflLoadedMainViewPresenterTest()
  {
    FrameworkManager::Instance();
  }

  void testSave()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);
    presenter.notify(SaveFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testSaveAs()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //The user hits "save as" but cancels when choosing a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return(""));
    presenter.notify(SaveAsFlag);

    //The user hits "save as" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(SaveAsFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testSaveProcess()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //The user hits "save as" but cancels when choosing a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return(""));
    presenter.notify(SaveAsFlag);

    //The user hits "save as" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(SaveAsFlag);

    //The user hits "save" and is not asked to enter a workspace name
    EXPECT_CALL(mockView, askUserString(_,_,_)).Times(0);
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAddRow()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_THROWS(ws->Int(4, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(5, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(6, GroupCol), std::runtime_error);

    //The user hits "add row" twice with no rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(2).WillRepeatedly(Return(std::vector<size_t>()));
    presenter.notify(AddRowFlag);
    presenter.notify(AddRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check the calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the table has been modified correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 0);
    TS_ASSERT_THROWS(ws->Int(6, GroupCol), std::runtime_error);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAddRowSpecify()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(1);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 1);
    TS_ASSERT_THROWS(ws->Int(4, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(5, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(6, GroupCol), std::runtime_error);

    //The user hits "add row" twice, with the second row selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(2).WillRepeatedly(Return(rowlist));
    presenter.notify(AddRowFlag);
    presenter.notify(AddRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check the calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the table has been modified correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "13470");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);
    TS_ASSERT_THROWS(ws->Int(6, GroupCol), std::runtime_error);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAddRowSpecifyPlural()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    rowlist.push_back(2);
    rowlist.push_back(3);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 1);
    TS_ASSERT_THROWS(ws->Int(4, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(5, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(6, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(7, GroupCol), std::runtime_error);

    //The user hits "add row" once, with the second, third, and fourth row selected.
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(AddRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check the calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the table was modified correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 7);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->String(6, RunCol), "13470");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(6, GroupCol), 1);
    TS_ASSERT_THROWS(ws->Int(7, GroupCol), std::runtime_error);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowNone()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);

    //The user hits "delete row" with no rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(std::vector<size_t>()));
    presenter.notify(DeleteRowFlag);

    //The user hits save
    presenter.notify(SaveFlag);

    //Check the calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the table was not modified
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowSingle()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(1);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);

    //The user hits "delete row" with the second row selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(DeleteRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check the calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 1);
    TS_ASSERT_THROWS(ws->Int(3, GroupCol), std::runtime_error);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowPlural()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(0);
    rowlist.push_back(1);
    rowlist.push_back(2);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "13460");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 3);

    //The user hits "delete row" with the first three rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(DeleteRowFlag);

    //The user hits save
    presenter.notify(SaveFlag);

    //Check the calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check the rows were deleted as expected
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "13470");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 1);
    TS_ASSERT_THROWS(ws->Int(1, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(2, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(3, GroupCol), std::runtime_error);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testProcess()
  {
    MockView mockView;
    ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace("TestWorkspace"),&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(0);
    rowlist.push_back(1);

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //The user hits the "process" button with the first two rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockView, getProcessInstrument()).WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockView, setProgressRange(_,_));
    EXPECT_CALL(mockView, setProgress(_)).Times(4);
    presenter.notify(ProcessFlag);

    //Check the calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check output workspaces were created as expected
    TS_ASSERT(AnalysisDataService::Instance().doesExist("13460_IvsQ"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("13460_IvsLam"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("13460_TOF"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("13462_IvsQ"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("13462_IvsLam"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("13462_TOF"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("13460_13462_IvsQ"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_13463_13464"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("13460_IvsQ");
    AnalysisDataService::Instance().remove("13460_IvsLam");
    AnalysisDataService::Instance().remove("13460_TOF");
    AnalysisDataService::Instance().remove("13462_IvsQ");
    AnalysisDataService::Instance().remove("13462_IvsLam");
    AnalysisDataService::Instance().remove("13462_TOF");
    AnalysisDataService::Instance().remove("13460_13462_IvsQ");
    AnalysisDataService::Instance().remove("TRANS_13463_13464");
  }

  void testBadWorkspaceName()
  {
    MockView mockView;
    TS_ASSERT_THROWS(ReflLoadedMainViewPresenter presenter(createPrefilledWorkspace(),&mockView), std::runtime_error&);
  }

  void testBadWorkspaceType()
  {
    MockView mockView;
    TS_ASSERT_THROWS(ReflLoadedMainViewPresenter presenter(createBadTypedWorkspace(),&mockView), std::runtime_error&);
  }

  void testBadWorkspaceShort()
  {
    MockView mockView;
    TS_ASSERT_THROWS(ReflLoadedMainViewPresenter presenter(createBadLengthWorkspace(false),&mockView), std::runtime_error&);
  }

  void testBadWorkspaceLong()
  {
    MockView mockView;
    TS_ASSERT_THROWS(ReflLoadedMainViewPresenter presenter(createBadLengthWorkspace(true),&mockView), std::runtime_error&);
  }

};


#endif /* MANTID_CUSTOMINTERFACES_REFLLOADEDMAINVIEWPRESENTERTEST_H_ */
