#ifndef MANTID_ALGORITHMS_ESTIMATEDIVERGENCETEST_H_
#define MANTID_ALGORITHMS_ESTIMATEDIVERGENCETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/EstimateDivergence.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"

using Mantid::Algorithms::EstimateDivergence;
using Mantid::DataHandling::LoadEmptyInstrument;
using namespace Mantid::API;

class EstimateDivergenceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimateDivergenceTest *createSuite() {
    return new EstimateDivergenceTest();
  }
  static void destroySuite(EstimateDivergenceTest *suite) { delete suite; }

  void test_Init() {
    EstimateDivergence alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Create test input if necessary
    auto inputWS = createInstrument();

    EstimateDivergence alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm. The type here will probably
    // need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    AnalysisDataService::Instance().remove("PG3_EstimateDivergence");

    const size_t numspec = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(numspec, 25873);

    for (size_t i = 0; i < numspec; ++i) {
      const double y = outputWS->y(i)[0];
      TS_ASSERT(y >= 0. && y < 0.004);
    }
  }

  MatrixWorkspace_sptr createInstrument() {
    // Create empty workspace
    LoadEmptyInstrument loader;
    loader.initialize();

    loader.setProperty("Filename", "POWGEN_Definition_2013-06-01.xml");
    loader.setProperty("OutputWorkspace", "PG3_EstimateDivergence");

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PG3_EstimateDivergence"));

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_ESTIMATEDIVERGENCETEST_H_ */
