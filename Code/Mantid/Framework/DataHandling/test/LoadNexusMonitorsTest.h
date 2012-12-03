#ifndef LOADNEXUSMONITORSTEST_H_
#define LOADNEXUSMONITORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadNexusMonitors.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadNexusMonitorsTest : public CxxTest::TestSuite
{
public:
  void testExec()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors ld;
    std::string outws_name = "cncs";
    ld.initialize();
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace", outws_name);

    ld.execute();
    TS_ASSERT( ld.isExecuted() );


    MatrixWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outws_name);
    //Valid WS and it is an MatrixWorkspace
    TS_ASSERT( WS );
    //Correct number of monitors found
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 3 );
    // Check some histogram data
    // TOF
    TS_ASSERT_EQUALS( (*WS->refX(1)).size(), 200002 );
    TS_ASSERT_DELTA( (*WS->refX(1))[3412], 3412.0, 1e-6 );
    // Data
    TS_ASSERT_EQUALS( WS->dataY(1).size(), 200001 );
    TS_ASSERT_DELTA( WS->dataY(1)[3412], 197., 1e-6 );
    // Error
    TS_ASSERT_EQUALS( WS->dataE(1).size(), 200001 );
    TS_ASSERT_DELTA( WS->dataE(1)[3412], 14.03567, 1e-4 );
    // Check geometry for a monitor
    IDetector_const_sptr mon = WS->getDetector(2);
    TS_ASSERT( mon->isMonitor() );
    TS_ASSERT_EQUALS( mon->getID(), -3 );
    boost::shared_ptr<const IComponent> sample = WS->getInstrument()->getSample();
    TS_ASSERT_DELTA( mon->getDistance(*sample), 1.426, 1e-6 );
    //Check if filename is saved
    TS_ASSERT_EQUALS(ld.getPropertyValue("Filename"),WS->run().getProperty("Filename")->value());
  }

  void testExecEvent()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors ld;
    std::string outws_name = "hyspec";
    ld.initialize();
    ld.setPropertyValue("Filename", "HYSA_2411_monitors.nxs.h5");
    ld.setPropertyValue("OutputWorkspace", outws_name);

    ld.execute();
    TS_ASSERT( ld.isExecuted() );
    EventWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name);
    //Valid WS and it is an MatrixWorkspace
    TS_ASSERT( WS );
    //Correct number of monitors found
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 2 );
    // Verify number of events loaded
    TS_ASSERT_EQUALS( WS->getEventList(0).getNumberEvents(), 15000);
    TS_ASSERT_EQUALS( WS->getEventList(1).getNumberEvents(), 15000);
  }

  void testOldFile()
  {
    // Just need to make sure it runs.
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors ld;
    std::string outws_name = "ARCS_2963_monitors";
    ld.initialize();
    ld.setPropertyValue("Filename", "ARCS_2963.nxs");
    ld.setPropertyValue("OutputWorkspace", outws_name);
    TS_ASSERT_THROWS_NOTHING( ld.execute() );
    TS_ASSERT( ld.isExecuted() );
  }
};

#endif /*LOADNEXUSMONITORSTEST_H_*/
