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

namespace boost{
  template<class CharType, class CharTrait>
  std::basic_ostream<CharType, CharTrait>& operator<<(std::basic_ostream<CharType, CharTrait>& out, optional<std::pair<double,double> > const& maybe)
  {
    if (maybe)
        out << maybe->first << ", " << maybe->second;
    return out;
  }
}

class MockALCDataLoadingView : public IALCDataLoadingView
{
  // XXX: A workaround, needed because of the way the comma is treated in a macro
  typedef std::pair<double,double> PAIR_OF_DOUBLES;

public:
  MOCK_CONST_METHOD0(firstRun, std::string());
  MOCK_CONST_METHOD0(lastRun, std::string());
  MOCK_CONST_METHOD0(log, std::string());
  MOCK_CONST_METHOD0(calculationType, std::string());
  MOCK_CONST_METHOD0(timeRange, boost::optional<PAIR_OF_DOUBLES>());

  MOCK_METHOD0(initialize, void());
  MOCK_METHOD1(setDataCurve, void(const QwtData&));
  MOCK_METHOD1(displayError, void(const std::string&));
  MOCK_METHOD1(setAvailableLogs, void(const std::vector<std::string>&));
  MOCK_METHOD0(setWaitingCursor, void());
  MOCK_METHOD0(restoreCursor, void());

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

    // Set some valid default return values for the view mock object getters
    ON_CALL(*m_view, firstRun()).WillByDefault(Return("MUSR00015189.nxs"));
    ON_CALL(*m_view, lastRun()).WillByDefault(Return("MUSR00015191.nxs"));
    ON_CALL(*m_view, calculationType()).WillByDefault(Return("Integral"));
    ON_CALL(*m_view, log()).WillByDefault(Return("sample_magn_field"));
    ON_CALL(*m_view, timeRange()).WillByDefault(Return(boost::none));
  }

  void tearDown()
  {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
    delete m_presenter;
    delete m_view;
  }

  void test_initialize()
  {
    MockALCDataLoadingView view;
    ALCDataLoadingPresenter presenter(&view);
    EXPECT_CALL(view, initialize());
    presenter.initialize();
  }

  void test_defaultLoad()
  {
    InSequence s;
    EXPECT_CALL(*m_view, setWaitingCursor());

    EXPECT_CALL(*m_view, setDataCurve(AllOf(Property(&QwtData::size,3),
                                            QwtDataX(0, 1350, 1E-8),
                                            QwtDataX(1, 1360, 1E-8),
                                            QwtDataX(2, 1370, 1E-8),
                                            QwtDataY(0, 0.150, 1E-3),
                                            QwtDataY(1, 0.143, 1E-3),
                                            QwtDataY(2, 0.128, 1E-3))));

    EXPECT_CALL(*m_view, restoreCursor());

    m_view->requestLoading();
  }

  void test_load_differential()
  {
    // Change to differential calculation type
    ON_CALL(*m_view, calculationType()).WillByDefault(Return("Differential"));

    EXPECT_CALL(*m_view, setDataCurve(AllOf(Property(&QwtData::size,3),
                                            QwtDataY(0, 3.00349, 1E-3),
                                            QwtDataY(1, 2.3779, 1E-3),
                                            QwtDataY(2, 2.47935, 1E-3))));

    m_view->requestLoading();
  }

  void test_load_timeLimits()
  {
    // Set time limit
    ON_CALL(*m_view, timeRange()).WillByDefault(Return(boost::make_optional(std::make_pair(5.0,10.0))));

    EXPECT_CALL(*m_view, setDataCurve(AllOf(Property(&QwtData::size,3),
                                            QwtDataY(0, 0.137, 1E-3),
                                            QwtDataY(1, 0.141, 1E-3),
                                            QwtDataY(2, 0.111, 1E-3))));

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
    ON_CALL(*m_view, firstRun()).WillByDefault(Return(""));
    EXPECT_CALL(*m_view, setAvailableLogs(ElementsAre())); // Empty array expected
    TS_ASSERT_THROWS_NOTHING(m_view->selectFirstRun());
  }

  void test_updateAvailableLogs_unsupportedFirstRun()
  {
    ON_CALL(*m_view, firstRun()).WillByDefault(Return("LOQ49886.nxs")); // XXX: not a Muon file
    EXPECT_CALL(*m_view, setAvailableLogs(ElementsAre())); // Empty array expected
    TS_ASSERT_THROWS_NOTHING(m_view->selectFirstRun());
  }

  void test_load_error()
  {
    // Set last run to one of the different instrument - should cause error within algorithms exec
    ON_CALL(*m_view, lastRun()).WillByDefault(Return("EMU00006473.nxs"));
    EXPECT_CALL(*m_view, setDataCurve(_)).Times(0);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    m_view->requestLoading();
  }

  void test_load_invalidRun()
  {
    ON_CALL(*m_view, firstRun()).WillByDefault(Return(""));
    EXPECT_CALL(*m_view, setDataCurve(_)).Times(0);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    m_view->requestLoading();
  }

  void test_load_nonExistentFile()
  {
    ON_CALL(*m_view, lastRun()).WillByDefault(Return("non-existent-file"));
    EXPECT_CALL(*m_view, setDataCurve(_)).Times(0);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    m_view->requestLoading();
  }
};


#endif /* MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_ */
