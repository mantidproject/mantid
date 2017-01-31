#ifndef MANTID_DATAHANDLING_LOADILLINDIRECTTEST_H_
#define MANTID_DATAHANDLING_LOADILLINDIRECTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadILLIndirect.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLIndirect;

class LoadILLIndirectTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLIndirectTest *createSuite() {
    return new LoadILLIndirectTest();
  }
  static void destroySuite(LoadILLIndirectTest *suite) { delete suite; }

  LoadILLIndirectTest()
      : m_dataFile2013("ILLIN16B_034745.nxs"),
        m_dataFile2015("ILLIN16B_127500.nxs") {}

  void test_Init() {
    LoadILLIndirect loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
  }

  void test_Name() {
    LoadILLIndirect loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadILLIndirect");
  }

  void test_Version() {
    LoadILLIndirect loader;
    TS_ASSERT_EQUALS(loader.version(), 1);
  }

  void test_Load_2013_Format() { doExecTest(m_dataFile2013); }

  void test_Load_2015_Format() { doExecTest(m_dataFile2015); }

  void test_Confidence_2013_Format() { doConfidenceTest(m_dataFile2013); }

  void test_Confidence_2015_Format() { doConfidenceTest(m_dataFile2015); }

  void doConfidenceTest(const std::string &file) {
    LoadILLIndirect alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("Filename", file);
    Mantid::Kernel::NexusDescriptor descr(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(alg.confidence(descr), 70);
  }

  void doExecTest(const std::string &file) {
    // Name of the output workspace.
    std::string outWSName("LoadILLIndirectTest_OutputWS");

    LoadILLIndirect loader;
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

    if (!output)
      return;

    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 2057);

    const Mantid::API::Run &runlogs = output->run();
    TS_ASSERT(runlogs.hasProperty("Facility"));
    TS_ASSERT_EQUALS(runlogs.getProperty("Facility")->value(), "ILL");

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

private:
  const std::string m_dataFile2013;
  const std::string m_dataFile2015;
};

class LoadILLIndirectTestPerformance : public CxxTest::TestSuite {
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
  std::vector<LoadILLIndirect *> loadAlgPtrs;

  const int numberOfIterations = 5;

  const std::string inFileName = "ILLIN16B_127500.nxs";
  const std::string outWSName = "LoadILLWsOut";

  LoadILLIndirect *setupAlg() {
    LoadILLIndirect *loader = new LoadILLIndirect;
    loader->initialize();
    loader->isInitialized();
    loader->setPropertyValue("Filename", inFileName);
    loader->setPropertyValue("OutputWorkspace", outWSName);

    loader->setRethrows(true);
    return loader;
  }
};
#endif /* MANTID_DATAHANDLING_LOADILLINDIRECTTEST_H_ */
