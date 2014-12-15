#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"

#include "ReflMainViewMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace Mantid::Kernel;
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

  void createTOFWorkspace(const std::string& wsName, const std::string& runNumber = "")
  {
    auto tinyWS = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
    auto inst = tinyWS->getInstrument();

    inst->getParameterMap()->addDouble(inst.get(), "I0MonitorIndex", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "PointDetectorStart", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "PointDetectorStop", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "LambdaMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "LambdaMax", 10.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorBackgroundMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorBackgroundMax", 10.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorIntegralMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorIntegralMax", 10.0);

    tinyWS->mutableRun().addLogData(new PropertyWithValue<double>("Theta", 0.12345));
    if(!runNumber.empty())
      tinyWS->mutableRun().addLogData(new PropertyWithValue<std::string>("run_number", runNumber));

    AnalysisDataService::Instance().addOrReplace(wsName, tinyWS);
  }

  ITableWorkspace_sptr createPrefilledWorkspace(const std::string& wsName)
  {
    auto ws = createWorkspace(wsName);
    TableRow row = ws->appendRow();
    row << "12345" << "0.5" << "" << "0.1" << "1.6" << "0.04" << 1.0 << 0 << "";
    row = ws->appendRow();
    row << "12346" << "1.5" << "" << "1.4" << "2.9" << "0.04" << 1.0 << 0 << "";
    row = ws->appendRow();
    row << "24681" << "0.5" << "" << "0.1" << "1.6" << "0.04" << 1.0 << 1 << "";
    row = ws->appendRow();
    row << "24682" << "1.5" << "" << "1.4" << "2.9" << "0.04" << 1.0 << 1 << "";
    return ws;
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

  void testSaveNew()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    presenter.notify(IReflPresenter::NewTableFlag);

    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::SaveFlag);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("TestWorkspace"));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testSaveExisting()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(0);
    presenter.notify(IReflPresenter::SaveFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testSaveAs()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    //The user hits "save as" but cancels when choosing a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return(""));
    presenter.notify(IReflPresenter::SaveAsFlag);

    //The user hits "save as" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(IReflPresenter::SaveAsFlag);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));

    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAppendRow()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "append row" twice with no rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::AppendRowFlag);
    presenter.notify(IReflPresenter::AppendRowFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    //Check that the table has been modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 3);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAppendRowSpecify()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "append row" twice, with the second row selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::AppendRowFlag);
    presenter.notify(IReflPresenter::AppendRowFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    //Check that the table has been modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAppendRowSpecifyPlural()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);
    rowlist.insert(2);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "append row" once, with the second, third, and fourth row selected.
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::AppendRowFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    //Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testPrependRow()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "prepend row" twice with no rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::PrependRowFlag);
    presenter.notify(IReflPresenter::PrependRowFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    //Check that the table has been modified correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testPrependRowSpecify()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "prepend row" twice, with the second row selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::PrependRowFlag);
    presenter.notify(IReflPresenter::PrependRowFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    //Check that the table has been modified correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testPrependRowSpecifyPlural()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);
    rowlist.insert(2);
    rowlist.insert(3);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "prepend row" once, with the second, third, and fourth row selected.
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::PrependRowFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    //Check that the table was modified correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowNone()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "delete row" with no rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::DeleteRowFlag);

    //The user hits save
    presenter.notify(IReflPresenter::SaveFlag);

    //Check that the table has not lost any rows
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowSingle()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "delete row" with the second row selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::DeleteRowFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowPlural()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);
    rowlist.insert(2);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "delete row" with the first three rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::DeleteRowFlag);

    //The user hits save
    presenter.notify(IReflPresenter::SaveFlag);

    //Check the rows were deleted as expected
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "24682");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testProcess()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits the "process" button with the first two rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::ProcessFlag);

    //Check output workspaces were created as expected
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_12345_12346"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_12345");
    AnalysisDataService::Instance().remove("IvsLam_12345");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_12346");
    AnalysisDataService::Instance().remove("IvsLam_12346");
    AnalysisDataService::Instance().remove("TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_12345_12346");
  }

  /*
   * Test processing workspaces with non-standard names, with
   * and without run_number information in the sample log.
   */
  void testProcessCustomNames()
  {
    auto ws = createWorkspace("TestWorkspace");
    TableRow row = ws->appendRow();
    row << "dataA" << "0.7" << "" << "0.1" << "1.6" << "0.04" << 1.0 << 1;
    row = ws->appendRow();
    row << "dataB" << "2.3" << "" << "1.4" << "2.9" << "0.04" << 1.0 << 1;

    createTOFWorkspace("dataA");
    createTOFWorkspace("dataB", "12346");

    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits the "process" button with the first two rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::ProcessFlag);

    //Check output workspaces were created as expected
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_dataA"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_dataA_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_dataA"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_12346"));

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("dataA");
    AnalysisDataService::Instance().remove("dataB");
    AnalysisDataService::Instance().remove("IvsQ_dataA");
    AnalysisDataService::Instance().remove("IvsLam_dataA");
    AnalysisDataService::Instance().remove("IvsQ_12346");
    AnalysisDataService::Instance().remove("IvsLam_12346");
    AnalysisDataService::Instance().remove("IvsQ_dataA_12346");
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
    ReflMainViewPresenter presenter(&mockView);

    //We should receive an error
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(1);

    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testBadWorkspaceLength()
  {
    MockView mockView;
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
    presenter.notify(IReflPresenter::OpenTableFlag);

    ws->addColumn("str","OptionsA");
    ws->addColumn("str","OptionsB");
    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    //Try to open with too many columns
    presenter.notify(IReflPresenter::OpenTableFlag);

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
    ReflMainViewPresenter presenter(&mockView);

    //User hits "append row"
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::AppendRowFlag);

    //The user will decide not to discard their changes
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(false));

    //Then hits "new table" without having saved
    presenter.notify(IReflPresenter::NewTableFlag);

    //The user saves
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(IReflPresenter::SaveFlag);

    //The user tries to create a new table again, and does not get bothered
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(0);
    presenter.notify(IReflPresenter::NewTableFlag);
  }

  void testPromptSaveAfterDeleteRow()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    //User hits "append row" a couple of times
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::AppendRowFlag);
    presenter.notify(IReflPresenter::AppendRowFlag);

    //The user saves
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillOnce(Return("Workspace"));
    presenter.notify(IReflPresenter::SaveFlag);

    //...then deletes the 2nd row
    std::set<int> rows;
    rows.insert(1);
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rows));
    presenter.notify(IReflPresenter::DeleteRowFlag);

    //The user will decide not to discard their changes when asked
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(false));

    //Then hits "new table" without having saved
    presenter.notify(IReflPresenter::NewTableFlag);

    //The user saves
    presenter.notify(IReflPresenter::SaveFlag);

    //The user tries to create a new table again, and does not get bothered
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(0);
    presenter.notify(IReflPresenter::NewTableFlag);
  }

  void testPromptSaveAndDiscard()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    //User hits "append row" a couple of times
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::AppendRowFlag);
    presenter.notify(IReflPresenter::AppendRowFlag);

    //Then hits "new table", and decides to discard
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(true));
    presenter.notify(IReflPresenter::NewTableFlag);

    //These next two times they don't get prompted - they have a new table
    presenter.notify(IReflPresenter::NewTableFlag);
    presenter.notify(IReflPresenter::NewTableFlag);
  }

  void testPromptSaveOnOpen()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");

    //User hits "append row"
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::AppendRowFlag);

    //and tries to open a workspace, but gets prompted and decides not to discard
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(false));
    presenter.notify(IReflPresenter::OpenTableFlag);

    //the user does it again, but discards
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    //the user does it one more time, and is not prompted
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    EXPECT_CALL(mockView, askUserYesNo(_,_)).Times(0);
    presenter.notify(IReflPresenter::OpenTableFlag);
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
    ReflMainViewPresenter presenter(&mockView);

    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    std::set<int> selection;
    std::set<int> expected;

    selection.insert(0);
    expected.insert(0);

    //With row 0 selected, we shouldn't expand at all
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(selection));
    EXPECT_CALL(mockView, setSelection(ContainerEq(expected))).Times(1);
    presenter.notify(IReflPresenter::ExpandSelectionFlag);

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
    presenter.notify(IReflPresenter::ExpandSelectionFlag);

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
    presenter.notify(IReflPresenter::ExpandSelectionFlag);

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
    presenter.notify(IReflPresenter::ExpandSelectionFlag);

    //With nothing selected, we should finish with nothing selected
    selection.clear();
    expected.clear();

    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(selection));
    EXPECT_CALL(mockView, setSelection(ContainerEq(expected))).Times(1);
    presenter.notify(IReflPresenter::ExpandSelectionFlag);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testClearRows()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);
    rowlist.insert(2);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "clear selected" with the second and third rows selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::ClearSelectedFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    //Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    //Check the group ids have been set correctly
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 1);

    //Make sure the selected rows are clear
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "");
    TS_ASSERT_EQUALS(ws->String(2, ThetaCol), "");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "");
    TS_ASSERT_EQUALS(ws->String(2, TransCol), "");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "");
    TS_ASSERT_EQUALS(ws->String(2, QMinCol), "");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "");
    TS_ASSERT_EQUALS(ws->String(2, QMaxCol), "");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "");
    TS_ASSERT_EQUALS(ws->String(2, DQQCol), "");
    TS_ASSERT_EQUALS(ws->Double(1, ScaleCol), 1.0);
    TS_ASSERT_EQUALS(ws->Double(2, ScaleCol), 1.0);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testCopyRow()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);

    const std::string expected = "12346\t1.5\t\t1.4\t2.9\t0.04\t1\t0\t";

    //The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockView, setClipboard(expected));
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::CopySelectedFlag);
  }

  void testCopyRows()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);
    rowlist.insert(2);
    rowlist.insert(3);

    const std::string expected = "12345\t0.5\t\t0.1\t1.6\t0.04\t1\t0\t\n"
                                 "12346\t1.5\t\t1.4\t2.9\t0.04\t1\t0\t\n"
                                 "24681\t0.5\t\t0.1\t1.6\t0.04\t1\t1\t\n"
                                 "24682\t1.5\t\t1.4\t2.9\t0.04\t1\t1\t";

    //The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockView, setClipboard(expected));
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::CopySelectedFlag);
  }

  void testCutRow()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);

    const std::string expected = "12346\t1.5\t\t1.4\t2.9\t0.04\t1\t0\t";

    //The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockView, setClipboard(expected));
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::CutSelectedFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    //Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24682");
  }

  void testCutRows()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(0);
    rowlist.insert(1);
    rowlist.insert(2);

    const std::string expected = "12345\t0.5\t\t0.1\t1.6\t0.04\t1\t0\t\n"
                                 "12346\t1.5\t\t1.4\t2.9\t0.04\t1\t0\t\n"
                                 "24681\t0.5\t\t0.1\t1.6\t0.04\t1\t1\t";

    //The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockView, setClipboard(expected));
    EXPECT_CALL(mockView, getSelectedRows()).Times(2).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::CutSelectedFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    //Check the only unselected row is left behind
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "24682");
  }

  void testPasteRow()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);

    const std::string clipboard = "123\t0.5\t456\t1.2\t3.4\t3.14\t5\t6\tabc";

    //The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockView, getClipboard()).Times(1).WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::PasteSelectedFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    //Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    //Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->Double(1, ScaleCol), 5.0);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 6);
    TS_ASSERT_EQUALS(ws->String(1, OptionsCol), "abc");
  }

  void testPasteNewRow()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    const std::string clipboard = "123\t0.5\t456\t1.2\t3.4\t3.14\t5\t6\tabc";

    //The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockView, getClipboard()).Times(1).WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::PasteSelectedFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    //Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    //Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(4, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(4, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(4, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(4, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(4, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->Double(4, ScaleCol), 5.0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 6);
    TS_ASSERT_EQUALS(ws->String(4, OptionsCol), "abc");
  }

  void testPasteRows()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(1);
    rowlist.insert(2);

    const std::string clipboard = "123\t0.5\t456\t1.2\t3.4\t3.14\t5\t6\tabc\n"
                                  "345\t2.7\t123\t2.1\t4.3\t2.17\t3\t2\tdef";

    //The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockView, getClipboard()).Times(1).WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::PasteSelectedFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    //Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    //Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->Double(1, ScaleCol), 5.0);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 6);
    TS_ASSERT_EQUALS(ws->String(1, OptionsCol), "abc");

    TS_ASSERT_EQUALS(ws->String(2, RunCol), "345");
    TS_ASSERT_EQUALS(ws->String(2, ThetaCol), "2.7");
    TS_ASSERT_EQUALS(ws->String(2, TransCol), "123");
    TS_ASSERT_EQUALS(ws->String(2, QMinCol), "2.1");
    TS_ASSERT_EQUALS(ws->String(2, QMaxCol), "4.3");
    TS_ASSERT_EQUALS(ws->String(2, DQQCol), "2.17");
    TS_ASSERT_EQUALS(ws->Double(2, ScaleCol), 3.0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->String(2, OptionsCol), "def");
  }

  void testPasteNewRows()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    const std::string clipboard = "123\t0.5\t456\t1.2\t3.4\t3.14\t5\t6\tabc\n"
                                  "345\t2.7\t123\t2.1\t4.3\t2.17\t3\t2\tdef";

    //The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockView, getClipboard()).Times(1).WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(std::set<int>()));
    presenter.notify(IReflPresenter::PasteSelectedFlag);

    //The user hits "save"
    presenter.notify(IReflPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    //Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    //Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(4, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(4, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(4, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(4, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(4, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->Double(4, ScaleCol), 5.0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 6);
    TS_ASSERT_EQUALS(ws->String(4, OptionsCol), "abc");

    TS_ASSERT_EQUALS(ws->String(5, RunCol), "345");
    TS_ASSERT_EQUALS(ws->String(5, ThetaCol), "2.7");
    TS_ASSERT_EQUALS(ws->String(5, TransCol), "123");
    TS_ASSERT_EQUALS(ws->String(5, QMinCol), "2.1");
    TS_ASSERT_EQUALS(ws->String(5, QMaxCol), "4.3");
    TS_ASSERT_EQUALS(ws->String(5, DQQCol), "2.17");
    TS_ASSERT_EQUALS(ws->Double(5, ScaleCol), 3.0);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->String(5, OptionsCol), "def");
  }

  void testImportTable()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);
    EXPECT_CALL(mockView, showAlgorithmDialog("LoadReflTBL"));
    presenter.notify(IReflPresenter::ImportTableFlag);
  }

  void testExportTable()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);
    EXPECT_CALL(mockView, showAlgorithmDialog("SaveReflTBL"));
    presenter.notify(IReflPresenter::ExportTableFlag);
  }

  void testPlotRowWarn()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    createTOFWorkspace("TOF_12345", "12345");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(0);

    //We should be warned
    EXPECT_CALL(mockView, giveUserWarning(_,_));

    //The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::PlotRowFlag);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_12345");
  }

  void testPlotGroupWarn()
  {
    MockView mockView;
    ReflMainViewPresenter presenter(&mockView);

    createPrefilledWorkspace("TestWorkspace");
    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");
    EXPECT_CALL(mockView, getWorkspaceToOpen()).Times(1).WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(IReflPresenter::OpenTableFlag);

    std::set<int> rowlist;
    rowlist.insert(0);

    //We should be warned
    EXPECT_CALL(mockView, giveUserWarning(_,_));

    //The user hits "plot groups" with the first row selected
    EXPECT_CALL(mockView, getSelectedRows()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(IReflPresenter::PlotGroupFlag);

    //Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("TOF_12346");
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H */
