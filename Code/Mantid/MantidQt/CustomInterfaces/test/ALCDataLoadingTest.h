#ifndef MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_
#define MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoading.h"

using namespace MantidQt::CustomInterfaces;

class MockALCDataLoadingView : public IALCDataLoadingView
{
public:
  MOCK_METHOD0(firstRun, std::string());
  MOCK_METHOD0(lastRun, std::string());
  MOCK_METHOD1(setData, void(MatrixWorkspace_const_sptr));

  void requestLoading() { emit loadData(); }
};

class ALCDataLoadingTest : public CxxTest::TestSuite
{
  MockALCDataLoadingView* m_view;
  ALCDataLoading* m_dataLoading;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCDataLoadingTest *createSuite() { return new ALCDataLoadingTest(); }
  static void destroySuite( ALCDataLoadingTest *suite ) { delete suite; }

  void setUp()
  {
    m_view = new MockALCDataLoadingView();
    m_dataLoading = new ALCDataLoading(m_view);
    TS_ASSERT_THROWS_NOTHING(m_dataLoading->initialize());
  }

  void tearDown()
  {
    delete m_view;
    delete m_dataLoading;
  }

  void test_basicLoading()
  {
    EXPECT_CALL(*m_view, setData(MatrixWorkspace_const_sptr())).Times(1);

    m_view->requestLoading();
  }
};


#endif /* MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_ */
