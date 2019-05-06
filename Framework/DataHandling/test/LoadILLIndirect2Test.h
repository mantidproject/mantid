// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADILLINDIRECT2TEST_H_
#define MANTID_DATAHANDLING_LOADILLINDIRECT2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadILLIndirect2.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLIndirect2;

class LoadILLIndirect2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLIndirect2Test *createSuite() {
    return new LoadILLIndirect2Test();
  }
  static void destroySuite(LoadILLIndirect2Test *suite) { delete suite; }

  void test_Init() {
    LoadILLIndirect2 loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
  }

  void test_Name() {
    LoadILLIndirect2 loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadILLIndirect");
  }

  void test_Version() {
    LoadILLIndirect2 loader;
    TS_ASSERT_EQUALS(loader.version(), 2);
  }

  void test_Load_2013_Format() {
    doExecTest(m_dataFile2013, 2057, 1024); // all single detectors
  }

  void test_Load_2015_Format() {
    doExecTest(m_dataFile2015, 2051); // only 2 out of 8 single detectors
  }

  void test_Confidence_2013_Format() { doConfidenceTest(m_dataFile2013); }

  void test_Confidence_2015_Format() { doConfidenceTest(m_dataFile2015); }

  void doConfidenceTest(const std::string &file) {
    LoadILLIndirect2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("Filename", file);
    Mantid::Kernel::NexusDescriptor descr(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(alg.confidence(descr), 80);
  }

  void test_bats() {
      doExecTest(m_batsFile);
  }

  void doExecTest(const std::string &file, int numHist = 2051,
                  int numChannels = 2048) {
    // Name of the output workspace.
    std::string outWSName("LoadILLIndirectTest_OutputWS");

    LoadILLIndirect2 loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", file));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(output);

    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), numHist);
    TS_ASSERT_EQUALS(output2D->blocksize(), numChannels);

    const Mantid::API::Run &runlogs = output->run();
    TS_ASSERT(runlogs.hasProperty("Facility"));
    TS_ASSERT_EQUALS(runlogs.getProperty("Facility")->value(), "ILL");

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

private:
  std::string m_dataFile2013{"ILLIN16B_034745.nxs"};
  std::string m_dataFile2015{"ILLIN16B_127500.nxs"};
  std::string m_batsFile{"ILL/IN16B/215962.nxs"};
};

class LoadILLIndirect2TestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    for (int i = 0; i < numberOfIterations; ++i) {
      loadAlgPtrs.emplace_back(setupAlg());
    }
  }

  void testLoadILLIndirectPerformance() {
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
  std::vector<LoadILLIndirect2 *> loadAlgPtrs;

  const int numberOfIterations = 5;

  const std::string inFileName = "ILLIN16B_127500.nxs";
  const std::string outWSName = "LoadILLWsOut";

  LoadILLIndirect2 *setupAlg() {
    LoadILLIndirect2 *loader = new LoadILLIndirect2;
    loader->initialize();
    loader->isInitialized();
    loader->setPropertyValue("Filename", inFileName);
    loader->setPropertyValue("OutputWorkspace", outWSName);

    loader->setRethrows(true);
    return loader;
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLINDIRECT2TEST_H_ */
