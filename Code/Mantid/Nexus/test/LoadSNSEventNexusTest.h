#ifndef LOADLOGSFROMSNSNEXUSTEST_H_
#define LOADLOGSFROMSNSNEXUSTEST_H_

#include "MantidNexus/LoadSNSEventNexus.h"
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

class LoadSNSEventNexusTest : public CxxTest::TestSuite
{
public:


    void xtestExec()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadSNSEventNexus ld;
        std::string outws_name = "topaz";
        ld.initialize();
        ld.setPropertyValue("OutputWorkspace","cncs");
        ld.setPropertyValue("Filename","/home/janik/data/CNCS_7850_event.nxs");

        ld.execute();
        TS_ASSERT( ld.isExecuted() );

    }
};

#endif /*LOADLOGSFROMSNSNEXUSTEST_H_*/
