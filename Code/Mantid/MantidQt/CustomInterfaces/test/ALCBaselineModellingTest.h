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
  MOCK_CONST_METHOD0(sections, std::vector<Section>());
  MOCK_METHOD1(setData, void(MatrixWorkspace_const_sptr));
  MOCK_METHOD1(setCorrectedData, void(MatrixWorkspace_const_sptr));
  MOCK_METHOD1(setFunction, void(IFunction_const_sptr));
};

class ALCBaselineModellingTest : public CxxTest::TestSuite
{
  MockALCBaselineModellingView* m_view;
  ALCBaselineModellingPresenter* m_presenter;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running otherl tests
  static ALCBaselineModellingTest *createSuite() { return new ALCBaselineModellingTest(); }
  static void destroySuite( ALCBaselineModellingTest *suite ) { delete suite; }

  ALCBaselineModellingTest()
  {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp()
  {
    m_view = new MockALCBaselineModellingView();
    m_presenter = new ALCBaselineModellingPresenter(m_view);
    EXPECT_CALL(*m_view, initialize()).Times(1);
    m_presenter->initialize();
  }

  void tearDown()
  {
    delete m_presenter;
    delete m_view;
  }

  void setData(MatrixWorkspace_const_sptr data)
  {
    EXPECT_CALL(*m_view, setData(_)).Times(1);
    m_presenter->setData(data);
  }

  void test_basicUsage()
  {
    MatrixWorkspace_sptr data = WorkspaceFactory::Instance().create("Workspace2D", 1, 3, 3);

    using boost::assign::list_of;
    data->dataY(0) = list_of(1)(2)(100)(3)(4)(5)(100)(100).to_container(data->dataY(0));
    data->dataX(0) = list_of(1)(2)( 3 )(4)(5)(6)( 7 )( 8 ).to_container(data->dataX(0));

    setData(data);

    IFunction_const_sptr func = FunctionFactory::Instance().createInitialized("name=FlatBackground,A0=0");

    std::vector<IALCBaselineModellingView::Section> sections;
    sections.push_back(std::make_pair(1,2));
    sections.push_back(std::make_pair(4,6));

    IFunction_const_sptr fittedFunc;
    MatrixWorkspace_const_sptr corrected;

    EXPECT_CALL(*m_view, function()).WillRepeatedly(Return(func));
    EXPECT_CALL(*m_view, sections()).WillRepeatedly(Return(sections));

    EXPECT_CALL(*m_view, setFunction(_)).Times(1).WillOnce(SaveArg<0>(&fittedFunc));
    EXPECT_CALL(*m_view, setCorrectedData(_)).Times(1).WillOnce(SaveArg<0>(&corrected));
    EXPECT_CALL(*m_view, setData(_)).Times(0);

    m_view->requestFit();

    TS_ASSERT(fittedFunc);

    if (fittedFunc)
    {
      TS_ASSERT_EQUALS(fittedFunc->name(), "FlatBackground");
      TS_ASSERT_DELTA(fittedFunc->getParameter("A0"), 3, 1E-8);
    }

    TS_ASSERT(corrected);

    if ( corrected )
    {
      TS_ASSERT_EQUALS(corrected->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(corrected->blocksize(), 8);

      TS_ASSERT_DELTA(corrected->readY(0)[0], -2.0, 1E-8);
      TS_ASSERT_DELTA(corrected->readY(0)[2], 97, 1E-8);
      TS_ASSERT_DELTA(corrected->readY(0)[5], 2.0, 1E-8);
      TS_ASSERT_DELTA(corrected->readY(0)[7], 97, 1E-8);
    }
  }

};


#endif /* MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGTEST_H_ */
