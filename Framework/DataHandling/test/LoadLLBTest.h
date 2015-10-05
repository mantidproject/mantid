#ifndef MANTID_DATAHANDLING_LOADLLBTEST_H_
#define MANTID_DATAHANDLING_LOADLLBTEST_H_

#include <cxxtest/TestSuite.h>

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

    (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
         outputSpace));
    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 84);

    AnalysisDataService::Instance().clear();
  }

private:
  std::string m_testFile;
};

#endif /* MANTID_DATAHANDLING_LOADLLBTEST_H_ */
