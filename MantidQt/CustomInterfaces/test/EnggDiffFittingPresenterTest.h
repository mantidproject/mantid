#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffFittingPresenter.h"

#include "EnggDiffFittingViewMock.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

// Use this mocked presenter for tests that will start the focusing
// workers/threads. Otherwise you'll run into trouble with issues like
// "QEventLoop: Cannot be used without QApplication", as there is not
// Qt application here and the normal Qt thread used by the presenter
// uses signals/slots.
class EnggDiffFittingPresenterNoThread : public EnggDiffFittingPresenter {
public:
  EnggDiffFittingPresenterNoThread(IEnggDiffFittingView *view)
      : EnggDiffFittingPresenter(view, nullptr, nullptr) {}

private:
  // not async at all
  void startAsyncFittingWorker(const std::vector<std::string> &focusedRunNo,
                               const std::string &ExpectedPeaks) override {

    std::string runNo = focusedRunNo[0];
    doFitting(runNo, ExpectedPeaks);
    fittingFinished();
  }
};

class EnggDiffFittingPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent tghe suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static EnggDiffFittingPresenterTest *createSuite() {
    return new EnggDiffFittingPresenterTest();
  }

  static void destroySuite(EnggDiffFittingPresenterTest *suite) {
    delete suite;
  }

  EnggDiffFittingPresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure framework is
                                               // initialized
  }

  void setUp() override {
    m_view.reset(new testing::NiceMock<MockEnggDiffFittingView>());
    m_presenter.reset(new MantidQt::CustomInterfaces::EnggDiffFittingPresenter(
        m_view.get(), nullptr, nullptr));

    // default banks
    m_ex_enginx_banks.push_back(true);
    m_ex_enginx_banks.push_back(false);

    // default run number
    m_ex_empty_run_num.emplace_back("");
    m_invalid_run_number.emplace_back("");
    m_ex_run_number.push_back(g_validRunNo);
    g_vanNo.emplace_back("8899999988");
    g_ceriaNo.emplace_back("9999999999");

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

  void test_load_with_missing_param() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    MantidQt::CustomInterfaces::EnggDiffFittingPresenter pres(&mockView,
                                                              nullptr, nullptr);

    EXPECT_CALL(mockView, getFittingRunNo()).Times(1).WillOnce(Return(""));

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(1);

    // No errors/1 warnings. There will be an error log from the algorithms
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::Load);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_fitting_with_missing_param() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    MantidQt::CustomInterfaces::EnggDiffFittingPresenter pres(&mockView,
                                                              nullptr, nullptr);

    EXPECT_CALL(mockView, getFittingRunNo()).Times(1).WillOnce(Return(""));
    EXPECT_CALL(mockView, fittingPeaksData()).Times(1).WillOnce(Return(""));

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/1 warnings. There will be an error log from the algorithms
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::FitPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // This would test the fitting tab with no focused workspace
  // which should produce a warning
  void test_fitting_without_focused_run() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    // inputs from user
    const std::string mockFname = "";
    EXPECT_CALL(mockView, getFittingRunNo())
        .Times(1)
        .WillOnce(Return(mockFname));
    EXPECT_CALL(mockView, fittingPeaksData())
        .Times(1)
        .WillOnce(Return("2.57,,4.88,5.78"));

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(1);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/1 warnings. There will be an error log from the algorithms
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::FitPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // This would test the fitting tab with invalid expected peaks but should only
  // produce a warning
  void test_fitting_with_invalid_expected_peaks() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    // inputs from user
    EXPECT_CALL(mockView, getFittingRunNo())
        .Times(1)
        .WillOnce(Return(g_focusedRun));
    EXPECT_CALL(mockView, fittingPeaksData())
        .Times(1)
        .WillOnce(Return(",3.5,7.78,r43d"));
    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(1);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/1 warnings. There will be an error log from the algorithms
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::FitPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // Fitting test begin here
  void test_fitting_runno_valid_single_run() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    // inputs from user
    EXPECT_CALL(mockView, getFittingRunNo())
        .Times(1)
        .WillRepeatedly(Return(std::string(g_focusedBankFile)));

    EXPECT_CALL(mockView, getFittingRunNumVec()).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/0 warnings. There will be no errors or warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(2);

    pres.notify(IEnggDiffFittingPresenter::FittingRunNo);
  }

  void test_fitting_runno_invalid_run() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    // inputs from user - invalid run given this can't be numerical
    // only as that has the chance of matching a file so use a prefix
    EXPECT_CALL(mockView, getFittingRunNo())
        .Times(1)
        .WillOnce(Return(std::string("ENGINX1")));

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);
    EXPECT_CALL(mockView, getFittingRunNumVec()).Times(0);

    // No errors/1 warnings. There will be an warning for invalid run number
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(2);

    pres.notify(IEnggDiffFittingPresenter::FittingRunNo);
  }

  void test_fitting_with_blank_input() {
    testing::StrictMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, getFittingRunNo())
        .Times(1)
        .WillOnce(Return(std::string("")));

    pres.notify(IEnggDiffFittingPresenter::FittingRunNo);

    testing::Mock::VerifyAndClearExpectations(&mockView);
  }

  void test_fitting_file_not_found_with_multiple_runs() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);
    // 23931-23934
    std::vector<std::string> RunNumDir;
    RunNumDir.emplace_back("241391");
    RunNumDir.emplace_back("241392");
    RunNumDir.emplace_back("241393");
    RunNumDir.emplace_back("241394");

    // empty vector
    std::vector<std::string> splittedFileVec;

    // inputs from user - given multiple run
    EXPECT_CALL(mockView, getFittingRunNo())
        .Times(1)
        .WillOnce(Return(g_focusedFittingRunNo));

    EXPECT_CALL(mockView, getFittingRunNumVec()).Times(0);

    // SplitFittingDir()

    // could possibly feature to create unique path
    EXPECT_CALL(mockView, focusingDir()).Times(1);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/1 warnings. The warning will be produced because there
    // is no focus output directory within the settings tab
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::FittingRunNo);
  }

  void diable_test_fitting_runno_single_run() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    // focus directory need to be set for this in the settings

    // 23931-23934
    std::vector<std::string> RunNumDir;
    RunNumDir.emplace_back("241391");

    // empty vector
    std::vector<std::string> splittedFileVec;

    // inputs from user - given multiple run
    EXPECT_CALL(mockView, getFittingRunNo())
        .Times(2)
        .WillRepeatedly(Return("241391"));

    EXPECT_CALL(mockView, getFittingRunNumVec())
        .Times(1)
        .WillOnce(Return(RunNumDir));

    EXPECT_CALL(mockView, getFittingMultiRunMode())
        .Times(1)
        .WillOnce(Return(false));

    EXPECT_CALL(mockView, setFittingRunNumVec(testing::_)).Times(1);

    EXPECT_CALL(mockView, addRunNoItem(testing::_)).Times(1);

    EXPECT_CALL(mockView, addBankItem(testing::_)).Times(1);

    EXPECT_CALL(mockView, focusingDir()).Times(0);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/0 warnings.
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::FittingRunNo);
  }

  void test_fitting_runno_browsed_run_add_run_item() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);
    // Tests the browse directory file
    std::vector<std::string> RunNumDir;
    RunNumDir.emplace_back("241395");

    std::vector<std::string> splittedFileVec;
    splittedFileVec.emplace_back("ENGINX");
    splittedFileVec.emplace_back("241395");
    splittedFileVec.emplace_back("focused");
    splittedFileVec.emplace_back("bank");
    splittedFileVec.emplace_back("1");

    // inputs from user - given multiple run
    EXPECT_CALL(mockView, getFittingRunNo())
        .Times(1)
        .WillOnce(Return(g_focusedBankFile));

    EXPECT_CALL(mockView, getFittingRunNumVec()).Times(0);

    EXPECT_CALL(mockView, getFittingMultiRunMode()).Times(0);

    EXPECT_CALL(mockView, setFittingRunNumVec(testing::_)).Times(0);

    EXPECT_CALL(mockView, addBankItem(testing::_)).Times(0);

    EXPECT_CALL(mockView, setBankIdComboBox(testing::_)).Times(0);

    EXPECT_CALL(mockView, addRunNoItem(testing::_)).Times(0);

    EXPECT_CALL(mockView, setFittingListWidgetCurrentRow(testing::_)).Times(0);

    EXPECT_CALL(mockView, focusingDir()).Times(0);

    // No errors/1 warnings. File entered is not found
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(2);

    pres.notify(IEnggDiffFittingPresenter::FittingRunNo);
  }

  // Fit All Peaks test begin here
  void test_fit_all_runno_valid_single_run() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, getFittingRunNo()).Times(0);
    EXPECT_CALL(mockView, fittingPeaksData())
        .Times(1)
        .WillOnce(Return("2.3445,3.3433,4.5664"));

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(1);

    EXPECT_CALL(mockView, enableFitAllButton(testing::_)).Times(1);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/1 warnings. There will be an error log because dir vector
    // is empty
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::FitAllPeaks);
  }

  // This would test the fitting tab with invalid expected peaks but should only
  // produce a warning
  void test_fit_all_with_invalid_expected_peaks() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    // inputs from user
    EXPECT_CALL(mockView, fittingPeaksData())
        .Times(1)
        .WillOnce(Return(",3.5,7.78,r43d"));
    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(1);

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/1 warnings. There will be an error log from the algorithms
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::FitPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_browse_peaks_list() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, focusingDir()).Times(1);

    EXPECT_CALL(mockView, getPreviousDir()).Times(1);

    EXPECT_CALL(mockView, getOpenFile(testing::_)).Times(1);

    EXPECT_CALL(mockView, getSaveFile(testing::_)).Times(0);

    // No errors/0 warnings.
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::browsePeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_browse_peaks_list_with_warning() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    std::string dummyDir = "I/am/a/dummy/directory";

    EXPECT_CALL(mockView, focusingDir()).Times(1);

    EXPECT_CALL(mockView, getPreviousDir()).Times(1);

    EXPECT_CALL(mockView, getOpenFile(testing::_))
        .Times(1)
        .WillOnce(Return(dummyDir));

    EXPECT_CALL(mockView, setPreviousDir(dummyDir)).Times(1);

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(1);

    // No errors/0 warnings.
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::browsePeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_save_peaks_list() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, focusingDir()).Times(1);

    EXPECT_CALL(mockView, getPreviousDir()).Times(1);

    EXPECT_CALL(mockView, getSaveFile(testing::_)).Times(1);

    // No errors/No warnings.
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::savePeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_save_peaks_list_with_warning() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    std::string dummyDir = "/dummy/directory/";

    EXPECT_CALL(mockView, focusingDir()).Times(1);

    EXPECT_CALL(mockView, getPreviousDir()).Times(1);

    EXPECT_CALL(mockView, getSaveFile(testing::_))
        .Times(1)
        .WillOnce(Return(dummyDir));

    EXPECT_CALL(mockView, fittingPeaksData()).Times(0);

    // No errors/1 warnings. Dummy file entered is not found
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::savePeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_add_peaks_to_empty_list() {

    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, peakPickerEnabled()).Times(1).WillOnce(Return(true));

    EXPECT_CALL(mockView, getPeakCentre()).Times(1);

    EXPECT_CALL(mockView, fittingPeaksData()).Times(1).WillOnce(Return(""));
    ;

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(1);

    // should not be updating the status
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/0 warnings.
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::addPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_add_peaks_with_disabled_peak_picker() {

    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, peakPickerEnabled()).Times(1).WillOnce(Return(false));

    EXPECT_CALL(mockView, getPeakCentre()).Times(0);

    EXPECT_CALL(mockView, fittingPeaksData()).Times(0);

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(0);

    // should not be updating the status
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/0 warnings.
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::addPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_add_valid_peaks_to_list_with_comma() {

    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, peakPickerEnabled()).Times(1).WillOnce(Return(true));

    EXPECT_CALL(mockView, getPeakCentre()).Times(1).WillOnce(Return(2.0684));

    EXPECT_CALL(mockView, fittingPeaksData())
        .Times(1)
        .WillOnce(Return("1.7906,2.0684,1.2676,"));

    EXPECT_CALL(mockView, setPeakList("1.7906,2.0684,1.2676,2.0684")).Times(1);

    // No errors/0 warnings.
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::addPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_add_customised_valid_peaks_to_list_without_comma() {

    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, peakPickerEnabled()).Times(1).WillOnce(Return(true));

    EXPECT_CALL(mockView, getPeakCentre()).Times(1).WillOnce(Return(3.0234));

    EXPECT_CALL(mockView, fittingPeaksData())
        .Times(1)
        .WillOnce(Return("2.0684,1.2676"));

    EXPECT_CALL(mockView, setPeakList("2.0684,1.2676,3.0234")).Times(1);

    // should not be updating the status
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/0 warnings.
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::addPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_add_invalid_peaks_to_list() {

    testing::NiceMock<MockEnggDiffFittingView> mockView;
    EnggDiffFittingPresenterNoThread pres(&mockView);

    EXPECT_CALL(mockView, peakPickerEnabled()).Times(1).WillOnce(Return(true));

    EXPECT_CALL(mockView, getPeakCentre()).Times(1).WillOnce(Return(0.0133));

    EXPECT_CALL(mockView, fittingPeaksData()).Times(1).WillOnce(Return(""));

    // string should be "0.133," instead
    EXPECT_CALL(mockView, setPeakList("0.0133")).Times(0);
    EXPECT_CALL(mockView, setPeakList(",0.0133")).Times(0);
    EXPECT_CALL(mockView, setPeakList("0.0133,")).Times(1);

    // No errors/0 warnings. File entered is not found
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::addPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_shutDown() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    MantidQt::CustomInterfaces::EnggDiffFittingPresenter pres(&mockView,
                                                              nullptr, nullptr);

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(0);
    EXPECT_CALL(mockView, getFittingRunNo()).Times(0);
    EXPECT_CALL(mockView, getFittingRunNumVec()).Times(0);
    EXPECT_CALL(mockView, focusingDir()).Times(0);

    EXPECT_CALL(mockView, getFittingMultiRunMode()).Times(0);

    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    EXPECT_CALL(mockView, saveSettings()).Times(1);
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::ShutDown);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

private:
  std::unique_ptr<testing::NiceMock<MockEnggDiffFittingView>> m_view;
  std::unique_ptr<MantidQt::CustomInterfaces::EnggDiffFittingPresenter>
      m_presenter;

  std::vector<bool> m_ex_enginx_banks;
  const static std::string g_validRunNo;
  const static std::string g_focusedRun;
  const static std::string g_focusedBankFile;
  const static std::string g_focusedFittingRunNo;
  EnggDiffCalibSettings m_basicCalibSettings;

  std::vector<std::string> m_ex_empty_run_num;
  std::vector<std::string> m_invalid_run_number;
  std::vector<std::string> m_ex_run_number;
  std::vector<std::string> g_vanNo;
  std::vector<std::string> g_ceriaNo;
};

const std::string EnggDiffFittingPresenterTest::g_focusedRun =
    "focused_texture_bank_1";

const std::string EnggDiffFittingPresenterTest::g_validRunNo = "228061";

const std::string EnggDiffFittingPresenterTest::g_focusedBankFile =
    "ENGINX_241395_focused_texture_bank_1";

const std::string EnggDiffFittingPresenterTest::g_focusedFittingRunNo =
    "241391-241394";

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGPRESENTERTEST_H
