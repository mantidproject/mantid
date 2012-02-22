#ifndef LOADLOGSFROMSNSNEXUSTEST_H_
#define LOADLOGSFROMSNSNEXUSTEST_H_

#include "MantidDataHandling/LoadLogsFromSNSNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"

using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using Mantid::DataObjects::Workspace2D;
using Mantid::DataObjects::Workspace2D_sptr;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>

class LoadLogsFromSNSNexusTest : public CxxTest::TestSuite
{
public:

  void testExec()
  {
      Mantid::API::FrameworkManager::Instance();
      LoadLogsFromSNSNexus ld;
      std::string outws_name = "REF_L_instrument";
      ld.initialize();
      ld.setPropertyValue("Filename","REF_L_32035.nxs");

      //Create an empty workspace with some fake size, to start from.
      Workspace2D_sptr ws = boost::dynamic_pointer_cast<Workspace2D>
          (WorkspaceFactory::Instance().create("Workspace2D",1000,18+1,18));
      //Put it in the object.
      ld.setProperty("Workspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws));

      ld.execute();
      TS_ASSERT( ld.isExecuted() );

        double val;
        Run& run = ws->mutableRun();
        // The expected number
        const std::vector< Property* >& logs = ws->run().getLogData();
        TS_ASSERT_EQUALS(logs.size(), 72);
        
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
        TS_ASSERT_DELTA( DateAndTime::secondsFromDuration(tsp->nthInterval(0).length()), 0.328, 0.01);

        //Now the stats

    }
};

#endif /*LOADLOGSFROMSNSNEXUSTEST_H_*/
