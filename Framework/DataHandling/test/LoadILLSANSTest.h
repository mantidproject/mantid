#ifndef MANTID_DATAHANDLING_LOADILLSANSTEST_H_
#define MANTID_DATAHANDLING_LOADILLSANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLSANS.h"

using Mantid::DataHandling::LoadILLSANS;
using namespace Mantid::API;

class LoadILLSANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLSANSTest *createSuite() { return new LoadILLSANSTest(); }
  static void destroySuite(LoadILLSANSTest *suite) { delete suite; }

  LoadILLSANSTest()
      : m_testFileTof("ILLD33_001030.nxs"),
        m_testFileNonTof("ILLD33_041714_NonTof.nxs") {}
  void testName() {
    LoadILLSANS alg;
    TS_ASSERT_EQUALS(alg.name(), "LoadILLSANS");
  }

  void testVersion() {
    LoadILLSANS alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_Init() {
    LoadILLSANS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_TOF() {
    LoadILLSANS loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_testFileTof);

    std::string outputSpace = "LoadILLSANSTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    //  test workspace, copied from LoadMuonNexusTest.h
    MatrixWorkspace_sptr output;

    (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
         outputSpace));
    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 65536 + 2);
    TS_ASSERT_EQUALS(output2D->blocksize(), 100);
    TS_ASSERT_DIFFERS(output2D->run().getPropertyValueAsType<double>("monitor"),
                      0.0);
    AnalysisDataService::Instance().clear();
  }

  void test_exec_nonTOF() {
    LoadILLSANS loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_testFileNonTof);

    std::string outputSpace = "LoadILLSANSTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    //  test workspace, copied from LoadMuonNexusTest.h
    MatrixWorkspace_sptr output;

    (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
         outputSpace));
    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 65536 + 2);
    TS_ASSERT_EQUALS(output2D->blocksize(), 1);
    TS_ASSERT_DIFFERS(output2D->run().getPropertyValueAsType<double>("monitor"),
                      0.0);
    AnalysisDataService::Instance().clear();
  }

private:
  std::string m_testFileTof;
  std::string m_testFileNonTof;
};

#endif /* MANTID_DATAHANDLING_LOADILLSANSTEST_H_ */
