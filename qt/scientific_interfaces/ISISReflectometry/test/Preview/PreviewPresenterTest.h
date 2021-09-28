// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../ReflMockObjects.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MockInstViewModel.h"
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

using ::testing::_;
using ::testing::ByRef;
using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

class PreviewPresenterTest : public CxxTest::TestSuite {
  using MockInstViewModelT = std::unique_ptr<MockInstViewModel>;
  using MockJobManagerT = std::unique_ptr<IJobManager>;
  using MockModelT = std::unique_ptr<MockPreviewModel>;
  using MockViewT = std::unique_ptr<MockPreviewView>;

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
    EXPECT_CALL(*mockView, setInstViewPanState(Eq(false))).Times(1);
    EXPECT_CALL(*mockView, setInstViewZoomState(Eq(false))).Times(1);
    EXPECT_CALL(*mockView, setInstViewSelectRectState(Eq(true))).Times(1);
    EXPECT_CALL(*mockView, setInstViewSelectRectMode()).Times(1);
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.notifyInstViewSelectRectRequested();
  }

  void test_notify_inst_view_pan_requested() {
    auto mockView = makeView();
    EXPECT_CALL(*mockView, setInstViewSelectRectState(Eq(false))).Times(1);
    EXPECT_CALL(*mockView, setInstViewZoomState(Eq(false))).Times(1);
    EXPECT_CALL(*mockView, setInstViewPanState(Eq(true))).Times(1);
    EXPECT_CALL(*mockView, setInstViewPanMode()).Times(1);
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.notifyInstViewPanRequested();
  }

  void test_notify_inst_view_zoom_requested() {
    auto mockView = makeView();
    EXPECT_CALL(*mockView, setInstViewSelectRectState(Eq(false))).Times(1);
    EXPECT_CALL(*mockView, setInstViewPanState(Eq(false))).Times(1);
    EXPECT_CALL(*mockView, setInstViewZoomState(Eq(true))).Times(1);
    EXPECT_CALL(*mockView, setInstViewZoomMode()).Times(1);
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.notifyInstViewZoomRequested();
  }

  void test_notify_inst_view_shape_changed() {
    auto mockView = makeView();
    // TODO check that the model is called to sum banks
    auto presenter = PreviewPresenter(packDeps(mockView.get()));
    presenter.notifyInstViewShapeChanged();
  }

private:
  MockViewT makeView() {
    auto mockView = std::make_unique<MockPreviewView>();
    EXPECT_CALL(*mockView, subscribe(NotNull())).Times(1);
    return mockView;
  }

  MockModelT makeModel() { return std::make_unique<MockPreviewModel>(); }

  std::unique_ptr<IJobManager> makeJobManager() {
    auto mockJobManager = std::make_unique<MockJobManager>();
    EXPECT_CALL(*mockJobManager, subscribe(NotNull())).Times(1);
    return mockJobManager;
  }

  MockInstViewModelT makeInstViewModel() { return std::make_unique<MockInstViewModel>(); }

  PreviewPresenter::Dependencies packDeps(MockPreviewView *view,
                                          MockModelT model = std::make_unique<MockPreviewModel>(),
                                          MockJobManagerT jobManager = std::make_unique<NiceMock<MockJobManager>>(),
                                          MockInstViewModelT instView = std::make_unique<MockInstViewModel>()) {
    return PreviewPresenter::Dependencies{view, std::move(model), std::move(jobManager), std::move(instView)};
  }

  void expectLoadWorkspaceCompleted(MockPreviewView &mockView, MockPreviewModel &mockModel,
                                    MockInstViewModel &mockInstViewModel) {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    auto samplePos = V3D(1, 2, 3);
    auto axis = V3D(4, 5, 6);

    EXPECT_CALL(mockModel, getLoadedWs).Times(1).WillOnce(Return(ws));
    EXPECT_CALL(mockInstViewModel, updateWorkspace(Eq(ws))).Times(1);
    EXPECT_CALL(mockInstViewModel, getInstrumentViewActor()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(mockInstViewModel, getSamplePos()).Times(1).WillOnce(Return(samplePos));
    EXPECT_CALL(mockInstViewModel, getAxis()).Times(1).WillOnce(Return(axis));
    EXPECT_CALL(mockView, plotInstView(Eq(nullptr), Eq(samplePos), Eq(axis)));
  }
};
