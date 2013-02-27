#ifndef APPLYCALIBRATIONTEST_H_
#define APPLYCALIBRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/ApplyCalibration.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "WorkspaceCreationHelperTest.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_const_sptr;

class ApplyCalibrationTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( appCalib.name(), "ApplyCalibration" )
  }

  void testInit()
  {
    appCalib.initialize();
    TS_ASSERT( appCalib.isInitialized() )
  }

  void testExec()
  {

     int ndets = 3; 

     // Create workspace with paremeterised instrument and put into data store
     Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(ndets, 10, true);
     const std::string wsName("ApplyCabrationWs");
     AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
     dataStore.add(wsName, ws);

     // Create Calibration Table
     ITableWorkspace_sptr posTableWs = WorkspaceFactory::Instance().createTable();
     posTableWs->addColumn("int","Detector ID");
     posTableWs->addColumn("V3D","Detector Position");

     for(int i=0; i < ndets; ++i)
     {
       TableRow row = posTableWs->appendRow();
       row << i+1 << V3D(1.0,0.01*i,2.0); 
     }
     TS_ASSERT_THROWS_NOTHING(appCalib.setPropertyValue("Workspace", wsName ));
     TS_ASSERT_THROWS_NOTHING(appCalib.setProperty<ITableWorkspace_sptr>("PositionTable", posTableWs ));
     TS_ASSERT_THROWS_NOTHING(appCalib.execute());

     TS_ASSERT( appCalib.isExecuted() );
     
     IDetector_const_sptr det = ws->getDetector(0);
     int id = det->getID();
     V3D newPos = det->getPos();
     TS_ASSERT_EQUALS( id, 1);
     TS_ASSERT_DELTA( newPos.X() , 1.0, 0.0001);
     TS_ASSERT_DELTA( newPos.Y() , 0.0, 0.0001);
     TS_ASSERT_DELTA( newPos.Z() , 2.0, 0.0001);

     det = ws->getDetector(ndets-1);
     id = det->getID();
     newPos = det->getPos();
     TS_ASSERT_EQUALS( id, ndets);
     TS_ASSERT_DELTA( newPos.X() , 1.0, 0.0001);
     TS_ASSERT_DELTA( newPos.Y() , 0.01*(ndets-1), 0.0001);
     TS_ASSERT_DELTA( newPos.Z() , 2.0, 0.0001);
  }

private:
  ApplyCalibration appCalib;


};

#endif /*APPLYCALIBRATIONTEST_H_*/
