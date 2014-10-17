#ifndef MANTID_CUSTOMINTERFACES_REFLLOADEDMAINVIEWPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLLOADEDMAINVIEWPRESENTERTEST_H_

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

  ITableWorkspace_sptr createPrefilledWorkspace(const std::string& wsName = "")
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

  ITableWorkspace_sptr createBadTypedWorkspace()
  {
    ITableWorkspace_sptr ws = createWorkspace();

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << 2.0 << 1 << "";

    return ws;
  }

  ITableWorkspace_sptr createBadLengthWorkspace(bool longer)
  {
    ITableWorkspace_sptr ws = createWorkspace();

    if(longer)
      ws->addColumn("str","extracolumn");
    else
      ws->removeColumn("Options");

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
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 2);
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
    ReflLoadedMainViewPresenter presenter(ws,&mockView);
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
    ReflLoadedMainViewPresenter presenter(ws,&mockView);
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

    //The user hits the "save" button
    presenter.notify(SaveFlag);

    //Check the calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check the table was updated as expected
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->String(0, ThetaCol), "0.7");
    TS_ASSERT_EQUALS(ws->String(0,   DQQCol), "0.0340301");
    TS_ASSERT_EQUALS(ws->String(0,  QMinCol), "0.009");
    TS_ASSERT_EQUALS(ws->String(0,  QMaxCol), "0.154");

    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "2.3");
    TS_ASSERT_EQUALS(ws->String(1,   DQQCol), "0.0340505");
    TS_ASSERT_EQUALS(ws->String(1,  QMinCol), "0.03");
    TS_ASSERT_EQUALS(ws->String(1,  QMaxCol), "0.504");

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TRANS_13463_13464");
    AnalysisDataService::Instance().remove("TOF_13460");
    AnalysisDataService::Instance().remove("TOF_13463");
    AnalysisDataService::Instance().remove("TOF_13464");
    AnalysisDataService::Instance().remove("IvsQ_13460");
    AnalysisDataService::Instance().remove("IvsLam_13460");
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
