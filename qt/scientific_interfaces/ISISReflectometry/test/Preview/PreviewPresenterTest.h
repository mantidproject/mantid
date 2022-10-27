// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../../widgets/regionselector/test/MockRegionSelector.h"
#include "../ReflMockObjects.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MockInstViewModel.h"
#include "MockPlotPresenter.h"
#include "MockPreviewModel.h"
#include "MockPreviewView.h"
#include "PreviewPresenter.h"
#include "ROIType.h"
#include "Reduction/RowExceptions.h"
#include "TestHelpers/ModelCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using namespace MantidQt::API;
using namespace Mantid::Kernel;
using namespace MantidQt::Widgets;

using MantidQt::MantidWidgets::AxisID;
using MantidQt::MantidWidgets::MockPlotPresenter;

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ByRef;
using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Throw;

class PreviewPresenterTest : public CxxTest::TestSuite {
  using MockInstViewModelT = std::unique_ptr<MockInstViewModel>;
  using MockJobManagerT = std::unique_ptr<MockJobManager>;
  using MockModelT = std::unique_ptr<MockPreviewModel>;
  using MockViewT = std::unique_ptr<MockPreviewView>;
  using MockRegionSelectorT = std::unique_ptr<MockRegionSelector>;
  using MockPlotPresenterT = std::unique_ptr<MockPlotPresenter>;

public:
  void test_notify_load_workspace_requested_loads_from_file_if_not_in_ads() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto mockJobManager = makeJobManager();
    auto const workspaceName = std::string("test workspace");

