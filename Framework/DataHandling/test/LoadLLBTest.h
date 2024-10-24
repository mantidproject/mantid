// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadLLB.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadLLB;

class LoadLLBTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadLLBTest *createSuite() { return new LoadLLBTest(); }
  static void destroySuite(LoadLLBTest *suite) { delete suite; }

  LoadLLBTest() : m_testFile("LLB_d22418.nxs") {}
  void testName() {
    LoadLLB alg;
    TS_ASSERT_EQUALS(alg.name(), "LoadLLB");
  }

  void testVersion() {
    LoadLLB alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_Init() {
    LoadLLB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    LoadLLB loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_testFile);

    std::string outputSpace = "LoadLLBTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    //  test workspace, copied from LoadMuonNexusTest.h
    MatrixWorkspace_sptr output;

    (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));
    MatrixWorkspace_sptr output2D = std::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 84);

    AnalysisDataService::Instance().clear();
  }

private:
  std::string m_testFile;
};

class LoadLLBTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    for (int i = 0; i < numberOfIterations; ++i) {
      loadAlgPtrs.emplace_back(setupAlg());
    }
  }

  void testLoadLLBPerformance() {
    for (auto alg : loadAlgPtrs) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void tearDown() override {
    for (int i = 0; i < numberOfIterations; i++) {
      delete loadAlgPtrs[i];
      loadAlgPtrs[i] = nullptr;
    }
    Mantid::API::AnalysisDataService::Instance().remove(outWSName);
  }

private:
  std::vector<LoadLLB *> loadAlgPtrs;

  const int numberOfIterations = 5;

  const std::string inFileName = "LLB_d22418.nxs";
  const std::string outWSName = "LoadLLBWsOut";

  LoadLLB *setupAlg() {
    LoadLLB *loader = new LoadLLB;
    loader->initialize();
    loader->isInitialized();
    loader->setPropertyValue("Filename", inFileName);
    loader->setPropertyValue("OutputWorkspace", outWSName);

    loader->setRethrows(true);
    return loader;
  }
};
