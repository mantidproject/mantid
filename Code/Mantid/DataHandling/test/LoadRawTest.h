#ifndef LOADRAWTEST_H_
#define LOADRAWTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRaw.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
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
    TS_ASSERT_THROWS(loader.execute(),std::runtime_error);    
    
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

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->XUnit()->unitID(), "TOF" )
    TS_ASSERT( ! output-> isDistribution() )

    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    boost::shared_ptr<Instrument> i = output->getInstrument();
    Mantid::Geometry::Component* source = i->getSource();

    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    Mantid::Geometry::Component* samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 10.0,0.01);

    Mantid::Geometry::Detector *ptrDet103 = dynamic_cast<Mantid::Geometry::Detector*>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), -0.229,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 4.0199,0.01);
    

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    Sample& sample = output->getSample();
    Property *l_property = sample.getLogData( std::string("../../../../Test/Data/HET15869_TEMP1.txt") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );
    
  }

  void testarrayin()
  {
    if ( !loader2.isInitialized() ) loader2.initialize();
    
    // Now set it...  
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HET15869.RAW";
    loader2.setPropertyValue("Filename", inputFile);    
    loader2.setPropertyValue("OutputWorkspace", "outWS");    
    loader2.setPropertyValue("spectrum_list", "998,999,1000");
    loader2.setPropertyValue("spectrum_min", "5");
    loader2.setPropertyValue("spectrum_max", "10");
    
    TS_ASSERT_THROWS_NOTHING(loader2.execute());    
    TS_ASSERT( loader2.isExecuted() );    
    
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    
    // Should be 6 for selected input
    TS_ASSERT_EQUALS( output2D->getHistogramNumber(), 9);
    
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(1)) == (output2D->dataX(5)) );
    
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(2).size(), output2D->dataY(7).size() );

    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(8)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(8)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataX(8)[777], 554.1875);
    
  }
  
  void testfail()
  {
    if ( !loader3.isInitialized() ) loader3.initialize();
    
    // Now set it...  
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HET15869.RAW";
    loader3.setPropertyValue("Filename", inputFile);    
    loader3.setPropertyValue("OutputWorkspace", "out");    
    loader3.setPropertyValue("spectrum_list", "0,999,1000");
    loader3.setPropertyValue("spectrum_min", "5");
    loader3.setPropertyValue("spectrum_max", "10");
    loader3.execute();
    Workspace_sptr output;
    // test that there is no workspace as it should have failed
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out"),std::runtime_error);    
 
    loader3.setPropertyValue("spectrum_min", "5");
    loader3.setPropertyValue("spectrum_max", "0");
     loader3.execute();   
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out"),std::runtime_error);    

    loader3.setPropertyValue("spectrum_min", "5");
    loader3.setPropertyValue("spectrum_max", "3");
     loader3.execute();   
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out"),std::runtime_error);    

    loader3.setPropertyValue("spectrum_min", "5");
    loader3.setPropertyValue("spectrum_max", "5");
     loader3.execute();   
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out"),std::runtime_error);    

    loader3.setPropertyValue("spectrum_min", "5");
    loader3.setPropertyValue("spectrum_max", "3000");
    loader3.execute();   
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out"),std::runtime_error);    

    loader3.setPropertyValue("spectrum_min", "5");
    loader3.setPropertyValue("spectrum_max", "10");
    loader3.setPropertyValue("spectrum_list", "999,3000");
    loader3.execute();   
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out"),std::runtime_error);    

    loader3.setPropertyValue("spectrum_list", "999,2000");
    loader3.execute();   
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out"));    
  }
   
  void testWithManagedWorkspace()
  {
    ConfigService::Instance().loadConfig("UseManagedWS.properties");
    LoadRaw loader4;
    loader4.initialize();
    loader4.setPropertyValue("Filename", "../../../../Test/Data/HET15869.RAW");    
    loader4.setPropertyValue("OutputWorkspace", "managedws");    
    TS_ASSERT_THROWS_NOTHING( loader4.execute() )
    TS_ASSERT( loader4.isExecuted() )

    // Get back workspace and check it really is a ManagedWorkspace2D
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("managedws") );    
    TS_ASSERT( dynamic_cast<ManagedWorkspace2D*>(output.get()) )
  }
   
private:
  LoadRaw loader,loader2,loader3;
  std::string inputFile;
  std::string outputSpace;
  
};
  
#endif /*LOADRAWTEST_H_*/