    EXPECT_CALL(*mockView, getWorkspaceName()).Times(1).WillOnce(Return(workspaceName));
    EXPECT_CALL(*mockModel, loadWorkspaceFromAds(workspaceName)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*mockModel, loadAndPreprocessWorkspaceAsync(Eq(workspaceName), Ref(*mockJobManager))).Times(1);

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager)));
    presenter.notifyLoadWorkspaceRequested();
  }

  void test_notify_load_workspace_requested_does_not_load_from_file_if_in_ads() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto mockInstViewModel = makeInstViewModel();
    auto mockJobManager = makeJobManager();
    auto const workspaceName = std::string("test workspace");

    EXPECT_CALL(*mockView, getWorkspaceName()).Times(1).WillOnce(Return(workspaceName));
    EXPECT_CALL(*mockModel, loadWorkspaceFromAds(workspaceName)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockModel, loadAndPreprocessWorkspaceAsync(_, _)).Times(0);
    expectLoadWorkspaceCompletedUpdatesInstrumentView(*mockView, *mockModel, *mockInstViewModel);

    auto presenter = PreviewPresenter(
        packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager), std::move(mockInstViewModel)));
    presenter.notifyLoadWorkspaceRequested();
  }

  void test_notify_load_workspace_catches_runtime_error() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto const workspaceName = std::string("test workspace");
    auto error = std::runtime_error("Test error");

    EXPECT_CALL(*mockView, getWorkspaceName()).Times(1).WillOnce(Return(workspaceName));
    EXPECT_CALL(*mockModel, loadWorkspaceFromAds(workspaceName)).Times(1).WillOnce(Throw(error));
    EXPECT_CALL(*mockModel, loadAndPreprocessWorkspaceAsync(_, _)).Times(0);
    EXPECT_CALL(*mockView, plotInstView(_, _, _)).Times(0);

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel)));
    TS_ASSERT_THROWS_NOTHING(presenter.notifyLoadWorkspaceRequested());
  }

  void test_notify_load_workspace_does_not_catch_unexpected_error() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto const workspaceName = std::string("test workspace");
    auto error = std::invalid_argument("Test error");

    EXPECT_CALL(*mockView, getWorkspaceName()).Times(1).WillOnce(Return(workspaceName));
    EXPECT_CALL(*mockModel, loadWorkspaceFromAds(workspaceName)).Times(1).WillOnce(Throw(error));
    EXPECT_CALL(*mockModel, loadAndPreprocessWorkspaceAsync(_, _)).Times(0);
    EXPECT_CALL(*mockView, plotInstView(_, _, _)).Times(0);

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel)));
    TS_ASSERT_THROWS(presenter.notifyLoadWorkspaceRequested(), std::invalid_argument const &);
  }

  void test_notify_load_workspace_updates_model_and_view_for_linear_detector() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto mockJobManager = makeJobManager();
    auto mockRegionSelector = makeRegionSelector();

    expectRegionSelectorToolbarEnabled(*mockView, false);
    expectLoadWorkspaceCompletedForLinearDetector(*mockView, *mockModel, *mockJobManager, *mockRegionSelector);

    auto deps = packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager), makeInstViewModel(),
                         std::move(mockRegionSelector));
    auto presenter = PreviewPresenter(std::move(deps));

    expectRegionSelectorToolbarEnabled(*mockView, true);

    presenter.notifyLoadWorkspaceCompleted();
  }

  void test_notify_load_workspace_complete_reloads_inst_view() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto mockJobManager = makeJobManager();
    auto mockInstViewModel = makeInstViewModel();

    expectLoadWorkspaceCompletedUpdatesInstrumentView(*mockView, *mockModel, *mockInstViewModel);

    auto deps = packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager), std::move(mockInstViewModel));
    auto presenter = PreviewPresenter(std::move(deps));
    presenter.notifyLoadWorkspaceCompleted();
  }

  void test_angle_is_set_when_workspace_loaded() {
    auto mockModel = makeModel();
    auto mockView = std::make_unique<MockPreviewView>();

    expectLoadWorkspaceCompletedUpdatesAngle(*mockView, *mockModel);

    auto deps = packDeps(mockView.get(), std::move(mockModel));
    auto presenter = PreviewPresenter(std::move(deps));
    presenter.notifyLoadWorkspaceCompleted();
  }

  void test_notify_inst_view_select_rect_requested() {
    auto mockView = makeView();
    expectInstViewSetToSelectRectMode(*mockView);
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.notifyInstViewSelectRectRequested();
  }

  void test_notify_inst_view_pan_requested() {
    auto mockView = makeView();
    expectInstViewSetToEditMode(*mockView);
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.notifyInstViewEditRequested();
  }

  void test_notify_inst_view_zoom_requested() {
    auto mockView = makeView();
    expectInstViewSetToZoomMode(*mockView);
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.notifyInstViewZoomRequested();
  }

  void test_notify_inst_view_shape_changed() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto mockInstViewModel = makeInstViewModel();
    auto mockJobManager = makeJobManager();
    expectInstViewSetToEditMode(*mockView);
    expectSumBanksCalledOnSelectedDetectors(*mockView, *mockModel, *mockInstViewModel, *mockJobManager);
    // TODO check that the model is called to sum banks
    auto presenter = PreviewPresenter(
        packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager), std::move(mockInstViewModel)));
    presenter.notifyInstViewShapeChanged();
  }

  void test_notify_region_selector_export_to_ads_requested() {
    auto mockView = makeView();
    auto mockModel = makeModel();

    EXPECT_CALL(*mockModel, exportSummedWsToAds()).Times(1);
    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel)));

    presenter.notifyRegionSelectorExportAdsRequested();
  }

  void test_notify_1D_plot_export_to_ads_requested() {
    auto mockView = makeView();
    auto mockModel = makeModel();

    EXPECT_CALL(*mockModel, exportReducedWsToAds()).Times(1);
    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel)));

    presenter.notifyLinePlotExportAdsRequested();
  }

  void test_sum_banks_completed_plots_region_selector() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto mockJobManager = makeJobManager();
    auto mockRegionSelector = makeRegionSelector();

    auto ws = createRectangularDetectorWorkspace();
    EXPECT_CALL(*mockModel, getSummedWs).Times(1).WillOnce(Return(ws));
    EXPECT_CALL(*mockRegionSelector, updateWorkspace(Eq(ws))).Times(1);
    expectRegionSelectorToolbarEnabled(*mockView, false);

    expectRunReduction(*mockView, *mockModel, *mockJobManager, *mockRegionSelector);

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager),
                                               makeInstViewModel(), std::move(mockRegionSelector)));

    expectRegionSelectorToolbarEnabled(*mockView, true);

    presenter.notifySumBanksCompleted();
  }

  void test_notify_update_angle_will_run_a_reduction() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto mockJobManager = makeJobManager();
    auto mockRegionSelector = makeRegionSelector();

    expectRunReduction(*mockView, *mockModel, *mockJobManager, *mockRegionSelector);

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager),
                                               makeInstViewModel(), std::move(mockRegionSelector)));

    presenter.notifyUpdateAngle();
  }

  void test_rectangular_roi_requested() {
    auto mockView = makeView();
    auto mockRegionSelector_uptr = makeRegionSelector();
    auto mockRegionSelector = mockRegionSelector_uptr.get();
    const std::string regionType = roiTypeToString(ROIType::Signal);
    const std::string color = roiTypeToColor(ROIType::Signal);

    EXPECT_CALL(*mockView, getRegionType()).Times(1).WillOnce(Return(regionType));
    expectRectangularROIMode(*mockView);
    EXPECT_CALL(*mockRegionSelector, addRectangularRegion(regionType, color)).Times(1);
    auto presenter = PreviewPresenter(packDeps(mockView.get(), makeModel(), makeJobManager(), makeInstViewModel(),
                                               std::move(mockRegionSelector_uptr)));

    presenter.notifyRectangularROIModeRequested();
  }

  void test_edit_roi_mode_requested() {
    auto mockView = makeView();
    auto mockRegionSelector_uptr = makeRegionSelector();
    auto mockRegionSelector = mockRegionSelector_uptr.get();

    expectEditROIMode(*mockView);
    EXPECT_CALL(*mockRegionSelector, cancelDrawingRegion()).Times(1);
    auto presenter = PreviewPresenter(packDeps(mockView.get(), makeModel(), makeJobManager(), makeInstViewModel(),
                                               std::move(mockRegionSelector_uptr)));

    presenter.notifyEditROIModeRequested();
  }

  void test_notify_region_changed_starts_reduction() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto mockJobManager = makeJobManager();
    auto mockRegionSelector = makeRegionSelector();

    expectEditROIMode(*mockView);
    expectRunReduction(*mockView, *mockModel, *mockJobManager, *mockRegionSelector);

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager),
                                               makeInstViewModel(), std::move(mockRegionSelector)));
    presenter.notifyRegionChanged();
  }

  void test_line_plot_is_displayed_when_reduction_completed() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto mockLinePlot = std::make_unique<MockPlotPresenter>();
    auto lineLabel = std::string("line_label");
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    EXPECT_CALL(*mockModel, getReducedWs()).Times(1).WillOnce(Return(ws));
    EXPECT_CALL(*mockLinePlot, setSpectrum(ws, 0)).Times(1);
    EXPECT_CALL(*mockLinePlot, plot()).Times(1);

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), makeJobManager(),
                                               makeInstViewModel(), makeRegionSelector(), std::move(mockLinePlot)));

    presenter.notifyReductionCompleted();
  }

  void test_notify_reduction_resumed_disables_view() {
    auto mockView = makeView();
    auto mainPresenter = MockBatchPresenter();

    expectProcessingEnabled(mainPresenter);
    expectApplyButtonDisabled(*mockView);

    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.acceptMainPresenter(&mainPresenter);
    presenter.notifyReductionResumed();
  }

  void test_notify_reduction_paused_enables_view() {
    auto mockView = makeView();
    auto mainPresenter = MockBatchPresenter();

    expectProcessingDisabled(mainPresenter);
    expectApplyButtonEnabled(*mockView);

    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.acceptMainPresenter(&mainPresenter);
    presenter.notifyReductionPaused();
  }

  void test_notify_autoreduction_resumed_disables_view() {
    auto mockView = makeView();
    auto mainPresenter = MockBatchPresenter();

    expectAutoreducingEnabled(mainPresenter);
    expectApplyButtonDisabled(*mockView);

    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.acceptMainPresenter(&mainPresenter);
    presenter.notifyAutoreductionResumed();
  }

  void test_notify_autoreduction_paused_enables_view() {
    auto mockView = makeView();
    auto mainPresenter = MockBatchPresenter();

    expectAutoreducingDisabled(mainPresenter);
    expectApplyButtonEnabled(*mockView);

    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.acceptMainPresenter(&mainPresenter);
    presenter.notifyAutoreductionPaused();
  }

  void test_notify_apply_requested_notifies_main_presenter() {
    auto mockView = makeView();
    auto mainPresenter = MockBatchPresenter();

    EXPECT_CALL(mainPresenter, notifyPreviewApplyRequested()).Times(1);

    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.acceptMainPresenter(&mainPresenter);
    presenter.notifyApplyRequested();
  }

  void test_get_preview_row() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto previewRow = PreviewRow({"12345"});

    EXPECT_CALL(*mockModel, getPreviewRow()).Times(1).WillOnce(ReturnRef(previewRow));

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel)));
    presenter.getPreviewRow();
  }

  void test_notify_apply_requested_will_catch_RowNotFoundException() {
    auto mockView = makeView();
    auto mainPresenter = MockBatchPresenter();
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.acceptMainPresenter(&mainPresenter);

    EXPECT_CALL(mainPresenter, notifyPreviewApplyRequested())
        .Times(1)
        .WillRepeatedly(Throw(RowNotFoundException("Error message")));

    presenter.notifyApplyRequested();
  }

  void test_notify_apply_requested_will_catch_MultipleRowsFoundException() {
    auto mockView = makeView();
    auto mainPresenter = MockBatchPresenter();
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.acceptMainPresenter(&mainPresenter);

    EXPECT_CALL(mainPresenter, notifyPreviewApplyRequested())
        .Times(1)
        .WillRepeatedly(Throw(MultipleRowsFoundException("Error message")));

    presenter.notifyApplyRequested();
  }

  void test_region_selector_and_reduction_plot_is_cleared_on_a_sum_banks_algorithm_error() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto mockRegionSelector = makeRegionSelector();
    auto mockPlotPresenter = std::make_unique<MockPlotPresenter>();

    // Get the raw pointers before moving ownership of the unique ptrs to the PreviewPresenter, so we can set
    // expectations later
    auto rawMockRegionSelector = mockRegionSelector.get();
    auto rawMockPlotPresenter = mockPlotPresenter.get();

    auto presenter =
        PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), makeJobManager(), makeInstViewModel(),
                                  std::move(mockRegionSelector), std::move(mockPlotPresenter)));

    expectRegionSelectorCleared(*mockView, rawMockRegionSelector);
    expectReductionPlotCleared(rawMockPlotPresenter);

    presenter.notifySumBanksAlgorithmError();
  }

  void test_reduction_plot_is_cleared_on_a_reduction_algorithm_error() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto mockPlotPresenter = std::make_unique<MockPlotPresenter>();

    // Get the raw pointer before moving ownership of the unique ptr to the PreviewPresenter, so we can set
    // expectations later
    auto rawMockPlotPresenter = mockPlotPresenter.get();

    auto presenter =
        PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), makeJobManager(), makeInstViewModel(),
                                  makeRegionSelector(), std::move(mockPlotPresenter)));

    expectReductionPlotCleared(rawMockPlotPresenter);

    presenter.notifyReductionAlgorithmError();
  }

