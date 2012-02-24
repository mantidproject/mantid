#ifndef LOADNEXUSLOGSTEST_H_
#define LOADNEXUSLOGSTEST_H_

#include "MantidDataHandling/LoadNexusLogs.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"
using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>

class LoadNexusLogsTest : public CxxTest::TestSuite
{
public:

  void test_File_With_DASLogs()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadNexusLogs ld;
    std::string outws_name = "REF_L_instrument";
    ld.initialize();
    ld.setPropertyValue("Filename","REF_L_32035.nxs");
    MatrixWorkspace_sptr ws = createTestWorkspace();
    //Put it in the object.
    ld.setProperty("Workspace", ws);
    ld.execute();
    TS_ASSERT( ld.isExecuted() );

    double val;
    Run& run = ws->mutableRun();
    // Do we have all we expect
    const std::vector< Property* >& logs = run.getLogData();
    TS_ASSERT_EQUALS(logs.size(), 74);
    Property * prop;
    TimeSeriesProperty<double> * dProp;

    prop = run.getLogData("Speed3");
    TS_ASSERT(prop);
    //TS_ASSERT_EQUALS( prop->value(), "60");
    TS_ASSERT_EQUALS( prop->units(), "Hz");

    prop = run.getLogData("PhaseRequest1");
    dProp = dynamic_cast< TimeSeriesProperty<double> * >(prop);
    TS_ASSERT(dProp);
    val = dProp->nthValue(0);
    TS_ASSERT_DELTA( val, 13712.77, 1e-2);
    TS_ASSERT_EQUALS(prop->units(), "microsecond");

    TimeSeriesProperty<double> * tsp;

    prop = run.getLogData("Phase1");
    tsp = dynamic_cast< TimeSeriesProperty<double> * >(prop);
    TS_ASSERT(tsp);
    TS_ASSERT_EQUALS(tsp->units(), "microsecond");
    TS_ASSERT_DELTA( tsp->nthValue(1), 13715.55, 2);

    //The time diff between the 0th and 1st entry is 0.328 seconds
    TS_ASSERT_DELTA( Kernel::DateAndTime::secondsFromDuration(tsp->nthInterval(0).length()), 0.328, 0.01);

    //Now the stats

  }

  void test_File_With_Runlog_And_Selog()
  {
    LoadNexusLogs loader;
    loader.initialize();
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Workspace",testWS));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename","LOQ49886.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    const API::Run & run = testWS->run();
    const std::vector< Property* >& logs = run.getLogData();
    TS_ASSERT_EQUALS(logs.size(), 34);

    TimeSeriesProperty<std::string>* slog = dynamic_cast<TimeSeriesProperty<std::string>*>(run.getLogData("icp_event"));
    TS_ASSERT(slog);
    std::string str = slog->value();
    TS_ASSERT_EQUALS(str.size(),1023);
    TS_ASSERT_EQUALS(str.substr(0,37),"2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

    slog = dynamic_cast<TimeSeriesProperty<std::string>*>(run.getLogData("icp_debug"));
    TS_ASSERT(slog);
    TS_ASSERT_EQUALS(slog->size(),50);

    TimeSeriesProperty<int>* ilog = dynamic_cast<TimeSeriesProperty<int>*>(run.getLogData("total_counts"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(),172);

    ilog = dynamic_cast<TimeSeriesProperty<int>*>(run.getLogData("period"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(),172);

    TimeSeriesProperty<double> *dlog = dynamic_cast<TimeSeriesProperty<double>*>(run.getLogData("proton_charge"));
    TS_ASSERT(dlog);
    TS_ASSERT_EQUALS(dlog->size(),172);
  }

private:
  
  API::MatrixWorkspace_sptr createTestWorkspace()
  {
    return WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
  }

};

#endif /* LOADNEXUSLOGS_H_*/
