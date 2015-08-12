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

    const std::string mockFname = "foo.par";
    EXPECT_CALL(mockView, askExistingCalibFilename()).Times(1).WillOnce(Return(mockFname));
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, mockFname))
        .Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::LoadExistingCalib);
  }

  void test_calcCalibWithoutRunNumbers() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    // No errors, 1 warning (no Vanadium, no Ceria run numbers given)
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    // does not update the current calibration as it must have failed
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, testing::_))
        .Times(0);

    pres.notify(IEnggDiffractionPresenter::CalcCalib);
  }

  void test_calcCalibWithRunNumbersButError() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    const std::string vanNo = "9999999999"; // use a number that won't be found!
    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(vanNo));

    const std::string ceriaNo =
        "9999999999"; // use a number that won't be found!
    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(ceriaNo));

    const std::string instr = "FAKE_INSTR";
    EXPECT_CALL(mockView, currentInstrument()).Times(1).WillOnce(Return(instr));

    const std::string filename = "fake_calib_filename.par";
    EXPECT_CALL(mockView,
                askNewCalibrationFilename("UNKNOWN_INST_" + vanNo + "_" +
                                          ceriaNo + "_both_banks.prm"))
        .Times(1)
        .WillOnce(Return(filename));

    // No warnings, 1 error: some exception(s) are thrown (because there are
    // missing settings and/or files) but these must be caught
    // and an error shown to the user
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(1);

    // does not update the current calibration as it must have failed
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, testing::_))
        .Times(0);

    TS_ASSERT_THROWS_NOTHING(pres.notify(IEnggDiffractionPresenter::CalcCalib));
  }

  // TODO: disabled for now, as this one would need to load files
  void disabled_test_calcCalibOK() {
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
