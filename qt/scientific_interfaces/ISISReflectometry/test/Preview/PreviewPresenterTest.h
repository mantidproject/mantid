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

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::API;

using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::ReturnRef;

class PreviewPresenterTest : public CxxTest::TestSuite {
public:
  void test_notify_load_workspace_requested() {
    auto mockModel = std::make_unique<MockPreviewModel>();
    auto mockView = std::make_unique<MockPreviewView>();
    auto const workspaceName = std::string("test workspace");

    EXPECT_CALL(*mockView, subscribe(NotNull())).Times(1);
    EXPECT_CALL(*mockView, getWorkspaceName()).Times(1).WillOnce(Return(workspaceName));
    EXPECT_CALL(*mockModel, loadWorkspace(Eq(workspaceName))).Times(1);

    auto presenter = PreviewPresenter(mockView.get(), std::move(mockModel));
    presenter.notifyLoadWorkspaceRequested();
  }

  void test_notify_preprocessing_algorithm_complete() {
    // TODO
  }
};
