#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/make_shared.hpp>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"

#include "ReflMainViewMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflMainViewPresenterTest : public CxxTest::TestSuite
{

private:

  ITableWorkspace_sptr createWorkspace(const std::string& wsName)
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    auto colRuns = ws->addColumn("str","Run(s)");
    auto colTheta = ws->addColumn("str","ThetaIn");
    auto colTrans = ws->addColumn("str","TransRun(s)");
    auto colQmin = ws->addColumn("str","Qmin");
    auto colQmax = ws->addColumn("str","Qmax");
    auto colDqq = ws->addColumn("str","dq/q");
    auto colScale = ws->addColumn("double","Scale");
    auto colStitch = ws->addColumn("int","StitchGroup");
    auto colOptions = ws->addColumn("str","Options");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);
    colStitch->setPlotType(0);
    colOptions->setPlotType(0);

    if(wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  ITableWorkspace_sptr createPrefilledWorkspace(const std::string& wsName)
  {
    auto ws = createWorkspace(wsName);

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << 1.0 << 3 << "";
    row = ws->appendRow();
    row << "13462" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << 1.0 << 3 << "";
    row = ws->appendRow();
    row << "13469" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << 1.0 << 1 << "";
    row = ws->appendRow();
    row << "13470" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << 1.0 << 1 << "";
    return ws;
  }

  Workspace_sptr loadWorkspace(const std::string& filename, const std::string& wsName)
  {
    IAlgorithm_sptr algLoad = AlgorithmManager::Instance().create("Load");
    algLoad->initialize();
    algLoad->setProperty("Filename", filename);
    algLoad->setProperty("OutputWorkspace", wsName);
    algLoad->execute();
    return algLoad->getProperty("OutputWorkspace");
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflMainViewPresenterTest *createSuite() { return new ReflMainViewPresenterTest(); }
  static void destroySuite( ReflMainViewPresenterTest *suite ) { delete suite; }
  
  ReflMainViewPresenterTest()
  {
    FrameworkManager::Instance();
  }

  void testSave()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    presenter.notify(SaveFlag);
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testSaveAs()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "save as" but cancels when choosing a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return(""));
    presenter.notify(SaveAsFlag);

    //The user hits "save as" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(SaveAsFlag);

    //Check that the workspace was saved
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testSaveProcess()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "save as" but cancels when choosing a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return(""));
    presenter.notify(SaveAsFlag);

    //The user hits "save as" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(SaveAsFlag);

    //The user hits "save" and is not asked to enter a workspace name
    EXPECT_CALL(mockView, askUserString(_,_,_)).Times(0);
    presenter.notify(SaveFlag);

    //Check that the workspace was saved
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAppendRow()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_THROWS(ws->Int(4, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(5, GroupCol), std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(6, GroupCol), std::runtime_error);

    //The user hits "append row" twice with no rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(std::set<size_t>()));
    presenter.notify(AppendRowFlag);
    presenter.notify(AppendRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check that the table has been modified correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 2);
    TS_ASSERT_THROWS(ws->Int(6, GroupCol), std::runtime_error);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAppendRowSpecify()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

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

    //The user hits "append row" twice, with the second row selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(rowlist));
    presenter.notify(AppendRowFlag);
    presenter.notify(AppendRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check that the table has been modified correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "13470");
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);
    TS_ASSERT_THROWS(ws->Int(6, GroupCol), std::runtime_error);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAppendRowSpecifyPlural()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(1);
    rowlist.insert(2);
    rowlist.insert(3);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

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

    //The user hits "append row" once, with the second, third, and fourth row selected.
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(AppendRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check that the table was modified correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "13470");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 0);
    TS_ASSERT_THROWS(ws->Int(5, GroupCol), std::runtime_error);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testPrependRow()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "prepend row" twice with no rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(std::set<size_t>()));
    presenter.notify(PrependRowFlag);
    presenter.notify(PrependRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check that the table has been modified correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testPrependRowSpecify()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "prepend row" twice, with the second row selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(rowlist));
    presenter.notify(PrependRowFlag);
    presenter.notify(PrependRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check that the table has been modified correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testPrependRowSpecifyPlural()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(1);
    rowlist.insert(2);
    rowlist.insert(3);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "prepend row" once, with the second, third, and fourth row selected.
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(PrependRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

    //Check that the table was modified correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowNone()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);

    //The user hits "delete row" with no rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(std::set<size_t>()));
    presenter.notify(DeleteRowFlag);

    //The user hits save
    presenter.notify(SaveFlag);

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
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);

    //The user hits "delete row" with the second row selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(DeleteRowFlag);

    //The user hits "save"
    presenter.notify(SaveFlag);

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
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);
    rowlist.insert(2);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //Check the initial state of the table
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "13460");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 3);

    //The user hits "delete row" with the first three rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(DeleteRowFlag);

    //The user hits save
    presenter.notify(SaveFlag);

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
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits the "process" button with the first two rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockView, getProcessInstrument()).WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockView, setProgressRange(_,_));
    EXPECT_CALL(mockView, setProgress(_)).Times(4);
    presenter.notify(ProcessFlag);

    //Check output workspaces were created as expected
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13462"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13462"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13462"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460_13462"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_13463_13464"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_13460");
    AnalysisDataService::Instance().remove("IvsLam_13460");
    AnalysisDataService::Instance().remove("TOF_13460");
    AnalysisDataService::Instance().remove("IvsQ_13462");
    AnalysisDataService::Instance().remove("IvsLam_13462");
    AnalysisDataService::Instance().remove("TOF_13462");
    AnalysisDataService::Instance().remove("IvsQ_13460_13462");
    AnalysisDataService::Instance().remove("TRANS_13463_13464");
  }

  /*
   * Test processing workspaces with non-standard names, with
   * and without run_number information in the sample log.
   */
  void testProcessCustomNames()
  {
    auto ws = createWorkspace("TestWorkspace");
    TableRow row = ws->appendRow();
    row << "dataA" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << 1.0 << 1;
    row = ws->appendRow();
    row << "dataB" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << 1.0 << 1;

    loadWorkspace("INTER13460", "dataA");
    loadWorkspace("INTER13462", "dataB");

    //Remove the `run_number` entry from dataA's log so its run number cannot be determined that way
    IAlgorithm_sptr algDelLog = AlgorithmManager::Instance().create("DeleteLog");
    algDelLog->initialize();
    algDelLog->setProperty("Workspace", "dataA");
    algDelLog->setProperty("Name", "run_number");
    algDelLog->execute();

    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits the "process" button with the first two rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockView, getProcessInstrument()).WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockView, setProgressRange(_,_));
    EXPECT_CALL(mockView, setProgress(_)).Times(4);
    presenter.notify(ProcessFlag);

    //Check output workspaces were created as expected
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_dataA"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13462"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_dataA_13462"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_dataA"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13462"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("dataA");
    AnalysisDataService::Instance().remove("dataB");
    AnalysisDataService::Instance().remove("IvsQ_dataA");
    AnalysisDataService::Instance().remove("IvsLam_dataA");
    AnalysisDataService::Instance().remove("IvsQ_13462");
    AnalysisDataService::Instance().remove("IvsLam_13462");
    AnalysisDataService::Instance().remove("IvsQ_dataA_13462");
    AnalysisDataService::Instance().remove("TRANS_13463_13464");
  }

  /*
   * Test autofilling workspace values.
   */
  void testAutofill()
  {
    auto ws = createWorkspace("TestWorkspace");
    //Autofill everything we can
    TableRow row = ws->appendRow();
    row << "13460" << "" << "13463,13464" << "" << "" << "" << 1.0 << 1;
    row = ws->appendRow();
    row << "13462" << "" << "13463,13464" << "" << "" << "" << 1.0 << 1;

    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    std::set<size_t> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits the "process" button with the first two rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockView, getProcessInstrument()).WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockView, setProgressRange(_,_));
    EXPECT_CALL(mockView, setProgress(_)).Times(4);
    presenter.notify(ProcessFlag);

    //The user hits the "save" button
    presenter.notify(SaveFlag);

    //Check the table was updated as expected
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->String(0, ThetaCol), "0.70002");
    TS_ASSERT_EQUALS(ws->String(0,   DQQCol), "0.0340292");
    TS_ASSERT_EQUALS(ws->String(0,  QMinCol), "0.00903104");
    TS_ASSERT_EQUALS(ws->String(0,  QMaxCol), "0.153528");

    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "2.3");
    TS_ASSERT_EQUALS(ws->String(1,   DQQCol), "0.0340505");
    TS_ASSERT_EQUALS(ws->String(1,  QMinCol), "0.0296654");
    TS_ASSERT_EQUALS(ws->String(1,  QMaxCol), "0.504311");

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TRANS_13463_13464");
    AnalysisDataService::Instance().remove("TOF_13460");
    AnalysisDataService::Instance().remove("TOF_13463");
    AnalysisDataService::Instance().remove("TOF_13464");
    AnalysisDataService::Instance().remove("IvsQ_13460");
    AnalysisDataService::Instance().remove("IvsLam_13460");
  }

  void testBadWorkspaceType()
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    //Wrong types
    ws->addColumn("str","Run(s)");
    ws->addColumn("str","ThetaIn");
    ws->addColumn("str","TransRun(s)");
    ws->addColumn("str","Qmin");
    ws->addColumn("str","Qmax");
    ws->addColumn("str","dq/q");
    ws->addColumn("str","Scale");
    ws->addColumn("str","StitchGroup");
    ws->addColumn("str","Options");

    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    //We should receive an error
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(1);

    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testBadWorkspaceLength()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    //Because we to open twice, get an error twice
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(2);
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(2).WillRepeatedly(Return("TestWorkspace"));

    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str","Run(s)");
    ws->addColumn("str","ThetaIn");
    ws->addColumn("str","TransRun(s)");
    ws->addColumn("str","Qmin");
    ws->addColumn("str","Qmax");
    ws->addColumn("str","dq/q");
    ws->addColumn("double","Scale");
    ws->addColumn("int","StitchGroup");
    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    //Try to open with too few columns
    presenter.notify(OpenTableFlag);

    ws->addColumn("str","OptionsA");
    ws->addColumn("str","OptionsB");
    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    //Try to open with too many columns
    presenter.notify(OpenTableFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testParseKeyValueString()
  {
    std::map<std::string,std::string> kvp = ReflMainViewPresenter::parseKeyValueString("a = 1,b=2.0, c=3, d='1,2,3',e=\"4,5,6\",f=1+1=2, g = '\\''");

    TS_ASSERT_EQUALS(kvp["a"], "1");
    TS_ASSERT_EQUALS(kvp["b"], "2.0");
    TS_ASSERT_EQUALS(kvp["c"], "3");
    TS_ASSERT_EQUALS(kvp["d"], "1,2,3");
    TS_ASSERT_EQUALS(kvp["e"], "4,5,6");
    TS_ASSERT_EQUALS(kvp["f"], "1+1=2");
    TS_ASSERT_EQUALS(kvp["g"], "'");

    TS_ASSERT_THROWS(ReflMainViewPresenter::parseKeyValueString("a = 1, b = 2, c = 3,"), std::runtime_error);
    TS_ASSERT_THROWS(ReflMainViewPresenter::parseKeyValueString("a = 1, b = 2, c = 3,d"), std::runtime_error);
    TS_ASSERT_THROWS(ReflMainViewPresenter::parseKeyValueString(",a = 1"), std::runtime_error);
    TS_ASSERT_THROWS(ReflMainViewPresenter::parseKeyValueString(",a = 1 = 2,="), std::runtime_error);
    TS_ASSERT_THROWS(ReflMainViewPresenter::parseKeyValueString("=,=,="), std::runtime_error);
  }

  void testPromptSaveAfterAppendRow()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    //User hits "append row"
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(std::set<size_t>()));
    presenter.notify(AppendRowFlag);

    //The user will decide not to discard their changes
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(false));

    //Then hits "new table" without having saved
    presenter.notify(NewTableFlag);

    //The user saves
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(SaveFlag);

    //The user tries to create a new table again, and does not get bothered
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(0);
    presenter.notify(NewTableFlag);
  }

  void testPromptSaveAfterDeleteRow()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    //User hits "append row" a couple of times
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(std::set<size_t>()));
    presenter.notify(AppendRowFlag);
    presenter.notify(AppendRowFlag);

    //The user saves
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(SaveFlag);

    //...then deletes the 2nd row
    std::set<size_t> rows;
    rows.insert(1);
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rows));
    presenter.notify(DeleteRowFlag);

    //The user will decide not to discard their changes when asked
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(false));

    //Then hits "new table" without having saved
    presenter.notify(NewTableFlag);

    //The user saves
    presenter.notify(SaveFlag);

    //The user tries to create a new table again, and does not get bothered
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(0);
    presenter.notify(NewTableFlag);
  }

  void testPromptSaveAndDiscard()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    //User hits "append row" a couple of times
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(std::set<size_t>()));
    presenter.notify(AppendRowFlag);
    presenter.notify(AppendRowFlag);

    //Then hits "new table", and decides to discard
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(true));
    presenter.notify(NewTableFlag);

    //These next two times they don't get prompted - they have a new table
    presenter.notify(NewTableFlag);
    presenter.notify(NewTableFlag);
  }

  void testPromptSaveOnOpen()
  {
    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");

    //User hits "append row"
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(std::set<size_t>()));
    presenter.notify(AppendRowFlag);

    //and tries to open a workspace, but gets prompted and decides not to discard
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(false));
    presenter.notify(OpenTableFlag);

    //the user does it again, but discards
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    //the user does it one more time, and is not prompted
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(0);
    presenter.notify(OpenTableFlag);
  }

  void testExpandSelection()
  {
    auto ws = createWorkspace("TestWorkspace");
    TableRow row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 0 << ""; //Row 0
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 1 << ""; //Row 1
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 1 << ""; //Row 2
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 2 << ""; //Row 3
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 2 << ""; //Row 4
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 2 << ""; //Row 5
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 3 << ""; //Row 6
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 4 << ""; //Row 7
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 4 << ""; //Row 8
    row = ws->appendRow();
    row << "" << "" << "" << "" << "" << "" << 1.0 << 5 << ""; //Row 9

    MockView mockView;
    EXPECT_CALL(mockView, setInstrumentList(_,_)).Times(1);
    EXPECT_CALL(mockView, setTableList(_)).Times(AnyNumber());
    ReflMainViewPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    std::set<size_t> selection;
    std::set<size_t> expected;

    selection.insert(0);
    expected.insert(0);

    //With row 0 selected, we shouldn't expand at all
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(selection));
    EXPECT_CALL(mockView, setSelection(ContainerEq(expected))).Times(1);
    presenter.notify(ExpandSelectionFlag);

    //With 0,1 selected, we should finish with 0,1,2 selected
    selection.clear();
    selection.insert(0);
    selection.insert(1);

    expected.clear();
    expected.insert(0);
    expected.insert(1);
    expected.insert(2);

    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(selection));
    EXPECT_CALL(mockView, setSelection(ContainerEq(expected))).Times(1);
    presenter.notify(ExpandSelectionFlag);

    //With 1,6 selected, we should finish with 1,2,6 selected
    selection.clear();
    selection.insert(1);
    selection.insert(6);

    expected.clear();
    expected.insert(1);
    expected.insert(2);
    expected.insert(6);

    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(selection));
    EXPECT_CALL(mockView, setSelection(ContainerEq(expected))).Times(1);
    presenter.notify(ExpandSelectionFlag);

    //With 4,8 selected, we should finish with 3,4,5,7,8 selected
    selection.clear();
    selection.insert(4);
    selection.insert(8);

    expected.clear();
    expected.insert(3);
    expected.insert(4);
    expected.insert(5);
    expected.insert(7);
    expected.insert(8);

    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(selection));
    EXPECT_CALL(mockView, setSelection(ContainerEq(expected))).Times(1);
    presenter.notify(ExpandSelectionFlag);

    //With nothing selected, we should finish with nothing selected
    selection.clear();
    expected.clear();

    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(selection));
    EXPECT_CALL(mockView, setSelection(ContainerEq(expected))).Times(1);
    presenter.notify(ExpandSelectionFlag);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H */
