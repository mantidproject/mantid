// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataValidationHelper.h"
#include "IndirectSqwModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class IndirectSqwModelTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectSqwModelTest() = default;

  void setUp() override { m_model = std::make_unique<IndirectSqwModel>(); }

  void test_algorrithm_set_up() {
    // The Moments algorithm is a python algorithm and so can not be called in
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_red", m_workspace);
    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setEMin(-0.4);
    m_model->setEWidth(0.005);
    m_model->setEMax(0.4);
    m_model->setQMin(0.8);
    m_model->setQWidth(0.05);
    m_model->setQMax(1.8);
    m_model->setEFixed("0.4");
    m_model->setRebinInEnergy(true);
  }

  void test_output_workspace() {
    m_model->setInputWorkspace("Workspace_name_red");
    auto outputWorkspaceName = m_model->getOutputWorkspace();
    TS_ASSERT(outputWorkspaceName == "Workspace_name_sqw");
  }

  void test_setupRebinAlgorithm() {
    MantidQt::API::BatchAlgorithmRunner batch;
    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setEMin(-0.4);
    m_model->setEWidth(0.005);
    m_model->setEMax(0.4);
    m_model->setRebinInEnergy(true);
    m_model->setupRebinAlgorithm(&batch);
    batch.executeBatch();
  }

  void test_setupRebinAlgorithmFalse() {
    MantidQt::API::BatchAlgorithmRunner batch;
    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setEMin(-0.4);
    m_model->setEWidth(0.005);
    m_model->setEMax(0.4);
    m_model->setRebinInEnergy(false);
    m_model->setupRebinAlgorithm(&batch);
    batch.executeBatch();
  }

  void test_setupSofQWAlgorithm() {
    MantidQt::API::BatchAlgorithmRunner batch;
    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setQMin(0.8);
    m_model->setQWidth(0.05);
    m_model->setQMax(1.8);
    m_model->setEFixed("0.4");
    m_model->setupSofQWAlgorithm(&batch);
    batch.executeBatch();
  }

  void test_setupAddSampleLogAlgorithm() {
    MantidQt::API::BatchAlgorithmRunner batch;
    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setupAddSampleLogAlgorithm(&batch);
    batch.executeBatch();
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<IndirectSqwModel> m_model;
};
