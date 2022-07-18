// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../../widgets/regionselector/test/MockRegionSelector.h"
#include "../ReflMockObjects.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MockInstViewModel.h"
#include "MockPlotPresenter.h"
#include "MockPreviewModel.h"
#include "MockPreviewView.h"
#include "PreviewPresenter.h"
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
    expectLoadWorkspaceCompleted(*mockView, *mockModel, *mockInstViewModel);

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
  void test_notify_load_workspace_complete_reloads_inst_view() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto mockJobManager = makeJobManager();
    auto mockInstViewModel = makeInstViewModel();

    expectLoadWorkspaceCompleted(*mockView, *mockModel, *mockInstViewModel);

    auto deps = packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager), std::move(mockInstViewModel));
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
    auto mockRegionSelector_uptr = makeRegionSelector();
    auto mockRegionSelector = mockRegionSelector_uptr.get();

    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    EXPECT_CALL(*mockModel, getSummedWs).Times(1).WillOnce(Return(ws));
    EXPECT_CALL(*mockRegionSelector, updateWorkspace(Eq(ws))).Times(1);
    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), makeJobManager(),
                                               makeInstViewModel(), std::move(mockRegionSelector_uptr)));

    presenter.notifySumBanksCompleted();
  }

  void test_rectangular_roi_requested_updates_view() {
    auto mockView = makeView();
    auto mockRegionSelector_uptr = makeRegionSelector();
    auto mockRegionSelector = mockRegionSelector_uptr.get();

    expectActivateRectangularROIMode(*mockView, *mockRegionSelector);
    auto presenter = PreviewPresenter(packDeps(mockView.get(), makeModel(), makeJobManager(), makeInstViewModel(),
                                               std::move(mockRegionSelector_uptr)));

    presenter.notifyRectangularROIModeRequested();
  }

  void test_notify_region_changed_starts_reduction() {
    auto mockView = makeView();
    auto mockModel = makeModel();
    auto mockJobManager = makeJobManager();
    auto mockRegionSelector_uptr = makeRegionSelector();
    auto mockRegionSelector = mockRegionSelector_uptr.get();

    // TODO reset edit mode after region is selected
    // expectRegionSelectorSetToEditMode(*mockView);
    expectReduceAsyncCalledOnSelectedRegion(*mockView, *mockModel, *mockJobManager, *mockRegionSelector);

    auto presenter = PreviewPresenter(packDeps(mockView.get(), std::move(mockModel), std::move(mockJobManager),
                                               makeInstViewModel(), std::move(mockRegionSelector_uptr)));
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

  void expectLoadWorkspaceCompleted(MockPreviewView &mockView, MockPreviewModel &mockModel,
                                    MockInstViewModel &mockInstViewModel) {
    expectInstViewModelUpdatedWithLoadedWorkspace(mockModel, mockInstViewModel);
    expectPlotInstView(mockView, mockInstViewModel);
    expectInstViewToolbarEnabled(mockView);
    expectInstViewSetToZoomMode(mockView);
  }

  void expectInstViewModelUpdatedWithLoadedWorkspace(MockPreviewModel &mockModel,
                                                     MockInstViewModel &mockInstViewModel) {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    EXPECT_CALL(mockModel, getLoadedWs).Times(1).WillOnce(Return(ws));
    EXPECT_CALL(mockInstViewModel, updateWorkspace(Eq(ws))).Times(1);
  }

  void expectSumBanksCalledOnSelectedDetectors(MockPreviewView &mockView, MockPreviewModel &mockModel,
                                               MockInstViewModel &mockInstViewModel, MockJobManager &mockJobManager) {
    auto detIndices = std::vector<size_t>{44, 45, 46};
    auto detIDs = std::vector<Mantid::detid_t>{2, 3, 4};
    auto detIDsStr = std::string{"2, 3, 4"};
    EXPECT_CALL(mockView, getSelectedDetectors()).Times(1).WillOnce(Return(detIndices));
    EXPECT_CALL(mockInstViewModel, detIndicesToDetIDs(Eq(detIndices))).Times(1).WillOnce(Return(detIDs));
    EXPECT_CALL(mockModel, setSelectedBanks(detIDs)).Times(1);
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

  void expectActivateRectangularROIMode(MockPreviewView &mockView, MockRegionSelector &mockRegionSelector) {
    // TODO Disable edit button when implemented
    // EXPECT_CALL(mockView, setEditROIState(Eq(false))).Times(1);
    EXPECT_CALL(mockView, setRectangularROIState(Eq(true))).Times(1);
    EXPECT_CALL(mockRegionSelector, addRectangularRegion()).Times(1);
  }

  void expectReduceAsyncCalledOnSelectedRegion(MockPreviewView &mockView, MockPreviewModel &mockModel,
                                               MockJobManager &mockJobManager, MockRegionSelector &mockRegionSelector) {
    // Check ROI is set
    auto roi = IRegionSelector::Selection{3.5, 11.23};
    EXPECT_CALL(mockRegionSelector, getRegion()).Times(1).WillOnce(Return(roi));
    EXPECT_CALL(mockModel, setSelectedRegion(roi)).Times(1);
    // Check theta is set
    auto theta = 0.3;
    EXPECT_CALL(mockView, getAngle()).Times(1).WillOnce(Return(theta));
    EXPECT_CALL(mockModel, setTheta(theta)).Times(1);
    // Check reduction is executed
    EXPECT_CALL(mockModel, reduceAsync(Ref(mockJobManager))).Times(1);
  }
};
