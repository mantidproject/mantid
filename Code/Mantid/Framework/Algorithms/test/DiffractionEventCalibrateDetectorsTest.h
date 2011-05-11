#ifndef MANTID_ALGORITHMS_DIFFRACTIONEVENTCALIBRATEDETECTORSTEST_H_
#define MANTID_ALGORITHMS_DIFFRACTIONEVENTCALIBRATEDETECTORSTEST_H_

#include "MantidAlgorithms/DiffractionEventCalibrateDetectors.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class DiffractionEventCalibrateDetectorsTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    DiffractionEventCalibrateDetectors alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    EventWorkspace_sptr eventWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 50);
    AnalysisDataService::Instance().addOrReplace("temp_event_ws", eventWS);

    // Name of the output workspace.
    std::string filename = "./DiffractionEventCalibrateDetectorsTest.cal";

    DiffractionEventCalibrateDetectors alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "temp_event_ws" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Params", "1.9, 0.001, 2.2") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MaxIterations", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("LocationOfPeakToOptimize", "2.038") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("BankName", "bank1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DetCalFilename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    filename = alg.getPropertyValue("DetCalFilename");

    // Simple test that it at least runs.
    // The algorithm is far too slow for real testing.
    TS_ASSERT(Poco::File(filename).exists());

//    if (Poco::File(filename).exists())
//      Poco::File(filename).remove();

    AnalysisDataService::Instance().remove("temp_event_ws");
  }


};


#endif /* MANTID_ALGORITHMS_DIFFRACTIONEVENTCALIBRATEDETECTORSTEST_H_ */

