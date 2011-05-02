#ifndef MANTID_CRYSTAL_PREDICTPEAKSTEST_H_
#define MANTID_CRYSTAL_PREDICTPEAKSTEST_H_

#include "MantidCrystal/PredictPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class PredictPeaksTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("PredictPeaksTest_OutputWS");

    // Make the fake input workspace
    Workspace2D_sptr inWS = WorkspaceCreationHelper::Create2DWorkspace(10000, 1);
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);
    inWS->setInstrument(inst);
    //TODO: set ub and stuff
  
    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(inWS) ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("WavelengthMin", "0.1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("WavelengthMax", "10.0") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // TODO: Check the results
//    TS_ASSERT_EQUALS( ws->getNumberPeaks(), 9);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_CRYSTAL_PREDICTPEAKSTEST_H_ */

