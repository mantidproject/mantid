// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "PreviewModel.h"
#include "test/ReflMockObjects.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace Mantid::API;
using ::testing::_;
using ::testing::Invoke;

class PreviewModelTest : public CxxTest::TestSuite {
public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_load_workspace_from_ads() {
    auto mockJobManager = MockJobManager();
    EXPECT_CALL(mockJobManager, startPreprocessing(_)).Times(0);

    PreviewModel model;
    auto workspaceName = std::string("test workspace");
    AnalysisDataService::Instance().addOrReplace(workspaceName, createWorkspace());

    TS_ASSERT(model.loadWorkspaceFromAds(workspaceName));
    auto workspace = model.getLoadedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace->getName(), workspaceName);
  }

  void test_load_workspace_from_file() {
    auto mockJobManager = MockJobManager();

    auto expectedWs = createWorkspace();
    auto wsLoadEffect = [&expectedWs](PreviewRow &row) { row.setLoadedWs(expectedWs); };

    EXPECT_CALL(mockJobManager, startPreprocessing(_)).Times(1).WillOnce(Invoke(wsLoadEffect));

    PreviewModel model;
    auto workspaceName = std::string("not there");

    model.loadAndPreprocessWorkspaceAsync(workspaceName, mockJobManager);
    auto workspace = model.getLoadedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace, expectedWs);
  }

private:
  MatrixWorkspace_sptr createWorkspace() { return WorkspaceCreationHelper::create2DWorkspace(1, 1); }
};
