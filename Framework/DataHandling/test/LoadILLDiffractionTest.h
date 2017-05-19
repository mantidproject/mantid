#ifndef MANTID_DATAHANDLING_LOADILLDIFFRACTIONTEST_H_
#define MANTID_DATAHANDLING_LOADILLDIFFRACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLDiffraction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::DataHandling::LoadILLDiffraction;

class LoadILLDiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLDiffractionTest *createSuite() {
    return new LoadILLDiffractionTest();
  }
  static void destroySuite(LoadILLDiffractionTest *suite) { delete suite; }

  void setUp() override {
    ConfigService::Instance().appendDataSearchSubDir("ILL/D20/");
  }

  void test_Init() {
    LoadILLDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_no_scan() {
    // Tests the no-scan case for D20

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "967100.nxs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3073);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1);
  }

  void test_scan() {
    // Tests the scanned case for D20

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    // Note that, this is the older type of file, and is modified manually
    // to match the final configuration having the custom NX_class attribute
    // So this will not run with generic Load
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "000017.nxs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3073);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 21);
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLDIFFRACTIONTEST_H_ */
