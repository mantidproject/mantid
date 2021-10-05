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
#include "MockPreviewModel.h"
#include "MockPreviewView.h"
#include "PreviewPresenter.h"
#include "Reduction/PreviewRow.h"
#include "TestHelpers/ModelCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <memory>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using namespace MantidQt::API;

using ::testing::ByRef;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

class PreviewPresenterTest : public CxxTest::TestSuite {
  using MockViewT = std::unique_ptr<MockPreviewView>;
  using MockModelT = std::unique_ptr<MockPreviewModel>;

public:
  void test_notify_load_workspace_requested() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto mockJobManager = makeJobManager();
    auto const workspaceName = std::string("test workspace");

    EXPECT_CALL(*mockView, getWorkspaceName()).Times(1).WillOnce(Return(workspaceName));
    EXPECT_CALL(*mockModel, loadWorkspace(Eq(workspaceName), Ref(*mockJobManager))).Times(1);

    auto presenter = PreviewPresenter(mockView.get(), std::move(mockModel), std::move(mockJobManager));
    presenter.notifyLoadWorkspaceRequested();
  }

  void test_notify_load_workspace_complete() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto mockJobManager = makeJobManager();

    auto row = PreviewRow({"12345"});

    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    EXPECT_CALL(*mockModel, getLoadedWs).Times(1).WillOnce(Return(ws));

    auto presenter = PreviewPresenter(mockView.get(), std::move(mockModel), std::move(mockJobManager));
    presenter.notifyLoadWorkspaceCompleted();
  }

private:
  MockViewT makeView() {
    auto mockView = std::make_unique<MockPreviewView>();
    EXPECT_CALL(*mockView, subscribe(NotNull())).Times(1);
    return mockView;
  }

  MockModelT makeModel() {
    auto mockModel = std::make_unique<MockPreviewModel>();
    return mockModel;
  }

  std::unique_ptr<IJobManager> makeJobManager() {
    auto mockJobManager = std::make_unique<MockJobManager>();
    EXPECT_CALL(*mockJobManager, subscribe(NotNull())).Times(1);
    return mockJobManager;
  }
};
