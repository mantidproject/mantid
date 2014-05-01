#ifndef CUSTOM_INTERFACES_REFLMAINVIEWPRESENTER_TEST_H_
#define CUSTOM_INTERFACES_REFLMAINVIEWPRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/make_shared.hpp>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflMainViewPresenterTest : public CxxTest::TestSuite
{

private:

  class MockView : public ReflMainView
  {
  public:
    MockView(){};
    MOCK_METHOD1(showTable, void(Mantid::API::ITableWorkspace_sptr));
    virtual ~MockView(){}
  };

  ITableWorkspace_sptr createWorkspace()
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

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << "2" << 1;
    return ws;
  }

  ITableWorkspace_sptr createbadWorkspace(int typeflag = 1)
  {
    //lengthflag: <1 = short , 1 = normal with bad typing, >1 = long
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
    if (typeflag == 1)
    {
      auto colStitch = ws->addColumn("str","StitchGroup");
      colStitch->setPlotType(0);
    }
    else if(typeflag > 1)
    {
      auto colStitch = ws->addColumn("int","StitchGroup");
      auto colPlot = ws->addColumn("str","Plot");
      colStitch->setPlotType(0);
      colPlot->setPlotType(0);
    }
    TableRow row = ws->appendRow();
    if(typeflag == 1)
    {
      row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << "2" << "1";
    }
    else if(typeflag > 1)
    {
      row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << "2" << 1 << "plot";
    }
    else
    {
      row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << "2";
    }
    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflMainViewPresenterTest *createSuite() { return new ReflMainViewPresenterTest(); }
  static void destroySuite( ReflMainViewPresenterTest *suite ) { delete suite; }

  void testConstruction()
  {
    MockView mockView;
    EXPECT_CALL(mockView, showTable(_)).Times(1);
    ReflMainViewPresenter presenter(createWorkspace(),&mockView);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testBadWorkspaceType()
  {
    MockView mockView;
    TS_ASSERT_THROWS(ReflMainViewPresenter presenter(createbadWorkspace(),&mockView), std::runtime_error&);
  }

  void testBadWorkspaceShort()
  {
    MockView mockView;
    TS_ASSERT_THROWS(ReflMainViewPresenter presenter(createbadWorkspace(0),&mockView), std::runtime_error&);
  }

  void testBadWorkspaceLong()
  {
    MockView mockView;
    TS_ASSERT_THROWS(ReflMainViewPresenter presenter(createbadWorkspace(2),&mockView), std::runtime_error&);
  }

};
#endif
