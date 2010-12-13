#ifndef LOADLOGSFROMSNSNEXUSTEST_H_
#define LOADLOGSFROMSNSNEXUSTEST_H_

#include "MantidNexus/LoadLogsFromSNSNexus.h"
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
using namespace Mantid::NeXus;

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
      std::string outws_name = "topaz_instrument";
      ld.initialize();
      ld.setPropertyValue("Filename","../../../../Test/AutoTestData/TOPAZ_900.nxs");

      //Create an empty workspace with some fake size, to start from.
      DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
          (WorkspaceFactory::Instance().create("Workspace2D",1000,18+1,18));
      //Put it in the object.
      ld.setProperty("Workspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws));

      ld.execute();
      TS_ASSERT( ld.isExecuted() );

        double val;
        Run& run = ws->mutableRun();
        Property * prop;
        TimeSeriesProperty<double> * dProp;

        prop = run.getLogData("Speed3");
        TS_ASSERT(prop);
        //TS_ASSERT_EQUALS( prop->value(), "60");
        TS_ASSERT_EQUALS( prop->units(), "Phase,H");

        prop = run.getLogData("PhaseRequest1");
        dProp = dynamic_cast< TimeSeriesProperty<double> * >(prop);
        TS_ASSERT(dProp);
        val = dProp->nthValue(0);
        TS_ASSERT_DELTA( val, 10914.857421875, 1e-6);
        TS_ASSERT_EQUALS(prop->units(), "Phase,uS");

        //NXPositioner
        prop = run.getLogData("chi");
        dProp = dynamic_cast< TimeSeriesProperty<double> * >(prop);
        val = dProp->nthValue(0);
        TS_ASSERT_DELTA( val, 45.0, 1e-6);
        TS_ASSERT_EQUALS(prop->units(), "degree");

        TimeSeriesProperty<double> * tsp;

        prop = run.getLogData("Phase1");
        tsp = dynamic_cast< TimeSeriesProperty<double> * >(prop);
        TS_ASSERT(tsp);
        TS_ASSERT_EQUALS(tsp->units(), "Phase,uS");
        TS_ASSERT_EQUALS(tsp->realSize(), 1770);
        TS_ASSERT_DELTA( tsp->nthValue(1), 10915, 20);

        //The time diff between the 0th and 1st entry is 2.172 seconds
        TS_ASSERT_DELTA( Kernel::DateAndTime::seconds_from_duration(tsp->nthInterval(0).length()), 2.17, 0.01);

        //Now the stats

    }
};

#endif /*LOADLOGSFROMSNSNEXUSTEST_H_*/
