// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataValidationHelper.h"
#include "InelasticDataManipulationSymmetriseTabModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class InelasticDataManipulationSymmetriseTabModelTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  InelasticDataManipulationSymmetriseTabModelTest() = default;

  void setUp() override { m_model = std::make_unique<InelasticDataManipulationSymmetriseTabModel>(); }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_setupPropertiesAlgorithm() {
    QString inputWS = "Workspace_name_red";
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS.toStdString(), m_workspace);
    MantidQt::API::BatchAlgorithmRunner batch;
    m_model->setEMin(0.05);
    m_model->setEMax(0.6);
    m_model->setWorkspaceName(inputWS);
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<InelasticDataManipulationSymmetriseTabModel> m_model;
};