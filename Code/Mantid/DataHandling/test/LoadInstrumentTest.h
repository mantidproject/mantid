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
#include "Component.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadInstrumentTest : public CxxTest::TestSuite
{
public: 
  
  LoadInstrumentTest()
  {	
	//initialise framework manager to allow logging
	Mantid::API::FrameworkManager::Instance().initialize();
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
    wsName = "LoadInstrumentTest";
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
    inputFile = "../../../../Test/Instrument/HET_definition.xml";
    loader.setPropertyValue("Filename", inputFile);

    loader.setPropertyValue("Workspace", wsName);
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    //loader.execute();  
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    TS_ASSERT( loader.isExecuted() );    

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(wsName));
    
    Instrument& i = output->getInstrument();
    Mantid::Geometry::Component* source = i.getSource();
    TS_ASSERT_EQUALS( source->getName(), "Source");
    //TS_ASSERT_EQUALS( source->getPos(), Mantid::Geometry::V3D(0,0,0));
    TS_ASSERT_DELTA( source->getPos().Y(), 10.0,0.01);

    Mantid::Geometry::Component* samplepos = i.getSamplePos();
    TS_ASSERT_EQUALS( samplepos->getName(), "SamplePos");
    //TS_ASSERT_EQUALS( samplepos->getPos(), Mantid::Geometry::V3D(0,10,0));
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    TS_ASSERT_EQUALS(i.getDetectors()->nelements(),2184);

    Mantid::Geometry::Detector *ptrDet103 = i.getDetector(103);
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 3.6527,0.01);
    //TS_ASSERT_DELTA( ptrDet103->getPos().Y(), -1.7032,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 0.2222,0.01);
    double d = ptrDet103->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,9.0905,0.0001);
    double cmpDistance = ptrDet103->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,9.0905,0.0001);

    TS_ASSERT_EQUALS( ptrDet103->type(), "DetectorComponent");

    // Test input data is unchanged
    Workspace2D_sptr output2DInst = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584 
    TS_ASSERT_EQUALS( output2DInst->getHistogramNumber(), histogramNumber);

  }
 
  
private:
  LoadInstrument loader;
  std::string inputFile;
  std::string wsName;
  
};
  
#endif /*LOADINSTRUMENTTEST_H_*/
