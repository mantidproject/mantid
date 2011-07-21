#ifndef MANTID_ALGORITHMS_REMOVEPROMPTPULSETEST_H_
#define MANTID_ALGORITHMS_REMOVEPROMPTPULSETEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <iomanip>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include "MantidAlgorithms/RemovePromptPulse.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class RemovePromptPulseTest : public CxxTest::TestSuite
{
private:
  double BIN_DELTA;
  int NUMPIXELS;
  int NUMBINS;

  void makeFakeEventWorkspace(std::string wsName)
  {
    //Make an event workspace with 2 events in each bin.
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateEventWorkspace(NUMPIXELS, NUMBINS, NUMBINS, 0.0, BIN_DELTA, 2);
    //Fake a TOF unit in the data.
    test_in->getAxis(0)->unit() =UnitFactory::Instance().create("TOF");
    test_in->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(NUMPIXELS/9, false) );
    // Make sure the detector IDs are ok
    for (int i = 0; i < NUMPIXELS; i++)
      test_in->getSpectrum(i)->setDetectorID(i+1);

    //Add it to the workspace
    AnalysisDataService::Instance().add(wsName, test_in);
  }

public:
  RemovePromptPulseTest()
  {
    BIN_DELTA = 2.;
    NUMPIXELS = 36;
    NUMBINS = 50;
  }

  void test_Init()
  {
    RemovePromptPulse alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    std::string inWSName("RemovePromptPulseTest_InputWS");
    std::string outWSName("RemovePromptPulseTest_OutputWS");
  
    RemovePromptPulse alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
//    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Width", "30.") );
//    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
//    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
//    Workspace_sptr ws;
//    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
//    TS_ASSERT(ws);
//    if (!ws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
//    AnalysisDataService::Instance().remove(outWSName);
  }

};

#endif /* MANTID_ALGORITHMS_REMOVEPROMPTPULSETEST_H_ */
