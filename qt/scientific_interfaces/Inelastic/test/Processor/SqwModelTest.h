// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "Processor/SqwModel.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class SqwModelTest : public CxxTest::TestSuite {
public:
  static SqwModelTest *createSuite() { return new SqwModelTest(); }

  static void destroySuite(SqwModelTest *suite) { delete suite; }

  void setUp() override { m_model = std::make_unique<SqwModel>(); }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    m_model.reset();
  }

  void test_algorrithm_set_up() {
    // The Moments algorithm is a python algorithm and so can not be called in
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_red", m_workspace);
    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setEMin(-0.4);
    m_model->setEWidth(0.005);
    m_model->setEMax(0.4);
    m_model->setQMin(0.8);
    m_model->setQWidth(0.05);
    m_model->setQMax(1.8);
    m_model->setEFixed(0.4);
    m_model->setRebinInEnergy(true);
  }

  void test_output_workspace() {
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_red", m_workspace);
    m_model->setInputWorkspace("Workspace_name_red");
    auto outputWorkspaceName = m_model->getOutputWorkspace();
    TS_ASSERT(outputWorkspaceName == "Workspace_name_sqw");
  }

  void test_setupRebinAlgorithm() {
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_red", m_workspace);
    MantidQt::API::BatchAlgorithmRunner batch;
    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setEMin(-0.4);
    m_model->setEWidth(0.005);
    m_model->setEMax(0.4);
    m_model->setRebinInEnergy(true);
    auto const rebinAlg = m_model->setupRebinAlgorithm();
    batch.setQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>{rebinAlg});
    batch.executeBatch();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace_name_r"));
  }

  void test_setupAlgorithmsERebinFalse() {
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_red", m_workspace);
    MantidQt::API::BatchAlgorithmRunner batch;

    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setEMin(-0.4);
    m_model->setEWidth(0.005);
    m_model->setEMax(0.4);
    m_model->setQMin(0.8);
    m_model->setQWidth(0.05);
    m_model->setQMax(1.8);
    m_model->setEFixed(0.4);
    std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algoQueue = {};
    algoQueue.emplace_back(m_model->setupSofQWAlgorithm());
    algoQueue.emplace_back(m_model->setupAddSampleLogAlgorithm());
    batch.setQueue(algoQueue);
    batch.executeBatch();
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("Workspace_name_r"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace_name_sqw"));
  }

  void test_setupAlgorithmsERebinTrue() {
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_red", m_workspace);
    MantidQt::API::BatchAlgorithmRunner batch;

    m_model->setInputWorkspace("Workspace_name_red");
    m_model->setEMin(-0.4);
    m_model->setEWidth(0.005);
    m_model->setEMax(0.4);
    m_model->setQMin(0.8);
    m_model->setQWidth(0.05);
    m_model->setQMax(1.8);
    m_model->setEFixed(0.4);
    m_model->setRebinInEnergy(true);
    std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algoQueue = {};
    algoQueue.emplace_back(m_model->setupRebinAlgorithm());
    algoQueue.emplace_back(m_model->setupSofQWAlgorithm());
    algoQueue.emplace_back(m_model->setupAddSampleLogAlgorithm());
    batch.setQueue(algoQueue);
    batch.executeBatch();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace_name_r"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace_name_sqw"));
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SqwModel> m_model;
};
