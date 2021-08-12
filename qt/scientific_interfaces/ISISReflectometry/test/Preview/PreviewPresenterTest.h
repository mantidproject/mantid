// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Batch/BatchJobAlgorithm.h"
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

using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::ReturnRef;

class PreviewPresenterTest : public CxxTest::TestSuite {
  using MockViewT = std::unique_ptr<MockPreviewView>;
  using MockModelT = std::unique_ptr<MockPreviewModel>;

public:
  void test_notify_load_workspace_requested() {
    auto mockModel = makeModel();
    auto mockView = makeView();
    auto const workspaceName = std::string("test workspace");

    EXPECT_CALL(*mockView, getWorkspaceName()).Times(1).WillOnce(Return(workspaceName));
    EXPECT_CALL(*mockModel, loadWorkspace(Eq(workspaceName))).Times(1);

    auto presenter = PreviewPresenter(mockView.get(), std::move(mockModel));
    presenter.notifyLoadWorkspaceRequested();
  }

  void test_notify_preprocessing_algorithm_complete() {
    auto mockModel = makeModel();
    auto mockView = makeView();

    auto row = PreviewRow({"12345"});
    auto configuredAlg = makeConfiguredAlg(row);

    EXPECT_CALL(*mockModel, getLoadedWs).Times(1);

    auto presenter = PreviewPresenter(mockView.get(), std::move(mockModel));
    presenter.notifyAlgorithmComplete(configuredAlg);
  }

  void test_notify_preprocessing_algorithm_skips_non_existing_rows() {
    auto row = makeEmptyRow();
    auto group = makeEmptyGroup();

    auto items = std::array<Item *, 2>{&row, &group};

    for (auto *item : items) {
      auto mockModel = makeModel();
      auto mockView = makeView();

      auto configuredAlg = makeConfiguredAlg(*item);

      EXPECT_CALL(*mockModel, getLoadedWs).Times(0);

      auto presenter = PreviewPresenter(mockView.get(), std::move(mockModel));
      presenter.notifyAlgorithmComplete(configuredAlg);
    }
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

  std::shared_ptr<BatchJobAlgorithm> makeConfiguredAlg(Item &item) {
    Mantid::API::IAlgorithm_sptr mockAlg = std::make_shared<WorkspaceCreationHelper::StubAlgorithm>();
    auto properties = IConfiguredAlgorithm::AlgorithmRuntimeProps();
    auto configuredAlg = std::make_shared<BatchJobAlgorithm>(mockAlg, properties, nullptr, &item);
    return configuredAlg;
  }
};
