#ifndef MANTID_ALGORITHMS_ESTIMATEPDDETECTORRESOLUTIONTEST_H_
#define MANTID_ALGORITHMS_ESTIMATEPDDETECTORRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/EstimatePDDetectorResolution.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

using Mantid::Algorithms::EstimatePDDetectorResolution;

using namespace Mantid::API;
using namespace Mantid::Kernel;

class EstimatePDDetectorResolutionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimatePDDetectorResolutionTest *createSuite() { return new EstimatePDDetectorResolutionTest(); }
  static void destroySuite( EstimatePDDetectorResolutionTest *suite ) { delete suite; }


  /** Test init
    */
  void test_Init()
  {
    EstimatePDDetectorResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /** Test POWGEN
    */
  void test_PG3()
  {
    // Load data file
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","PG3_2538_meta.nxs");
    loader.setProperty("OutputWorkspace", "PG3_2538");
    loader.execute();

    // Set up and run
    EstimatePDDetectorResolution alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "PG3_2538"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "PG3_Resolution"));

    alg.execute();
    TS_ASSERT(alg.isExecuted());



    AnalysisDataService::Instance().remove("PG3_2538");

    TS_ASSERT_EQUALS(1, 3);
  }


};


#endif /* MANTID_ALGORITHMS_ESTIMATEPDDETECTORRESOLUTIONTEST_H_ */
