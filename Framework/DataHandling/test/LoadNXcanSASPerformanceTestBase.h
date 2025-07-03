// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadNXcanSAS.h"
#include "NXcanSASTestHelper.h"

using namespace NXcanSASTestHelper;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;

class ILoadNXcanSASPerformanceTest {
public:
  virtual void test_execute() = 0;
  void setUp() {
    setupUniqueParams();
    setupParamsAndAlg();
  }
  void tearDown() {
    AnalysisDataService::Instance().clear();
    removeFile(m_parameters.filePath());
  }

protected:
  LoadNXcanSAS alg;
  NXcanSASTestParameters m_parameters;

  void save_no_assert(const MatrixWorkspace_sptr &ws, NXcanSASTestParameters &parameters) {
    auto saveAlg = AlgorithmManager::Instance().createUnmanaged("SaveNXcanSAS");
    saveAlg->initialize();
    saveAlg->setProperty("Filename", parameters.filePath());
    saveAlg->setProperty("InputWorkspace", ws);
    saveAlg->setProperty("RadiationSource", parameters.radiationSource);
    if (!parameters.detectors.empty()) {
      std::string detectorsAsString = concatenateStringVector(parameters.detectors);
      saveAlg->setProperty("DetectorNames", detectorsAsString);
    }
    saveAlg->execute();
  }

  void setupParamsAndAlg() {
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;

    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";
    alg.initialize();
    alg.setPropertyValue("Filename", m_parameters.filePath());
    alg.setProperty("LoadTransmission", true);
    alg.setPropertyValue("OutputWorkspace", outWsName);
  }

  virtual void setupUniqueParams() = 0;
};
