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
#include "Processor/IqtModel.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class TransformToIqt : public Algorithm {
public:
  const std::string name() const override { return "TransformToIqt"; };
  int version() const override { return 1; };
  const std::string summary() const override { return "A mock the TransformToIqt algorithm"; };

private:
  void init() override {
    declareProperty("SampleWorkspace", "SampleWorkspace");
    declareProperty("ResolutionWorkspace", "ResolutionWorkspace");

    declareProperty("OutputWorkspace", "OutputWorkspace");

    declareProperty("NumberOfIterations", "NumberOfIterations");
    declareProperty("CalculateErrors", false);
    declareProperty("DryRun", true);
    declareProperty("EnergyMin", 0.0);
    declareProperty("EnergyMax", 1.0);
    declareProperty("BinReductionFactor", 2.0);
    declareProperty("EnforceNormalization", true);
  };
  void exec() override {
    ITableWorkspace_sptr outputWS = WorkspaceFactory::Instance().createTable();
    outputWS->addColumn("str", "SampleWorkspace");
    outputWS->addColumn("str", "ResolutionWorkspace");
    outputWS->addColumn("str", "OutputWorkspace");
    outputWS->addColumn("str", "NumberOfIterations");
    outputWS->addColumn("str", "CalculateErrors");
    outputWS->addColumn("str", "DryRun");
    outputWS->addColumn("double", "EnergyMin");
    outputWS->addColumn("double", "EnergyMax");
    outputWS->addColumn("double", "BinReductionFactor");
    outputWS->addColumn("str", "EnforceNormalization");

    TableRow newRow = outputWS->appendRow();
    auto inWS = getPropertyValue("SampleWorkspace");
    auto resWS = getPropertyValue("ResolutionWorkspace");
    auto outWS = getPropertyValue("OutputWorkspace");
    auto nIt = getPropertyValue("NumberOfIterations");
    auto calcErr = getPropertyValue("CalculateErrors");
    auto dryRun = getPropertyValue("DryRun");
    double eMin = getProperty("EnergyMin");
    double eMax = getProperty("EnergyMax");
    double bRF = getProperty("BinReductionFactor");
    auto enfNorm = getPropertyValue("EnforceNormalization");

    newRow << inWS << resWS << outWS << nIt << calcErr << dryRun << eMin << eMax << bRF << enfNorm;

    Mantid::API::AnalysisDataService::Instance().addOrReplace("outputWS", outputWS);
  };
};
DECLARE_ALGORITHM(TransformToIqt)

class IqtModelTest : public CxxTest::TestSuite {
public:
  static IqtModelTest *createSuite() { return new IqtModelTest(); }

  static void destroySuite(IqtModelTest *suite) { delete suite; }

  void setUp() override { m_model = std::make_unique<IqtModel>(); }

  void test_algorrithm_set_up() {
    MantidQt::API::BatchAlgorithmRunner batch;
    // The Moments algorithm is a python algorithm and so can not be called in c++ tests
    m_sampWorkspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("sample_name_sqw", m_sampWorkspace);
    m_resWorkspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("res_name_sqw", m_resWorkspace);

    m_model->setSampleWorkspace("sample_name_sqw");
    m_model->setResWorkspace("res_name_sqw");
    m_model->setEnergyMin(-0.1);
    m_model->setEnergyMax(0.1);
    m_model->setNumBins(10);
    m_model->setCalculateErrors(true);
    m_model->setEnforceNormalization(true);
    m_model->setNIterations("50");

    m_model->setupTransformToIqt(&batch, "outputWS");
    batch.executeBatch();

    ITableWorkspace_sptr outputWS =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("outputWS");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 0), "sample_name_sqw");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 1), "res_name_sqw");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 2), "outputWS");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 3), "50");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 4), "1");
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 5), "0");
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 6), -0.1);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 7), 0.1);
    TS_ASSERT_EQUALS(outputWS->cell<double>(0, 8), 10);
    TS_ASSERT_EQUALS(outputWS->cell<std::string>(0, 9), "1");
  }

private:
  MatrixWorkspace_sptr m_sampWorkspace;
  MatrixWorkspace_sptr m_resWorkspace;
  std::unique_ptr<IqtModel> m_model;
};