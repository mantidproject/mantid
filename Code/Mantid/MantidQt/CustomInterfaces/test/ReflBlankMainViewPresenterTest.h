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

  class MockView : public ReflMainView
  {
  public:
    MockView(){};
    MOCK_METHOD0(clearNotifyFlags, void());
    MOCK_METHOD1(showTable, void(Mantid::API::ITableWorkspace_sptr));
    MOCK_METHOD0(askUserString, bool());
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_CONST_METHOD0(getSaveFlag, bool());
    MOCK_CONST_METHOD0(getSaveAsFlag, bool());
    MOCK_CONST_METHOD0(getAddRowFlag, bool());
    MOCK_CONST_METHOD0(getDeleteRowFlag, bool());
    MOCK_CONST_METHOD0(getProcessFlag, bool());
    virtual ~MockView(){}
  };

  class FakeView : public ReflMainView
  {
  public:
    FakeView(){};
    MOCK_METHOD0(clearNotifyFlags, void());
    virtual void showTable(Mantid::API::ITableWorkspace_sptr model)
    {
    TableRow row = model->appendRow();

    row << "13464" << "0.6" << "13465" << "0.02" << "0.03" << "0.05" << "8" << 3;
    }
    MOCK_METHOD0(askUserString, bool());
    MOCK_CONST_METHOD0(getUserString, std::string());
    MOCK_CONST_METHOD0(getSaveFlag, bool());
    MOCK_CONST_METHOD0(getSaveAsFlag, bool());
    MOCK_CONST_METHOD0(getAddRowFlag, bool());
    MOCK_CONST_METHOD0(getDeleteRowFlag, bool());
    MOCK_CONST_METHOD0(getProcessFlag, bool());
    virtual ~FakeView(){}
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflBlankMainViewPresenterTest *createSuite() { return new ReflBlankMainViewPresenterTest(); }
  static void destroySuite( ReflBlankMainViewPresenterTest *suite ) { delete suite; }

 void testConstruction()
  {
    MockView mockView;
    EXPECT_CALL(mockView, showTable(_)).Times(1);
    ReflBlankMainViewPresenter presenter(&mockView);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
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
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(ws->String(1,0), "13464");
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
    MockView mockView;
    EXPECT_CALL(mockView, getSaveFlag()).Times(3).WillOnce(Return(false)).WillOnce(Return(false)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, getSaveAsFlag()).WillRepeatedly(Return(false));
    EXPECT_CALL(mockView, getAddRowFlag()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(mockView, clearNotifyFlags()).Times(3);
    EXPECT_CALL(mockView, getUserString()).Times(1).WillRepeatedly(Return("Workspace"));
    EXPECT_CALL(mockView, askUserString()).Times(1).WillRepeatedly(Return(true));
    ReflBlankMainViewPresenter presenter(&mockView);
    presenter.notify();
    presenter.notify();
    presenter.notify();
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
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