#ifndef LOADSNSEVENTNEXUSMONITORSTEST_H_
#define LOADSNSEVENTNEXUSMONITORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidNexus/LoadSNSEventNexusMonitors.h"

using namespace Mantid::API;
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
  }
};

#endif /*LOADSNSEVENTNEXUSMONITORSTEST_H_*/
