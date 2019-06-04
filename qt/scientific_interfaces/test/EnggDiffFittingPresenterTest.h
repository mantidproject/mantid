// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGPRESENTERTEST_H

#include "../EnggDiffraction/EnggDiffFittingPresenter.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "EnggDiffFittingModelMock.h"
#include "EnggDiffFittingViewMock.h"
#include "EnggDiffractionParamMock.h"
#include <cxxtest/TestSuite.h>
#include <vector>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::ReturnRef;
using testing::TypedEq;

// Use this mocked presenter for tests that will start the focusing
// workers/threads. Otherwise you'll run into trouble with issues like
// "QEventLoop: Cannot be used without QApplication", as there is not
// Qt application here and the normal Qt thread used by the presenter
// uses signals/slots.
class EnggDiffFittingPresenterNoThread : public EnggDiffFittingPresenter {
public:
  EnggDiffFittingPresenterNoThread(IEnggDiffFittingView *view)
      : EnggDiffFittingPresenterNoThread(
            view, std::make_unique<
                      testing::NiceMock<MockEnggDiffFittingModel>>()) {}

  EnggDiffFittingPresenterNoThread(IEnggDiffFittingView *view,
                                   std::unique_ptr<IEnggDiffFittingModel> model)
      : EnggDiffFittingPresenter(view, std::move(model), nullptr, nullptr) {}

  EnggDiffFittingPresenterNoThread(
      IEnggDiffFittingView *view, std::unique_ptr<IEnggDiffFittingModel> model,
      boost::shared_ptr<IEnggDiffractionParam> mainParam)
      : EnggDiffFittingPresenter(view, std::move(model), nullptr, mainParam) {}

private:
  // not async at all
  void startAsyncFittingWorker(const std::vector<RunLabel> &runLabels,
                               const std::string &ExpectedPeaks) override {
    assert(runLabels.size() == 1);
    doFitting(runLabels, ExpectedPeaks);
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
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();

    m_presenter.reset(new MantidQt::CustomInterfaces::EnggDiffFittingPresenter(
        m_view.get(), std::move(mockModel), nullptr, nullptr));

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
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    auto *mockModel_ptr = mockModel.get();
    MantidQt::CustomInterfaces::EnggDiffFittingPresenter pres(
        &mockView, std::move(mockModel), nullptr, nullptr);

    EXPECT_CALL(mockView, getFocusedFileNames()).Times(1).WillOnce(Return(""));

    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);

    // Should never get as far as trying to load
    EXPECT_CALL(*mockModel_ptr, loadWorkspaces(testing::_)).Times(0);

    pres.notify(IEnggDiffFittingPresenter::Load);
    TSM_ASSERT(
        "View mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
    TSM_ASSERT(
        "Model mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(mockModel_ptr))
  }

  void test_fitting_with_missing_param() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    MantidQt::CustomInterfaces::EnggDiffFittingPresenter pres(
        &mockView, std::move(mockModel), nullptr, nullptr);

    EXPECT_CALL(mockView, getFittingListWidgetCurrentValue())
        .Times(1)
        .WillOnce(Return(boost::none));

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(0);
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
    EXPECT_CALL(mockView, getFittingListWidgetCurrentValue())
        .Times(1)
        .WillOnce(Return(boost::none));

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(0);
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
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    auto *mockModel_ptr = mockModel.get();

    EnggDiffFittingPresenterNoThread pres(&mockView, std::move(mockModel));

    EXPECT_CALL(mockView, getFittingListWidgetCurrentValue())
        .Times(1)
        .WillOnce(Return(boost::optional<std::string>(
            boost::optional<std::string>("123_1"))));
    EXPECT_CALL(*mockModel_ptr, getWorkspaceFilename(testing::_))
        .Times(1)
        .WillOnce(ReturnRef(EMPTY));

    EXPECT_CALL(mockView, getExpectedPeaksInput())
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
        "View mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
    TSM_ASSERT(
        "Model mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(mockModel_ptr))
  }

  // Fit All Peaks test begin here
  void test_fit_all_runno_valid_single_run() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    auto *mockModel_ptr = mockModel.get();

    EnggDiffFittingPresenterNoThread pres(&mockView, std::move(mockModel));

    EXPECT_CALL(mockView, getExpectedPeaksInput())
        .Times(1)
        .WillOnce(Return("2.3445,3.3433,4.5664"));

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*mockModel_ptr, getRunLabels())
        .Times(1)
        .WillOnce(Return(std::vector<RunLabel>({runLabel})));

