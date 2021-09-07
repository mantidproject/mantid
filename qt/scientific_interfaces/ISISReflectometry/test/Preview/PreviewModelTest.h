// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "PreviewModel.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace Mantid::API;

class PreviewModelTest : public CxxTest::TestSuite {
public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_load_workspace_from_ads() {
    PreviewModel model;
    auto workspaceName = std::string("test workspace");
    AnalysisDataService::Instance().addOrReplace(workspaceName, createWorkspace());

    model.loadWorkspace(workspaceName);
    auto workspace = model.getInstViewWorkspace();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(model.getInstViewWorkspace()->getName(), workspaceName);
  }

private:
  MatrixWorkspace_sptr createWorkspace() { return WorkspaceCreationHelper::create2DWorkspace(1, 1); }
};
