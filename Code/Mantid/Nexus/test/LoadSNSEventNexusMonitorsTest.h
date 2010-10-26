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
  }
};

#endif /*LOADSNSEVENTNEXUSMONITORSTEST_H_*/
