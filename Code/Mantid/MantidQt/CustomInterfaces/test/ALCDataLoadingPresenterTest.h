#ifndef MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_
#define MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidQtCustomInterfaces/Muon/IALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class MockALCDataLoadingView : public IALCDataLoadingView
{
public:
  MOCK_CONST_METHOD0(firstRun, std::string());
  MOCK_CONST_METHOD0(lastRun, std::string());
  MOCK_CONST_METHOD0(log, std::string());

  MOCK_METHOD0(initialize, void());
  MOCK_METHOD1(setDataCurve, void(const QwtData&));
  MOCK_METHOD1(displayError, void(const std::string&));
  MOCK_METHOD1(setAvailableLogs, void(const std::vector<std::string>&));

  void requestLoading() { emit loadRequested(); }
  void selectFirstRun() { emit firstRunSelected(); }
};

MATCHER_P3(QwtDataX, i, value, delta, "") { return fabs(arg.x(i) - value) < delta; }
MATCHER_P3(QwtDataY, i, value, delta, "") { return fabs(arg.y(i) - value) < delta; }

class ALCDataLoadingPresenterTest : public CxxTest::TestSuite
{
  MockALCDataLoadingView* m_view;
  ALCDataLoadingPresenter* m_presenter;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCDataLoadingPresenterTest *createSuite() { return new ALCDataLoadingPresenterTest(); }
  static void destroySuite( ALCDataLoadingPresenterTest *suite ) { delete suite; }

  ALCDataLoadingPresenterTest()
  {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp()
  {
    m_view = new NiceMock<MockALCDataLoadingView>();
    m_presenter = new ALCDataLoadingPresenter(m_view);
    m_presenter->initialize();
  }

  void tearDown()
  {
    delete m_view;
    delete m_presenter;
  }

  void test_initialize()
  {
    MockALCDataLoadingView view;
    ALCDataLoadingPresenter presenter(&view);
    EXPECT_CALL(view, initialize());
    presenter.initialize();
  }

  void test_load()
  {
    EXPECT_CALL(*m_view, firstRun()).WillRepeatedly(Return("MUSR00015189.nxs"));
    EXPECT_CALL(*m_view, lastRun()).WillRepeatedly(Return("MUSR00015191.nxs"));
    EXPECT_CALL(*m_view, log()).WillRepeatedly(Return("sample_magn_field"));

    EXPECT_CALL(*m_view, setDataCurve(AllOf(Property(&QwtData::size,3),
                                            QwtDataX(0, 1350, 1E-8),
                                            QwtDataX(1, 1360, 1E-8),
                                            QwtDataX(2, 1370, 1E-8),
                                            QwtDataY(0, 0.150, 1E-3),
                                            QwtDataY(1, 0.143, 1E-3),
                                            QwtDataY(2, 0.128, 1E-3))));

    m_view->requestLoading();
  }

  void test_updateAvailableLogs()
  {
    EXPECT_CALL(*m_view, firstRun()).WillRepeatedly(Return("MUSR00015189.nxs"));
    EXPECT_CALL(*m_view, setAvailableLogs(AllOf(Property(&std::vector<std::string>::size, 33),
                                                Contains("run_number"),
                                                Contains("sample_magn_field"),
                                                Contains("Field_Danfysik"))));
    m_view->selectFirstRun();
  }

  void test_updateAvailableLogs_invalidFirstRun()
  {
    EXPECT_CALL(*m_view, firstRun()).WillRepeatedly(Return("LOQ49886.nxs")); // XXX: not a Muon file
    EXPECT_CALL(*m_view, setAvailableLogs(ElementsAre())); // Empty array expectedB
    TS_ASSERT_THROWS_NOTHING(m_view->selectFirstRun());
  }

  void test_load_error()
  {
    // TODO: with algorithm being executed asynchronously, check that errors are caught propertly
  }

  void test_load_nonExistentFile()
  {
    EXPECT_CALL(*m_view, firstRun()).WillRepeatedly(Return("MUSR00015189.nxs"));
    EXPECT_CALL(*m_view, lastRun()).WillRepeatedly(Return("non-existent-file"));
    EXPECT_CALL(*m_view, log()).WillRepeatedly(Return("sample_magn_field"));
    EXPECT_CALL(*m_view, setDataCurve(_)).Times(0);
    EXPECT_CALL(*m_view, displayError(_)).Times(1);

    m_view->requestLoading();
  }
};


#endif /* MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_ */
