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

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflLoadedMainViewPresenterTest : public CxxTest::TestSuite
{

private:

  class ConstructView : public ReflMainView
  {
  public:
    ConstructView(){};
    MOCK_METHOD1(showTable, void(Mantid::API::ITableWorkspace_sptr));
    MOCK_METHOD0(askUserString, bool());
    MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
    MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
    MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
    MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_METHOD0(getFlag, Flag());
    MOCK_CONST_METHOD0(flagSet, bool());
    MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
    virtual ~ConstructView(){}
  };

  class MockView : public ReflMainView
  {
  public:
    MockView(){};
    virtual void showTable(Mantid::API::ITableWorkspace_sptr model){(void)model;}
    MOCK_METHOD0(askUserString, bool());
    MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
    MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
    MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
    MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_METHOD0(getFlag, Flag());
    MOCK_CONST_METHOD0(flagSet, bool());
    MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
    virtual ~MockView(){}
  };

  class FakeView : public ReflMainView
  {
  public:
    FakeView(){};
    virtual void showTable(Mantid::API::ITableWorkspace_sptr model)
    {
      TableRow row = model->appendRow();

      row << "13464" << "0.6" << "13465" << "0.02" << "0.03" << "0.05" << "8" << 2;
    }
    MOCK_METHOD0(askUserString, bool());
    MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
    MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
    MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
    MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_METHOD0(getFlag, Flag());
    MOCK_CONST_METHOD0(flagSet, bool());
    MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
    virtual ~FakeView(){}
  };

  ITableWorkspace_sptr createWorkspace(bool ADS = true)
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    if (ADS)
    {
      AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);
    }
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
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    auto colRuns = ws->addColumn("str","Run(s)");
    auto colTheta = ws->addColumn("str","ThetaIn");
    auto colTrans = ws->addColumn("str","TransRun(s)");
    auto colQmin = ws->addColumn("str","Qmin");
    auto colQmax = ws->addColumn("str","Qmax");
    auto colDqq = ws->addColumn("str","dq/q");
    auto colScale = ws->addColumn("str","Scale");
    auto colStitch = ws->addColumn("str","StitchGroup");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);
    colStitch->setPlotType(0);

    TableRow row = ws->appendRow();

    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << "2" << "1";

    return ws;
  }

  ITableWorkspace_sptr createBadLengthWorkspace(bool longer)
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    auto colRuns = ws->addColumn("str","Run(s)");
    auto colTheta = ws->addColumn("str","ThetaIn");
    auto colTrans = ws->addColumn("str","TransRun(s)");
    auto colQmin = ws->addColumn("str","Qmin");
    auto colQmax = ws->addColumn("str","Qmax");
    auto colDqq = ws->addColumn("str","dq/q");
    auto colScale = ws->addColumn("str","Scale");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);

    if(longer)
    {
      auto colStitch = ws->addColumn("int","StitchGroup");
      auto colPlot = ws->addColumn("str","Plot");
      colStitch->setPlotType(0);
      colPlot->setPlotType(0);
    }

    TableRow row = ws->appendRow();

    if(longer)
    {
      row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << "2" << 1 << "plot";
    }
    else
    {
      row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << "2";
    }

    return ws;
  }

  //Clean flag aliases for use within tests.
  static const ReflMainView::Flag saveAsFlag = ReflMainView::SaveAsFlag;
  static const ReflMainView::Flag saveFlag = ReflMainView::SaveFlag;
  static const ReflMainView::Flag processFlag = ReflMainView::ProcessFlag;
  static const ReflMainView::Flag addRowFlag = ReflMainView::AddRowFlag;
  static const ReflMainView::Flag deleteRowFlag = ReflMainView::DeleteRowFlag;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflLoadedMainViewPresenterTest *createSuite() { return new ReflLoadedMainViewPresenterTest(); }
  static void destroySuite( ReflLoadedMainViewPresenterTest *suite ) { delete suite; }
  
  ReflLoadedMainViewPresenterTest()
  {
    FrameworkManager::Instance();
  }

  void testConstruction()
  {
    ConstructView constructView;
    EXPECT_CALL(constructView, showTable(_)).Times(1);
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&constructView);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&constructView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testSave()
  {
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(2)
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag()).WillOnce(Return(saveFlag));

    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    presenter.notify();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testEditSave()
  {
    FakeView fakeView;
    EXPECT_CALL(fakeView, flagSet())
      .Times(2)
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(fakeView, getFlag()).WillOnce(Return(saveFlag));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&fakeView);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace")->rowCount(),4);
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(0,0), "13460");
    TS_ASSERT_EQUALS(ws->Int(0,7), 3);
    TS_ASSERT_EQUALS(ws->String(4,0), "13464");
    TS_ASSERT_EQUALS(ws->Int(4,7), 2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&fakeView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testSaveAs()
  {
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(4)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(2)
      .WillRepeatedly(Return(saveAsFlag));
    EXPECT_CALL(mockView, getUserString())
      .Times(1)
      .WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString())
      .Times(2)
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    presenter.notify();
    presenter.notify();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testSaveProcess()
  {
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(6)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(3)
      .WillOnce(Return(saveAsFlag))
      .WillOnce(Return(saveAsFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getUserString())
      .Times(1)
      .WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString())
      .Times(2)
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    presenter.notify();
    presenter.notify();
    presenter.notify();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAddRow()
  {
    std::vector<size_t> rowlist = std::vector<size_t>();
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(6)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(3)
      .WillOnce(Return(addRowFlag))
      .WillOnce(Return(addRowFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(2)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_THROWS(ws->Int(4,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(5,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(6,7),std::runtime_error);
    presenter.notify();
    presenter.notify();
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_THROWS_NOTHING(ws->Int(4,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(5,7));
    TS_ASSERT_EQUALS(ws->String(4,0), "");
    TS_ASSERT_EQUALS(ws->Int(4,7), 0);
    TS_ASSERT_EQUALS(ws->String(5,0), "");
    TS_ASSERT_EQUALS(ws->Int(5,7), 0);
    TS_ASSERT_THROWS(ws->Int(6,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAddRowSpecify()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(6)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(3)
      .WillOnce(Return(addRowFlag))
      .WillOnce(Return(addRowFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(2)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_EQUALS(ws->String(2,0), "13469");
    TS_ASSERT_EQUALS(ws->Int(2,7), 1);
    TS_ASSERT_THROWS(ws->Int(4,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(5,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(6,7),std::runtime_error);
    presenter.notify();
    presenter.notify();
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(1,0), "");
    TS_ASSERT_EQUALS(ws->Int(1,7), 0);
    TS_ASSERT_EQUALS(ws->String(2,0), "");
    TS_ASSERT_EQUALS(ws->Int(2,7), 0);
    TS_ASSERT_THROWS_NOTHING(ws->Int(4,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(5,7));
    TS_ASSERT_EQUALS(ws->String(4,0), "13469");
    TS_ASSERT_EQUALS(ws->Int(4,7), 1);
    TS_ASSERT_EQUALS(ws->String(5,0), "13470");
    TS_ASSERT_EQUALS(ws->Int(5,7), 1);
    TS_ASSERT_THROWS(ws->Int(6,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAddRowSpecifyPlural()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    rowlist.push_back(2);
    rowlist.push_back(3);
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(4)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(2)
      .WillOnce(Return(addRowFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(1)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_EQUALS(ws->String(2,0), "13469");
    TS_ASSERT_EQUALS(ws->Int(2,7), 1);
    TS_ASSERT_THROWS(ws->Int(4,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(5,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(6,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(7,7),std::runtime_error);
    presenter.notify();
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 7);
    TS_ASSERT_EQUALS(ws->String(1,0), "");
    TS_ASSERT_EQUALS(ws->Int(1,7), 0);
    TS_ASSERT_EQUALS(ws->String(2,0), "");
    TS_ASSERT_EQUALS(ws->Int(2,7), 0);
    TS_ASSERT_EQUALS(ws->String(3,0), "");
    TS_ASSERT_EQUALS(ws->Int(3,7), 0);
    TS_ASSERT_THROWS_NOTHING(ws->Int(4,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(5,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(6,7));
    TS_ASSERT_EQUALS(ws->String(4,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(4,7), 3);
    TS_ASSERT_EQUALS(ws->String(5,0), "13469");
    TS_ASSERT_EQUALS(ws->Int(5,7), 1);
    TS_ASSERT_EQUALS(ws->String(6,0), "13470");
    TS_ASSERT_EQUALS(ws->Int(6,7), 1);
    TS_ASSERT_THROWS(ws->Int(7,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testAddRowNotSequential()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(2);
    rowlist.push_back(3);
    rowlist.push_back(0);
    //The rowlist is sorted internally, so 0 will go to the top and be classed as the highest.
    //So 3 rows will be added to the top. We can do this as we are only expecting chunks of
    //sequential rows, thus this is the expected behavior
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(4)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(2)
      .WillOnce(Return(addRowFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(1)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(0,0), "13460");
    TS_ASSERT_EQUALS(ws->Int(0,7), 3);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_EQUALS(ws->String(2,0), "13469");
    TS_ASSERT_EQUALS(ws->Int(2,7), 1);
    TS_ASSERT_THROWS(ws->Int(4,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(5,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(6,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(7,7),std::runtime_error);
    presenter.notify();
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 7);
    TS_ASSERT_EQUALS(ws->String(0,0), "");
    TS_ASSERT_EQUALS(ws->Int(0,7), 0);
    TS_ASSERT_EQUALS(ws->String(1,0), "");
    TS_ASSERT_EQUALS(ws->Int(1,7), 0);
    TS_ASSERT_EQUALS(ws->String(2,0), "");
    TS_ASSERT_EQUALS(ws->Int(2,7), 0);
    TS_ASSERT_THROWS_NOTHING(ws->Int(4,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(5,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(6,7));
    TS_ASSERT_EQUALS(ws->String(3,0), "13460");
    TS_ASSERT_EQUALS(ws->Int(3,7), 3);
    TS_ASSERT_EQUALS(ws->String(4,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(4,7), 3);
    TS_ASSERT_EQUALS(ws->String(5,0), "13469");
    TS_ASSERT_EQUALS(ws->Int(5,7), 1);
    TS_ASSERT_EQUALS(ws->String(6,0), "13470");
    TS_ASSERT_EQUALS(ws->Int(6,7), 1);
    TS_ASSERT_THROWS(ws->Int(7,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowNone()
  {
    std::vector<size_t> rowlist = std::vector<size_t>();
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(4)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(2)
      .WillOnce(Return(deleteRowFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(1)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_THROWS_NOTHING(ws->Int(0,7));
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_THROWS_NOTHING(ws->Int(2,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(3,7));
    presenter.notify();
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_THROWS_NOTHING(ws->Int(0,7));
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_THROWS_NOTHING(ws->Int(2,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(3,7));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowSingle()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(4)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(2)
      .WillOnce(Return(deleteRowFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(1)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_THROWS_NOTHING(ws->Int(3,7));
    presenter.notify();
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),3);
    TS_ASSERT_EQUALS(ws->String(1,0), "13469");
    TS_ASSERT_EQUALS(ws->Int(1,7), 1);
    TS_ASSERT_THROWS(ws->Int(3,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowPlural()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(0);
    rowlist.push_back(1);
    rowlist.push_back(2);
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(4)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(2)
      .WillOnce(Return(deleteRowFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(1)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(0,0), "13460");
    TS_ASSERT_EQUALS(ws->Int(0,7), 3);
    TS_ASSERT_THROWS_NOTHING(ws->Int(1,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(2,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(3,7));
    presenter.notify();
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),1);
    TS_ASSERT_EQUALS(ws->String(0,0), "13470");
    TS_ASSERT_EQUALS(ws->Int(0,7), 1);
    TS_ASSERT_THROWS(ws->Int(1,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(2,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(3,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testDeleteRowNotSequential()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(2);
    rowlist.push_back(3);
    rowlist.push_back(0);
    //The rowlist is sorted internally, so 0 will go to the top and be classed as the highest.
    //So 3 rows will be removed from the top. We can do this as we are only expecting chunks of
    //sequential rows, thus this is the expected behavior
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(4)
      .WillOnce(Return(true)).WillOnce(Return(false))
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(2)
      .WillOnce(Return(deleteRowFlag))
      .WillOnce(Return(saveFlag));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(1)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(0,0), "13460");
    TS_ASSERT_EQUALS(ws->Int(0,7), 3);
    TS_ASSERT_THROWS_NOTHING(ws->Int(1,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(2,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(3,7));
    presenter.notify();
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(),1);
    TS_ASSERT_EQUALS(ws->String(0,0), "13470");
    TS_ASSERT_EQUALS(ws->Int(0,7), 1);
    TS_ASSERT_THROWS(ws->Int(1,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(2,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(3,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }
  
  void testProcessAll()
  {
    std::vector<size_t> rowlist = std::vector<size_t>();
    MockView mockView;
    EXPECT_CALL(mockView, flagSet())
      .Times(2)
      .WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getFlag())
      .Times(1)
      .WillOnce(Return(processFlag));
    EXPECT_CALL(mockView, askUserYesNo(_,_))
      .Times(1)
      .WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSelectedRowIndexes())
      .Times(1)
      .WillRepeatedly(Return(rowlist));
    ReflLoadedMainViewPresenter presenter(createWorkspace(),&mockView);
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    presenter.notify();
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("TestWorkspace");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("TestWorkspace");
  }

  void testBadWorkspaceName()
  {
    MockView mockView;
    TS_ASSERT_THROWS(ReflLoadedMainViewPresenter presenter(createWorkspace(false),&mockView), std::runtime_error&);
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