    EXPECT_CALL(*mockModel_ptr, getWorkspaceFilename(runLabel))
        .Times(1)
        .WillOnce(ReturnRef(EMPTY));

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(1);

    EXPECT_CALL(mockView, enableFitAllButton(testing::_)).Times(0);

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
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    auto *mockModel_ptr = mockModel.get();

    EnggDiffFittingPresenterNoThread pres(&mockView, std::move(mockModel));

    // inputs from user
    EXPECT_CALL(mockView, getExpectedPeaksInput())
        .Times(1)
        .WillOnce(Return(",3.5,7.78,r43d"));
    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(1);

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*mockModel_ptr, getRunLabels())
        .Times(1)
        .WillOnce(Return(std::vector<RunLabel>({runLabel})));

    EXPECT_CALL(*mockModel_ptr, getWorkspaceFilename(runLabel))
        .Times(1)
        .WillOnce(ReturnRef(EMPTY));

    // should not get to the point where the status is updated
    EXPECT_CALL(mockView, showStatus(testing::_)).Times(0);

    // No errors/1 warnings. There will be an error log from the algorithms
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IEnggDiffFittingPresenter::FitAllPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_browse_peaks_list() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    const auto paramMock =
        boost::make_shared<testing::NiceMock<MockEnggDiffractionParam>>();
    EnggDiffFittingPresenterNoThread pres(
        &mockView,
        std::make_unique<
            testing::NiceMock<MockEnggDiffFittingModel>>(),
        paramMock);

    const auto &userDir(Poco::Path::home());
    EXPECT_CALL(*paramMock, outFilesUserDir(""))
        .Times(1)
        .WillOnce(Return(userDir));

    EXPECT_CALL(mockView, getOpenFile(userDir)).Times(1);

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
    const auto paramMock =
        boost::make_shared<testing::NiceMock<MockEnggDiffractionParam>>();
    EnggDiffFittingPresenterNoThread pres(
        &mockView,
        std::make_unique<
            testing::NiceMock<MockEnggDiffFittingModel>>(),
        paramMock);

    const auto &userDir(Poco::Path::home());
    EXPECT_CALL(*paramMock, outFilesUserDir(""))
        .Times(1)
        .WillOnce(Return(userDir));

    std::string dummyDir = "I/am/a/dummy/directory";

    EXPECT_CALL(mockView, getOpenFile(userDir))
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
    const auto paramMock =
        boost::make_shared<testing::NiceMock<MockEnggDiffractionParam>>();
    EnggDiffFittingPresenterNoThread pres(
        &mockView,
        std::make_unique<
            testing::NiceMock<MockEnggDiffFittingModel>>(),
        paramMock);

    const auto &userDir(Poco::Path::home());
    EXPECT_CALL(*paramMock, outFilesUserDir(""))
        .Times(1)
        .WillOnce(Return(userDir));

    EXPECT_CALL(mockView, getSaveFile(userDir)).Times(1);

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
    const auto paramMock =
        boost::make_shared<testing::NiceMock<MockEnggDiffractionParam>>();
    EnggDiffFittingPresenterNoThread pres(
        &mockView,
        std::make_unique<
            testing::NiceMock<MockEnggDiffFittingModel>>(),
        paramMock);

    const auto &userDir(Poco::Path::home());
    EXPECT_CALL(*paramMock, outFilesUserDir(""))
        .Times(1)
        .WillOnce(Return(userDir));

    std::string dummyDir = "/dummy/directory/";
    EXPECT_CALL(mockView, getSaveFile(userDir))
        .Times(1)
        .WillOnce(Return(dummyDir));

    EXPECT_CALL(mockView, getExpectedPeaksInput()).Times(0);

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

    EXPECT_CALL(mockView, getExpectedPeaksInput())
        .Times(1)
        .WillOnce(Return(""));
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

    EXPECT_CALL(mockView, getExpectedPeaksInput()).Times(0);

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

    EXPECT_CALL(mockView, getExpectedPeaksInput())
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

    EXPECT_CALL(mockView, getExpectedPeaksInput())
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

    EXPECT_CALL(mockView, getExpectedPeaksInput())
        .Times(1)
        .WillOnce(Return(""));

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
    MantidQt::CustomInterfaces::EnggDiffFittingPresenter pres(
        &mockView,
        std::make_unique<
            testing::NiceMock<MockEnggDiffFittingModel>>(),
        nullptr, nullptr);

    EXPECT_CALL(mockView, setPeakList(testing::_)).Times(0);
    EXPECT_CALL(mockView, getFocusedFileNames()).Times(0);
    EXPECT_CALL(mockView, getFittingRunNumVec()).Times(0);

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

  void test_removeRun() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    auto *mockModel_ptr = mockModel.get();
    MantidQt::CustomInterfaces::EnggDiffFittingPresenter pres(
        &mockView, std::move(mockModel), nullptr, nullptr);

    EXPECT_CALL(mockView, getFittingListWidgetCurrentValue())
        .Times(1)
        .WillOnce(Return(boost::optional<std::string>("123_1")));
    EXPECT_CALL(*mockModel_ptr, removeRun(RunLabel("123", 1)));
    EXPECT_CALL(*mockModel_ptr, getRunLabels())
        .Times(1)
        .WillOnce(Return(
            std::vector<RunLabel>({RunLabel("123", 2), RunLabel("456", 1)})));
    EXPECT_CALL(mockView, updateFittingListWidget(
                              std::vector<std::string>({"123_2", "456_1"})));

    pres.notify(IEnggDiffFittingPresenter::removeRun);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_updatePlotFittedPeaksValidFittedPeaks() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    auto *mockModel_ptr = mockModel.get();

    EnggDiffFittingPresenterNoThread pres(&mockView, std::move(mockModel));

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(mockView, getFittingListWidgetCurrentValue())
        .Times(2)
        .WillRepeatedly(Return(boost::optional<std::string>("123_1")));
    EXPECT_CALL(*mockModel_ptr, hasFittedPeaksForRun(runLabel))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*mockModel_ptr, getAlignedWorkspace(runLabel))
        .Times(1)
        .WillOnce(Return(WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    EXPECT_CALL(mockView, plotFittedPeaksEnabled())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*mockModel_ptr, getFittedPeaksWS(runLabel))
        .Times(1)
        .WillOnce(Return(WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    EXPECT_CALL(mockView,
                setDataVector(testing::_, testing::_, testing::_, testing::_))
        .Times(2);

    pres.notify(IEnggDiffFittingPresenter::updatePlotFittedPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_updatePlotFittedPeaksNoFittedPeaks() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    auto *mockModel_ptr = mockModel.get();

    EnggDiffFittingPresenterNoThread pres(&mockView, std::move(mockModel));

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(mockView, getFittingListWidgetCurrentValue())
        .Times(1)
        .WillOnce(Return(boost::optional<std::string>("123_1")));
    EXPECT_CALL(*mockModel_ptr, hasFittedPeaksForRun(runLabel))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*mockModel_ptr, getFocusedWorkspace(runLabel))
        .Times(1)
        .WillOnce(Return(WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    EXPECT_CALL(mockView, plotFittedPeaksEnabled())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*mockModel_ptr, getFittedPeaksWS(runLabel)).Times(0);
    EXPECT_CALL(mockView,
                setDataVector(testing::_, testing::_, testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(mockView, userWarning("Cannot plot fitted peaks", testing::_))
        .Times(1);

    pres.notify(IEnggDiffFittingPresenter::updatePlotFittedPeaks);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_updatePlotSuccessfulFitPlotPeaksDisabled() {
    testing::NiceMock<MockEnggDiffFittingView> mockView;
    auto mockModel = std::make_unique<
        testing::NiceMock<MockEnggDiffFittingModel>>();
    auto *mockModel_ptr = mockModel.get();

    EnggDiffFittingPresenterNoThread pres(&mockView, std::move(mockModel));

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(mockView, getFittingListWidgetCurrentValue())
        .Times(2)
        .WillRepeatedly(Return(boost::optional<std::string>("123_1")));
    EXPECT_CALL(*mockModel_ptr, hasFittedPeaksForRun(runLabel))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*mockModel_ptr, getAlignedWorkspace(runLabel))
        .Times(1)
        .WillOnce(Return(WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    EXPECT_CALL(mockView, plotFittedPeaksEnabled())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*mockModel_ptr, getFittedPeaksWS(runLabel)).Times(0);
    EXPECT_CALL(mockView,
                setDataVector(testing::_, testing::_, testing::_, testing::_))
        .Times(1);

    pres.notify(IEnggDiffFittingPresenter::updatePlotFittedPeaks);
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
  const static std::string EMPTY;
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

const std::string EnggDiffFittingPresenterTest::EMPTY = "";

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGPRESENTERTEST_H
