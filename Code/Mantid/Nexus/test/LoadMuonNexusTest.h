#ifndef LOADMUONNEXUSTEST_H_
#define LOADMUONNEXUSTEST_H_



// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataHandling/LoadInstrument.h" 
//

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidNexus/LoadMuonNexus.h"
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
using namespace Mantid::NeXus;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadMuonNexusTest : public CxxTest::TestSuite
{
public:
  
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(nxLoad.initialize());
    TS_ASSERT( nxLoad.isInitialized() );
  }
  

  void testExec()
  {
    //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
    if ( !nxLoad.isInitialized() ) nxLoad.initialize();
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(nxLoad.execute(),std::runtime_error);

    // Now set required filename and output workspace name
    inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Nexus/emu00006473.nxs").toString();
    nxLoad.setPropertyValue("FileName", inputFile);

    outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = nxLoad.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile)); 
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 32 for file inputFile = "../../../../Test/Nexus/emu00006473.nxs";
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 32);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(11)[686], 9);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    /*  - other tests from LoadRawTest - These test data not in current Nexus files
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
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);
    */
    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    //boost::shared_ptr<Sample> sample = output->getSample();
    Property *l_property = output->run().getLogData( std::string("beamlog_current") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,27), "2006-Nov-21 07:03:08  182.8" );
    //check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), "Cr2.7Co0.3Si");
    
	/*
    //----------------------------------------------------------------------
    // Tests to check that Loading SpectraDetectorMap is done correctly
    //----------------------------------------------------------------------
    map= output->getSpectraMap();
    
    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(map->nElements(),24964);
    
    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(map->ndet(6),1);
    
    // Test one to many mapping, for example 10 pixels contribute to spectra 2084
    TS_ASSERT_EQUALS(map->ndet(2084),10);
    // Check the id number of all pixels contributing
    std::vector<Mantid::Geometry::IDetector*> detectorgroup;
    detectorgroup=map->getDetectors(2084);
    std::vector<Mantid::Geometry::IDetector*>::iterator it;
    int pixnum=101191;
    for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
    TS_ASSERT_EQUALS((*it)->getID(),pixnum++);
    
    // Test with spectra that does not exist
    // Test that number of pixel=0
    TS_ASSERT_EQUALS(map->ndet(5),0);
    // Test that trying to get the Detector throws.
    boost::shared_ptr<Mantid::Geometry::IDetector> test;
    TS_ASSERT_THROWS(test=map->getDetector(5),std::runtime_error);
    */
   
#endif /*_WIN64*/  
  }
//  void testWithManagedWorkspace()
//  {
//    ConfigService::Instance().updateConfig("UseManagedWS.properties");
//    //LoadRaw loader4;
//    //loader4.initialize();
//    //loader4.setPropertyValue("Filename", inputFile);    
//    //loader4.setPropertyValue("OutputWorkspace", "managedws");    
//   // TS_ASSERT_THROWS_NOTHING( loader4.execute() )
//    //TS_ASSERT( loader4.isExecuted() )
//
//    // Get back workspace and check it really is a ManagedWorkspace2D
//    MatrixWorkspace_sptr output;
//    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("managedws") );    
//    TS_ASSERT( dynamic_cast<ManagedWorkspace2D*>(output.get()) )
//  }

  void testExec2()
  {
  //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
    //test for multi period
    // Now set required filename and output workspace name
    inputFile2 = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Nexus/emu00006475.nxs").toString();
    nxLoad.setPropertyValue("FileName", inputFile2);

    outputSpace="outer2";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace); 
	nxLoad.setPropertyValue("EntryNumber", "1");  
	int entryNumber=nxLoad.getProperty("EntryNumber");
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = nxLoad.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile2)); 
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data - should be 4 separate workspaces for this 4 period file
    //
	if(entryNumber==0)
	{
		WorkspaceGroup_sptr outGrp;
    TS_ASSERT_THROWS_NOTHING(outGrp = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(outputSpace)));

	}
	//if entry number is given
	if(entryNumber==1)
	{
		MatrixWorkspace_sptr output;
		output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace));

		Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
		//Workspace2D_sptr output2D2 = boost::dynamic_pointer_cast<Workspace2D>(output2);
		// Should be 32 for file inputFile = "../../../../Test/Nexus/emu00006475.nxs";
		TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 32);
		// Check two X vectors are the same
		TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
		// Check two Y arrays have the same number of elements
		TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
		// Check that the time is as expected from bin boundary update
		TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

		// Check the unit has been set correctly
		TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

			//check that sample name has been set correctly
			//boost::shared_ptr<Sample> sample,sample2;
		//sample = output->getSample();
		//sample2 = output2->getSample();
		//TS_ASSERT_EQUALS(sample->getName(), sample2->getName());
		TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");

	}
    MatrixWorkspace_sptr output,output2,output3,output4;
	WorkspaceGroup_sptr outGrp;
	//if no entry number load the group workspace
	if(entryNumber==0)
	{
		TS_ASSERT_THROWS_NOTHING(outGrp = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(outputSpace)));

		(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_1")));
		(output2 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_2")));
		(output3 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_3")));
		(output4 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_4")));
	
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    Workspace2D_sptr output2D2 = boost::dynamic_pointer_cast<Workspace2D>(output2);
    // Should be 32 for file inputFile = "../../../../Test/Nexus/emu00006475.nxs";
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 32);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D2->dataY(8)[502], 121);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D2->dataE(8)[502], 11);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    //check that sample name has been set correctly
   // boost::shared_ptr<Sample> sample,sample2;
    //sample = output->getSample();
   // sample2 = output2->getSample();
    TS_ASSERT_EQUALS(output->sample().getName(), output2->sample().getName());
    TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");
	
	}  
