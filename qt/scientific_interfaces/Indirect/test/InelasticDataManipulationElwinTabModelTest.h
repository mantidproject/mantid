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
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class ElasticWindowMultiple : public Algorithm {
public:
  const std::string name() const override { return "ElasticWindowMultiple"; };
  int version() const override { return 1; };
  const std::string summary() const override { return "A mock the ElasticWindowMultiple algorithm"; };

private:
  void init() override {
    declareProperty("InputWorkspaces", "InputWorkspaces");

    declareProperty("OutputInQ", "OutputInQ");
    declareProperty("OutputInQSquared", "OutputInQSquared");
    declareProperty("OutputELF", "OutputELF");
    declareProperty("OutputELT", "OutputELT");

    declareProperty("SampleEnvironmentLogName", "SampleEnvironmentLogName");
    declareProperty("SampleEnvironmentLogValue", "SampleEnvironmentLogValue");
    declareProperty("IntegrationRangeStart", 0.0);
    declareProperty("IntegrationRangeEnd", 1.0);
    declareProperty("BackgroundRangeStart", 0.0);
    declareProperty("BackgroundRangeEnd", 1.0);
  };
  void exec() override {
    ITableWorkspace_sptr outputWS = WorkspaceFactory::Instance().createTable();
    outputWS->addColumn("str", "InputWorkspaces");
    outputWS->addColumn("str", "OutputInQ");
    outputWS->addColumn("str", "OutputInQSquared");
    outputWS->addColumn("str", "OutputELF");
    outputWS->addColumn("str", "OutputELT");
    outputWS->addColumn("str", "SampleEnvironmentLogName");
    outputWS->addColumn("str", "SampleEnvironmentLogValue");
    outputWS->addColumn("double", "IntegrationRangeStart");
    outputWS->addColumn("double", "IntegrationRangeEnd");
    outputWS->addColumn("double", "BackgroundRangeStart");
    outputWS->addColumn("double", "BackgroundRangeEnd");

    TableRow newRow = outputWS->appendRow();
    auto inWS = getPropertyValue("InputWorkspaces");
    auto outInQ = getPropertyValue("OutputInQ");
    auto outInQSq = getPropertyValue("OutputInQSquared");
    auto outELF = getPropertyValue("OutputELF");
    auto outELT = getPropertyValue("OutputELT");
    auto samLogName = getPropertyValue("SampleEnvironmentLogName");
    auto samLogVal = getPropertyValue("SampleEnvironmentLogValue");
    double iRS = getProperty("IntegrationRangeStart");
    double iRE = getProperty("IntegrationRangeEnd");
    double bRS = getProperty("BackgroundRangeStart");
    double bRE = getProperty("BackgroundRangeEnd");

    newRow << inWS << outInQ << outInQSq << outELF << outELT << samLogName << samLogVal << iRS << iRE << bRS << bRE;

    Mantid::API::AnalysisDataService::Instance().addOrReplace("outputWS", outputWS);
  };
};
DECLARE_ALGORITHM(ElasticWindowMultiple)

class InelasticDataManipulationElwinTabModelTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  InelasticDataManipulationElwinTabModelTest() = default;

  void setUp() override { m_model = std::make_unique<InelasticDataManipulationElwinTabModel>(); }

  void test_algorithm_set_up() {
    MantidQt::API::BatchAlgorithmRunner batch;
    QString wsBaseName = "Workspace_name";
    // The ElasticWindowMultiple algorithm is a python algorithm and so can not be called in c++ tests
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("Workspace_name_sqw", m_workspace);
    m_model->setIntegrationStart(-0.1);
    m_model->setIntegrationEnd(0.1);
    m_model->setBackgroundStart(-0.2);
    m_model->setBackgroundEnd(-0.15);
    m_model->setBackgroundSubtraction(true);
    m_model->setNormalise(true);

    m_model->setupElasticWindowMultiple(&batch, wsBaseName, "Workspace_name_sqw", "sampleLogName", "sampleLogValue");

    batch.executeBatch();

    ITableWorkspace_sptr outputWS =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("outputWS");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 0), "Workspace_name_sqw");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 1), "Workspace_name_elwin_eq");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 2), "Workspace_name_elwin_eq2");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 3), "Workspace_name_elwin_elf");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 4), "Workspace_name_elwin_elt");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 5), "sampleLogName");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 6), "sampleLogValue");
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 7), -0.1);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 8), 0.1);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 9), -0.2);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 10), -0.15);
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