#ifndef COPYINSTRUMENTPARAMETERSTEST_H_
#define COPYINSTRUMENTPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/CopyInstrumentParameters.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "WorkspaceCreationHelperTest.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_const_sptr;
using Mantid::Geometry::IComponent_const_sptr;
using namespace Geometry::ComponentHelper;

class CopyInstrumentParametersTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( copyInstParam.name(), "CopyInstrumentParameters" )
  }

  void testInit()
  {
    copyInstParam.initialize();
    TS_ASSERT( copyInstParam.isInitialized() )
  }

  void testExec()
  {
     // Create input workspace with paremeterised instrument and put into data store
     MatrixWorkspace_sptr ws1 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
     const std::string wsName1("CopyInstParamWs1");
     AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
     dataStore.add(wsName1, ws1);
     /// Create output workspace with the same base instrument and put into data store
     MatrixWorkspace_sptr ws2 = WorkspaceFactory::Instance().create( ws1 );
     const std::string wsName2("CopyInstParamWs2");
     dataStore.add(wsName2, ws2);

     // Set properties
     TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("InputWorkspace", wsName1 ));
     TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("OutputWorkspace", wsName2 ));
     // Get instrument of input workspace and move some detectors
     Geometry::ParameterMap *pmap;
     pmap = &(ws1->instrumentParameters());
     Geometry::Instrument_const_sptr instrument = ws1->getInstrument();
     IComponent_const_sptr det1 =instrument->getDetector(1);
     Geometry::ComponentHelper::moveComponent(*det1, *pmap, V3D(6.0,0.0,0.7), Absolute );
     IComponent_const_sptr det2 =instrument->getDetector(2);
     Geometry::ComponentHelper::moveComponent(*det2, *pmap, V3D(6.0,0.1,0.7), Absolute );

     // Verify that a detector moved in the input workspace has not yet been moved in the output workspace
     IDetector_const_sptr deto = ws2->getDetector(0);
     V3D newPos = deto->getPos();
     TS_ASSERT_DELTA( newPos.X() , 5.0, 0.0001);

     // Execute Algorithm
     TS_ASSERT_THROWS_NOTHING(copyInstParam.execute());
     TS_ASSERT( copyInstParam.isExecuted() );

     // Verify that the detectors in the output workspace have been moved as in the input workspace before execution
     IDetector_const_sptr deto1 = ws2->getDetector(0);
     int id1 = deto1->getID();
     V3D newPos1 = deto1->getPos();
     TS_ASSERT_EQUALS( id1, 1);
     TS_ASSERT_DELTA( newPos1.X() , 6.0, 0.0001);
     TS_ASSERT_DELTA( newPos1.Y() , 0.0, 0.0001);
     TS_ASSERT_DELTA( newPos1.Z() , 0.7, 0.0001);
     IDetector_const_sptr deto2 = ws2->getDetector(1);
     int id2 = deto2->getID();
     V3D newPos2 = deto2->getPos();
     TS_ASSERT_EQUALS( id2, 2);
     TS_ASSERT_DELTA( newPos2.X() , 6.0, 0.0001);
     TS_ASSERT_DELTA( newPos2.Y() , 0.1, 0.0001);
     TS_ASSERT_DELTA( newPos2.Z() , 0.7, 0.0001);

     dataStore.remove(wsName1);
     dataStore.remove(wsName2);
  }

	// #8186: it was decided to relax the previous requirement that it does not copy the instrument
	// parameter for different instruments
  void testDifferent_BaseInstrument_Warns()
  {
    // Create input workspace with parameterised instrument and put into data store
     MatrixWorkspace_sptr ws1 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
     const std::string wsName1("CopyInstParamWs1");
     AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
     dataStore.add(wsName1, ws1);
     // Create output workspace with another parameterised instrument and put into data store
     MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
     const std::string wsName2("CopyInstParamWs2");
     dataStore.add(wsName2, ws2);

     // Set properties
     TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("InputWorkspace", wsName1 ));
     TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("OutputWorkspace", wsName2 ));

     // Execute Algorithm, should warn but proceed
     copyInstParam.setRethrows(true);
     TS_ASSERT(copyInstParam.execute());
     TS_ASSERT( copyInstParam.isExecuted() );

     dataStore.remove(wsName1);
     dataStore.remove(wsName2);
  }

private:
  CopyInstrumentParameters copyInstParam;


};

#endif /*COPYINSTRUMENTPARAMETERSTEST_H_*/
