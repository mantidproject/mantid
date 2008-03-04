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
    inputFile = "../../../../Test/Instrument/HET_Definition.txt";
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
    
    Instrument& i = output->getInstrument();
    Mantid::Geometry::Component* source = i.getSource();
    TS_ASSERT_EQUALS( source->getName(), "Source");
    TS_ASSERT_EQUALS( source->getPos(), Mantid::Geometry::V3D(0,0,0));

    Mantid::Geometry::Component* samplepos = i.getSamplePos();
    TS_ASSERT_EQUALS( samplepos->getName(), "SamplePos");
    TS_ASSERT_EQUALS( samplepos->getPos(), Mantid::Geometry::V3D(0,10,0));

    TS_ASSERT_EQUALS(i.getDetectors()->nelements(),2184);

    Mantid::Geometry::Detector *ptrDet1000 = i.getDetector(1000);
    TS_ASSERT_EQUALS( ptrDet1000->getID(), 1000);
    TS_ASSERT_EQUALS( ptrDet1000->getName(), "PSD");
    TS_ASSERT_DELTA( ptrDet1000->getPos().X(), 3.86,0.01);
    TS_ASSERT_DELTA( ptrDet1000->getPos().Y(), 11.12,0.01);
    TS_ASSERT_DELTA( ptrDet1000->getPos().Z(), 0.43,0.01);
    TS_ASSERT_EQUALS( ptrDet1000->type(), "DetectorComponent");
    double d = ptrDet1000->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,4.0435,0.0001);
    double cmpDistance = ptrDet1000->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,4.0435,0.0001);

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
