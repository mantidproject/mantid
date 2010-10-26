#ifndef LOADSNSEVENTNEXUSMONITORSTEST_H_
#define LOADSNSEVENTNEXUSMONITORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidNexus/LoadSNSEventNexusMonitors.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;

class LoadSNSEventNexusMonitorsTest : public CxxTest::TestSuite
{
public:
  void testExec()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadSNSEventNexusMonitors ld;
    std::string outws_name = "cncs";
    ld.initialize();
    ld.setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7850_event.nxs");
    ld.setPropertyValue("OutputWorkspace", outws_name);

    ld.execute();
    TS_ASSERT( ld.isExecuted() );

    MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
    //Valid WS and it is an MatrixWorkspace
    TS_ASSERT( WS );
    //Correct number of monitors found
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 3);
    // Check some histogram data
    // TOF
    TS_ASSERT_EQUALS( (*WS->refX(0)).size(), 200002);
    TS_ASSERT_DELTA( (*WS->refX(0))[1], 1.0, 1e-6);
    // Data
    TS_ASSERT_EQUALS( WS->dataY(0).size(), 200001);
    TS_ASSERT_DELTA( WS->dataY(0)[12], 0.0, 1e-6);
    // Error
    TS_ASSERT_EQUALS( WS->dataE(0).size(), 200001);
    TS_ASSERT_DELTA( WS->dataE(0)[12], 0.0, 1e-6);
  }
};

#endif /*LOADSNSEVENTNEXUSMONITORSTEST_H_*/
