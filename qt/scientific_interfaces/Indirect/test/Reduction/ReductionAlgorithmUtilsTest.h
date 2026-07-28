// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/ScopedFileHelper.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"
#include "Reduction/ReductionAlgorithmUtils.h"
#include <filesystem>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class ReductionAlgorithmUtilsTest : public CxxTest::TestSuite {
public:
  static ReductionAlgorithmUtilsTest *createSuite() { return new ReductionAlgorithmUtilsTest(); }
  static void destroySuite(ReductionAlgorithmUtilsTest *suite) { delete suite; }

  void setUp() override {
    m_filename = "C:/path/to/file.raw";
    m_inputWorkspace = "InputName";
    m_detectorList = std::vector<int>{1, 2, 3};
    m_startX = 1.1;
    m_endX = 2.2;
    m_outputWorkspace = "OutputName";
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().clear();

    for (auto const &filename : m_testFiles)
      std::filesystem::remove(filename);

    m_testWorkspaceNames.clear();
    m_testFiles.clear();
  }

  void test_loadConfiguredAlg_returns_the_expected_properties_for_TOSCA() {
    auto alg = loadConfiguredAlg(m_filename, "TOSCA", m_detectorList, m_outputWorkspace);

    auto const &properties = alg->getAlgorithmRuntimeProps();
    TS_ASSERT_EQUALS(2, properties.propertyCount());

    std::string filename = properties.getProperty("Filename");
    std::string outputWorkspace = properties.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(filename, m_filename);
    TS_ASSERT_EQUALS(outputWorkspace, m_outputWorkspace);
  }

  void test_loadConfiguredAlg_returns_the_expected_properties_for_TFXA() {
    auto alg = loadConfiguredAlg(m_filename, "TFXA", m_detectorList, m_outputWorkspace);

    auto const &properties = alg->getAlgorithmRuntimeProps();
    TS_ASSERT_EQUALS(5, properties.propertyCount());

    std::string filename = properties.getProperty("Filename");
    bool loadLogFiles = properties.getProperty("LoadLogFiles");
    int spectrumMin = properties.getProperty("SpectrumMin");
    int spectrumMax = properties.getProperty("SpectrumMax");
    std::string outputWorkspace = properties.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(filename, m_filename);
    TS_ASSERT(!loadLogFiles);
    TS_ASSERT_EQUALS(spectrumMin, m_detectorList.front());
    TS_ASSERT_EQUALS(spectrumMax, m_detectorList.back());
    TS_ASSERT_EQUALS(outputWorkspace, m_outputWorkspace);
  }

  void test_calculateFlatBackgroundConfiguredAlg_returns_the_expected_properties() {
    auto alg = calculateFlatBackgroundConfiguredAlg(m_inputWorkspace, m_startX, m_endX, m_outputWorkspace);

    auto const &properties = alg->getAlgorithmRuntimeProps();
    std::string inputWorkspace = properties.getProperty("InputWorkspace");
    double startX = properties.getProperty("StartX");
    double endX = properties.getProperty("EndX");
    std::string outputWorkspace = properties.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(inputWorkspace, m_inputWorkspace);
    TS_ASSERT_EQUALS(startX, m_startX);
    TS_ASSERT_EQUALS(endX, m_endX);
    TS_ASSERT_EQUALS(outputWorkspace, m_outputWorkspace);
  }

  void test_groupDetectorsConfiguredAlg_returns_the_expected_properties() {
    auto alg = groupDetectorsConfiguredAlg(m_inputWorkspace, m_detectorList, m_outputWorkspace);

    auto const &properties = alg->getAlgorithmRuntimeProps();
    std::string inputWorkspace = properties.getProperty("InputWorkspace");
    std::vector<int> detectorList = properties.getProperty("DetectorList");
    std::string outputWorkspace = properties.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(inputWorkspace, m_inputWorkspace);
    TS_ASSERT_EQUALS(detectorList, m_detectorList);
    TS_ASSERT_EQUALS(outputWorkspace, m_outputWorkspace);
  }

  void test_loadFilesWithSum_loads_and_averages_two_regular_runs() {
    auto const parameterFile = createTestParameterFile();

    auto const run1 = createTestRun("run1", std::vector<double>{2.0, 4.0, 6.0, 8.0});
    auto const run2 = createTestRun("run2", std::vector<double>{4.0, 8.0, 12.0, 16.0});

    std::string outputName;
    TS_ASSERT_THROWS_NOTHING(outputName = loadFilesWithSum({run1, run2}, parameterFile.getFileName(), false));

    auto const outName1 = std::filesystem::path(run1).stem().string();
    auto const outName2 = std::filesystem::path(run2).stem().string();

    TS_ASSERT_EQUALS(outputName, outputName);

    auto &ads = Mantid::API::AnalysisDataService::Instance();
    Mantid::API::MatrixWorkspace_sptr outputWorkspace;

    TS_ASSERT_THROWS_NOTHING(outputWorkspace = ads.retrieveWS<Mantid::API::MatrixWorkspace>(outputName));

    TS_ASSERT_EQUALS(outputWorkspace->getNumberHistograms(), 4);

    // loadFilesWithSum averages the merged runs.
    TS_ASSERT_DELTA(outputWorkspace->y(0)[0], 3.0, 1e-12);
    TS_ASSERT_DELTA(outputWorkspace->y(1)[0], 6.0, 1e-12);
    TS_ASSERT_DELTA(outputWorkspace->y(2)[0], 9.0, 1e-12);
    TS_ASSERT_DELTA(outputWorkspace->y(3)[0], 12.0, 1e-12);

    // The secondary detector workspace should be removed
    TS_ASSERT(!ads.doesExist(run2));
  }