#endif /*_WIN64*/  
  }
   void testExec2withZeroEntryNumber()
  {
  //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
    //test for multi period
    // Now set required filename and output workspace name
    inputFile2 = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Nexus/emu00006475.nxs").toString();
    nxLoad.setPropertyValue("FileName", inputFile2);

    outputSpace="outer2";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace); 
	nxLoad.setPropertyValue("EntryNumber", "0");  
	int entryNumber=nxLoad.getProperty("EntryNumber");
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = nxLoad.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile2)); 
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data - should be 4 separate workspaces for this 4 period file
    //
	WorkspaceGroup_sptr outGrp;
    TS_ASSERT_THROWS_NOTHING(outGrp = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(outputSpace)));
	
    MatrixWorkspace_sptr output,output2,output3,output4;
	//WorkspaceGroup_sptr outGrp;
	//if no entry number load the group workspace
	if(entryNumber==0)
	{
		//TS_ASSERT_THROWS_NOTHING(outGrp = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(outputSpace)));

		(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_1")));
		(output2 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_2")));
		(output3 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_3")));
		(output4 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_4")));
	
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    Workspace2D_sptr output2D2 = boost::dynamic_pointer_cast<Workspace2D>(output2);
    // Should be 32 for file inputFile = "../../../../Test/Nexus/emu00006475.nxs";
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 32);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D2->dataY(8)[502], 121);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D2->dataE(8)[502], 11);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    //check that sample name has been set correctly
    //boost::shared_ptr<Sample> sample,sample2;
  //  sample = output->getSample();
  //  sample2 = output2->getSample();
    TS_ASSERT_EQUALS(output->sample().getName(), output2->sample().getName());
    TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");
	
	}  
#endif /*_WIN64*/  
  }

  void testarrayin()
  {
  //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
    if ( !nxload3.isInitialized() ) nxload3.initialize();
    
    nxload3.setPropertyValue("Filename", inputFile);    
    nxload3.setPropertyValue("OutputWorkspace", "outWS");    
    nxload3.setPropertyValue("SpectrumList", "29,30,31");
    nxload3.setPropertyValue("SpectrumMin", "5");
    nxload3.setPropertyValue("SpectrumMax", "10");
    
    TS_ASSERT_THROWS_NOTHING(nxload3.execute());    
    TS_ASSERT( nxload3.isExecuted() );    
    
    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    (output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS")));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    
    // Should be 6 for selected input
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 9);
    
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(1)) == (output2D->dataX(5)) );
    
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(2).size(), output2D->dataY(7).size() );

    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(8)[479], 144);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(8)[479], 12);
    // Check that the error on that value is correct
    TS_ASSERT_DELTA( output2D->dataX(8)[479], 7.410, 0.0001);
    
#endif /*_WIN64*/  
  }
  
private:
  LoadMuonNexus nxLoad,nxload2,nxload3;
  std::string outputSpace;
  std::string entryName;
  std::string inputFile;
  std::string inputFile2;
  boost::shared_ptr<SpectraDetectorMap> map;
};
#endif /*LOADMUONNEXUSTEST_H_*/
