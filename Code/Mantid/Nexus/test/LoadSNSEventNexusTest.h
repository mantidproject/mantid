#ifndef LOADLOGSFROMSNSNEXUSTEST_H_
#define LOADLOGSFROMSNSNEXUSTEST_H_

#include "MantidNexus/LoadSNSEventNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::DataObjects;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>

class LoadSNSEventNexusTest : public CxxTest::TestSuite
{
public:


    void testExec()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadSNSEventNexus ld;
        std::string outws_name = "cncs";
        ld.initialize();
        ld.setPropertyValue("OutputWorkspace",outws_name);
        //ld.setPropertyValue("Filename","/home/janik/data/PG3_732_event.nxs");
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7850_event.nxs");

        ld.execute();
        TS_ASSERT( ld.isExecuted() );

        EventWorkspace_sptr WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
        //Valid WS and it is an EventWorkspace
        TS_ASSERT( WS );
        //Pixels have to be padded
        TS_ASSERT_EQUALS( WS->getNumberHistograms(), 51200);
        //Events
        TS_ASSERT_EQUALS( WS->getNumberEvents(), 1208875);
        //TOF limits found. There is a pad of +-1 given around the actual TOF founds.
        TS_ASSERT_DELTA( (*WS->refX(0))[0],  44138.7, 0.05);
        TS_ASSERT_DELTA( (*WS->refX(0))[1],  60830.4, 0.05);

        //TODO: Check that pixel positions are good.

    }
};

#endif /*LOADLOGSFROMSNSNEXUSTEST_H_*/
