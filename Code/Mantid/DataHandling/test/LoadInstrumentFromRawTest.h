#ifndef LOADINSTRUMENTTESTFROMRAW_H_
#define LOADINSTRUMENTTESTFROMRAW_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrumentFromRaw.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadInstrumentFromRawTest : public CxxTest::TestSuite
{
public:

  LoadInstrumentFromRawTest()
  {
	//initialise framework manager to allow logging
	//Mantid::API::FrameworkManager::Instance().initialize();
  }
  void testInit()
  {
    TS_ASSERT( !loader.isInitialized() );
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT( loader.isInitialized() );
  }

  void testExecHET()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    //create a workspace with some sample data
    wsName = "LoadInstrumentFromRawTest";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // set properties and check this are set ok
    loader.setPropertyValue("Filename", "../../../../Test/AutoTestData/HET15869.raw");
    inputFile = loader.getPropertyValue("Filename");
    loader.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    // execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();
    TS_ASSERT_EQUALS( i->getName(), "HET     ");
    boost::shared_ptr<IComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "Source");
    TS_ASSERT_DELTA( source->getPos().Z(), -11.8,0.01);

    boost::shared_ptr<IComponent> samplepos = i->getSample();
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<Detector> ptrDetSp = boost::dynamic_pointer_cast<Detector>(i->getDetector(5));
    TS_ASSERT_EQUALS( ptrDetSp->getID(), 5);
    TS_ASSERT_EQUALS( ptrDetSp->getName(), "det");
    TS_ASSERT_DELTA( ptrDetSp->getPos().X(), -0.5141,0.01); // using phi values from raw file changes sign of this
    TS_ASSERT_DELTA( ptrDetSp->getPos().Z(), 2.4588,0.01);
    double d = ptrDetSp->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d, 2.5120, 0.0001);
    double cmpDistance = ptrDetSp->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance, 2.5120, 0.0001);

    TS_ASSERT_EQUALS( ptrDetSp->type(), "DetectorComponent");

    // also a few tests on the last detector and a test for the one beyond the last
    boost::shared_ptr<Detector> ptrDetLast = boost::dynamic_pointer_cast<Detector>(i->getDetector(718048));
    TS_ASSERT_EQUALS( ptrDetLast->getID(), 718048);
    TS_ASSERT_THROWS(i->getDetector(718049), Exception::NotFoundError);

    // Check the monitors are correctly marked
    TS_ASSERT( i->getDetector(601)->isMonitor() )
    TS_ASSERT( i->getDetector(602)->isMonitor() )
    TS_ASSERT( i->getDetector(603)->isMonitor() )
    TS_ASSERT( i->getDetector(604)->isMonitor() )
    // ...and that a normal detector isn't
    TS_ASSERT( ! i->getDetector(1)->isMonitor() )
    TS_ASSERT( ! i->getDetector(101256)->isMonitor() )
    TS_ASSERT( ! i->getDetector(718048)->isMonitor() )

	AnalysisDataService::Instance().remove(wsName);
  }



private:
  LoadInstrumentFromRaw loader;
  std::string inputFile;
  std::string wsName;

};

#endif /*LOADINSTRUMENTTESTFROMRAW_H_*/
