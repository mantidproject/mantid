// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"
#include "Reduction/ReductionAlgorithmUtils.h"

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

private:
  std::string m_filename;
  std::string m_inputWorkspace;
  std::vector<int> m_detectorList;
  double m_startX;
  double m_endX;
  std::string m_outputWorkspace;
};
