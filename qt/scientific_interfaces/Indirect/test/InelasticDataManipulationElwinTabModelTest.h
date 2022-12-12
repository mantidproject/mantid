// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataValidationHelper.h"
#include "InelasticDataManipulationElwinTabModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class InelasticDataManipulationElwinTabModelTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  InelasticDataManipulationElwinTabModelTest() = default;

  void setUp() override { m_model = std::make_unique<InelasticDataManipulationElwinTabModel>(); }

  void test_algorithm_set_up() {
    // The ElasticWindowMultiple algorithm is a python algorithm and so can not be called in c++ tests
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_sqw", m_workspace);

    m_model->setIntegrationStart(-0.1);
    m_model->setIntegrationEnd(0.1);
    m_model->setBackgroundStart(-0.2);
    m_model->setBackgroundEnd(-0.15);
    m_model->setBackgroundSubtraction(true);
    m_model->setNormalise(true);
  }

  void test_groupAlgorithm_ungroupAlgorithm_set_up() {
    MantidQt::API::BatchAlgorithmRunner batch;
    auto workspace1 = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name1_sqw", workspace1);
    auto workspace2 = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name2_sqw", workspace2);
    std::string workspaceInputString = "Workspace_name1_sqw, Workspace_name2_sqw";
    m_model->setupGroupAlgorithm(&batch, workspaceInputString, "groupedWS");
    batch.executeBatch();
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("groupedWS"));

    m_model->ungroupAlgorithm("groupedWS");
    TS_ASSERT(!Mantid::API::AnalysisDataService::Instance().doesExist("groupedWS"));
  }

  void test_LoadAlgorithm_set_up() {
    // The ElasticWindowMultiple algorithm is a python algorithm and so can not be called in c++ tests

    MantidQt::API::BatchAlgorithmRunner batch;
    m_model->setupLoadAlgorithm(&batch, "MultispectralTestData.nxs", "LoadedWsName");
    batch.executeBatch();
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("LoadedWsName"));
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<InelasticDataManipulationElwinTabModel> m_model;
};