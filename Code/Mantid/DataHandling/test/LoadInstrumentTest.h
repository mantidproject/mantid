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
#include <iostream>

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
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();
    boost::shared_ptr<IComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<Detector> ptrDet103 = boost::dynamic_pointer_cast<Detector>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);
    double d = ptrDet103->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.512,0.0001);
    double cmpDistance = ptrDet103->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.512,0.0001);

    TS_ASSERT_EQUALS( ptrDet103->type(), "DetectorComponent");

    // test if detector with det_id=603 has been marked as a monitor
    boost::shared_ptr<Detector> ptrMonitor = boost::dynamic_pointer_cast<Detector>(i->getDetector(601));
    TS_ASSERT( ptrMonitor->isMonitor() );


    // also a few tests on the last detector and a test for the one beyond the last
    boost::shared_ptr<Detector> ptrDetLast = boost::dynamic_pointer_cast<Detector>(i->getDetector(413256));
    TS_ASSERT_EQUALS( ptrDetLast->getID(), 413256);
    TS_ASSERT_EQUALS( ptrDetLast->getName(), "pixel");
    TS_ASSERT_THROWS(i->getDetector(413257), Exception::NotFoundError);

    // Test input data is unchanged
    Workspace2D_sptr output2DInst = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584
    TS_ASSERT_EQUALS( output2DInst->getNumberHistograms(), histogramNumber);


    // Check running algorithm for same XML file leads to same instrument object being attached
    boost::shared_ptr<Instrument> instr(new Instrument());
    output->setInstrument(instr);
    TS_ASSERT_EQUALS( output->getInstrument(), instr )
    LoadInstrument loadAgain;
    TS_ASSERT_THROWS_NOTHING( loadAgain.initialize() )
    loadAgain.setPropertyValue("Filename", inputFile);
    loadAgain.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING( loadAgain.execute() )
    TS_ASSERT_EQUALS( output->getInstrument(), i )
  }

  void testExecGEM()
  {
    LoadInstrument loaderGEM;

    TS_ASSERT_THROWS_NOTHING(loaderGEM.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestGEM";
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
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
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();
    boost::shared_ptr<IObjComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -17.0,0.01);

    boost::shared_ptr<IObjComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<Detector> ptrDet = boost::dynamic_pointer_cast<Detector>(i->getDetector(101001));
    TS_ASSERT_EQUALS( ptrDet->getID(), 101001);
    TS_ASSERT_DELTA( ptrDet->getPos().X(),  0.2607, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Y(), -0.1505, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(),  2.3461, 0.0001);
    double d = ptrDet->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.3653,0.0001);
    double cmpDistance = ptrDet->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.3653,0.0001);
    TS_ASSERT_EQUALS( ptrDet->type(), "DetectorComponent");

    // test if detector with det_id=621 has been marked as a monitor
    boost::shared_ptr<Detector> ptrMonitor = boost::dynamic_pointer_cast<Detector>(i->getDetector(621));
    TS_ASSERT( ptrMonitor->isMonitor() );

    // test if shape on for 1st monitor which is located at (0,0,-10.78)
    boost::shared_ptr<Detector> ptrMonitorShape = boost::dynamic_pointer_cast<Detector>(i->getDetector(611));
    TS_ASSERT( ptrMonitorShape->isMonitor() );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(0.0,0.0,0.001)+ptrMonitorShape->getPos()) );
    TS_ASSERT( ptrMonitorShape->isValid(V3D(0.0,0.0,-0.01)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(0.0,0.0,-0.04)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(-2.1,-2.01,-2.01)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(100,100,100)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(-200.0,-200.0,-2000.1)+ptrMonitorShape->getPos()) );

    // test of some detector...
    boost::shared_ptr<Detector> ptrDetShape = boost::dynamic_pointer_cast<Detector>(i->getDetector(101001));
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.0)+ptrDetShape->getPos()) );

  }

  void testExecSLS()
  {
    LoadInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestSLS";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    inputFile = "../../../../Test/Instrument/SLS_Definition.xml";
    loaderSLS.setPropertyValue("Filename", inputFile);

    loaderSLS.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());

    TS_ASSERT( loaderSLS.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();
    boost::shared_ptr<IObjComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -11.016,0.01);

    boost::shared_ptr<IObjComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<Detector> ptrDet = boost::dynamic_pointer_cast<Detector>(i->getDetector(101));
    TS_ASSERT_EQUALS( ptrDet->getID(), 101);
    TS_ASSERT_EQUALS( ptrDet->type(), "DetectorComponent");

    boost::shared_ptr<Detector> ptrMonitor = boost::dynamic_pointer_cast<Detector>(i->getDetector(1));
    TS_ASSERT( ptrMonitor->isMonitor() );

    boost::shared_ptr<Detector> ptrDetShape = boost::dynamic_pointer_cast<Detector>(i->getDetector(102));
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.000001)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(0.005,0.1,0.000002)+ptrDetShape->getPos()) );


    // test of sample shape
    TS_ASSERT( samplepos->isValid(V3D(0.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(0.0,0.0,0.05)+samplepos->getPos()) );
  }

  void testExecHRP()
  {
    LoadInstrument loaderHRP;

    TS_ASSERT_THROWS_NOTHING(loaderHRP.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestHRP";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    inputFile = "../../../../Test/Instrument/HRP_Definition.xml";
    loaderHRP.setPropertyValue("Filename", inputFile);

    loaderHRP.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderHRP.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderHRP.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderHRP.execute());

    TS_ASSERT( loaderHRP.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();

    boost::shared_ptr<Detector> ptrDetShape = boost::dynamic_pointer_cast<Detector>(i->getDetector(3100));
    TS_ASSERT_EQUALS( ptrDetShape->getName(), "Det0");

    // Test of backscattering detector
    TS_ASSERT( ptrDetShape->isValid(V3D(0.002,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(-0.002,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( !ptrDetShape->isValid(V3D(0.003,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( !ptrDetShape->isValid(V3D(-0.003,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(-0.0069,0.0227,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( !ptrDetShape->isValid(V3D(-0.0071,0.0227,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(-0.0069,0.0227,0.000009)+ptrDetShape->getPos()) );
    TS_ASSERT( !ptrDetShape->isValid(V3D(-0.0069,0.0227,0.011)+ptrDetShape->getPos()) );
  }

  void testExecIDF_for_unit_testing() // IDF stands for Instrument Definition File
  {
    LoadInstrument loaderIDF;

    TS_ASSERT_THROWS_NOTHING(loaderIDF.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestIDF";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Instrument/IDF_for_unit_testing.xml";
    loaderIDF.setPropertyValue("Filename", inputFile);

    loaderIDF.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderIDF.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderIDF.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderIDF.execute());

    TS_ASSERT( loaderIDF.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();
    boost::shared_ptr<IObjComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -17.0,0.01);

    boost::shared_ptr<IObjComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<Detector> ptrDet1 = boost::dynamic_pointer_cast<Detector>(i->getDetector(1));
    TS_ASSERT_EQUALS( ptrDet1->getID(), 1);
    TS_ASSERT_DELTA( ptrDet1->getPos().X(),  0.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet1->getPos().Y(), 10.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet1->getPos().Z(),  0.0, 0.0001);
    double d = ptrDet1->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,10.0,0.0001);
    double cmpDistance = ptrDet1->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,10.0,0.0001);
    TS_ASSERT_EQUALS( ptrDet1->type(), "DetectorComponent");

    boost::shared_ptr<Detector> ptrDet2 = boost::dynamic_pointer_cast<Detector>(i->getDetector(2));
    TS_ASSERT_EQUALS( ptrDet2->getID(), 2);
    TS_ASSERT_DELTA( ptrDet2->getPos().X(),  0.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet2->getPos().Y(), -10.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet2->getPos().Z(),  0.0, 0.0001);
    d = ptrDet2->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,10.0,0.0001);
    cmpDistance = ptrDet2->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,10.0,0.0001);
    TS_ASSERT_EQUALS( ptrDet2->type(), "DetectorComponent");


    // test if detectors face sample
    TS_ASSERT( !ptrDet1->isValid(V3D(0.02,0.0,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(-0.02,0.0,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( ptrDet1->isValid(V3D(0.0,0.02,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,-0.02,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,0.0,0.02)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,0.0,-0.02)+ptrDet1->getPos()) );

    TS_ASSERT( !ptrDet2->isValid(V3D(0.02,0.0,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(-0.02,0.0,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.02,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( ptrDet2->isValid(V3D(0.0,-0.02,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.0,0.02)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.0,-0.02)+ptrDet2->getPos()) );

    boost::shared_ptr<Detector> ptrDet3 = boost::dynamic_pointer_cast<Detector>(i->getDetector(3));
    TS_ASSERT( !ptrDet3->isValid(V3D(0.02,0.0,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(-0.02,0.0,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,0.02,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,-0.02,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( ptrDet3->isValid(V3D(0.0,0.0,0.02)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,0.0,-0.02)+ptrDet3->getPos()) );

    boost::shared_ptr<Detector> ptrDet4 = boost::dynamic_pointer_cast<Detector>(i->getDetector(4));
    TS_ASSERT( !ptrDet4->isValid(V3D(0.02,0.0,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(-0.02,0.0,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,0.02,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,-0.02,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,0.0,0.02)+ptrDet4->getPos()) );
    TS_ASSERT( ptrDet4->isValid(V3D(0.0,0.0,-0.02)+ptrDet4->getPos()) );

    // test of facing as a sub-element of location
    boost::shared_ptr<Detector> ptrDet5 = boost::dynamic_pointer_cast<Detector>(i->getDetector(5));
    TS_ASSERT( !ptrDet5->isValid(V3D(0.02,0.0,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( ptrDet5->isValid(V3D(-0.02,0.0,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.02,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,-0.02,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.0,0.02)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.0,-0.02)+ptrDet5->getPos()) );

    // test of infinite-cone. 
    boost::shared_ptr<Detector> ptrDet6 = boost::dynamic_pointer_cast<Detector>(i->getDetector(6));
    TS_ASSERT( !ptrDet6->isValid(V3D(0.02,0.0,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(-0.02,0.0,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,0.02,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,-0.02,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,0.0,0.02)+ptrDet6->getPos()) );
    TS_ASSERT( ptrDet6->isValid(V3D(0.0,0.0,-0.02)+ptrDet6->getPos()) );
    TS_ASSERT( ptrDet6->isValid(V3D(0.0,0.0,-1.02)+ptrDet6->getPos()) );

    // test of (finite) cone. 
    boost::shared_ptr<Detector> ptrDet7 = boost::dynamic_pointer_cast<Detector>(i->getDetector(7));
    TS_ASSERT( !ptrDet7->isValid(V3D(0.02,0.0,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(-0.02,0.0,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.02,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,-0.02,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.0,0.02)+ptrDet7->getPos()) );
    TS_ASSERT( ptrDet7->isValid(V3D(0.0,0.0,-0.02)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.0,-1.02)+ptrDet7->getPos()) );

    // test of hexahedron. 
    boost::shared_ptr<Detector> ptrDet8 = boost::dynamic_pointer_cast<Detector>(i->getDetector(8));
    TS_ASSERT( ptrDet8->isValid(V3D(0.4,0.4,0.0)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.8,0.8,0.0)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.4,0.4,2.0)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.8,0.8,2.0)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.0,0.0,-0.02)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.0,0.0,2.02)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.5,0.5,0.1)+ptrDet8->getPos()) );

    // test for "cuboid-rotating-test". 
    boost::shared_ptr<Detector> ptrDet10 = boost::dynamic_pointer_cast<Detector>(i->getDetector(10));
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.0,0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.0,-0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.02,0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.02,-0.1)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,0.05,0.0)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.05,0.0)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.01,0.05)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.01,-0.05)+ptrDet10->getPos()) );
    boost::shared_ptr<Detector> ptrDet11 = boost::dynamic_pointer_cast<Detector>(i->getDetector(11));
    TS_ASSERT( ptrDet11->isValid(V3D(-0.07,0.0,-0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(0.07,0.0,0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(0.07,0.01,0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(-0.07,0.01,-0.07)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,0.05,0.0)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.05,0.0)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.01,0.05)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.01,-0.05)+ptrDet11->getPos()) );

    // test for "infinite-cylinder-test". 
    boost::shared_ptr<Detector> ptrDet12 = boost::dynamic_pointer_cast<Detector>(i->getDetector(12));
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,0.1)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,-0.1)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.1,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,-0.1,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.1,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(-0.1,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( !ptrDet12->isValid(V3D(2.0,0.0,0.0)+ptrDet12->getPos()) );

    // test for "finite-cylinder-test". 
    boost::shared_ptr<Detector> ptrDet13 = boost::dynamic_pointer_cast<Detector>(i->getDetector(13));
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.0,0.1)+ptrDet13->getPos()) );
    TS_ASSERT( !ptrDet13->isValid(V3D(0.0,0.0,-0.1)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.1,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,-0.1,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.1,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(-0.1,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( !ptrDet13->isValid(V3D(2.0,0.0,0.0)+ptrDet13->getPos()) );

    // test for "complement-test". 
    boost::shared_ptr<Detector> ptrDet14 = boost::dynamic_pointer_cast<Detector>(i->getDetector(14));
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,-0.04)+ptrDet14->getPos()) );
    TS_ASSERT( ptrDet14->isValid(V3D(0.0,0.0,-0.06)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.04,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( ptrDet14->isValid(V3D(0.0,0.06,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.06,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.51,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.51,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,0.51)+ptrDet14->getPos()) );

    // test for "rotation-of-element-test". 
    boost::shared_ptr<Detector> ptrDet15 = boost::dynamic_pointer_cast<Detector>(i->getDetector(15));
    TS_ASSERT( !ptrDet15->isValid(V3D(0.0,0.09,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( !ptrDet15->isValid(V3D(0.0,-0.09,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( ptrDet15->isValid(V3D(0.09,0.0,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( ptrDet15->isValid(V3D(-0.09,0.0,0.01)+ptrDet15->getPos()) );
    boost::shared_ptr<Detector> ptrDet16 = boost::dynamic_pointer_cast<Detector>(i->getDetector(16));
    TS_ASSERT( ptrDet16->isValid(V3D(0.0,0.0,0.09)+ptrDet16->getPos()) );
    TS_ASSERT( ptrDet16->isValid(V3D(0.0,0.0,-0.09)+ptrDet16->getPos()) );
    TS_ASSERT( !ptrDet16->isValid(V3D(0.0,0.09,0.0)+ptrDet16->getPos()) );
    TS_ASSERT( !ptrDet16->isValid(V3D(0.0,0.09,0.0)+ptrDet16->getPos()) );
    boost::shared_ptr<Detector> ptrDet17 = boost::dynamic_pointer_cast<Detector>(i->getDetector(17));
    TS_ASSERT( ptrDet17->isValid(V3D(0.0,0.09,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( ptrDet17->isValid(V3D(0.0,-0.09,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( !ptrDet17->isValid(V3D(0.09,0.0,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( !ptrDet17->isValid(V3D(-0.09,0.0,0.01)+ptrDet17->getPos()) );

    // test of sample shape
    TS_ASSERT( samplepos->isValid(V3D(0.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(0.0,0.0,0.05)+samplepos->getPos()) );
    TS_ASSERT( samplepos->isValid(V3D(10.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(10.0,0.0,0.05)+samplepos->getPos()) );

    // test of source shape
    TS_ASSERT( source->isValid(V3D(0.0,0.0,0.005)+source->getPos()) );
    TS_ASSERT( !source->isValid(V3D(0.0,0.0,-0.005)+source->getPos()) );
    TS_ASSERT( !source->isValid(V3D(0.0,0.0,0.02)+source->getPos()) );
  }


  void testExecIDF_for_unit_testing2() // IDF stands for Instrument Definition File
  {
    LoadInstrument loaderIDF2;

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestIDF2";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Instrument/IDF_for_unit_testing2.xml";
    loaderIDF2.setPropertyValue("Filename", inputFile);

    loaderIDF2.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderIDF2.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderIDF2.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.execute());

    TS_ASSERT( loaderIDF2.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();

    boost::shared_ptr<Detector> ptrDetShape = boost::dynamic_pointer_cast<Detector>(i->getDetector(1100));
    TS_ASSERT_EQUALS( ptrDetShape->getID(), 1100);
    TS_ASSERT_EQUALS( ptrDetShape->type(), "DetectorComponent");

    // test slice-of-cylinder-ring shape
    // comment out for now, since need to make simpler monitor shape below
    // work first
    //TS_ASSERT( ptrDetShape->isValid(V3D(0.002,0.0,0.0)+ptrDetShape->getPos()) );
    //TS_ASSERT( ptrDetShape->isValid(V3D(-0.002,0.0,0.0)+ptrDetShape->getPos()) );
    //TS_ASSERT( !ptrDetShape->isValid(V3D(0.003,0.0,0.0)+ptrDetShape->getPos()) );
    //TS_ASSERT( !ptrDetShape->isValid(V3D(-0.003,0.0,0.0)+ptrDetShape->getPos()) );
    //TS_ASSERT( ptrDetShape->isValid(V3D(-0.0069,0.0227,0.0)+ptrDetShape->getPos()) );
    //TS_ASSERT( !ptrDetShape->isValid(V3D(-0.0071,0.0227,0.0)+ptrDetShape->getPos()) );
    //TS_ASSERT( ptrDetShape->isValid(V3D(-0.0069,0.0227,0.009)+ptrDetShape->getPos()) );
    //TS_ASSERT( !ptrDetShape->isValid(V3D(-0.0069,0.0227,0.011)+ptrDetShape->getPos()) );
    //TS_ASSERT( !ptrDetShape->isValid(V3D(-0.1242,0.0,0.0)+ptrDetShape->getPos()) );
    //TS_ASSERT( !ptrDetShape->isValid(V3D(-0.0621,0.0621,0.0)+ptrDetShape->getPos()) );

    // Test of monitor shape
    boost::shared_ptr<Detector> ptrMonShape = boost::dynamic_pointer_cast<Detector>(i->getDetector(1001));
    TS_ASSERT( ptrMonShape->isValid(V3D(0.002,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.002,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(0.003,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.003,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0069,0.0227,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0071,0.0227,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0069,0.0227,0.009)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0069,0.0227,0.011)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.1242,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0621,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,-0.0621,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0641,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0651,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0595,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0641,0.01)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0641,0.011)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0651,0.01)+ptrMonShape->getPos()) );
  }


private:
  LoadInstrument loader;
  std::string inputFile;
  std::string wsName;

};

#endif /*LOADINSTRUMENTTEST_H_*/