private:
  MockViewT makeView() {
    auto mockView = std::make_unique<MockPreviewView>();
    EXPECT_CALL(*mockView, subscribe(NotNull())).Times(1);
    EXPECT_CALL(*mockView, setInstViewToolbarEnabled(Eq(false))).Times(1);
    return mockView;
  }

  MockModelT makeModel() { return std::make_unique<MockPreviewModel>(); }

  MockJobManagerT makeJobManager() {
    auto mockJobManager = std::make_unique<MockJobManager>();
    EXPECT_CALL(*mockJobManager, subscribe(NotNull())).Times(1);
    return mockJobManager;
  }

  MockInstViewModelT makeInstViewModel() { return std::make_unique<MockInstViewModel>(); }

  MockRegionSelectorT makeRegionSelector() { return std::make_unique<MockRegionSelector>(); }

  PreviewPresenter::Dependencies packDeps(MockPreviewView *view,
                                          MockModelT model = std::make_unique<MockPreviewModel>(),
                                          MockJobManagerT jobManager = std::make_unique<NiceMock<MockJobManager>>(),
                                          MockInstViewModelT instView = std::make_unique<MockInstViewModel>(),
                                          MockRegionSelectorT regionSelector = std::make_unique<MockRegionSelector>(),
                                          MockPlotPresenterT linePlot = std::make_unique<MockPlotPresenter>()) {
    expectPresenterConstructed(*regionSelector, *linePlot);
    return PreviewPresenter::Dependencies{view,
                                          std::move(model),
                                          std::move(jobManager),
                                          std::move(instView),
                                          std::move(regionSelector),
                                          std::move(linePlot)};
  }

  void expectPresenterConstructed(MockRegionSelector &regionSelector, MockPlotPresenter &linePlot) {
    EXPECT_CALL(regionSelector, subscribe(NotNull())).Times(1);
    EXPECT_CALL(linePlot, setScaleLog(AxisID::YLeft)).Times(1);
    EXPECT_CALL(linePlot, setScaleLog(AxisID::XBottom)).Times(1);
    EXPECT_CALL(linePlot, setPlotErrorBars(true)).Times(1);
  }

  void expectLoadWorkspaceCompletedForLinearDetector(MockPreviewView &mockView, MockPreviewModel &mockModel,
                                                     MockJobManager &mockJobManager,
                                                     MockRegionSelector &mockRegionSelector) {
    auto ws = createLinearDetectorWorkspace();

    EXPECT_CALL(mockModel, getLoadedWs()).Times(1).WillOnce(Return(ws));

    EXPECT_CALL(mockView, resetInstView()).Times(1);
    EXPECT_CALL(mockModel, setSummedWs(ws)).Times(1);

    expectRunReduction(mockView, mockModel, mockJobManager, mockRegionSelector);
  }

  void expectLoadWorkspaceCompletedUpdatesInstrumentView(MockPreviewView &mockView, MockPreviewModel &mockModel,
                                                         MockInstViewModel &mockInstViewModel) {
    expectInstViewModelUpdatedWithLoadedWorkspace(mockModel, mockInstViewModel);
    expectPlotInstView(mockView, mockInstViewModel);
    expectInstViewToolbarEnabled(mockView);
    expectInstViewSetToZoomMode(mockView);
  }

  void expectLoadWorkspaceCompletedUpdatesAngle(MockPreviewView &mockView, MockPreviewModel &mockModel) {
    auto ws = createRectangularDetectorWorkspace();
    auto angle = 2.3;

    EXPECT_CALL(mockModel, getLoadedWs()).Times(1).WillOnce(Return(ws));
    EXPECT_CALL(mockModel, getDefaultTheta()).Times(1).WillOnce(Return(angle));
    EXPECT_CALL(mockView, setAngle(angle)).Times(1);
  }

  void expectInstViewModelUpdatedWithLoadedWorkspace(MockPreviewModel &mockModel,
                                                     MockInstViewModel &mockInstViewModel) {
    auto ws = createRectangularDetectorWorkspace();
    EXPECT_CALL(mockModel, getLoadedWs()).Times(1).WillOnce(Return(ws));
    EXPECT_CALL(mockInstViewModel, updateWorkspace(Eq(ws))).Times(1);
  }

  void expectSumBanksCalledOnSelectedDetectors(MockPreviewView &mockView, MockPreviewModel &mockModel,
                                               MockInstViewModel &mockInstViewModel, MockJobManager &mockJobManager) {
    auto detIndices = std::vector<size_t>{44, 45, 46};
    auto detIDs = std::vector<Mantid::detid_t>{2, 3, 4};
    auto detIDsStr = ProcessingInstructions{"2,3,4"};
    EXPECT_CALL(mockView, getSelectedDetectors()).Times(1).WillOnce(Return(detIndices));
    EXPECT_CALL(mockInstViewModel, detIndicesToDetIDs(Eq(detIndices))).Times(1).WillOnce(Return(detIDs));
    EXPECT_CALL(mockModel, setSelectedBanks(Eq(detIDsStr))).Times(1);
    EXPECT_CALL(mockModel, sumBanksAsync(Ref(mockJobManager))).Times(1);
  }

  void expectPlotInstView(MockPreviewView &mockView, MockInstViewModel &mockInstViewModel) {
    auto samplePos = V3D(1, 2, 3);
    auto axes = V3D(4, 5, 6);
    EXPECT_CALL(mockInstViewModel, getInstrumentViewActor()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(mockInstViewModel, getSamplePos()).Times(1).WillOnce(Return(samplePos));
    EXPECT_CALL(mockInstViewModel, getAxis()).Times(1).WillOnce(Return(axes));
    EXPECT_CALL(mockView, plotInstView(Eq(nullptr), Eq(samplePos), Eq(axes))).Times(1);
  }

  void expectInstViewToolbarEnabled(MockPreviewView &mockView) {
    EXPECT_CALL(mockView, setInstViewToolbarEnabled(Eq(true))).Times(1);
  }

  void expectRegionSelectorToolbarEnabled(MockPreviewView &mockView, bool enable) {
    EXPECT_CALL(mockView, setRegionSelectorToolbarEnabled(Eq(enable))).Times(1);
  }

  void expectInstViewSetToZoomMode(MockPreviewView &mockView) {
    EXPECT_CALL(mockView, setInstViewSelectRectState(Eq(false))).Times(1);
    EXPECT_CALL(mockView, setInstViewEditState(Eq(false))).Times(1);
    EXPECT_CALL(mockView, setInstViewZoomState(Eq(true))).Times(1);
    EXPECT_CALL(mockView, setInstViewZoomMode()).Times(1);
  }

  void expectInstViewSetToEditMode(MockPreviewView &mockView) {
    EXPECT_CALL(mockView, setInstViewZoomState(Eq(false))).Times(1);
    EXPECT_CALL(mockView, setInstViewSelectRectState(Eq(false))).Times(1);
    EXPECT_CALL(mockView, setInstViewEditState(Eq(true))).Times(1);
    EXPECT_CALL(mockView, setInstViewEditMode()).Times(1);
  }

  void expectInstViewSetToSelectRectMode(MockPreviewView &mockView) {
    EXPECT_CALL(mockView, setInstViewEditState(Eq(false))).Times(1);
    EXPECT_CALL(mockView, setInstViewZoomState(Eq(false))).Times(1);
    EXPECT_CALL(mockView, setInstViewSelectRectState(Eq(true))).Times(1);
    EXPECT_CALL(mockView, setInstViewSelectRectMode()).Times(1);
  }

  void expectRectangularROIMode(MockPreviewView &mockView) {
    EXPECT_CALL(mockView, setEditROIState(Eq(false))).Times(1);
    EXPECT_CALL(mockView, setRectangularROIState(Eq(true))).Times(1);
  }

  void expectEditROIMode(MockPreviewView &mockView) {
    EXPECT_CALL(mockView, setEditROIState(Eq(true))).Times(1);
    EXPECT_CALL(mockView, setRectangularROIState(Eq(false))).Times(1);
  }

  void expectRunReduction(MockPreviewView &mockView, MockPreviewModel &mockModel, MockJobManager &mockJobManager,
                          MockRegionSelector &mockRegionSelector) {
    EXPECT_CALL(mockView, setUpdateAngleButtonEnabled(false)).Times(1);
    // Check theta is set
    auto theta = 0.3;
    EXPECT_CALL(mockView, getAngle()).Times(1).WillOnce(Return(theta));
    EXPECT_CALL(mockModel, setTheta(theta)).Times(1);
    // Check ROI is set
    auto roi = IRegionSelector::Selection{3.5, 11.23};
    EXPECT_CALL(mockRegionSelector, getRegion(roiTypeToString(ROIType::Signal))).Times(1).WillOnce(Return(roi));
    EXPECT_CALL(mockRegionSelector, getRegion(roiTypeToString(ROIType::Background))).Times(1).WillOnce(Return(roi));
    EXPECT_CALL(mockRegionSelector, getRegion(roiTypeToString(ROIType::Transmission))).Times(1).WillOnce(Return(roi));
    EXPECT_CALL(mockModel, setSelectedRegion(ROIType::Signal, roi)).Times(1);
    EXPECT_CALL(mockModel, setSelectedRegion(ROIType::Background, roi)).Times(1);
    EXPECT_CALL(mockModel, setSelectedRegion(ROIType::Transmission, roi)).Times(1);
    // Check reduction is executed
    EXPECT_CALL(mockModel, reduceAsync(Ref(mockJobManager))).Times(1);
  }

  void expectApplyButtonDisabled(MockPreviewView &mockView) { EXPECT_CALL(mockView, disableApplyButton()).Times(1); }

  void expectApplyButtonEnabled(MockPreviewView &mockView) { EXPECT_CALL(mockView, enableApplyButton()).Times(1); }

  void expectProcessingEnabled(MockBatchPresenter &mainPresenter) {
    EXPECT_CALL(mainPresenter, isProcessing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
  }

  void expectProcessingDisabled(MockBatchPresenter &mainPresenter) {
    EXPECT_CALL(mainPresenter, isProcessing()).Times(AtLeast(1)).WillRepeatedly(Return(false));
  }

  void expectAutoreducingEnabled(MockBatchPresenter &mainPresenter) {
    EXPECT_CALL(mainPresenter, isAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
  }

  void expectAutoreducingDisabled(MockBatchPresenter &mainPresenter) {
    EXPECT_CALL(mainPresenter, isAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(false));
  }

  void expectRegionSelectorCleared(MockPreviewView &mockView, MockRegionSelector *mockRegionSelector) {
    EXPECT_CALL(*mockRegionSelector, clearWorkspace()).Times(1);
    EXPECT_CALL(mockView, setRegionSelectorToolbarEnabled(false)).Times(1);
  }

  void expectReductionPlotCleared(MockPlotPresenter *mockPlotPresenter) {
    EXPECT_CALL(*mockPlotPresenter, clearModel()).Times(1);
    EXPECT_CALL(*mockPlotPresenter, plot()).Times(1);
  }

  Mantid::API::MatrixWorkspace_sptr createLinearDetectorWorkspace() {
    return WorkspaceCreationHelper::create2DWorkspace(1, 1);
  }

  Mantid::API::MatrixWorkspace_sptr createRectangularDetectorWorkspace() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    auto rectangularInstrument = ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);
    ws->setInstrument(rectangularInstrument);
    return ws;
  }
};
