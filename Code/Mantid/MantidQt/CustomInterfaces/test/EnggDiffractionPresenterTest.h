#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"

#include <cxxtest/TestSuite.h>
#include "EnggDiffractionViewMock.h"

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

// Use this mocked presenter for tests that will start the calibration thread.
// Otherwise you'll run into trouble with issues like "QEventLoop: Cannot be
// used without QApplication", as there is not Qt application here and the
// normal Qt thread used by the presenter uses signals/slots.
class EnggDiffPresenterNoThread : public EnggDiffractionPresenter {
public:
  EnggDiffPresenterNoThread(IEnggDiffractionView *view)
      : EnggDiffractionPresenter(view) {}

private:
  // not async at all
  void startAsyncCalibWorker(const std::string &outFilename,
                            const std::string &vanNo,
                            const std::string &ceriaNo) {
    doNewCalibration(outFilename, vanNo, ceriaNo);
    calibrationFinished();
  }

  void startAsyncFocusWorker(const std::string &dir,
                             const std::string &outFilename,
                             const std::string &runNo, int bank) {
    doFocusRun(dir, outFilename, runNo, bank);
    focusingFinished();
  }
};

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

  // TODO: There should be a few basic tests on the presenter here, including
  // methods like: parseCalibrateFilename, buildCalibrateSuggestedFilename, etc.
  // Several of these are indirectly tested through some of the GUI-mock-based
  // tests below but should be tested as isolated methods here at the beginning.

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

  void test_loadExistingCalibWithWrongName() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    const std::string mockFname = "foo.par";
    EXPECT_CALL(mockView, askExistingCalibFilename()).Times(1).WillOnce(
        Return(mockFname));

    // should not get to the point where the calibration is calculated
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, mockFname))
        .Times(0);

    // Should show a warning but no errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::LoadExistingCalib);
  }

  void test_loadExistingCalibWithAcceptableName() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    const std::string mockFname = "ENGINX_111111_222222_foo_bar.par";
    EXPECT_CALL(mockView, askExistingCalibFilename()).Times(1).WillOnce(
        Return(mockFname));
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

    // would need basic calibration settings from the user, but it should not
    // get to that point because of early detected errors:
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    // No errors, 1 warning (no Vanadium, no Ceria run numbers given)
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    // does not update the current calibration as it must have failed
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, testing::_))
        .Times(0);

    pres.notify(IEnggDiffractionPresenter::CalcCalib);
  }

  // this can start the calibration thread, so watch out
  void test_calcCalibWithSettingsMissing() {
    testing::NiceMock<MockEnggDiffractionView> mockView;

    // this test can start a Qt thread that needs signals/slots
    // Don't do: MantidQt::CustomInterfaces::EnggDiffractionPresenter
    // pres(&mockView);
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    const std::string instr = "FAKEINSTR";
    const std::string vanNo = "9999999999"; // use a number that won't be found!
    const std::string ceriaNo =
        "9999999999"; // use a number that won't be found!

    // will need basic calibration settings from the user - but I forget to set
    // them
    EnggDiffCalibSettings calibSettings;

    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(vanNo));

    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(ceriaNo));

    // 1 warning because some required settings are missing/empty
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    // does not update the current calibration as it must have failed
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, testing::_))
        .Times(0);

    TS_ASSERT_THROWS_NOTHING(pres.notify(IEnggDiffractionPresenter::CalcCalib));
  }

  // this test actually starts the calibration process - which implies starting
  // the thread unless you use the mock without thread
  void test_calcCalibWithRunNumbersButError() {
    testing::NiceMock<MockEnggDiffractionView> mockView;

    // this test would start a Qt thread that needs signals/slots
    // Don't do: MantidQt::CustomInterfaces::EnggDiffractionPresenter
    // pres(&mockView);
    EnggDiffPresenterNoThread pres(&mockView);

    const std::string instr = "ENGINX";
    const std::string vanNo = "8899999988"; // use a number that won't be found!
    const std::string ceriaNo =
        "9999999999"; // use a number that won't be found!

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    calibSettings.m_pixelCalibFilename =
        instr + "_" + vanNo + "_" + ceriaNo + ".prm";
    calibSettings.m_templateGSAS_PRM = "fake.prm";
    EXPECT_CALL(mockView, currentCalibSettings()).Times(2).WillRepeatedly(
        Return(calibSettings));

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(vanNo));

    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(ceriaNo));

    EXPECT_CALL(mockView, currentInstrument()).Times(1).WillOnce(Return(instr));

    const std::string filename =
        "UNKNOWNINST_" + vanNo + "_" + ceriaNo + "_" + "foo.prm";
    EXPECT_CALL(mockView, askNewCalibrationFilename(
                              "UNKNOWNINST_" + vanNo + "_" + ceriaNo +
                              "_both_banks.prm")).Times(0);
    //  .WillOnce(Return(filename)); // if enabled ask user output filename

    // should disable actions at the beginning of the calculations
    EXPECT_CALL(mockView, enableCalibrateAndFocusActions(false)).Times(1);

    // and should enable them again at the (unsuccessful) end - this happens
    // when a separate thread finished (here the thread is mocked)
    EXPECT_CALL(mockView, enableCalibrateAndFocusActions(true)).Times(1);

    // No warnings/error pop-ups: some exception(s) are thrown (because there
    // are missing settings and/or files) but these must be caught
    // and error messages logged
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    // does not update the current calibration as it must have failed
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, testing::_))
        .Times(0);

    //TS_ASSERT_THROWS_NOTHING(pres.notify(IEnggDiffractionPresenter::CalcCalib));
    pres.notify(IEnggDiffractionPresenter::CalcCalib);
  }

  // TODO: disabled for now, as this one would need to load files
  void disabled_test_calcCalibOK() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(2).WillOnce(
        Return(calibSettings));

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::CalcCalib);
  }

  void test_focusWithoutRunNumber() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // empty run number!
    EXPECT_CALL(mockView, focusingRunNo()).Times(1).WillOnce(Return(""));
    // any bank will do
    EXPECT_CALL(mockView, focusingBank()).Times(1).WillOnce(Return(1));

    // should not get that far that it tries to get these parameters
    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
  }

  void test_focusWithRunNumberButWrongBank() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    EXPECT_CALL(mockView, focusingRunNo()).Times(1).WillOnce(Return("999999"));
    // negative bank number!
    EXPECT_CALL(mockView, focusingBank()).Times(1).WillOnce(Return(-34));

    // should not get that far that it tries to get these parameters
    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
  }

  // the focusing process starts but the input run number cannot be found
  void test_focusWithNumbersButError() {
    testing::NiceMock<MockEnggDiffractionView> mockView;

    // this test starts the focusing process which would start a Qt
    // thread that needs signals/slots Don't do:
    // MantidQt::CustomInterfaces::EnggDiffractionPresenter
    // pres(&mockView);
    EnggDiffPresenterNoThread pres(&mockView);

    // empty run number!
    EXPECT_CALL(mockView, focusingRunNo()).Times(1).WillOnce(Return("999999"));
    // any bank will do
    EXPECT_CALL(mockView, focusingBank()).Times(1).WillOnce(Return(1));

    // needs basic calibration settings from the user to start focusing
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    // it should not get there
    EXPECT_CALL(mockView, enableCalibrateAndFocusActions(false)).Times(0);
    EXPECT_CALL(mockView, enableCalibrateAndFocusActions(true)).Times(0);

    // 0 errors, 1 warning error pop-up to user
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
  }

  // TODO: disabled for now, as this one would need to load files
  void disabled_test_focusOK() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // an example run available in unit test data:
    EXPECT_CALL(mockView, focusingRunNo()).Times(1).WillOnce(Return("228061"));
    EXPECT_CALL(mockView, focusingBank()).Times(1).WillOnce(Return(1));

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1).WillOnce(
        Return(calibSettings));

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
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
