#ifndef IDENTIFYNOISYDETECTORSTEST_H_
#define IDENTIFYNOISYDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/IdentifyNoisyDetectors.h"
#include "MantidDataHandling/LoadRaw3.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

class IdentifyNoisyDetectorsTest : public CxxTest::TestSuite
{

public:
  IdentifyNoisyDetectorsTest() {}
  ~IdentifyNoisyDetectorsTest() {}

  void testMetaInfo()
  {
    alg = new IdentifyNoisyDetectors();
    TS_ASSERT_EQUALS(alg->name(), "IdentifyNoisyDetectors");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "General");
    delete alg;
  }

  void testInit()
  {
    alg = new IdentifyNoisyDetectors();
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    delete alg;
  }

  void testExec()
  {
    // Load Data for Test
    IAlgorithm* loader;
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
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("InputWorkspace", "identifynoisydetectors_input"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", "identifynoisydetectors_output"));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // identifynoisydetectors_output
    MatrixWorkspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("identifynoisydetectors_output")));
    
    // Check that it's got all the bad ones
    TS_ASSERT_EQUALS(workspace->readY(0)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(1)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(13)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(27)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(28)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(41)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(55)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(69)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(70)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(83)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(97)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(111)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(125)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(127)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->readY(139)[0], 0.0);

    // And a quick check of some of the good ones
    TS_ASSERT_EQUALS(workspace->readY(4)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->readY(17)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->readY(21)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->readY(75)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->readY(112)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->readY(134)[0], 1.0);

    delete alg;

    AnalysisDataService::Instance().remove("identifynoisydetectors_input");
    AnalysisDataService::Instance().remove("identifynoisydetectors_output");
  }

private:
  IdentifyNoisyDetectors* alg;

};

#endif /* IDENTIFYNOISYDETECTORSTEST_H_ */