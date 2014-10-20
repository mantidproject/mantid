#ifndef LOADNEXUSMONITORSTEST_H_
#define LOADNEXUSMONITORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadNexusMonitors.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include <nexus/NeXusFile.hpp>
#include <boost/shared_ptr.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>

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

  void testBrokenISISFile()
  {
    // Just need to make sure it runs.
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors ld;
    std::string outws_name = "LOQ_49886_monitors";
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", outws_name);
    TS_ASSERT_THROWS_NOTHING( ld.execute() );
    TS_ASSERT( ld.isExecuted() );

    MatrixWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outws_name);
    //Valid WS and it is an MatrixWorkspace
    TS_ASSERT( WS );
    //Correct number of monitors found
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 2 );
    //Monitors data is correct
    TS_ASSERT_EQUALS( WS->readY(0)[0], 0 );
    TS_ASSERT_EQUALS( WS->readY(1)[0], 0 );

    TS_ASSERT_EQUALS( WS->readX(0)[0], 5.0 );
    TS_ASSERT_EQUALS( WS->readX(1)[5],19995.0 );

  }

  void test_10_monitors()
  {
    Poco::Path path(ConfigService::Instance().getTempDir().c_str());
    path.append("LoadNexusMonitorsTestFile.nxs");
    std::string filename = path.toString();

    createFakeFile( filename );

    LoadNexusMonitors ld;
    std::string outws_name = "10monitors";
    ld.initialize();
    ld.setPropertyValue("Filename", filename);
    ld.setPropertyValue("OutputWorkspace", outws_name);
    ld.execute();
    TS_ASSERT( ld.isExecuted() );

    MatrixWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outws_name);
    //Valid WS and it is an MatrixWorkspace
    TS_ASSERT( WS );
    //Correct number of monitors found
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 3 );
    //Monitors are in the right order
    TS_ASSERT_EQUALS( WS->readY(0)[0], 1 );
    TS_ASSERT_EQUALS( WS->readY(1)[0], 2 );
    TS_ASSERT_EQUALS( WS->readY(2)[0], 10 );

    AnalysisDataService::Instance().clear();
    Poco::File(filename).remove();
  }

  void createFakeFile( const std::string& filename )
  {
    NeXus::File file(filename, NXACC_CREATE5);

    const bool openGroup = true;
    file.makeGroup("raw_data_1","NXentry",openGroup);
    {
      addMonitor( file, 1 );
      addMonitor( file, 10 );
      addMonitor( file, 2 );

      file.makeGroup("instrument","NXinstrument", openGroup);
      file.writeData("name","FakeInstrument");
      file.closeGroup();

    }
    file.closeGroup(); // raw_data_1
    file.close();
  }

  void addMonitor(NeXus::File &file, int i)
  {
    const size_t nbins = 3;
    std::string monitorName = "monitor_" + boost::lexical_cast<std::string>(i);
    const bool openGroup = true;
    file.makeGroup( monitorName, "NXmonitor", openGroup );
    file.writeData("monitor_number", i);
    file.writeData("spectrum_index", i);
    std::vector<int> dims(3,1);
    dims[2] = nbins;
    std::vector<int> data(nbins,i);
    file.writeData("data",data,dims);
    std::vector<float> timeOfFlight(nbins+1);
    file.writeData("time_of_flight", timeOfFlight);
    file.closeGroup();
  }
};

#endif /*LOADNEXUSMONITORSTEST_H_*/
