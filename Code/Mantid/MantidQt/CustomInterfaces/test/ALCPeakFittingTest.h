#ifndef MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingPresenter.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class MockALCPeakFittingView : public IALCPeakFittingView
{
public:
  void requestFit() { emit fit(); }

  MOCK_CONST_METHOD0(peaks, ListOfPeaks());
  MOCK_METHOD0(initialize, void());
  MOCK_METHOD1(displayData, void(MatrixWorkspace_const_sptr));
  MOCK_METHOD1(updatePeaks, void(const ListOfPeaks&));
};

using namespace MantidQt::CustomInterfaces;

class ALCPeakFittingTest : public CxxTest::TestSuite
{
  MockALCPeakFittingView* m_view;
  ALCPeakFittingPresenter* m_presenter;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCPeakFittingTest *createSuite() { return new ALCPeakFittingTest(); }
  static void destroySuite( ALCPeakFittingTest *suite ) { delete suite; }

  void setUp()
  {
    m_view = new MockALCPeakFittingView();
    m_presenter = new ALCPeakFittingPresenter(m_view);

    EXPECT_CALL(*m_view, initialize()).Times(1);

    m_presenter->initialize();
  }

  void tearDown()
  {
    delete m_presenter;
    delete m_view;
  }

  void test_fittingOnePeak()
  {
  }

};


#endif /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_ */
