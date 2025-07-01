// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include <utility>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadNXcanSAS.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"
#include "NXcanSASTestHelper.h"

using Mantid::DataHandling::NXcanSAS::LoadNXcanSAS;
using namespace NXcanSASTestHelper;
using namespace Mantid::API;
using namespace Mantid::Kernel;
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

class LoadNXcanSASPerformanceTest1D : public ILoadNXcanSASPerformanceTest, public CxxTest::TestSuite {
public:
  void setUp() override { ILoadNXcanSASPerformanceTest::setUp(); }
  void tearDown() override { ILoadNXcanSASPerformanceTest::tearDown(); }
  void test_execute() override { alg.execute(); }

  static LoadNXcanSASPerformanceTest1D *createSuite() { return new LoadNXcanSASPerformanceTest1D(); }
  static void destroySuite(LoadNXcanSASPerformanceTest1D *suite) { delete suite; }
  void setupUniqueParams() override {
    m_parameters.hasDx = true;

    const auto ws = provide1DWorkspace(m_parameters);
    setXValuesOn1DWorkspace(ws, m_parameters.xmin, m_parameters.xmax);
    m_parameters.idf = getIDFfromWorkspace(ws);

    save_no_assert(ws, m_parameters);
  }
};

class LoadNXcanSASPerformanceTest2D : public ILoadNXcanSASPerformanceTest, public CxxTest::TestSuite {
public:
  void setUp() override { ILoadNXcanSASPerformanceTest::setUp(); }
  void tearDown() override { ILoadNXcanSASPerformanceTest::tearDown(); }
  void test_execute() override { alg.execute(); }

  static LoadNXcanSASPerformanceTest2D *createSuite() { return new LoadNXcanSASPerformanceTest2D(); }
  static void destroySuite(LoadNXcanSASPerformanceTest2D *suite) { delete suite; }
  void setupUniqueParams() override {
    m_parameters.is2dData = true;

    const auto ws = provide2DWorkspace(m_parameters);
    set2DValues(ws);
    m_parameters.idf = getIDFfromWorkspace(ws);

    save_no_assert(ws, m_parameters);
  }
};
