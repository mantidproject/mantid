#ifndef IDENTIFYNOISYDETECTORSTEST_H_
#define IDENTIFYNOISYDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/IdentifyNoisyDetectors.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

class IdentifyNoisyDetectorsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IdentifyNoisyDetectorsTest *createSuite() {
    return new IdentifyNoisyDetectorsTest();
  }
  static void destroySuite(IdentifyNoisyDetectorsTest *suite) { delete suite; }

  void testMetaInfo() {
    alg = new IdentifyNoisyDetectors();
    TS_ASSERT_EQUALS(alg->name(), "IdentifyNoisyDetectors");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "Diagnostics");
    delete alg;
  }

  void testInit() {
    alg = new IdentifyNoisyDetectors();
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    delete alg;
  }

  void testExec() {
    // Load Data for Test
    IAlgorithm *loader;
    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "TSC10076.raw");
    loader->setPropertyValue("OutputWorkspace", "identifynoisydetectors_input");
    loader->setPropertyValue("SpectrumMin", "1");
    loader->setPropertyValue("SpectrumMax", "140");

    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());

    delete loader;

    alg = new IdentifyNoisyDetectors();
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue(
        "InputWorkspace", "identifynoisydetectors_input"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue(
        "OutputWorkspace", "identifynoisydetectors_output"));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // identifynoisydetectors_output
    MatrixWorkspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(
        workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "identifynoisydetectors_output"));

    // Check that it's got all the bad ones
    TS_ASSERT_EQUALS(workspace->y(0)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(1)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(13)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(27)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(28)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(41)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(55)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(69)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(70)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(83)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(97)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(111)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(125)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(127)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->y(139)[0], 0.0);

    // And a quick check of some of the good ones
    TS_ASSERT_EQUALS(workspace->y(4)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(17)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(21)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(75)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(112)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(134)[0], 1.0);

    delete alg;

    AnalysisDataService::Instance().remove("identifynoisydetectors_input");
    AnalysisDataService::Instance().remove("identifynoisydetectors_output");
  }

private:
  IdentifyNoisyDetectors *alg;
};

#endif /* IDENTIFYNOISYDETECTORSTEST_H_ */
