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

  void test_algorrithm_set_up() {
    // The Moments algorithm is a python algorithm and so can not be called in c++ tests
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_sqw", m_workspace);

    m_model->setIntegrationStart(-0.1);
    m_model->setIntegrationEnd(0.1);
    m_model->setBackgroundStart(-0.2);
    m_model->setBackgroundEnd(-0.15);
    m_model->setBackgroundSubtraction(true);
    m_model->setNormalise(true);
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<InelasticDataManipulationElwinTabModel> m_model;
};