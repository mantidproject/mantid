#ifndef LOADINSTRUMENTFROMSNSNEXUSTEST_H_
#define LOADINSTRUMENTFROMSNSNEXUSTEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidNexus/LoadInstrumentFromSNSNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
using namespace Mantid::NeXus;
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>

class LoadInstrumentFromSNSNexusTest : public CxxTest::TestSuite
{
public:
    void xtestExec()
    {
        std::cout << "Test starting?\n";

        Mantid::API::FrameworkManager::Instance();
        LoadInstrumentFromSNSNexus ld;
        std::string outws_name = "topaz_instrument";
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/Nexus/SNS/TOPAZ_900.nxs");

        //Create an empty workspace with some fake size, to start from.
        DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create("Workspace2D",1000,18+1,18));
        //Put it in the object.
        ld.setProperty("Workspace", boost::dynamic_pointer_cast<Workspace>(ws));

        std::cout << "Getting detectors\n";

        TS_ASSERT_THROWS_NOTHING(ld.execute());

        IInstrument_sptr inst = ws->getInstrument();

        TS_ASSERT_EQUALS(inst->getName(), "TOPAZ");

        std::map<int, Geometry::IDetector_sptr> detectors = inst->getDetectors();
        TS_ASSERT_EQUALS(detectors.size(), 14);
        //std::cerr << inst->getDetector(0);


    }
};

#endif /*LOADINSTRUMENTFROMSNSNEXUSTEST_H_*/
