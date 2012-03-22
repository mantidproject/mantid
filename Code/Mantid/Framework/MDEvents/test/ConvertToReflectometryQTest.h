#ifndef MANTID_MDEVENTS_CONVERTTOREFLECTOMETRYQTEST_H_
#define MANTID_MDEVENTS_CONVERTTOREFLECTOMETRYQTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDEvents/ConvertToReflectometryQ.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class ConvertToReflectometryQTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToReflectometryQTest *createSuite() { return new ConvertToReflectometryQTest(); }
  static void destroySuite( ConvertToReflectometryQTest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertToReflectometryQ alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    using Geometry::ReferenceFrame;

    MatrixWorkspace_sptr in_ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    in_ws->getAxis(0)->setUnit("TOF");
    
    //ConvertUnits convertUnits;
    //convertUnits.initialize();
    //convertUnits.setPropertyValue("Target", "TOF");
    //convertUnits.setPropertyValue("EMode", "Indirect");
    //convertUnits.setProperty("InputWorkspace", in_ws);
    //convertUnits.setPropertyValue("OutputWorkspace", convertUnits.getPropertyValue("InputWorkspace"));
    //convertUnits.execute();
    //in_ws = convertUnits.getProperty("OutputWorkspace");

    // Name of the output workspace.
    std::string outWSName("ConvertToReflectometryQTest_OutputWS");
  
    ConvertToReflectometryQ alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", in_ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_MDALGORITHMS_CONVERTTOREFLECTOMETRYQTEST_H_ */