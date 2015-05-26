#ifndef MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTIONTEST_H_
#define MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/VesuvioL1ThetaResolution.h"

using Mantid::Algorithms::VesuvioL1ThetaResolution;
using namespace Mantid::API;

class VesuvioL1ThetaResolutionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VesuvioL1ThetaResolutionTest *createSuite() { return new VesuvioL1ThetaResolutionTest(); }
  static void destroySuite( VesuvioL1ThetaResolutionTest *suite ) { delete suite; }


  void test_Init()
  {
    VesuvioL1ThetaResolution alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  /**
   * Tests execution with default options and no PAR file.
   */
  void test_runDefaultOptions()
  {
    // Name of the output workspace.
    std::string outWSName("VesuvioL1ThetaResolutionTest_OutputWS");

    VesuvioL1ThetaResolution alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // TODO: Check the results

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

};


#endif /* MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTIONTEST_H_ */
