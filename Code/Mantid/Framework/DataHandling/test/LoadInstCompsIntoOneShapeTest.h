#ifndef LOADINSTCOMPSINTOONESHAPETEST_H_
#define LOADINSTCOMPSINTOONESHAPETEST_H_

#include "MantidDataHandling/LoadInstCompsIntoOneShape.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iostream>
#include <vector>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadInstCompsIntoOneShapeTest : public CxxTest::TestSuite
{
public:

  void testGetAbsolutPositionInCompCoorSys()
  {
    CompAssembly base("base");
    base.setPos(1.0,1.0,1.0);
    base.rotate(Quat(90.0, V3D(0,0,1)));
    
    LoadInstCompsIntoOneShape helper;
    
    V3D test = helper.getAbsolutPositionInCompCoorSys(&base, V3D(1,0,0));

    TS_ASSERT_DELTA( test.X(),  1.0, 0.0001);
    TS_ASSERT_DELTA( test.Y(), 2.0, 0.0001);
    TS_ASSERT_DELTA( test.Z(),  1.0, 0.0001);    
  }

  // testing through Loading IDF_for_UNIT_TESTING5.xml method adjust()
  void testAdjust()
  {

    //system("pause");

    LoadEmptyInstrument loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT( loader.isInitialized() );
    loader.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING5.xml");
    std::string inputFile = loader.getPropertyValue("Filename");
    std::string wsName = "LoadInstCompsIntoOneShape_testAdjust";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    MatrixWorkspace_sptr ws;
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));

    boost::shared_ptr<Instrument> i = ws->getInstrument();

    // None rotated cuboid
    boost::shared_ptr<const IDetector> ptrNoneRot = i->getDetector(1400);
    TS_ASSERT( !ptrNoneRot->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( ptrNoneRot->isValid(V3D(0.0,0.0,3.0)) );
    TS_ASSERT( !ptrNoneRot->isValid(V3D(0.0,4.5,0.0)) );
    TS_ASSERT( ptrNoneRot->isValid(V3D(0.0,4.5,3.0)) );
    TS_ASSERT( !ptrNoneRot->isValid(V3D(0.0,5.5,3.0)) );
    TS_ASSERT( !ptrNoneRot->isValid(V3D(4.5,0.0,3.0)) );

    // rotated cuboids
    boost::shared_ptr<const IDetector> ptrRot = i->getDetector(1300);
    TS_ASSERT( ptrRot->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,0.0,3.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,4.5,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,4.5,3.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,7.5,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,10.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,10.0,4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,10.0,5.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,10.0,-4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.5,10.0,0.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.5,10.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(-0.5,10.0,0.0)) );

    // nested rotated cuboids
    ptrRot = i->getDetector(1350);
    TS_ASSERT( ptrRot->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,0.0,3.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,4.5,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,4.5,3.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,7.5,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,20.0,5.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,-4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.5,20.0,0.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.5,20.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(-0.5,20.0,0.0)) );

  }

};


#endif /*LOADINSTCOMPSINTOONESHAPETEST_H_*/

