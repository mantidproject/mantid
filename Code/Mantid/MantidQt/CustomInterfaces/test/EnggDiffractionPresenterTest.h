#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"

#include <cxxtest/TestSuite.h>
#include "EnggDiffractionViewMock.h"

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

class EnggDiffractionPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EnggDiffractionPresenterTest *createSuite() {
    return new EnggDiffractionPresenterTest();
  }

  static void destroySuite(EnggDiffractionPresenterTest *suite) {
    delete suite;
  }

  EnggDiffractionPresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure framework is
                                               // initialized
  }

  void setUp() {
    m_view.reset(new testing::NiceMock<MockEnggDiffractionView>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::EnggDiffractionPresenter(m_view.get()));
  }

  void tearDown() {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_start() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    std::vector<std::string> sv;
    sv.push_back("dummy msg");
    EXPECT_CALL(mockView, logMsgs()).Times(1).WillOnce(Return(sv));

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::LogMsg);
  }

  void test_loadExistingCalib() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::LoadExistingCalib);
  }

  void test_calcCalib() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::CalcCalib);
  }

  void test_logMsg() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    std::vector<std::string> sv;
    sv.push_back("dummy log");
    EXPECT_CALL(mockView, logMsgs()).Times(1).WillOnce(Return(sv));

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::LogMsg);
  }

  void test_instChange() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // 1 error, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::InstrumentChange);
  }

  void test_shutDown() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    EXPECT_CALL(mockView, saveSettings()).Times(1);
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::ShutDown);
  }

private:
  boost::scoped_ptr<testing::NiceMock<MockEnggDiffractionView>> m_view;
  boost::scoped_ptr<MantidQt::CustomInterfaces::EnggDiffractionPresenter>
      m_presenter;
};

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONPRESENTERTEST_H
