#ifndef LOADINSTRUMENTTEST_H_
#define LOADINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/IDetector.h"
#include <vector>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadInstrumentTest : public CxxTest::TestSuite
{
public:

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
      boost::shared_ptr<Mantid::MantidVec> timeChannelsVec(new Mantid::MantidVec),v(new Mantid::MantidVec),e(new Mantid::MantidVec);
      timeChannelsVec->resize(timechannels);
      v->resize(timechannels);
      e->resize(timechannels);
      //timechannels
      for (int j = 0; j < timechannels; j++)
      {
        (*timeChannelsVec)[j] = j*100;
        (*v)[j] = (i+j)%256;
        (*e)[j] = (i+j)%78;
      }
      // Populate the workspace.
      ws2D->setX(i, timeChannelsVec);
      ws2D->setData(i, v, e);
    }

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from SVN
    loader.setPropertyValue("Filename", "../../../../Test/Instrument/HET_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    loader.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") );
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

    boost::shared_ptr<IDetector> ptrDet103 = i->getDetector(103);
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);
    double d = ptrDet103->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.512,0.0001);
    double cmpDistance = ptrDet103->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.512,0.0001);

    // test if detector with det_id=603 has been marked as a monitor
    boost::shared_ptr<IDetector> ptrMonitor = i->getDetector(601);
    TS_ASSERT( ptrMonitor->isMonitor() );


    // also a few tests on the last detector and a test for the one beyond the last
    boost::shared_ptr<IDetector> ptrDetLast = i->getDetector(413256);
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
    TS_ASSERT_EQUALS( output->getInstrument(), instr );
    LoadInstrument loadAgain;
    TS_ASSERT_THROWS_NOTHING( loadAgain.initialize() );
    loadAgain.setPropertyValue("Filename", inputFile);
    loadAgain.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING( loadAgain.execute() );
    TS_ASSERT_EQUALS( output->getInstrument(), i );

    AnalysisDataService::Instance().remove(wsName);
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
    loaderGEM.setPropertyValue("Filename", "../../../../Test/Instrument/GEM_Definition.xml");
    inputFile = loaderGEM.getPropertyValue("Filename");

    loaderGEM.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderGEM.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderGEM.getPropertyValue("Workspace") );
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

    boost::shared_ptr<IDetector> ptrDet =i->getDetector(101001);
    TS_ASSERT_EQUALS( ptrDet->getID(), 101001);
    TS_ASSERT_DELTA( ptrDet->getPos().X(),  0.2607, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Y(), -0.1505, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(),  2.3461, 0.0001);
    double d = ptrDet->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.3653,0.0001);
    double cmpDistance = ptrDet->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.3653,0.0001);

    // test if detector with det_id=621 has been marked as a monitor
    boost::shared_ptr<IDetector> ptrMonitor = i->getDetector(621);
    TS_ASSERT( ptrMonitor->isMonitor() );

    // test if shape on for 1st monitor which is located at (0,0,-10.78)
    boost::shared_ptr<IDetector> ptrMonitorShape = i->getDetector(611);
    TS_ASSERT( ptrMonitorShape->isMonitor() );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(0.0,0.0,0.001)+ptrMonitorShape->getPos()) );
    TS_ASSERT( ptrMonitorShape->isValid(V3D(0.0,0.0,-0.01)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(0.0,0.0,-0.04)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(-2.1,-2.01,-2.01)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(100,100,100)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(-200.0,-200.0,-2000.1)+ptrMonitorShape->getPos()) );

    // test of some detector...
    boost::shared_ptr<IDetector> ptrDetShape = i->getDetector(101001);
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.0)+ptrDetShape->getPos()) );

    AnalysisDataService::Instance().remove(wsName);
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

    loaderSLS.setPropertyValue("Filename", "../../../../Test/Instrument/SANDALS_Definition.xml");
    inputFile = loaderSLS.getPropertyValue("Filename");

    loaderSLS.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Workspace") );
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

    boost::shared_ptr<IDetector> ptrDet = i->getDetector(101);
    TS_ASSERT_EQUALS( ptrDet->getID(), 101);

    boost::shared_ptr<IDetector> ptrMonitor = i->getDetector(1);
    TS_ASSERT( ptrMonitor->isMonitor() );

    boost::shared_ptr<IDetector> ptrDetShape = i->getDetector(102);
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.000001)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(0.005,0.1,0.000002)+ptrDetShape->getPos()) );


    // test of sample shape
    TS_ASSERT( samplepos->isValid(V3D(0.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(0.0,0.0,0.05)+samplepos->getPos()) );

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecNIMROD()
  {
    LoadInstrument loaderNIMROD;

    TS_ASSERT_THROWS_NOTHING(loaderNIMROD.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestNIMROD";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderNIMROD.setPropertyValue("Filename", "../../../../Test/Instrument/NIM_Definition.xml");
    inputFile = loaderNIMROD.getPropertyValue("Filename");

    loaderNIMROD.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderNIMROD.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderNIMROD.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderNIMROD.execute());

    TS_ASSERT( loaderNIMROD.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();

    boost::shared_ptr<IDetector> ptrDet = i->getDetector(20201001);
    TS_ASSERT_EQUALS( ptrDet->getName(), "det 1");
    TS_ASSERT_EQUALS( ptrDet->getID(), 20201001);
    TS_ASSERT_DELTA( ptrDet->getPos().X(),  -0.0909, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Y(), 0.3983, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(),  4.8888, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }


  void testExecHRP()
  {
    InstrumentDataService::Instance().remove("HRPD_Definition.xml");

    LoadInstrument loaderHRP;

    TS_ASSERT_THROWS_NOTHING(loaderHRP.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestHRPD";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderHRP.setPropertyValue("Filename", "../../../../Test/Instrument/HRPD_Definition.xml");
    inputFile = loaderHRP.getPropertyValue("Filename");

    loaderHRP.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderHRP.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderHRP.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderHRP.execute());

    TS_ASSERT( loaderHRP.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();

    boost::shared_ptr<IDetector> ptrDetShape = i->getDetector(3100);
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

    // test if a dummy parameter has been read in
    boost::shared_ptr<IComponent> comp = i->getComponentByName("bank_90degnew");
    TS_ASSERT_EQUALS( comp->getName(), "bank_90degnew");

    ParameterMap& paramMap = output->instrumentParameters();

    Parameter_sptr param = paramMap.getRecursive(&(*comp), "S", "fitting");
    const FitParameter& fitParam4 = param->value<FitParameter>();
    TS_ASSERT( fitParam4.getTie().compare("") == 0 );
    TS_ASSERT( fitParam4.getFunction().compare("BackToBackExponential") == 0 );

    AnalysisDataService::Instance().remove(wsName);
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
    loaderIDF.setPropertyValue("Filename", "../../../../Test/Instrument/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml");
    inputFile = loaderIDF.getPropertyValue("Filename");

    loaderIDF.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderIDF.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderIDF.getPropertyValue("Workspace") );
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

    boost::shared_ptr<IDetector> ptrDet1 = i->getDetector(1);
    TS_ASSERT_EQUALS( ptrDet1->getID(), 1);
    TS_ASSERT_DELTA( ptrDet1->getPos().X(),  0.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet1->getPos().Y(), 10.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet1->getPos().Z(),  0.0, 0.0001);
    double d = ptrDet1->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,10.0,0.0001);
    double cmpDistance = ptrDet1->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,10.0,0.0001);

    boost::shared_ptr<IDetector> ptrDet2 = i->getDetector(2);
    TS_ASSERT_EQUALS( ptrDet2->getID(), 2);
    TS_ASSERT_DELTA( ptrDet2->getPos().X(),  0.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet2->getPos().Y(), -10.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet2->getPos().Z(),  0.0, 0.0001);
    d = ptrDet2->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,10.0,0.0001);
    cmpDistance = ptrDet2->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,10.0,0.0001);


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

    boost::shared_ptr<IDetector> ptrDet3 = i->getDetector(3);
    TS_ASSERT( !ptrDet3->isValid(V3D(0.02,0.0,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(-0.02,0.0,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,0.02,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,-0.02,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( ptrDet3->isValid(V3D(0.0,0.0,0.02)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,0.0,-0.02)+ptrDet3->getPos()) );

    boost::shared_ptr<IDetector> ptrDet4 = i->getDetector(4);
    TS_ASSERT( !ptrDet4->isValid(V3D(0.02,0.0,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(-0.02,0.0,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,0.02,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,-0.02,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,0.0,0.02)+ptrDet4->getPos()) );
    TS_ASSERT( ptrDet4->isValid(V3D(0.0,0.0,-0.02)+ptrDet4->getPos()) );

    // test of facing as a sub-element of location
    boost::shared_ptr<IDetector> ptrDet5 = i->getDetector(5);
    TS_ASSERT( !ptrDet5->isValid(V3D(0.02,0.0,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( ptrDet5->isValid(V3D(-0.02,0.0,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.02,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,-0.02,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.0,0.02)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.0,-0.02)+ptrDet5->getPos()) );

    // test of infinite-cone.
    boost::shared_ptr<IDetector> ptrDet6 = i->getDetector(6);
    TS_ASSERT( !ptrDet6->isValid(V3D(0.02,0.0,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(-0.02,0.0,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,0.02,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,-0.02,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,0.0,0.02)+ptrDet6->getPos()) );
    TS_ASSERT( ptrDet6->isValid(V3D(0.0,0.0,-0.02)+ptrDet6->getPos()) );
    TS_ASSERT( ptrDet6->isValid(V3D(0.0,0.0,-1.02)+ptrDet6->getPos()) );

    // test of (finite) cone.
    boost::shared_ptr<IDetector> ptrDet7 = i->getDetector(7);
    TS_ASSERT( !ptrDet7->isValid(V3D(0.02,0.0,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(-0.02,0.0,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.02,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,-0.02,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.0,0.02)+ptrDet7->getPos()) );
    TS_ASSERT( ptrDet7->isValid(V3D(0.0,0.0,-0.02)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.0,-1.02)+ptrDet7->getPos()) );

    // test of hexahedron.
    boost::shared_ptr<IDetector> ptrDet8 = i->getDetector(8);
    TS_ASSERT( ptrDet8->isValid(V3D(0.4,0.4,0.0)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.8,0.8,0.0)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.4,0.4,2.0)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.8,0.8,2.0)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.0,0.0,-0.02)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.0,0.0,2.02)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.5,0.5,0.1)+ptrDet8->getPos()) );

    // test for "cuboid-rotating-test".
    boost::shared_ptr<IDetector> ptrDet10 = i->getDetector(10);
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.0,0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.0,-0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.02,0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.02,-0.1)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,0.05,0.0)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.05,0.0)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.01,0.05)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.01,-0.05)+ptrDet10->getPos()) );
    boost::shared_ptr<IDetector> ptrDet11 = i->getDetector(11);
    TS_ASSERT( ptrDet11->isValid(V3D(-0.07,0.0,-0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(0.07,0.0,0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(0.07,0.01,0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(-0.07,0.01,-0.07)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,0.05,0.0)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.05,0.0)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.01,0.05)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.01,-0.05)+ptrDet11->getPos()) );

    // test for "infinite-cylinder-test".
    boost::shared_ptr<IDetector> ptrDet12 = i->getDetector(12);
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,0.1)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,-0.1)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.1,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,-0.1,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.1,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(-0.1,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( !ptrDet12->isValid(V3D(2.0,0.0,0.0)+ptrDet12->getPos()) );

    // test for "finite-cylinder-test".
    boost::shared_ptr<IDetector> ptrDet13 = i->getDetector(13);
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.0,0.1)+ptrDet13->getPos()) );
    TS_ASSERT( !ptrDet13->isValid(V3D(0.0,0.0,-0.1)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.1,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,-0.1,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.1,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(-0.1,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( !ptrDet13->isValid(V3D(2.0,0.0,0.0)+ptrDet13->getPos()) );

    // test for "complement-test".
    boost::shared_ptr<IDetector> ptrDet14 = i->getDetector(14);
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
    boost::shared_ptr<IDetector> ptrDet15 = i->getDetector(15);
    TS_ASSERT( !ptrDet15->isValid(V3D(0.0,0.09,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( !ptrDet15->isValid(V3D(0.0,-0.09,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( ptrDet15->isValid(V3D(0.09,0.0,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( ptrDet15->isValid(V3D(-0.09,0.0,0.01)+ptrDet15->getPos()) );
    boost::shared_ptr<IDetector> ptrDet16 = i->getDetector(16);
    TS_ASSERT( ptrDet16->isValid(V3D(0.0,0.0,0.09)+ptrDet16->getPos()) );
    TS_ASSERT( ptrDet16->isValid(V3D(0.0,0.0,-0.09)+ptrDet16->getPos()) );
    TS_ASSERT( !ptrDet16->isValid(V3D(0.0,0.09,0.0)+ptrDet16->getPos()) );
    TS_ASSERT( !ptrDet16->isValid(V3D(0.0,0.09,0.0)+ptrDet16->getPos()) );
    boost::shared_ptr<IDetector> ptrDet17 = i->getDetector(17);
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

    AnalysisDataService::Instance().remove(wsName);
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
    loaderIDF2.setPropertyValue("Filename", "../../../../Test/Instrument/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
    inputFile = loaderIDF2.getPropertyValue("Filename");

    loaderIDF2.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderIDF2.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderIDF2.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.execute());

    TS_ASSERT( loaderIDF2.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();

    boost::shared_ptr<IDetector> ptrDetShape = i->getDetector(1100);
    TS_ASSERT_EQUALS( ptrDetShape->getID(), 1100);

    // Test of monitor shape
    boost::shared_ptr<IDetector> ptrMonShape = i->getDetector(1001);
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

    AnalysisDataService::Instance().remove(wsName);
  }

    void testExec_RectangularDetector()
    {
      LoadInstrument loaderIDF2;
      loaderIDF2.initialize();
      //create a workspace with some sample data
      wsName = "RectangularDetector";
      Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
      Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
      //put this workspace in the data service
      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

      // Path to test input file assumes Test directory checked out from SVN
      loaderIDF2.setPropertyValue("Filename", "../../../../Test/Instrument/IDFs_for_UNIT_TESTING/IDF_for_RECTANGULAR_UNIT_TESTING.xml");
      inputFile = loaderIDF2.getPropertyValue("Filename");
      loaderIDF2.setPropertyValue("Workspace", wsName);
      TS_ASSERT_THROWS_NOTHING(loaderIDF2.execute());
      TS_ASSERT( loaderIDF2.isExecuted() );

      // Get back the saved workspace
      MatrixWorkspace_sptr output;
      TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));
      boost::shared_ptr<IInstrument> i = output->getInstrument();

      // Now the XY detector in bank1
      boost::shared_ptr<RectangularDetector> bank1 = boost::dynamic_pointer_cast<RectangularDetector>( i->getComponentByName("bank1") );
      TS_ASSERT( bank1 );
      if (!bank1) return;

      //Right # of elements?
      TS_ASSERT_EQUALS( bank1->nelements(), 100*200);

      //Positions according to formula
      TS_ASSERT_DELTA( bank1->getAtXY(0,0)->getPos().X(), -0.1, 1e-4 );
      TS_ASSERT_DELTA( bank1->getAtXY(0,0)->getPos().Y(), -0.2, 1e-4 );
      TS_ASSERT_DELTA( bank1->getAtXY(1,0)->getPos().X(), -0.098, 1e-4 );
      TS_ASSERT_DELTA( bank1->getAtXY(1,1)->getPos().Y(), -0.198, 1e-4 );

      //Some IDs
      TS_ASSERT_EQUALS( bank1->getAtXY(0,0)->getID(), 1000);
      TS_ASSERT_EQUALS( bank1->getAtXY(0,1)->getID(), 1001);
      TS_ASSERT_EQUALS( bank1->getAtXY(1,0)->getID(), 1300);
      TS_ASSERT_EQUALS( bank1->getAtXY(1,1)->getID(), 1301);

      //The total number of detectors
      TS_ASSERT_EQUALS( i->getDetectors().size(), 100*200 * 2);

      AnalysisDataService::Instance().remove(wsName);
  }


//
//    /** Compare the old and new SNAP instrument definitions **/
//    void xtestExecSNAPComparison_SLOW() // This test is slow!
//    {
//      LoadInstrument * loaderIDF2;
//      MatrixWorkspace_sptr output;
//      Workspace_sptr ws;
//      Workspace2D_sptr ws2D;
//
//      std::cout << "Loading the NEW snap geometry\n";
//      loaderIDF2 = new LoadInstrument();
//      loaderIDF2->initialize();
//      wsName = "SNAP_NEW";
//      ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
//      ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
//      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));
//      loaderIDF2->setPropertyValue("Filename", "../../../../Test/Instrument/SNAP_Definition.xml");
//      inputFile = loaderIDF2->getPropertyValue("Filename");
//      loaderIDF2->setPropertyValue("Workspace", wsName);
//      loaderIDF2->execute();
//      TS_ASSERT( loaderIDF2->isExecuted() );
//      output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
//      boost::shared_ptr<IInstrument> i_new = output->getInstrument();
//      TS_ASSERT_EQUALS( i_new->getName(), "SNAP");
//
//      TS_ASSERT_EQUALS( i_new->nelements(), 21);
//
//      std::cout << "Loading the OLD snap geometry\n";
//      loaderIDF2 = new LoadInstrument();
//      loaderIDF2->initialize();
//      wsName = "SNAP_OLD";
//      ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
//      ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
//      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));
//      loaderIDF2->setPropertyValue("Filename", "../../../../Test/Instrument/SNAPOLD_Definition.xml");
//      inputFile = loaderIDF2->getPropertyValue("Filename");
//      loaderIDF2->setPropertyValue("Workspace", wsName);
//      loaderIDF2->execute();
//      TS_ASSERT( loaderIDF2->isExecuted() );
//      output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
//      boost::shared_ptr<IInstrument> i_old = output->getInstrument();
//      TS_ASSERT_EQUALS( i_old->getName(), "SNAPOLD");
//
//      std::cout << "Comparing\n";
//
//      TS_ASSERT_EQUALS( i_new->nelements(), i_old->nelements());
//
//      //Compare the list of detectors
//      std::map<int, Geometry::IDetector_sptr> bank_new = i_new->getDetectors();
//      std::map<int, Geometry::IDetector_sptr> bank_old = i_old->getDetectors();
//      TS_ASSERT_EQUALS( bank_new.size(), bank_old.size());
//      TS_ASSERT_EQUALS( bank_new.size(), 65536*18 + 1); //Plus one for the monitor
//
//      std::map<int, Geometry::IDetector_sptr>::iterator it;
//      int count = 0;
//      for (it = bank_new.begin(); it != bank_new.end(); it++)
//      {
//        count++;
//        Geometry::IDetector_sptr det_new = it->second;
//        Geometry::IDetector_sptr det_old = bank_old[it->first];
//        //Compare their positions
//        TS_ASSERT_EQUALS( det_new->getPos(), det_old->getPos() );
//      }
//
//      TS_ASSERT_LESS_THAN( 65536*18, count);
//
//
//    }

private:
  LoadInstrument loader;
  std::string inputFile;
  std::string wsName;

};

#endif /*LOADINSTRUMENTTEST_H_*/