private:
  Mantid::API::IAlgorithm_sptr createTestAlgorithm(std::string const &name) {
    auto algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(name);
    algorithm->initialize();
    algorithm->setRethrows(true);
    return algorithm;
  }

  std::string createTestRun(std::string const &fileName, std::vector<double> const &yValues) {
    constexpr int numberOfSpectra = 4;

    auto const wsName = "__" + fileName;
    m_testWorkspaceNames.emplace_back(wsName);

    auto const createWorkspace = createTestAlgorithm("CreateWorkspace");
    createWorkspace->setProperty("DataX", std::vector<double>{0.0, 1.0});
    createWorkspace->setProperty("DataY", yValues);
    createWorkspace->setProperty("DataE", std::vector<double>(yValues.size(), 0.0));
    createWorkspace->setProperty("NSpec", numberOfSpectra);
    createWorkspace->setPropertyValue("UnitX", "TOF");
    createWorkspace->setPropertyValue("OutputWorkspace", wsName);
    createWorkspace->execute();

    // DUM unit testing instrument
    auto const loadInstrument = createTestAlgorithm("LoadInstrument");
    loadInstrument->setPropertyValue("Filename", "unit_testing/DUM_Definition.xml");
    loadInstrument->setPropertyValue("Workspace", wsName);
    loadInstrument->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loadInstrument->execute();

    auto const filename =
        (std::filesystem::path(Mantid::Kernel::ConfigService::Instance().getTempDir()) / (fileName + ".nxs")).string();

    std::filesystem::remove(filename);
    m_testFiles.emplace_back(filename);

    auto saveWorkspace = createTestAlgorithm("SaveNexusProcessed");
    saveWorkspace->setPropertyValue("InputWorkspace", wsName);
    saveWorkspace->setPropertyValue("Filename", filename);
    saveWorkspace->execute();

    return saveWorkspace->getPropertyValue("Filename");
  }

  ScopedFileHelper::ScopedFile createTestParameterFile() {
    static std::string const contents = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<parameter-file instrument="DUM" valid-from="1900-01-01T00:00:00">
  <component-link name="DUM">
    <parameter name="Workflow.Monitor1-SpectrumNumber">
      <value val="0"/>
    </parameter>
    <parameter name="Workflow.ChopDataIfGreaterThan">
      <value val="100"/>
    </parameter>

  </component-link>
</parameter-file>
)xml";

    return ScopedFileHelper::ScopedFile(contents, "test_ParamFile.xml");
  }

  std::string m_filename;
  std::string m_inputWorkspace;
  std::vector<int> m_detectorList;
  double m_startX;
  double m_endX;
  std::string m_outputWorkspace;
  std::vector<std::string> m_testFiles;
  std::vector<std::string> m_testWorkspaceNames;
};
