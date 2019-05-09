// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONPRESENTERTEST_H

#include "../EnggDiffraction/EnggDiffractionPresenter.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"

#include "EnggDiffFittingViewMock.h"
#include "EnggDiffractionViewMock.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::TypedEq;

// Use this mocked presenter for tests that will start the calibration,
// focusing, rebinning, etc. workers/threads.
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
                             const std::string &ceriaNo,
                             const std::string &specNos) override {
    doNewCalibration(outFilename, vanNo, ceriaNo, specNos);
    calibrationFinished();
  }

  void startAsyncFocusWorker(const std::vector<std::string> &multi_RunNo,
                             const std::vector<bool> &banks,
                             const std::string &specNos,
                             const std::string &dgFile) override {
    std::cerr << "focus run \n";

    std::string runNo = multi_RunNo[0];

    doFocusRun(runNo, banks, specNos, dgFile);

    focusingFinished();
  }

  void startAsyncRebinningTimeWorker(const std::string &runNo, double bin,
                                     const std::string &outWSName) override {
    doRebinningTime(runNo, bin, outWSName);
    rebinningFinished();
  }

  void startAsyncRebinningPulsesWorker(const std::string &runNo,
                                       size_t nperiods, double timeStep,
                                       const std::string &outWSName) override {
    doRebinningPulses(runNo, nperiods, timeStep, outWSName);
    rebinningFinished();
  }
};

class EnggDiffractionPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent tghe suite being created
  // statically
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

  void setUp() override {
    m_view.reset(new testing::NiceMock<MockEnggDiffractionView>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::EnggDiffractionPresenter(m_view.get()));

    // default banks
    m_ex_enginx_banks.push_back(true);
    m_ex_enginx_banks.push_back(false);

    // default run number
    m_ex_empty_run_num.emplace_back("");
    m_invalid_run_number.emplace_back("");
    m_ex_run_number.push_back(g_validRunNo);
    g_vanNo.emplace_back("8899999988");
    g_ceriaNo.emplace_back("9999999999");
    g_rebinRunNo.push_back(g_eventModeRunNo);

    // provide personal directories in order to carry out the full disable tests
    m_basicCalibSettings.m_inputDirCalib = "GUI_calib_folder/";
    m_basicCalibSettings.m_inputDirRaw = "GUI_calib_folder/";

    m_basicCalibSettings.m_pixelCalibFilename =
        "ENGINX_full_pixel_calibration.csv";

    m_basicCalibSettings.m_templateGSAS_PRM = "GUI_calib_folder/"
                                              "template_ENGINX_241391_236516_"
                                              "North_and_South_banks.prm";

    m_basicCalibSettings.m_forceRecalcOverwrite = false;
    m_basicCalibSettings.m_rebinCalibrate = 1;
  }

  void tearDown() override {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  // TODO: There should be a few basic tests on the presenter here, including
  // methods like: parseCalibrateFilename, buildCalibrateSuggestedFilename, etc.
  // Several of these are indirectly tested through some of the GUI-mock-based
  // tests below but should be tested as isolated methods here at the beginning.

  void test_start() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // should set a ready or similar status
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::Start);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_loadExistingCalibWithWrongName() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    const std::string mockFname = "foo.par";
    EXPECT_CALL(mockView, askExistingCalibFilename())
        .Times(1)
        .WillOnce(Return(mockFname));

    // should not get to the point where the calibration is calculated
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, mockFname))
        .Times(0);

    // Should show a warning but no errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::LoadExistingCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_loadExistingCalibWithAcceptableName() {
    testing::NiceMock<MockEnggDiffractionView> mockView;

    // by setting this here, when initialising the presenter, the function will
    // be called and the instrument name will be updated
    const std::string instrumentName = "ENGINX";
    EXPECT_CALL(mockView, currentInstrument())
        .Times(2)
        .WillRepeatedly(Return(instrumentName));

    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // update the selected instrument
    const std::string mockFname = "ENGINX_111111_222222_foo_bar.par";
    EXPECT_CALL(mockView, askExistingCalibFilename())
        .Times(1)
        .WillOnce(Return(mockFname));

    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, mockFname))
        .Times(1);

    EXPECT_CALL(mockView, plotCalibWorkspace()).Times(0);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::LoadExistingCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_calcCalibWithoutRunNumbers() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // would need basic calibration settings from the user, but it should not
    // get to that point because of early detected errors:
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    EXPECT_CALL(mockView, newVanadiumNo())
        .Times(1)
        .WillOnce(Return(m_ex_empty_run_num));

    EXPECT_CALL(mockView, newCeriaNo())
        .Times(1)
        .WillOnce(Return(m_ex_empty_run_num));

    // No errors, 1 warning (no Vanadium, no Ceria run numbers given)
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    // does not update the current calibration as it must have failed
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, testing::_))
        .Times(0);

    pres.notify(IEnggDiffractionPresenter::CalcCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_calcCalibFailsWhenNoCalibDirectory() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    EnggDiffPresenterNoThread pres(&mockView);

    EnggDiffCalibSettings calibSettings;
    calibSettings.m_inputDirCalib = "";
    calibSettings.m_pixelCalibFilename = "/some/file.csv";
    calibSettings.m_templateGSAS_PRM = "/some/other/file.prm";

    const std::string testFilename("ENGINX00241391.nxs");
    const auto testFilePath =
        Mantid::API::FileFinder::Instance().getFullPath(testFilename);

    ON_CALL(mockView, newVanadiumNo())
        .WillByDefault(Return(std::vector<std::string>({testFilePath})));
    ON_CALL(mockView, newCeriaNo())
        .WillByDefault(Return(std::vector<std::string>({testFilePath})));
    ON_CALL(mockView, currentCalibSettings())
        .WillByDefault(Return(calibSettings));

    EXPECT_CALL(mockView, userWarning("Calibration Error", testing::_));

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

    EXPECT_CALL(mockView, currentCalibSettings()).Times(1);

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));

    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(g_ceriaNo));

    // 1 warning because some required settings are missing/empty
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    // does not update the current calibration as it must have failed
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, testing::_))
        .Times(0);

    TS_ASSERT_THROWS_NOTHING(pres.notify(IEnggDiffractionPresenter::CalcCalib));
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
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
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1);

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));

    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(g_ceriaNo));

    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    // if it got here, it would: .WillOnce(Return(instr));

    const std::string filename =
        "UNKNOWNINST_" + vanNo + "_" + ceriaNo + "_" + "foo.prm";
    EXPECT_CALL(mockView,
                askNewCalibrationFilename("UNKNOWNINST_" + vanNo + "_" +
                                          ceriaNo + "_both_banks.prm"))
        .Times(0);
    //  .WillOnce(Return(filename)); // if enabled ask user output filename

    // Should not try to use options for focusing
    EXPECT_CALL(mockView, focusingRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile()).Times(0);

    // only after the error, it should disable actions at the beginning of the
    // calculations
    EXPECT_CALL(mockView, enableCalibrateFocusFitUserActions(false)).Times(0);

    // and only after the error it should enable them again at the
    // (unsuccessful) end - this happens when a separate thread
    // finished (here the thread is mocked)
    EXPECT_CALL(mockView, enableCalibrateFocusFitUserActions(true)).Times(0);

    // plots peaks and curves
    // the test doesnt get to here as it finishes at EnggCalibrate algo
    EXPECT_CALL(mockView, plotCalibWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotCalibOutput(testing::_)).Times(0);

    // A warning about the vanadium number, and what it should look like
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    // does not update the current calibration as it must have failed
    EXPECT_CALL(mockView, newCalibLoaded(testing::_, testing::_, testing::_))
        .Times(0);

    // TS_ASSERT_THROWS_NOTHING(pres.notify(IEnggDiffractionPresenter::CalcCalib));
    pres.notify(IEnggDiffractionPresenter::CalcCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // TODO: disabled for now, as this one would need to load files
  void disable_test_calcCalibOK() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EXPECT_CALL(mockView, currentCalibSettings())
        .Times(1)
        .WillOnce(Return(m_basicCalibSettings));

    // As this is a positive test, personal directory/files should be
    // provided here instead
    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));
    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(g_ceriaNo));

    // plots peaks and curves
    // the test doesnt get to here as it finishes at EnggCalibrate algo
    EXPECT_CALL(mockView, plotCalibWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotCalibOutput(testing::_)).Times(0);
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::CalcCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // This would test the cropped calibration with no cerial number
  // which should produce a warning
  void test_calcCroppedCalibWithoutRunNumbers() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // would need basic calibration settings from the user, but it should not
    // get to that point because of early detected errors:
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));

    EXPECT_CALL(mockView, newCeriaNo())
        .Times(1)
        .WillOnce(Return(m_ex_empty_run_num));

    // No errors, 1 warning (no Vanadium, no Ceria run numbers given)
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::CropCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // this can start the cropped calibration thread, so watch out
  // the test provide gui with missing calib settings
  // which should return a single error
  void test_calcCroppedCalibWithSettingsMissing() {
    testing::NiceMock<MockEnggDiffractionView> mockView;

    // this test can start a Qt thread that needs signals/slots
    // Don't do: MantidQt::CustomInterfaces::EnggDiffractionPresenter
    // pres(&mockView);
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    const std::string instr = "FAKEINSTR";
    const std::string vanNo = "9999999999"; // use a number that won't be found!
    const std::string ceriaNo =
        "9999999999"; // use a number that won't be found!

    EnggDiffCalibSettings calibSettings;

    // doesn't get here
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1);

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));

    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(g_ceriaNo));

    // 1 warning because some required settings are missing/empty
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    TS_ASSERT_THROWS_NOTHING(pres.notify(IEnggDiffractionPresenter::CropCalib));
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // This should not start the process, tests with an empty spec number which
  // should generate a user warning that spec number is missing
  void test_calcCroppedCalibWithEmptySpec() {
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
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1);

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));

    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(g_ceriaNo));

    std::string specno = "";
    EXPECT_CALL(mockView, currentCalibSpecNos()).Times(0);
    // if it got here, it would: .WillOnce(Return(specno));

    EXPECT_CALL(mockView, currentCalibCustomisedBankName()).Times(0);

    // No warnings/error pop-ups: some exception(s) are thrown (because there
    // are missing settings and/or files) but these must be caught
    // and error messages logged
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::CropCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // this test actually starts the cropped calibration process - which implies
  // starting
  // the thread unless you use the mock without thread
  // this will utlise the bank name north and not still carry out the cropped
  // calibration process normal
  void test_calcCroppedCalibWithBankName() {
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
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1);
    // if it got here it would: .WillRepeatedly(Return(calibSettings));

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));

    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(g_ceriaNo));

    // North bank selected so the spectrum Number will not be called and
    // process should carry on without spec no input
    EXPECT_CALL(mockView, currentCropCalibBankName())
        .Times(1)
        .WillOnce(Return(1));

    const std::string filename =
        "UNKNOWNINST_" + vanNo + "_" + ceriaNo + "_" + "foo.prm";
    EXPECT_CALL(mockView,
                askNewCalibrationFilename("UNKNOWNINST_" + vanNo + "_" +
                                          ceriaNo + "_both_banks.prm"))
        .Times(0);
    //  .WillOnce(Return(filename)); // if enabled ask user output filename

    // with the normal thread should disable actions at the beginning
    // of the calculations
    EXPECT_CALL(mockView, enableCalibrateFocusFitUserActions(false)).Times(0);

    // and should enable them again at the (unsuccessful) end - this happens
    // when a separate thread finished (here the thread is mocked)
    EXPECT_CALL(mockView, enableCalibrateFocusFitUserActions(true)).Times(0);

    // tests whether the plot functions have been called
    EXPECT_CALL(mockView, plotCalibWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotCalibOutput(testing::_)).Times(0);

    // A warning about the vanadium run number
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::CropCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // this test actually starts the cropped calibration process - which implies
  // starting the thread unless you use the mock without thread
  // this test case includes all valid settings, run numbers, spectrum no
  // selected & valid spectrum no provided
  void test_calcCroppedCalibWithRunNumbers() {
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
    EXPECT_CALL(mockView, currentCalibSettings()).Times(1);
    // if it was called it would: .WillRepeatedly(Return(calibSettings));

    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));

    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(g_ceriaNo));

    EXPECT_CALL(mockView, currentCropCalibBankName())
        .Times(1)
        .WillOnce(Return(0));

    std::string specno = "100-200";
    EXPECT_CALL(mockView, currentCalibSpecNos()).Times(0);
    // if it was called it would: .WillRepeatedly(Return(specno));

    EXPECT_CALL(mockView, currentCalibCustomisedBankName()).Times(0);

    const std::string filename =
        "UNKNOWNINST_" + vanNo + "_" + ceriaNo + "_" + "foo.prm";
    EXPECT_CALL(mockView,
                askNewCalibrationFilename("UNKNOWNINST_" + vanNo + "_" +
                                          ceriaNo + "_both_banks.prm"))
        .Times(0);
    //  .WillOnce(Return(filename)); // if enabled ask user output filename

    // Should not try to use options for focusing
    EXPECT_CALL(mockView, focusingRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile()).Times(0);

    // it will not get to the next steps:

    // should disable actions at the beginning of the calculations
    EXPECT_CALL(mockView, enableCalibrateFocusFitUserActions(false)).Times(0);

    // and should enable them again at the (unsuccessful) end - this happens
    // when a separate thread finished (here the thread is mocked)
    EXPECT_CALL(mockView, enableCalibrateFocusFitUserActions(true)).Times(0);

    // A warning about the vanadium
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::CropCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // TODO: disabled for now, as this one would need to load files
  void disable_test_calcCropCalibOK() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // will need basic calibration settings from the user
    EXPECT_CALL(mockView, currentCalibSettings())
        .Times(1)
        .WillOnce(Return(m_basicCalibSettings));

    // As this is a positive test, personal directory/files should be
    // provided here instead
    EXPECT_CALL(mockView, newVanadiumNo()).Times(1).WillOnce(Return(g_vanNo));
    EXPECT_CALL(mockView, newCeriaNo()).Times(1).WillOnce(Return(g_ceriaNo));

    EXPECT_CALL(mockView, currentCropCalibBankName())
        .Times(1)
        .WillOnce(Return(0));

    std::string specno = "100-200";
    EXPECT_CALL(mockView, currentCalibSpecNos())
        .Times(2)
        .WillRepeatedly(Return(specno));

    EXPECT_CALL(mockView, currentCalibCustomisedBankName()).Times(0);

    EXPECT_CALL(mockView, showStatus(testing::_)).Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::CropCalib);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_focusWithoutRunNumber() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // empty run number!
    EXPECT_CALL(mockView, focusingRunNo())
        .Times(1)
        .WillOnce(Return(m_invalid_run_number));
    EXPECT_CALL(mockView, focusingBanks())
        .Times(1)
        .WillOnce(Return(m_ex_enginx_banks));

    // should not try to use these ones
    EXPECT_CALL(mockView, focusingCroppedRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile()).Times(0);
    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // should not get that far that it tries to get these parameters
    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_focusWithRunNumberButWrongBanks() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    EXPECT_CALL(mockView, focusingRunNo())
        .Times(1)
        .WillOnce(Return(m_invalid_run_number));
    // missing bank on/off vector!
    std::vector<bool> banks{false, false};
    EXPECT_CALL(mockView, focusingBanks()).Times(1).WillOnce(Return(banks));

    // would needs basic calibration settings, but only if there was at least
    // one bank selected
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    // should not get that far that it tries to get these parameters
    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // the focusing process starts but the input run number cannot be found
  void test_focusWithNumbersButError() {
    testing::NiceMock<MockEnggDiffractionView> mockView;

    // this test starts the focusing process which would start a Qt
    // thread that needs signals/slots Don't do:
    // MantidQt::CustomInterfaces::EnggDiffractionPresenter
    // pres(&mockView);
    EnggDiffPresenterNoThread pres(&mockView);

    // wrong run number!
    EXPECT_CALL(mockView, focusingRunNo())
        .Times(1)
        .WillOnce(Return(m_invalid_run_number));
    EXPECT_CALL(mockView, focusingBanks())
        .Times(1)
        .WillOnce(Return(m_ex_enginx_banks));

    // Should not try to use options for other types of focusing
    EXPECT_CALL(mockView, focusingCroppedRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile()).Times(0);
    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // it should not get there
    EXPECT_CALL(mockView, enableCalibrateFocusFitUserActions(false)).Times(0);
    EXPECT_CALL(mockView, enableCalibrateFocusFitUserActions(true)).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 0 errors, 1 warning error pop-up to user
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // TODO: disabled for now, as this one would need to load files
  void disable_test_focusOK() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    EnggDiffPresenterNoThread pres(&mockView);

    const std::string instr = "ENGINX";
    const std::string vanNo = "236516"; // use a number that can be found!

    // an example run available in unit test data:
    EXPECT_CALL(mockView, focusingRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_run_number));
    std::vector<bool> banks{true, false};
    EXPECT_CALL(mockView, focusingBanks()).Times(1).WillOnce(Return(banks));

    EXPECT_CALL(mockView, currentInstrument())
        .Times(2)
        .WillRepeatedly(Return(instr));

    EXPECT_CALL(mockView, currentCalibSettings())
        .Times(2)
        .WillRepeatedly(Return(m_basicCalibSettings));

    // when two banks are used then it will utlise currentVanadiumNo two times
    EXPECT_CALL(mockView, currentVanadiumNo()).Times(1).WillOnce(Return(vanNo));

    // it will not reach here on wards, would finish with a Warning message
    // "The Calibration did not finish correctly"

    // this is because of the python algorithm cannot be used with in c++ test

    // the test will not be able to read the python algorithm from here on
    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);

    // Should not try to use options for other types of focusing
    EXPECT_CALL(mockView, focusingCroppedRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile()).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 0 errors/ 0 warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void disabled_test_focusOK_allBanksOff() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // an example run available in unit test data:
    EXPECT_CALL(mockView, focusingRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_run_number));
    std::vector<bool> banks{false, false};
    EXPECT_CALL(mockView, focusingBanks()).Times(1).WillOnce(Return(banks));

    // will need basic calibration settings from the user
    EnggDiffCalibSettings calibSettings;
    EXPECT_CALL(mockView, currentCalibSettings())
        .Times(1)
        .WillOnce(Return(m_basicCalibSettings));

    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_focusCropped_withoutRunNo() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // empty run number!
    EXPECT_CALL(mockView, focusingCroppedRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_empty_run_num));
    EXPECT_CALL(mockView, focusingBanks())
        .Times(1)
        .WillOnce(Return(m_ex_enginx_banks));
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos())
        .Times(1)
        .WillOnce(Return("1"));

    // should not try to use these ones
    EXPECT_CALL(mockView, focusingRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile()).Times(0);

    // should not get that far that it tries to get these parameters
    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);
    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusCropped);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_focusCropped_withoutBanks() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // ok run number
    EXPECT_CALL(mockView, focusingCroppedRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_run_number));
    EXPECT_CALL(mockView, focusingBanks())
        .Times(1)
        .WillOnce(Return(std::vector<bool>()));
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos())
        .Times(1)
        .WillOnce(Return("1,5"));

    // should not try to use these ones
    EXPECT_CALL(mockView, focusingRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile()).Times(0);
    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // should not get that far that it tries to get these parameters
    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusCropped);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_focusCropped_withoutSpectrumNos() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // ok run number
    EXPECT_CALL(mockView, focusingCroppedRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_run_number));
    EXPECT_CALL(mockView, focusingBanks())
        .Times(1)
        .WillOnce(Return(m_ex_enginx_banks));
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos())
        .Times(1)
        .WillOnce(Return(""));

    // should not try to use these ones
    EXPECT_CALL(mockView, focusingRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile()).Times(0);
    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // should not get that far that it tries to get these parameters
    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusCropped);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_focusTexture_withoutRunNo() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // empty run number!
    EXPECT_CALL(mockView, focusingTextureRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_empty_run_num));
    EXPECT_CALL(mockView, focusingTextureGroupingFile())
        .Times(1)
        .WillOnce(Return(""));

    // should not try to use these ones
    EXPECT_CALL(mockView, focusingRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingBanks()).Times(0);

    EXPECT_CALL(mockView, focusingCroppedRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos()).Times(0);

    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusTexture);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_focusTexture_withoutFilename() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // goo run number
    EXPECT_CALL(mockView, focusingTextureRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_run_number));
    EXPECT_CALL(mockView, focusingBanks()).Times(0);
    EXPECT_CALL(mockView, focusingTextureGroupingFile())
        .Times(1)
        .WillOnce(Return(""));

    // should not try to use these ones
    EXPECT_CALL(mockView, focusingRunNo()).Times(0);

    EXPECT_CALL(mockView, focusingCroppedRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos()).Times(0);

    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusTexture);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_focusTexture_withInexistentTextureFile() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // goo run number
    EXPECT_CALL(mockView, focusingTextureRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_run_number));
    // non empty but absurd csv file of detector groups
    EXPECT_CALL(mockView, focusingTextureGroupingFile())
        .Times(1)
        .WillOnce(Return("i_dont_exist_dont_look_for_me.csv"));

    // should not try to use these ones
    EXPECT_CALL(mockView, focusingRunNo()).Times(0);

    EXPECT_CALL(mockView, focusingCroppedRunNo()).Times(0);
    EXPECT_CALL(mockView, focusingCroppedSpectrumNos()).Times(0);

    EXPECT_CALL(mockView, focusedOutWorkspace()).Times(0);
    EXPECT_CALL(mockView, plotFocusedSpectrum(testing::_)).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // 1 warning pop-up to user, 0 errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusTexture);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_resetFocus() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    EXPECT_CALL(mockView, resetFocus()).Times(1);

    // No errors/warnings when resetting options
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::ResetFocus);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_resetFocus_thenFocus() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // No errors/warnings when resetting options
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::ResetFocus);

    // empty run number!
    EXPECT_CALL(mockView, focusingRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_empty_run_num));
    EXPECT_CALL(mockView, focusingBanks())
        .Times(1)
        .WillOnce(Return(m_ex_enginx_banks));

    // should not get that far that it tries to get these parameters
    EXPECT_CALL(mockView, currentInstrument()).Times(0);
    EXPECT_CALL(mockView, currentCalibSettings()).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // Now one error shown as a warning-pop-up cause inputs and options are
    // empty
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::FocusRun);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_preproc_event_time_bin_missing_runno() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // inputs from user
    EXPECT_CALL(mockView, currentPreprocRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_empty_run_num));
    EXPECT_CALL(mockView, rebinningTimeBin()).Times(1).WillOnce(Return(0));

    // No errors/1 warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::RebinTime);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_preproc_event_time_wrong_bin() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // inputs from user
    EXPECT_CALL(mockView, currentPreprocRunNo())
        .Times(1)
        .WillOnce(Return(g_rebinRunNo));
    EXPECT_CALL(mockView, rebinningTimeBin()).Times(1).WillOnce(Return(0));

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // An error, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::RebinTime);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // this test does run Load and then Rebin //
  void test_preproc_event_time_ok() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    EnggDiffPresenterNoThread pres(&mockView);
    // inputs from user
    EXPECT_CALL(mockView, currentPreprocRunNo())
        .Times(2)
        .WillRepeatedly(Return(g_rebinRunNo));

    EXPECT_CALL(mockView, rebinningTimeBin())
        .Times(1)
        .WillRepeatedly(Return(0.100000));

    // doesn't effectively finish the processing
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(2);

    // A warning complaining about the run number / path Because the
    // real QtView uses MWRunFile::getFilenames, if we give a run
    // number (ENGINX00228061) without path, it will find the full
    // path if possible. Here we mock currentPreprocRunNo() by just
    // returning the run number without the path that would be
    // needed. EnggDiffractionPresenter::isValidRunNumber() will not
    // find the file (when it tries Poco::File(path).exists()).
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::RebinTime);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_preproc_event_multiperiod_missing_runno() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // inputs from user
    EXPECT_CALL(mockView, currentPreprocRunNo())
        .Times(1)
        .WillOnce(Return(m_ex_empty_run_num));
    // should not even call this one when the run number is obviously wrong
    EXPECT_CALL(mockView, rebinningTimeBin()).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::RebinMultiperiod);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_preproc_event_multiperiod_wrong_bin() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // inputs from user
    EXPECT_CALL(mockView, currentPreprocRunNo())
        .Times(1)
        .WillOnce(Return(g_rebinRunNo));
    EXPECT_CALL(mockView, rebinningPulsesNumberPeriods())
        .Times(1)
        .WillOnce(Return(1));
    EXPECT_CALL(mockView, rebinningPulsesTime()).Times(1).WillOnce(Return(0));

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors, warning because of wrong bin
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::RebinMultiperiod);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // this test does run Load but then RebinByPulseTimes should fail
  void test_preproc_event_multiperiod_file_wrong_type() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    EnggDiffPresenterNoThread pres(&mockView);

    // inputs from user
    // This file will be found but it is not a valid file for this re-binning
    EXPECT_CALL(mockView, currentPreprocRunNo())
        .Times(1)
        .WillOnce(Return(g_rebinRunNo));
    EXPECT_CALL(mockView, rebinningPulsesNumberPeriods())
        .Times(1)
        .WillOnce(Return(0.100000));
    // 1s is big enough
    EXPECT_CALL(mockView, rebinningPulsesTime()).Times(1).WillOnce(Return(1));

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/warnings. There will be an error log from the algorithms
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::RebinMultiperiod);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_logMsg() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    std::vector<std::string> sv;
    sv.emplace_back("dummy log");
    EXPECT_CALL(mockView, logMsgs()).Times(1).WillOnce(Return(sv));

    // should not change status
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::LogMsg);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_RBNumberChange_ok() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // as if the user has set an empty RB Number that looks correct
    EXPECT_CALL(mockView, getRBNumber()).Times(1).WillOnce(Return("RB000xxxx"));
    EXPECT_CALL(mockView, enableTabs(true)).Times(1);

    // should update status
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(1);

    // no errors/ warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::RBNumberChange);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_RBNumberChange_empty() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // as if the user has set an empty RB Number
    EXPECT_CALL(mockView, getRBNumber()).Times(1).WillOnce(Return(""));
    EXPECT_CALL(mockView, enableTabs(false)).Times(1);

    // should update status / invalid
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(1);

    // no errors/ warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::RBNumberChange);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_instChange() {
    testing::NiceMock<MockEnggDiffractionView> mockView;

    // by setting this here, when initialising the presenter, the function will
    // be called and the instrument name will be updated
    const std::string instrumentName = "ENGINX";

    // we are calling it twice, once on presenter initialisation
    // and a second time after when using pres.notify!
    EXPECT_CALL(mockView, currentInstrument())
        .Times(3)
        .WillRepeatedly(Return(instrumentName));

    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    // we don't expect any warnings or errors
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    // should not change status
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);
    EXPECT_CALL(mockView, updateTabsInstrument(testing::_)).Times(1);

    pres.notify(IEnggDiffractionPresenter::InstrumentChange);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_shutDown() {
    testing::NiceMock<MockEnggDiffractionView> mockView;
    MantidQt::CustomInterfaces::EnggDiffractionPresenter pres(&mockView);

    EXPECT_CALL(mockView, showStatus(testing::_)).Times(1);

    EXPECT_CALL(mockView, saveSettings()).Times(1);
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffractionPresenter::ShutDown);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

private:
  boost::scoped_ptr<testing::NiceMock<MockEnggDiffractionView>> m_view;
  boost::scoped_ptr<MantidQt::CustomInterfaces::EnggDiffractionPresenter>
      m_presenter;

  std::vector<bool> m_ex_enginx_banks;
  const static std::string g_eventModeRunNo;
  const static std::string g_validRunNo;
  const static std::string g_focusedRun;
  EnggDiffCalibSettings m_basicCalibSettings;

  std::vector<std::string> m_ex_empty_run_num;
  std::vector<std::string> m_invalid_run_number;
  std::vector<std::string> m_ex_run_number;
  std::vector<std::string> g_vanNo;
  std::vector<std::string> g_ceriaNo;
  std::vector<std::string> g_rebinRunNo;
};

// Note this is not a correct event mode run number. Using it here just
// as a run number that is found.
// A possible event mode file would be: 197019, but it is too big for
// unit test data. TODO: find a small one or crop a big one.
const std::string EnggDiffractionPresenterTest::g_eventModeRunNo =
    "ENGINX00228061"; // could also be given as "ENGINX228061"

const std::string EnggDiffractionPresenterTest::g_validRunNo = "228061";

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFRACTIONPRESENTERTEST_H
