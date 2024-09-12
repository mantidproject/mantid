// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "Processor/SymmetriseModel.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class Symmetrise : public Algorithm {
public:
  const std::string name() const override { return "Symmetrise"; };
  int version() const override { return 1; };
  const std::string summary() const override { return "Symmetrise Mock algorithm"; };

private:
  void init() override {
    declareProperty("InputWorkspace", "InputWorkspace");

    declareProperty("OutputWorkspace", "OutputWorkspace");
    declareProperty("OutputPropertiesTable", "OutputPropertiesTable");

    declareProperty("SpectraRange", std::vector<int>{0, 2});
    declareProperty("XMin", 0.05);
    declareProperty("XMax", 0.6);
  };

  void exec() override {
    ITableWorkspace_sptr outputWS = WorkspaceFactory::Instance().createTable();
    outputWS->addColumn("str", "InputWorkspace");
    outputWS->addColumn("str", "OutputWorkspace");
    outputWS->addColumn("str", "OutputPropertiesTable");
    outputWS->addColumn("str", "SpectraRange");
    outputWS->addColumn("double", "XMin");
    outputWS->addColumn("double", "XMax");

    TableRow newRow = outputWS->appendRow();
    auto inWS = getPropertyValue("InputWorkspace");
    auto outWs = getPropertyValue("OutputWorkspace");
    auto outPropWs = getPropertyValue("OutputPropertiesTable");
    auto spectraRange = getPropertyValue("SpectraRange");
    double xMin = getProperty("XMin");
    double xMax = getProperty("XMax");

    newRow << inWS << outWs << outPropWs << spectraRange << xMin << xMax;

    Mantid::API::AnalysisDataService::Instance().addOrReplace("outputWS", outputWS);
  };
};
DECLARE_ALGORITHM(Symmetrise)

class SymmetriseModelTest : public CxxTest::TestSuite {
public:
  static SymmetriseModelTest *createSuite() { return new SymmetriseModelTest(); }

  static void destroySuite(SymmetriseModelTest *suite) { delete suite; }

  void setUp() override { m_model = std::make_unique<SymmetriseModel>(); }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    m_model.reset();
  }

  void test_preview_positive_setup() {
    std::string inputWS = "Workspace_name_red";
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);

    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, m_workspace);
    MantidQt::API::BatchAlgorithmRunner batch;

    m_model->setEMin(0.05);
    m_model->setEMax(0.6);
    m_model->setWorkspaceName(inputWS);
    m_model->setIsPositiveReflect(true);

    std::vector<int> spectraRange(2, 4);
    auto const previewAlgo = m_model->setupPreviewAlgorithm(spectraRange);
    batch.setQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>{previewAlgo});
    batch.executeBatch();

    ITableWorkspace_sptr outputWS =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("outputWS");

    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 0), "Workspace_name_red");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 1), "__Symmetrise_temp");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 2), "__SymmetriseProps_temp");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 3), "4,4");
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 4), 0.05);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 5), 0.6);
  }

  void test_preview_negative_setup() {
    std::string inputWS = "Workspace_name_red";
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);

    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, m_workspace);
    MantidQt::API::BatchAlgorithmRunner batch;

    m_model->setEMin(0.05);
    m_model->setEMax(0.6);
    m_model->setWorkspaceName(inputWS);
    m_model->setIsPositiveReflect(false);

    std::vector<int> spectraRange(2, 4);
    auto const previewAlgo = m_model->setupPreviewAlgorithm(spectraRange);
    batch.setQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>{previewAlgo});
    batch.executeBatch();

    ITableWorkspace_sptr outputWS =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("outputWS");

    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 0), "Workspace_name_red_reflected");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 1), "__Symmetrise_temp");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 2), "__SymmetriseProps_temp");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 3), "4,4");
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 4), 0.05);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 5), 0.6);
  }

  void test_run_positive_setup() {
    std::string inputWS = "Workspace_name_red";
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);

    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, m_workspace);
    MantidQt::API::BatchAlgorithmRunner batch;

    m_model->setEMin(0.05);
    m_model->setEMax(0.6);
    m_model->setWorkspaceName(inputWS);
    m_model->setIsPositiveReflect(true);
    auto const previewAlgo = m_model->setupSymmetriseAlgorithm();
    batch.setQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>{previewAlgo});
    batch.executeBatch();

    ITableWorkspace_sptr outputWS =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("outputWS");

    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 0), "Workspace_name_red");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 1), "Workspace_name_sym_pn_red");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 2), "__SymmetriseProps_temp");
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 4), 0.05);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 5), 0.6);
  }

  void test_run_negative_setup() {
    std::string inputWS = "Workspace_name_red";
    m_workspace = WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(5, 6, true);

    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, m_workspace);
    MantidQt::API::BatchAlgorithmRunner batch;

    m_model->setEMin(0.05);
    m_model->setEMax(0.6);
    m_model->setWorkspaceName(inputWS);
    m_model->setIsPositiveReflect(false);
    auto previewAlgo = m_model->setupSymmetriseAlgorithm();
    batch.setQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>{previewAlgo});
    batch.executeBatch();

    ITableWorkspace_sptr outputWS =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("outputWS");

    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 0), "Workspace_name_red_reflected");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 1), "Workspace_name_sym_np_red");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 2), "__SymmetriseProps_temp");
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 4), 0.05);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 5), 0.6);
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SymmetriseModel> m_model;
};