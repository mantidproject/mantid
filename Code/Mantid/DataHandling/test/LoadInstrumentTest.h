#ifndef LOADINSTRUMENTTEST_H_
#define LOADINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Component.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadInstrumentTest : public CxxTest::TestSuite
{
public:

  LoadInstrumentTest()
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
    wsName = "LoadInstrumentTestHET";
    int histogramNumber = 2584;
    int timechannels = 100;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    //loop to create data
    for (int i = 0; i < histogramNumber; i++)
    {
      std::vector<double> timeChannelsVec(timechannels);
      std::vector<double> v(timechannels);
      // Create and fill another vector for the errors
      std::vector<double> e(timechannels);
      //timechannels
      for (int j = 0; j < timechannels; j++)
      {
        timeChannelsVec[j] = j*100;
        v[j] = (i+j)%256;
        e[j] = (i+j)%78;
      }
      // Populate the workspace.
      ws2D->setX(i, timeChannelsVec);
      ws2D->setData(i, v, e);
    }

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Instrument/HET_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);

    loader.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loader.execute());

    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(wsName));

    boost::shared_ptr<Instrument> i = output->getInstrument();
    Component* source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    Component* samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 10.0,0.01);

    Detector *ptrDet103 = dynamic_cast<Detector*>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 12.4470,0.01);
    double d = ptrDet103->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.512,0.0001);
    double cmpDistance = ptrDet103->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.512,0.0001);

    TS_ASSERT_EQUALS( ptrDet103->type(), "DetectorComponent");

    // test if detector with det_id=603 has been marked as a monitor
    Detector *ptrMonitor = dynamic_cast<Detector*>(i->getDetector(601));
    TS_ASSERT( ptrMonitor->isMonitor() );


    // also a few tests on the last detector and a test for the one beyond the last
    Detector *ptrDetLast = dynamic_cast<Detector*>(i->getDetector(718048));
    TS_ASSERT_EQUALS( ptrDetLast->getID(), 718048);
    TS_ASSERT_EQUALS( ptrDetLast->getName(), "pixel");
    TS_ASSERT_THROWS(i->getDetector(718049), Exception::NotFoundError);

    // Test input data is unchanged
    Workspace2D_sptr output2DInst = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584
    TS_ASSERT_EQUALS( output2DInst->getNumberHistograms(), histogramNumber);
  }

  void testExecGEM()
  {
    LoadInstrument loaderGEM;

    TS_ASSERT_THROWS_NOTHING(loaderGEM.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestGEM";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D");
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Instrument/GEM_Definition.xml";
    loaderGEM.setPropertyValue("Filename", inputFile);

    loaderGEM.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderGEM.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderGEM.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderGEM.execute());

    TS_ASSERT( loaderGEM.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(wsName));

    boost::shared_ptr<Instrument> i = output->getInstrument();
    Component* source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -17.0,0.01);

    Component* samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    Detector *ptrDet = dynamic_cast<Detector*>(i->getDetector(101001));
    TS_ASSERT_EQUALS( ptrDet->getID(), 101001);
    TS_ASSERT_EQUALS( ptrDet->getName(), "Det16");
    TS_ASSERT_DELTA( ptrDet->getPos().X(),  0.2607, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Y(), -0.1505, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(),  2.3461, 0.0001);
    double d = ptrDet->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.3653,0.0001);
    double cmpDistance = ptrDet->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.3653,0.0001);
    TS_ASSERT_EQUALS( ptrDet->type(), "DetectorComponent");

    // test if detector with det_id=621 has been marked as a monitor
    Detector *ptrMonitor = dynamic_cast<Detector*>(i->getDetector(621));
    TS_ASSERT( ptrMonitor->isMonitor() );

    // test if shape on for 1st monitor
    Detector *ptrMonitorShape = dynamic_cast<Detector*>(i->getDetector(611));
    TS_ASSERT( ptrMonitorShape->isMonitor() );
    TS_ASSERT( ptrMonitorShape->isValid(V3D(4.1,2.1,18.88)) );
    TS_ASSERT( ptrMonitorShape->isValid(V3D(-4.1,-2.1,2.68)) );

    TS_ASSERT( !ptrMonitorShape->isValid(V3D(0,0,0)) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(0,0,0.01)) );
    TS_ASSERT( ptrMonitorShape->isValid(V3D(100,100,100)) );
    TS_ASSERT( ptrMonitorShape->isValid(V3D(-200.0,-200.0,-200.1)) );

  }


private:
  LoadInstrument loader;
  std::string inputFile;
  std::string wsName;

};

#endif /*LOADINSTRUMENTTEST_H_*/
