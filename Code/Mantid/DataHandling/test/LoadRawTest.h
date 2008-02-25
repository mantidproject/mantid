#ifndef LOADRAWTEST_H_
#define LOADRAWTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRaw.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadRawTest : public CxxTest::TestSuite
{
public:
  
  LoadRawTest()
  {
    //initialise framework manager to allow logging
	Mantid::API::FrameworkManager::Instance().initialize();
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( loader.initialize());    
    TS_ASSERT( loader.isInitialized() );
  }
  
  void testExec()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    // Should fail because mandatory parameter has not been set    
    //TS_ASSERT_THROWS(loader.execute(),std::runtime_error);    
    
    // Now set it...  
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HET15869.RAW";
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "outer";
    loader.setPropertyValue("OutputWorkspace", outputSpace);    
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));
    
    TS_ASSERT_THROWS_NOTHING(loader.execute());    
    TS_ASSERT( loader.isExecuted() );    
    
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS( output2D->getHistogramNumber(), 2584);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(99)) == (output2D->dataX(1734)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(673).size(), output2D->dataY(2111).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(999)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(999)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataX(999)[777], 554.1875);
    
    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
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
    
    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    Sample& sample = output->getSample();
    Property *l_property = sample.getLogData( std::string("../../../../Test/Data/HET15869_TEMP1.txt") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );
    
  }
  
  
private:
  LoadRaw loader;
  std::string inputFile;
  std::string outputSpace;
  
};
  
#endif /*LOADRAWTEST_H_*/
