#ifndef LOADINSTRUMENTTESTFROMRAW_H_
#define LOADINSTRUMENTTESTFROMRAW_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadIDFFromNexus.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
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

class LoadIDFFromNexusTest : public CxxTest::TestSuite
{
public:

  static LoadIDFFromNexusTest *createSuite() { return new LoadIDFFromNexusTest(); }
  static void destroySuite(LoadIDFFromNexusTest *suite) { delete suite; }

  LoadIDFFromNexusTest()
  {
  }

  void testInit()
  {
    TS_ASSERT( !loader.isInitialized() );
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT( loader.isInitialized() );
  }

  void testExec()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    //create a workspace with some sample data
    wsName = "LoadIDFFromNexusTest";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // set properties
    loader.setPropertyValue("Workspace", wsName);
    loader.setPropertyValue("Filename", "LOQ48127.nxs");
    loader.setPropertyValue("InstrumentParentPath","mantid_workspace_1"); 
    inputFile = loader.getPropertyValue("Filename"); // get full pathname
    
    // check properties
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("InstrumentParentPath") )
    TS_ASSERT( ! result.compare("mantid_workspace_1"));

    // execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    boost::shared_ptr<const Instrument> i = output->getInstrument();
    TS_ASSERT_EQUALS( i->getName(), "LOQ");
    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "source");
    TS_ASSERT_DELTA( source->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<const IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(),"some-sample-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 11.0,0.01);

    // test third pixel in main detector bank, which has indices (2,0)
    boost::shared_ptr<const Detector> ptrDetMain = boost::dynamic_pointer_cast<const Detector>(i->getDetector(5));
    TS_ASSERT_EQUALS( ptrDetMain->getID(), 5);
    TS_ASSERT_EQUALS( ptrDetMain->getName(), "main-detector-bank(2,0)");
    TS_ASSERT_DELTA( ptrDetMain->getPos().X(), -0.3035,0.0001); 
    TS_ASSERT_DELTA( ptrDetMain->getPos().Y(), -0.3124,0.0001);
    double d = ptrDetMain->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d, 4.1727, 0.0001);
    double cmpDistance = ptrDetMain->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance, 4.1727, 0.0001);

    TS_ASSERT_EQUALS( ptrDetMain->type(), "RectangularDetectorPixel");

    // also a few tests on a HAB pixel detector
    boost::shared_ptr<const Detector> ptrDetHab = boost::dynamic_pointer_cast<const Detector>(i->getDetector(16734));
    TS_ASSERT_EQUALS( ptrDetHab->getID(), 16734);
    TS_ASSERT_EQUALS( ptrDetHab->getName(), "HAB-pixel");
    // test a non-existant detector
    TS_ASSERT_THROWS(i->getDetector(16735), Exception::NotFoundError);

    // Check the monitors are correctly marked
    TS_ASSERT( i->getDetector(1)->isMonitor() )
    TS_ASSERT( i->getDetector(2)->isMonitor() )
    // ...and that a normal detector isn't
    TS_ASSERT( ! i->getDetector(3)->isMonitor() )
    TS_ASSERT( ! i->getDetector(300)->isMonitor() )
    TS_ASSERT( ! i->getDetector(16500)->isMonitor() )

	AnalysisDataService::Instance().remove(wsName);
  }



private:
  LoadIDFFromNexus loader;
  std::string inputFile;
  std::string wsName;

};

#endif /*LOADINSTRUMENTTESTFROMRAW_H_*/
