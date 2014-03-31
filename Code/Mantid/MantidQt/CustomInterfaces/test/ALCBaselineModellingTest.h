#ifndef MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <boost/scoped_ptr.hpp>
#include <boost/assign/list_of.hpp>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidQtCustomInterfaces/Muon/IALCBaselineModellingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;
using boost::scoped_ptr;

class MockALCBaselineModellingView : public IALCBaselineModellingView
{
public:
  void requestFit() { emit fit(); }

  MOCK_METHOD0(initialize, void());
  MOCK_CONST_METHOD0(function, IFunction_const_sptr());
  MOCK_METHOD1(displayData, void(MatrixWorkspace_const_sptr));
  MOCK_METHOD1(updateFunction, void(IFunction_const_sptr));
};

class ALCBaselineModellingTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running otherl tests
  static ALCBaselineModellingTest *createSuite() { return new ALCBaselineModellingTest(); }
  static void destroySuite( ALCBaselineModellingTest *suite ) { delete suite; }

  ALCBaselineModellingTest()
  {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  MockALCBaselineModellingView* createView(MatrixWorkspace_const_sptr data)
  {
    auto view = new MockALCBaselineModellingView();
    auto presenter = new ALCBaselineModellingPresenter(view);
    EXPECT_CALL(*view, displayData(_)).Times(1);
    presenter->initialize();
    presenter->setData(data);
    return view;
  }

  void test_basicFitting()
  {
    MatrixWorkspace_sptr data = WorkspaceFactory::Instance().create("Workspace2D", 1, 3, 3);

    using boost::assign::list_of;

    data->dataX(0) = list_of(0)(2)(3).convert_to_container<Mantid::MantidVec>();
    data->dataY(0) = list_of(5)(5)(5).convert_to_container<Mantid::MantidVec>();

    IFunction_const_sptr func = FunctionFactory::Instance().createInitialized("name=FlatBackground,A0=0");
    IFunction_const_sptr fittedFunc;

    scoped_ptr<MockALCBaselineModellingView> view(createView(data));
    EXPECT_CALL(*view, function()).WillRepeatedly(Return(func));
    EXPECT_CALL(*view, updateFunction(_)).Times(1).WillOnce(SaveArg<0>(&fittedFunc));
    EXPECT_CALL(*view, displayData(_)).Times(0);

    view->requestFit();

    TS_ASSERT(fittedFunc);

    if (fittedFunc)
    {
      TS_ASSERT_EQUALS(fittedFunc->name(), "FlatBackground");
      TS_ASSERT_DELTA(fittedFunc->getParameter("A0"), 5, 1E-8);
    }
  }
};


#endif /* MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGTEST_H_ */
