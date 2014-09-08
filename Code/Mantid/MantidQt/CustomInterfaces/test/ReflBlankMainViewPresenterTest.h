#ifndef MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/make_shared.hpp>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtCustomInterfaces/ReflBlankMainViewPresenter.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

class ReflBlankMainViewPresenterTest : public CxxTest::TestSuite
{

private:

  class ConstructView : public ReflMainView
  {
  public:
    ConstructView(){};
    MOCK_METHOD0(clearNotifyFlags, void());
    MOCK_METHOD1(showTable, void(Mantid::API::ITableWorkspace_sptr));
    MOCK_METHOD0(askUserString, bool());
    MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
    MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
    MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
    MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_CONST_METHOD0(getSaveFlag, bool());
    MOCK_CONST_METHOD0(getSaveAsFlag, bool());
    MOCK_CONST_METHOD0(getAddRowFlag, bool());
    MOCK_CONST_METHOD0(getDeleteRowFlag, bool());
    MOCK_CONST_METHOD0(getProcessFlag, bool());
    MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
    virtual ~ConstructView(){}
  };

  class MockView : public ReflMainView
  {
  public:
    MockView(){};
    virtual void showTable(Mantid::API::ITableWorkspace_sptr model){(void)model;}
    MOCK_METHOD0(clearNotifyFlags, void());
    MOCK_METHOD0(askUserString, bool());
    MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
    MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
    MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
    MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_CONST_METHOD0(getSaveFlag, bool());
    MOCK_CONST_METHOD0(getSaveAsFlag, bool());
    MOCK_CONST_METHOD0(getAddRowFlag, bool());
    MOCK_CONST_METHOD0(getDeleteRowFlag, bool());
    MOCK_CONST_METHOD0(getProcessFlag, bool());
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
      row << "13460" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 3;
      row = model->appendRow();
      row << "13462" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 3;
      row = model->appendRow();
      row << "13469" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 3;
      row = model->appendRow();
      row << "13470" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 3;
    }
    MOCK_METHOD0(clearNotifyFlags, void());
    MOCK_METHOD0(askUserString, bool());
    MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
    MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
    MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
    MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_CONST_METHOD0(getSaveFlag, bool());
    MOCK_CONST_METHOD0(getSaveAsFlag, bool());
    MOCK_CONST_METHOD0(getAddRowFlag, bool());
    MOCK_CONST_METHOD0(getDeleteRowFlag, bool());
    MOCK_CONST_METHOD0(getProcessFlag, bool());
    MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
    virtual ~FakeView(){}
  };

  class AddDelProcView : public ReflMainView
  {
  public:
    AddDelProcView(){};
    virtual void showTable(Mantid::API::ITableWorkspace_sptr model)
    {
      m_model = model;
    }
    MOCK_METHOD0(clearNotifyFlags, void());
    MOCK_METHOD0(askUserString, bool());
    MOCK_METHOD2(askUserYesNo, bool(std::string, std::string));
    MOCK_METHOD2(giveUserCritical, void(std::string, std::string));
    MOCK_METHOD2(giveUserInfo, void(std::string, std::string));
    MOCK_METHOD2(giveUserWarning, void(std::string, std::string));
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_CONST_METHOD0(getSaveFlag, bool());
    MOCK_CONST_METHOD0(getSaveAsFlag, bool());
    MOCK_CONST_METHOD0(getAddRowFlag, bool());
    MOCK_CONST_METHOD0(getDeleteRowFlag, bool());
    MOCK_CONST_METHOD0(getProcessFlag, bool());
    MOCK_CONST_METHOD0(getSelectedRowIndexes, std::vector<size_t>());
    virtual ~AddDelProcView(){}
    void addDataForTest()
    {
      TableRow row = m_model->appendRow();
      row << "13460" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 3;
      row = m_model->appendRow();
      row << "13462" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 3;
      row = m_model->appendRow();
      row << "13469" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << "1" << 1;
      row = m_model->appendRow();
      row << "13470" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << "1" << 1;
      m_model->removeRow(0);
    }
  private:
    Mantid::API::ITableWorkspace_sptr m_model;
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflBlankMainViewPresenterTest *createSuite() { return new ReflBlankMainViewPresenterTest(); }
  static void destroySuite( ReflBlankMainViewPresenterTest *suite ) { delete suite; }

  void testConstruction()
  {
    ConstructView constructView;
    EXPECT_CALL(constructView, showTable(_)).Times(1);
    ReflBlankMainViewPresenter presenter(&constructView);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&constructView));
  }

  void testEditSave()
  {
    FakeView fakeView;
    EXPECT_CALL(fakeView, getSaveFlag()).WillRepeatedly(Return(true));
    EXPECT_CALL(fakeView, getSaveAsFlag()).WillRepeatedly(Return(false));
    EXPECT_CALL(fakeView, clearNotifyFlags()).Times(1);
    EXPECT_CALL(fakeView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(fakeView, askUserString()).Times(1).WillRepeatedly(Return(true));
    ReflBlankMainViewPresenter presenter(&fakeView);
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(1,0), "13460");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&fakeView));
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testSaveAs()
  {
    MockView mockView;
    EXPECT_CALL(mockView, getSaveAsFlag()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveFlag()).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(2).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(2);
    ReflBlankMainViewPresenter presenter(&mockView);
    presenter.notify();
    presenter.notify();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testSaveProcess()
  {
    MockView mockView;
    EXPECT_CALL(mockView, getSaveAsFlag()).WillOnce(Return(true)).WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getSaveFlag()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(2).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(3);
    ReflBlankMainViewPresenter presenter(&mockView);
    presenter.notify();
    presenter.notify();
    presenter.notify();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAddRow()
  {
    std::vector<size_t> rowlist = std::vector<size_t>();
    AddDelProcView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(3).WillOnce(Return(false)).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getDeleteRowFlag()).Times(0);
    EXPECT_CALL(mockView, getProcessFlag()).Times(0);
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(3);
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(2).WillRepeatedly(Return(rowlist));
    ReflBlankMainViewPresenter presenter(&mockView);
    mockView.addDataForTest();
    presenter.notify();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
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
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAddRowSpecify()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    AddDelProcView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(3).WillOnce(Return(false)).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getDeleteRowFlag()).Times(0);
    EXPECT_CALL(mockView, getProcessFlag()).Times(0);
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(3);
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(2).WillRepeatedly(Return(rowlist));
    ReflBlankMainViewPresenter presenter(&mockView);
    mockView.addDataForTest();
    presenter.notify();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
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
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAddRowSpecifyPlural()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    rowlist.push_back(2);
    rowlist.push_back(3);
    AddDelProcView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(2).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getDeleteRowFlag()).Times(0);
    EXPECT_CALL(mockView, getProcessFlag()).Times(0);
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(2);
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    ReflBlankMainViewPresenter presenter(&mockView);
    mockView.addDataForTest();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
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
    AnalysisDataService::Instance().remove("Workspace");
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
    AddDelProcView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(2).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getDeleteRowFlag()).Times(0);
    EXPECT_CALL(mockView, getProcessFlag()).Times(0);
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(2);
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    ReflBlankMainViewPresenter presenter(&mockView);
    mockView.addDataForTest();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
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
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testDeleteRowNone()
  {
    std::vector<size_t> rowlist = std::vector<size_t>();
    AddDelProcView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(2).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getProcessFlag()).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getDeleteRowFlag()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(2);
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    ReflBlankMainViewPresenter presenter(&mockView);
    mockView.addDataForTest();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_THROWS_NOTHING(ws->Int(0,7));
    TS_ASSERT_EQUALS(ws->rowCount(),4);
    TS_ASSERT_EQUALS(ws->String(1,0), "13462");
    TS_ASSERT_EQUALS(ws->Int(1,7), 3);
    TS_ASSERT_THROWS_NOTHING(ws->Int(2,7));
    TS_ASSERT_THROWS_NOTHING(ws->Int(3,7));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testDeleteRowSingle()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    AddDelProcView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(2).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getProcessFlag()).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getDeleteRowFlag()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(2);
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    ReflBlankMainViewPresenter presenter(&mockView);
    mockView.addDataForTest();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(),3);
    TS_ASSERT_EQUALS(ws->String(1,0), "13469");
    TS_ASSERT_EQUALS(ws->Int(1,7), 1);
    TS_ASSERT_THROWS(ws->Int(3,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testDeleteRowPlural()
  {
    std::vector<size_t> rowlist;
    rowlist.push_back(0);
    rowlist.push_back(1);
    rowlist.push_back(2);
    AddDelProcView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(2).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getProcessFlag()).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getDeleteRowFlag()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(2);
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    ReflBlankMainViewPresenter presenter(&mockView);
    mockView.addDataForTest();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(),1);
    TS_ASSERT_EQUALS(ws->String(0,0), "13470");
    TS_ASSERT_EQUALS(ws->Int(0,7), 1);
    TS_ASSERT_THROWS(ws->Int(1,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(2,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(3,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("Workspace");
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
    AddDelProcView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(2).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getProcessFlag()).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getDeleteRowFlag()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(2);
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    ReflBlankMainViewPresenter presenter(&mockView);
    mockView.addDataForTest();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(),1);
    TS_ASSERT_EQUALS(ws->String(0,0), "13470");
    TS_ASSERT_EQUALS(ws->Int(0,7), 1);
    TS_ASSERT_THROWS(ws->Int(1,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(2,7),std::runtime_error);
    TS_ASSERT_THROWS(ws->Int(3,7),std::runtime_error);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testDualFlags()
  {
    MockView mockView;
    EXPECT_CALL(mockView, getSaveAsFlag()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveFlag()).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getAddRowFlag()).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(1);
    ReflBlankMainViewPresenter presenter(&mockView);
    presenter.notify();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));
    AnalysisDataService::Instance().remove("Workspace");
  }
};


#endif /* MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTERTEST_H_ */
