#ifndef MANTID_DATAHANDLING_LOADSINQTEST_H_
#define MANTID_DATAHANDLING_LOADSINQTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSINQFocus.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadSINQFocus;

class LoadSINQFocusTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSINQFocusTest *createSuite() { return new LoadSINQFocusTest(); }
  static void destroySuite(LoadSINQFocusTest *suite) { delete suite; }

  LoadSINQFocusTest() : m_testFile("focus2010n000468.hdf") {}
  void testName() {
    LoadSINQFocus alg;
    TS_ASSERT_EQUALS(alg.name(), "LoadSINQFocus");
  }

  void testVersion() {
    LoadSINQFocus alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_Init() {
    LoadSINQFocus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    LoadSINQFocus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", m_testFile);

    std::string outputSpace = "LoadSINQFocusTest_out";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    //  test workspace, copied from LoadMuonNexusTest.h
    MatrixWorkspace_sptr output;

    (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
         outputSpace));
    MatrixWorkspace_sptr output2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 375);

    AnalysisDataService::Instance().clear();
  }

private:
  std::string m_testFile;
};

#endif /* MANTID_DATAHANDLING_LOADSINQTEST_H_ */
