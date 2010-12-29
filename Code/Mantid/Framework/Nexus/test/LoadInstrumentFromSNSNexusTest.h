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
#include "MantidKernel/PhysicalConstants.h"
using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>

class LoadInstrumentFromSNSNexusTest : public CxxTest::TestSuite
{
public:


    void testExec()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadInstrumentFromSNSNexus ld;
        std::string outws_name = "topaz_instrument";
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/TOPAZ_900.nxs");

        //Create an empty workspace with some fake size, to start from.
        DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create("Workspace2D",1000,18+1,18));
        //Put it in the object.
        ld.setProperty("Workspace", boost::dynamic_pointer_cast<Workspace>(ws));

        // std::cout << "Getting detectors\n";

        TS_ASSERT_THROWS_NOTHING(ld.execute());

        IInstrument_sptr inst = ws->getInstrument();

        TS_ASSERT_EQUALS(inst->getName(), "TOPAZ");
        std::map<int, Geometry::IDetector_sptr> detectors = inst->getDetectors();
        TS_ASSERT_EQUALS(detectors.size(), 14*65536); //256*256 pixels * 14 detectors

        //---------------------------------------------------
        Geometry::IDetector_sptr det;
        det = detectors[0];
        //Check the orientation of it
        Quat rot = det->getRotation();
        V3D pointer(0,0,-1.0);
        rot.rotate(pointer);

        V3D expected;
        expected.spherical_rad(1.0, -0.628319, 0);
        TS_ASSERT(pointer == expected);

        //---------------------------------------------------
        V3D pos(0,0,0);
        double rad2deg = 180.0/M_PI;
        //Test a few pixels in bank 1
        pos.spherical_rad(0.4104138, 0.6783125, -2.6941562);
        TS_ASSERT(detectors[0]->getRelativePos() == pos);
        //Row 0, column 1 (I think!)
        pos.spherical_rad(0.4102956, 0.67695636, -2.6950939);
        TS_ASSERT(detectors[1]->getRelativePos() == pos);
        //Row 1, column 0
        pos.spherical_rad(0.4102956, 0.6789524, -2.6962788);
        TS_ASSERT(detectors[256]->getRelativePos() == pos);

        //Now try bank 10
        int offset = 9*256*256;
        pos.spherical_rad(0.4393626, 1.5707995, -0.5356482);
        TS_ASSERT(detectors[offset+0]->getRelativePos() == pos);


    }
};

#endif /*LOADINSTRUMENTFROMSNSNEXUSTEST_H_*/
