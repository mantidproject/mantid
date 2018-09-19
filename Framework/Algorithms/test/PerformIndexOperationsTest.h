#ifndef MANTID_ALGORITHMS_PERFORMINDEXOPERATIONSTEST_H_
#define MANTID_ALGORITHMS_PERFORMINDEXOPERATIONSTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/PerformIndexOperations.h"
#include <cxxtest/TestSuite.h>

using Mantid::Algorithms::PerformIndexOperations;
using namespace Mantid::API;

MatrixWorkspace_const_sptr
doExecute(MatrixWorkspace_sptr inWS,
          const std::string &processingInstructions) {
  // Name of the output workspace.
  std::string outWSName("PerformIndexOperationsTest_OutputWS");

  PerformIndexOperations alg;
  TS_ASSERT_THROWS_NOTHING(alg.initialize())
  alg.setRethrows(true);
  TS_ASSERT(alg.isInitialized())
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS));
  TS_ASSERT_THROWS_NOTHING(
      alg.setPropertyValue("ProcessingInstructions", processingInstructions));
  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
  alg.execute();

  MatrixWorkspace_sptr ws;
  TS_ASSERT_THROWS_NOTHING(
      ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          outWSName));
  TS_ASSERT(ws);
  return ws;
}

class PerformIndexOperationsTest : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_testWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PerformIndexOperationsTest *createSuite() {
    return new PerformIndexOperationsTest();
  }
  static void destroySuite(PerformIndexOperationsTest *suite) { delete suite; }

  PerformIndexOperationsTest() {
    auto createAlg = AlgorithmManager::Instance().create("CreateWorkspace");
    createAlg->setChild(true);
    createAlg->initialize();
    createAlg->setPropertyValue("DataY", "1.0, 1.1, 1.2, 1.3, 1.4");
    createAlg->setPropertyValue("DataX", "0, 1");
    createAlg->setProperty("NSpec", 5);
    createAlg->setPropertyValue("OutputWorkspace", "out_ws");
    createAlg->execute();
    m_testWS = createAlg->getProperty("OutputWorkspace");
  }

  void test_Init() {
    PerformIndexOperations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_do_nothing() {
    auto outWS = doExecute(m_testWS, "");
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(),
                     m_testWS->getNumberHistograms());
  }

  void test_throw_if_bad_regex() {
    TSM_ASSERT_THROWS("Not a workspace index", doExecute(m_testWS, "x"),
                      std::invalid_argument &);
    TSM_ASSERT_THROWS("Not a positive index", doExecute(m_testWS, "-1"),
                      std::invalid_argument &);
    TSM_ASSERT_THROWS("One negative, one positive index",
                      doExecute(m_testWS, "-1,1"), std::invalid_argument &);
    TSM_ASSERT_THROWS("Invalid separator", doExecute(m_testWS, "1@2"),
                      std::invalid_argument &);
    TSM_ASSERT_THROWS("Dangling end separator", doExecute(m_testWS, "1,2,"),
                      std::invalid_argument &);
    TSM_ASSERT_THROWS("Test non-integer index", doExecute(m_testWS, "1.0"),
                      std::invalid_argument &);
  }

  void test_simple_crop() {
    auto outWS = doExecute(m_testWS, "0:2"); // Crop off the last spectra
    TS_ASSERT_EQUALS(3, outWS->getNumberHistograms());

    TS_ASSERT_EQUALS(1.0, outWS->readY(0)[0])
    TS_ASSERT_EQUALS(1.1, outWS->readY(1)[0])
    TS_ASSERT_EQUALS(1.2, outWS->readY(2)[0])
  }

  void test_split_crop() // Crop out workspace index 2
  {
    auto outWS =
        doExecute(m_testWS, "0:1,3:4"); // Crop off the middle spectra only
    TS_ASSERT_EQUALS(4, outWS->getNumberHistograms());

    TS_ASSERT_EQUALS(1.0, outWS->readY(0)[0])
    TS_ASSERT_EQUALS(1.1, outWS->readY(1)[0])
    TS_ASSERT_EQUALS(1.3, outWS->readY(2)[0])
    TS_ASSERT_EQUALS(1.4, outWS->readY(3)[0])
  }

  void test_add_spectra() {
    auto outWS = doExecute(m_testWS, "0+1");
    TS_ASSERT_EQUALS(1, outWS->getNumberHistograms());
    TS_ASSERT_EQUALS(1.0 + 1.1, outWS->readY(0)[0])
  }

  void test_add_spectra_that_are_not_neighbours() {
    auto outWS = doExecute(m_testWS, "0+4");
    TS_ASSERT_EQUALS(1, outWS->getNumberHistograms());
    TS_ASSERT_EQUALS(1.0 + 1.4, outWS->readY(0)[0])
  }

  void test_add_spectra_range() {
    auto outWS = doExecute(
        m_testWS, "0-2"); // Sum first and second spectra. Remove the rest.
    TS_ASSERT_EQUALS(1, outWS->getNumberHistograms());

    TS_ASSERT_EQUALS(1.0 + 1.1 + 1.2, outWS->readY(0)[0])
  }

  void test_combine_and_crop_ranges() {
    auto outWS = doExecute(m_testWS, "0-1,2,3,4"); //
    TS_ASSERT_EQUALS(4, outWS->getNumberHistograms());

    TS_ASSERT_EQUALS(1.0 + 1.1, outWS->readY(0)[0])
    TS_ASSERT_EQUALS(1.2, outWS->readY(1)[0])
    TS_ASSERT_EQUALS(1.3, outWS->readY(2)[0])
    TS_ASSERT_EQUALS(1.4, outWS->readY(3)[0])
  }

  void test_complex_schenario() {
    auto outWS = doExecute(m_testWS, "0:1,2-3"); //
    TS_ASSERT_EQUALS(3, outWS->getNumberHistograms());

    TS_ASSERT_EQUALS(1.0, outWS->readY(0)[0])
    TS_ASSERT_EQUALS(1.1, outWS->readY(1)[0])
    TS_ASSERT_EQUALS(1.2 + 1.3, outWS->readY(2)[0])
  }
};

#endif /* MANTID_ALGORITHMS_PERFORMINDEXOPERATIONSTEST_H_ */
