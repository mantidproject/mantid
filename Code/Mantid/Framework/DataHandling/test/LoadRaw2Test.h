#ifndef LOADRAW2TEST_H_
#define LOADRAW2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRaw2.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "Poco/Path.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadRaw2Test : public CxxTest::TestSuite
{
public:

  LoadRaw2Test()
  {
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "HET15869.raw";
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
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "outer";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 2584);
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
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output2D-> isDistribution() )

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA( output2D->run().getProtonCharge(), 171.0353, 0.0001 )

    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    boost::shared_ptr<IInstrument> i = output2D->getInstrument();
    boost::shared_ptr<Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<Mantid::Geometry::IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<Mantid::Geometry::Detector> ptrDet103 = boost::dynamic_pointer_cast<Mantid::Geometry::Detector>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
  //  boost::shared_ptr<Sample> sample = output2D->sample();
    Property *l_property = output2D->run().getLogData( std::string("TEMP1") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );

    //----------------------------------------------------------------------
    // Tests to check that Loading SpectraDetectorMap is done correctly
    //----------------------------------------------------------------------
    const SpectraDetectorMap& map= output2D->spectraMap();

    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(map.nElements(),24964);

    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(map.ndet(6),1);

    // Test one to many mapping, for example 10 pixels contribute to spectra 2084
    TS_ASSERT_EQUALS(map.ndet(2084),10);
    // Check the id number of all pixels contributing
    std::vector<int> detectorgroup;
    detectorgroup=map.getDetectors(2084);
    std::vector<int>::const_iterator it;
    int pixnum=101191;
    for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
    TS_ASSERT_EQUALS(*it,pixnum++);

    // Test with spectra that does not exist
    // Test that number of pixel=0
    TS_ASSERT_EQUALS(map.ndet(5),0);
    // Test that trying to get the Detector throws.
    std::vector<int> test = map.getDetectors(5);
    TS_ASSERT(test.empty());
    
    AnalysisDataService::Instance().remove(outputSpace);    
  }

  void testarrayin()
  {
    if ( !loader2.isInitialized() ) loader2.initialize();

    loader2.setPropertyValue("Filename", inputFile);
    loader2.setPropertyValue("OutputWorkspace", "outWS");
    loader2.setPropertyValue("SpectrumList", "998,999,1000");
    loader2.setPropertyValue("SpectrumMin", "5");
    loader2.setPropertyValue("SpectrumMax", "10");

    TS_ASSERT_THROWS_NOTHING(loader2.execute());
    TS_ASSERT( loader2.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

    // Should be 6 for selected input
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 9);

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

    AnalysisDataService::Instance().remove("outWS");
  }

  void testfail()
  {
    if ( !loader3.isInitialized() ) loader3.initialize();

    loader3.setPropertyValue("Filename", inputFile);
    loader3.setPropertyValue("OutputWorkspace", "out2");
    loader3.setPropertyValue("SpectrumList", "0,999,1000");
    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "10");
    loader3.execute();
    Workspace_sptr output;
    // test that there is no workspace as it should have failed
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out2"),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "1");
     loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out2"),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "3");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out2"),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "5");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out2"),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "3000");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out2"),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "10");
    loader3.setPropertyValue("SpectrumList", "999,3000");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve("out2"),std::runtime_error);

    loader3.setPropertyValue("SpectrumList", "999,2000");
    loader3.execute();
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out2"));
    
    AnalysisDataService::Instance().remove("out2");
  }

  void testMultiPeriod()
  {
    LoadRaw2 loader5;
    loader5.initialize();
    loader5.setPropertyValue("Filename", "EVS13895.raw");
    loader5.setPropertyValue("OutputWorkspace", "multiperiod2");
    loader5.setPropertyValue("SpectrumList", "10,50,100,195");
    //loader5.setPropertyValue("SpectrumMin", "1");
    //loader5.setPropertyValue("SpectrumMax", "2");
    TS_ASSERT_THROWS_NOTHING( loader5.execute() )
    TS_ASSERT( loader5.isExecuted() )

    // Get back the workspaces
    MatrixWorkspace_sptr output1;
    TS_ASSERT_THROWS_NOTHING( output1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("multiperiod2")) );
    TS_ASSERT_EQUALS( output1->getNumberHistograms(), 4 )
    MatrixWorkspace_sptr output2;
    TS_ASSERT_THROWS_NOTHING( output2 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("multiperiod2_2")) );
    TS_ASSERT_EQUALS( output2->getNumberHistograms(), 4 )
    MatrixWorkspace_sptr output3;
    TS_ASSERT_THROWS_NOTHING( output3 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("multiperiod2_3")) );
    TS_ASSERT_EQUALS( output3->getNumberHistograms(), 4 )
    MatrixWorkspace_sptr output4;
    TS_ASSERT_THROWS_NOTHING( output4 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("multiperiod2_4")) );
    TS_ASSERT_EQUALS( output4->getNumberHistograms(), 4 )
    MatrixWorkspace_sptr output5;
    TS_ASSERT_THROWS_NOTHING( output5 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("multiperiod2_5")) );
    TS_ASSERT_EQUALS( output5->getNumberHistograms(), 4 )
    MatrixWorkspace_sptr output6;
    TS_ASSERT_THROWS_NOTHING( output6 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("multiperiod2_6")) );
    TS_ASSERT_EQUALS( output6->getNumberHistograms(), 4 )

    // The histogram bins should be the same
    TS_ASSERT_EQUALS( output1->dataX(0), output2->dataX(0) )
    TS_ASSERT_EQUALS( output1->dataX(0), output3->dataX(0) )
    TS_ASSERT_EQUALS( output1->dataX(0), output4->dataX(0) )
    TS_ASSERT_EQUALS( output1->dataX(1), output5->dataX(1) )
    TS_ASSERT_EQUALS( output1->dataX(1), output6->dataX(1) )
    // But the data should be different
    TS_ASSERT_DIFFERS( output1->dataY(1)[555], output2->dataY(1)[555] )
    TS_ASSERT_DIFFERS( output1->dataY(1)[555], output3->dataY(1)[555] )
    TS_ASSERT_DIFFERS( output1->dataY(1)[555], output4->dataY(1)[555] )
    TS_ASSERT_DIFFERS( output1->dataY(1)[555], output5->dataY(1)[555] )
    TS_ASSERT_DIFFERS( output1->dataY(1)[555], output6->dataY(1)[555] )

    // Check these are the same
    TS_ASSERT_EQUALS( output1->getBaseInstrument(), output2->getBaseInstrument() )
    TS_ASSERT_EQUALS( &(output1->spectraMap()), &(output2->spectraMap()) )
    TS_ASSERT_EQUALS( &(output1->sample()), &(output2->sample()) )
    TS_ASSERT_DIFFERS( &(output1->run()), &(output2->run()) )
    TS_ASSERT_EQUALS( output1->getBaseInstrument(), output6->getBaseInstrument() )
    TS_ASSERT_EQUALS( &(output1->spectraMap()), &(output6->spectraMap()) )
    TS_ASSERT_EQUALS( &(output1->sample()), &(output6->sample()) )
    TS_ASSERT_DIFFERS( &(output1->run()), &(output6->run()) )

    
    AnalysisDataService::Instance().remove("multiperiod2");
    AnalysisDataService::Instance().remove("multiperiod2_2");
    AnalysisDataService::Instance().remove("multiperiod2_3");
    AnalysisDataService::Instance().remove("multiperiod2_4");
    AnalysisDataService::Instance().remove("multiperiod2_5");
    AnalysisDataService::Instance().remove("multiperiod2_6");

  }

  void testWithManagedWorkspace()
  {
    ConfigService::Instance().updateConfig("UseManagedWS.properties");
    LoadRaw2 loader4;
    loader4.initialize();
    loader4.setPropertyValue("Filename", inputFile);
    loader4.setPropertyValue("OutputWorkspace", "managedws2");
    TS_ASSERT_THROWS_NOTHING( loader4.execute() )
    TS_ASSERT( loader4.isExecuted() )

    // Get back workspace and check it really is a ManagedWorkspace2D
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("managedws2") );
    TS_ASSERT( dynamic_cast<ManagedWorkspace2D*>(output.get()) )
    
    ConfigService::Instance().updateConfig("Mantid.properties");
    AnalysisDataService::Instance().remove("managedws2");
  }

private:
  LoadRaw2 loader,loader2,loader3;
  std::string inputFile;
  std::string outputSpace;
};

#endif /*LOADRAW2TEST_H_*/
