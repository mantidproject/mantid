#ifndef MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtCustomInterfaces/Muon/IALCBaselineModellingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

using namespace MantidQt::CustomInterfaces;

class MockALCBaselineModellingView : public IALCBaselineModellingView
{
public:
  MockALCBaselineModellingView() : m_presenter(this) {}

  MOCK_METHOD1(setData, void(MatrixWorkspace_const_sptr));

private:
  ALCBaselineModellingPresenter m_presenter;
};

class ALCBaselineModellingTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running otherl tests
  static ALCBaselineModellingTest *createSuite() { return new ALCBaselineModellingTest(); }
  static void destroySuite( ALCBaselineModellingTest *suite ) { delete suite; }

  void test_initial()
  {
    MockALCBaselineModellingView view;
  }
};


#endif /* MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGTEST_H_ */
