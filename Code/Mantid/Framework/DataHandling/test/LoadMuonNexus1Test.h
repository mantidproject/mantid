#ifndef LOADMUONNEXUS1TEST_H_
#define LOADMUONNEXUS1TEST_H_



// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h" 

#include "MantidAPI/ScopedWorkspace.h"

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadMuonNexus1Test : public CxxTest::TestSuite
{
public:
  
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(nxLoad.initialize());
    TS_ASSERT( nxLoad.isInitialized() );
  }
  

  void testExec()
  {
    if ( !nxLoad.isInitialized() ) nxLoad.initialize();
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(nxLoad.execute(),std::runtime_error);

    // Now set required filename and output workspace name
    inputFile = "emu00006473.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     
    
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    

    // Test additional output parameters
    std::string field = nxLoad.getProperty("MainFieldDirection");
    TS_ASSERT( field == "Longitudinal" );

    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
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
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "Label" );
    TS_ASSERT( ! output-> isDistribution() );

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check Child Algorithm is running properly
    //----------------------------------------------------------------------
    Property *l_property = output->run().getLogData( std::string("beamlog_current") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,27), "2006-Nov-21 07:03:08  182.8" );
    //check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), "Cr2.7Co0.3Si");
    
  }

  void testTransvereDataset()
  {
    LoadMuonNexus1 nxL;
    if ( !nxL.isInitialized() ) nxL.initialize();

    // Now set required filename and output workspace name
    std::string inputFile_musr00022725 = "MUSR00022725.nxs";
    nxL.setPropertyValue("FileName", inputFile_musr00022725);

    outputSpace="outermusr00022725";
    nxL.setPropertyValue("OutputWorkspace", outputSpace);     

    TS_ASSERT_THROWS_NOTHING(nxL.execute());    
    TS_ASSERT( nxL.isExecuted() );    

    // Test additional output parameters
    std::string field = nxL.getProperty("MainFieldDirection");
    TS_ASSERT( field == "Transverse" );
    double timeZero = nxL.getProperty("TimeZero");
    TS_ASSERT_DELTA( timeZero, 0.55,0.001);
    double firstgood = nxL.getProperty("FirstGoodData");
    TS_ASSERT_DELTA( firstgood, 0.656,0.001);
  }

  void testExec2()
  {
    //test for multi period
    // Now set required filename and output workspace name
    inputFile2 = "emu00006475.nxs";
    nxLoad.setPropertyValue("FileName", inputFile2);

    outputSpace="outer2";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace); 
    nxLoad.setPropertyValue("EntryNumber", "1");  
    int64_t entryNumber=nxLoad.getProperty("EntryNumber");
    
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
    TS_ASSERT_THROWS_NOTHING(outGrp = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace));

	}
	//if entry number is given
	if(entryNumber==1)
	{
		MatrixWorkspace_sptr output;
		output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);

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
		TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "Label" );
    TS_ASSERT( ! output-> isDistribution() );

    //check that sample name has been set correctly
		TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");

	}
    MatrixWorkspace_sptr output,output2,output3,output4;
	WorkspaceGroup_sptr outGrp;
	//if no entry number load the group workspace
	if(entryNumber==0)
	{
		TS_ASSERT_THROWS_NOTHING(outGrp = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace));

		(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_1"));
		(output2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_2"));
		(output3 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_3"));
		(output4 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_4"));
	
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
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "Label" );
    TS_ASSERT( ! output-> isDistribution() );

    //check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), output2->sample().getName());
    TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");
	}   
  }
   void testExec2withZeroEntryNumber()
  {
    //test for multi period
    // Now set required filename and output workspace name
    inputFile2 = "emu00006475.nxs";
    nxLoad.setPropertyValue("FileName", inputFile2);

    outputSpace="outer2";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace); 
	nxLoad.setPropertyValue("EntryNumber", "0");  
	int64_t entryNumber=nxLoad.getProperty("EntryNumber");
    
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data - should be 4 separate workspaces for this 4 period file
    //
	WorkspaceGroup_sptr outGrp;
    TS_ASSERT_THROWS_NOTHING(outGrp = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace));
	
    MatrixWorkspace_sptr output,output2,output3,output4;
	//WorkspaceGroup_sptr outGrp;
	//if no entry number load the group workspace
	if(entryNumber==0)
	{
		(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_1"));
		(output2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_2"));
		(output3 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_3"));
		(output4 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_4"));
	
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
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "Label" );
    TS_ASSERT( ! output-> isDistribution() );

    //check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), output2->sample().getName());
    TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");
	
	}  
  }

  void testarrayin()
  {
    if ( !nxload3.isInitialized() ) nxload3.initialize();
    
    nxload3.setPropertyValue("Filename", inputFile);    
    nxload3.setPropertyValue("OutputWorkspace", "outWS");    
    nxload3.setPropertyValue("SpectrumList", "29,30,32");
    nxload3.setPropertyValue("SpectrumMin", "5");
    nxload3.setPropertyValue("SpectrumMax", "10");
    
    TS_ASSERT_THROWS_NOTHING(nxload3.execute());    
    TS_ASSERT( nxload3.isExecuted() );    
    
    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));
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

  }

    void testPartialSpectraLoading ()
  {
    LoadMuonNexus1 alg1;
    LoadMuonNexus1 alg2;

    // Execute alg1
    // It will only load some spectra
    TS_ASSERT_THROWS_NOTHING( alg1.initialize() );
    TS_ASSERT( alg1.isInitialized() );
    alg1.setPropertyValue("Filename", inputFile);    
    alg1.setPropertyValue("OutputWorkspace", "outWS1");    
    alg1.setPropertyValue("SpectrumList", "29,31");
    alg1.setPropertyValue("SpectrumMin", "5");
    alg1.setPropertyValue("SpectrumMax", "10");
    TS_ASSERT_THROWS_NOTHING(alg1.execute());    
    TS_ASSERT( alg1.isExecuted() );    
    // Get back the saved workspace
    MatrixWorkspace_sptr output1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS1");
    Workspace2D_sptr out1 = boost::dynamic_pointer_cast<Workspace2D>(output1);

    // Execute alg2
    // Load all the spectra
    TS_ASSERT_THROWS_NOTHING( alg2.initialize() );
    TS_ASSERT( alg2.isInitialized() );
    alg2.setPropertyValue("Filename", inputFile);    
    alg2.setPropertyValue("OutputWorkspace", "outWS2");    
    TS_ASSERT_THROWS_NOTHING(alg2.execute());    
    TS_ASSERT( alg2.isExecuted() );    
    // Get back the saved workspace
    MatrixWorkspace_sptr output2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS2");
    Workspace2D_sptr out2 = boost::dynamic_pointer_cast<Workspace2D>(output2);

    // Check common spectra
    // X values should match
    TS_ASSERT_EQUALS( out1->readX(0), out2->readX(0) );
    TS_ASSERT_EQUALS( out1->readX(4), out2->readX(5) );
    // Check some Y values
    TS_ASSERT_EQUALS( out1->readY(0), out2->readY(4) );
    TS_ASSERT_EQUALS( out1->readY(3), out2->readY(7) );
    TS_ASSERT_EQUALS( out1->readY(5), out2->readY(9) );
    TS_ASSERT_EQUALS( out1->readY(6), out2->readY(28) );
    TS_ASSERT_EQUALS( out1->readY(7), out2->readY(30) );
    // Check some E values
    TS_ASSERT_EQUALS( out1->readE(0), out2->readE(4) );
    TS_ASSERT_EQUALS( out1->readE(3), out2->readE(7) );
    TS_ASSERT_EQUALS( out1->readE(5), out2->readE(9) );
    TS_ASSERT_EQUALS( out1->readE(6), out2->readE(28) );
    TS_ASSERT_EQUALS( out1->readE(7), out2->readE(30) );

    AnalysisDataService::Instance().remove("outWS1");
    AnalysisDataService::Instance().remove("outWS2");

  }

  void test_loadingDeadTimes_singlePeriod()
  {
    const std::string outWSName = "LoadMuonNexus1Test_OutputWS";
    const std::string deadTimesWSName = "LoadMuonNexus1Test_DeadTimes";

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "emu00006473.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DeadTimeTable", deadTimesWSName) ); 

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    TableWorkspace_sptr deadTimesTable;
    
    TS_ASSERT_THROWS_NOTHING( deadTimesTable = 
      AnalysisDataService::Instance().retrieveWS<TableWorkspace>( deadTimesWSName ) );

    TS_ASSERT( deadTimesTable );

    if ( deadTimesTable )
    {
      TS_ASSERT_EQUALS( deadTimesTable->columnCount(), 2 );
      TS_ASSERT_EQUALS( deadTimesTable->rowCount(), 32 );

      TS_ASSERT_EQUALS( deadTimesTable->Int(0,0), 1 );
      TS_ASSERT_EQUALS( deadTimesTable->Int(15,0), 16 );
      TS_ASSERT_EQUALS( deadTimesTable->Int(31,0), 32 );

      TS_ASSERT_DELTA( deadTimesTable->Double(0,1), 0.00172168, 0.00000001 );
      TS_ASSERT_DELTA( deadTimesTable->Double(15,1),-0.00163397, 0.00000001 );
      TS_ASSERT_DELTA( deadTimesTable->Double(31,1), -0.03767336, 0.00000001 );
    }

    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(deadTimesWSName);
  }

  void test_loadingDeadTimes_multiPeriod()
  {

    const std::string outWSName = "LoadMuonNexus1Test_OutputWS";
    const std::string deadTimesWSName = "LoadMuonNexus1Test_DeadTimes";

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "MUSR00015189.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DeadTimeTable", deadTimesWSName) ); 

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    WorkspaceGroup_sptr deadTimesGroup;
    
    TS_ASSERT_THROWS_NOTHING( deadTimesGroup = 
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>( deadTimesWSName ) );

    TS_ASSERT( deadTimesGroup );

    if ( deadTimesGroup )
    {
      TS_ASSERT_EQUALS( deadTimesGroup->size(), 2 );

      TableWorkspace_sptr table1 = boost::dynamic_pointer_cast<TableWorkspace>( deadTimesGroup->getItem(0) );
      TS_ASSERT( table1 );

      if ( table1 )
      {
        TS_ASSERT_EQUALS( table1->columnCount(), 2 );
        TS_ASSERT_EQUALS( table1->rowCount(), 64 );

        TS_ASSERT_EQUALS( table1->Int(0,0), 1 );
        TS_ASSERT_EQUALS( table1->Int(31,0), 32 );
        TS_ASSERT_EQUALS( table1->Int(63,0), 64 );

        TS_ASSERT_DELTA( table1->Double(0,1), 0.01285629, 0.00000001 );
        TS_ASSERT_DELTA( table1->Double(31,1), 0.01893649, 0.00000001 );
        TS_ASSERT_DELTA( table1->Double(63,1), 0.01245339, 0.00000001 );
      }

      TableWorkspace_sptr table2 = boost::dynamic_pointer_cast<TableWorkspace>( deadTimesGroup->getItem(1) );
      TS_ASSERT( table2 );

      if ( table2 )
      {
        TS_ASSERT_EQUALS( table2->columnCount(), 2 );
        TS_ASSERT_EQUALS( table2->rowCount(), 64 );

        TS_ASSERT_EQUALS( table2->Int(0,0), 1 );
        TS_ASSERT_EQUALS( table2->Int(31,0), 32 );
        TS_ASSERT_EQUALS( table2->Int(63,0), 64 );

        TS_ASSERT_DELTA( table2->Double(0,1), 0.01285629, 0.00000001 );
        TS_ASSERT_DELTA( table2->Double(31,1),0.01893649, 0.00000001 );
        TS_ASSERT_DELTA( table2->Double(63,1), 0.01245339, 0.00000001 );
      }
    }

    AnalysisDataService::Instance().deepRemoveGroup(outWSName);
    AnalysisDataService::Instance().deepRemoveGroup(deadTimesWSName);
  }

  void test_loadingDetectorGrouping_singlePeriod()
  {
    const std::string outWSName = "LoadMuonNexus1Test_OutputWS";
    const std::string detectorGroupingWSName = "LoadMuonNexus1Test_DetectorGrouping";

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "emu00006473.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DetectorGroupingTable", detectorGroupingWSName) ); 

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    TableWorkspace_sptr detectorGrouping;
    
    TS_ASSERT_THROWS_NOTHING( detectorGrouping = 
        AnalysisDataService::Instance().retrieveWS<TableWorkspace>( detectorGroupingWSName ) );

    TS_ASSERT( detectorGrouping );

    if ( detectorGrouping )
    {
      TS_ASSERT_EQUALS( detectorGrouping->columnCount(), 1 );
      TS_ASSERT_EQUALS( detectorGrouping->rowCount(), 2 );

      TS_ASSERT_EQUALS( detectorGrouping->getColumn(0)->type(), "vector_int" );
      TS_ASSERT_EQUALS( detectorGrouping->getColumn(0)->name(), "Detectors" );

      std::vector<int> e1, e2;
      TS_ASSERT_THROWS_NOTHING( e1 = detectorGrouping->cell< std::vector<int> >(0,0) );
      TS_ASSERT_THROWS_NOTHING( e2 = detectorGrouping->cell< std::vector<int> >(1,0) );

      TS_ASSERT_EQUALS( e1.size(), 16);
      TS_ASSERT_EQUALS( e2.size(), 16);

      TS_ASSERT_EQUALS( e1[0], 1 );
      TS_ASSERT_EQUALS( e1[15], 16);

      TS_ASSERT_EQUALS( e2[0], 17 );
      TS_ASSERT_EQUALS( e2[15], 32 );
    }

    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(detectorGroupingWSName);
  }
  
  void test_loadingDetectorGrouping_multiPeriod()
  {
    const std::string outWSName = "LoadMuonNexus1Test_OutputWS";
    const std::string detectorGroupingWSName = "LoadMuonNexus1Test_DetectorGrouping";

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "MUSR00015189.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DetectorGroupingTable", detectorGroupingWSName) ); 

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    WorkspaceGroup_sptr detectorGrouping;
    
    TS_ASSERT_THROWS_NOTHING( detectorGrouping = 
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>( detectorGroupingWSName ) );

    TS_ASSERT( detectorGrouping );

    if ( detectorGrouping )
    {
      TS_ASSERT_EQUALS( detectorGrouping->size(), 2 );

      TableWorkspace_sptr table1 = boost::dynamic_pointer_cast<TableWorkspace>( detectorGrouping->getItem(0) );
      TS_ASSERT( table1 );

      if ( table1 )
      {
        TS_ASSERT_EQUALS( table1->columnCount(), 1 );
        TS_ASSERT_EQUALS( table1->rowCount(), 2 );

        std::vector<int> e1, e2;
        TS_ASSERT_THROWS_NOTHING( e1 = table1->cell< std::vector<int> >(0,0) );
        TS_ASSERT_THROWS_NOTHING( e2 = table1->cell< std::vector<int> >(1,0) );

        TS_ASSERT_EQUALS( e1.size(), 32);
        TS_ASSERT_EQUALS( e2.size(), 32);

        TS_ASSERT_EQUALS( e1[0], 33 );
        TS_ASSERT_EQUALS( e1[31], 64 );

        TS_ASSERT_EQUALS( e2[0], 1 );
        TS_ASSERT_EQUALS( e2[31], 32 );
      }

      TableWorkspace_sptr table2 = boost::dynamic_pointer_cast<TableWorkspace>( detectorGrouping->getItem(1) );
      TS_ASSERT( table2 );

      if ( table2 )
      {
        TS_ASSERT_EQUALS( table2->columnCount(), 1 );
        TS_ASSERT_EQUALS( table2->rowCount(), 2 );

        std::vector<int> e1, e2;
        TS_ASSERT_THROWS_NOTHING( e1 = table2->cell< std::vector<int> >(0,0) );
        TS_ASSERT_THROWS_NOTHING( e2 = table2->cell< std::vector<int> >(1,0) );

        TS_ASSERT_EQUALS( e1.size(), 32);
        TS_ASSERT_EQUALS( e2.size(), 32);

        TS_ASSERT_EQUALS( e1[0], 33 );
        TS_ASSERT_EQUALS( e1[31], 64 );

        TS_ASSERT_EQUALS( e2[0], 1 );
        TS_ASSERT_EQUALS( e2[31], 32);

      }
    }

    AnalysisDataService::Instance().deepRemoveGroup(outWSName);
    AnalysisDataService::Instance().deepRemoveGroup(detectorGroupingWSName);
  }

  void test_autoGroup_singlePeriod()
  {
    ScopedWorkspace outWsEntry;

    try
    {
      LoadMuonNexus1 alg;
      alg.initialize();
      alg.setRethrows(true);
      alg.setPropertyValue("Filename", "emu00006473.nxs");
      alg.setProperty("AutoGroup", true);
      alg.setPropertyValue("OutputWorkspace", outWsEntry.name());
      alg.execute();
    }
    catch(std::exception& e)
    {
      TS_FAIL(e.what());
      return;
    }

    auto outWs = boost::dynamic_pointer_cast<MatrixWorkspace>( outWsEntry.retrieve() );
    TS_ASSERT(outWs);

    if ( ! outWs )
      return; // Nothing to check

    TS_ASSERT_EQUALS( outWs->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS( outWs->blocksize(), 2000);

    TS_ASSERT_EQUALS( outWs->readY(0)[0], 461 );
    TS_ASSERT_EQUALS( outWs->readY(0)[1000], 192 );
    TS_ASSERT_EQUALS( outWs->readY(0)[1998], 1 );

    TS_ASSERT_EQUALS( outWs->readY(1)[0], 252 );
    TS_ASSERT_EQUALS( outWs->readY(1)[1000], 87 );
    TS_ASSERT_EQUALS( outWs->readY(1)[1998], 2 );
  }

  void test_autoGroup_multiPeriod()
  {
    ScopedWorkspace outWsEntry;

    try
    {
      LoadMuonNexus1 alg;
      alg.initialize();
      alg.setRethrows(true);
      alg.setPropertyValue("Filename", "MUSR00015189.nxs");
      alg.setProperty("AutoGroup", true);
      alg.setPropertyValue("OutputWorkspace", outWsEntry.name());
      alg.execute();
    }
    catch(std::exception& e)
    {
      TS_FAIL(e.what());
      return;
    }

    auto outWs = boost::dynamic_pointer_cast<WorkspaceGroup>( outWsEntry.retrieve() );
    TS_ASSERT(outWs);

    if ( ! outWs )
      return; // Nothing to check

    TS_ASSERT_EQUALS( outWs->size(), 2 );

    auto outWs1 = boost::dynamic_pointer_cast<MatrixWorkspace>( outWs->getItem(0) );
    TS_ASSERT(outWs1);

    if (outWs1)
    {
      TS_ASSERT_EQUALS( outWs1->getNumberHistograms(), 2);
      TS_ASSERT_EQUALS( outWs1->blocksize(), 2000);

      TS_ASSERT_EQUALS( outWs1->readY(0)[0], 82 );
      TS_ASSERT_EQUALS( outWs1->readY(0)[458], 115 );
      TS_ASSERT_EQUALS( outWs1->readY(0)[1997], 1 );

      TS_ASSERT_EQUALS( outWs1->readY(1)[0], 6 );
      TS_ASSERT_EQUALS( outWs1->readY(1)[458], 91 );
      TS_ASSERT_EQUALS( outWs1->readY(1)[1997], 0 );
    }

    auto outWs2 = boost::dynamic_pointer_cast<MatrixWorkspace>( outWs->getItem(1) );
    TS_ASSERT(outWs2);

    if (outWs2) {
      TS_ASSERT_EQUALS( outWs2->getNumberHistograms(), 2);
      TS_ASSERT_EQUALS( outWs2->blocksize(), 2000);

      TS_ASSERT_EQUALS( outWs2->readY(0)[0], 16 );
      TS_ASSERT_EQUALS( outWs2->readY(0)[458], 132 );
      TS_ASSERT_EQUALS( outWs2->readY(0)[1930], 0 );

      TS_ASSERT_EQUALS( outWs2->readY(1)[0], 17 );
      TS_ASSERT_EQUALS( outWs2->readY(1)[458], 81 );
      TS_ASSERT_EQUALS( outWs2->readY(1)[1930], 1 );
    }
  }

  void test_loadRunInformation()
  {
    ScopedWorkspace outWsEntry;

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "emu00006473.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWsEntry.name()) );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    Workspace_sptr outWs = outWsEntry.retrieve();
    TS_ASSERT(outWs);

    if (!outWs)
      return;

    auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(outWs);
    TS_ASSERT(ws);

    if (!ws)
      return;

    const Run& run = ws->run();

    // Check expected properties
    checkProperty(run, "run_number", std::string("6473"));
    checkProperty(run, "run_title", std::string("Cr2.7Co0.3Si T=200.0 F=5.0"));
    checkProperty(run, "run_start", std::string("2006-11-21T07:04:30"));
    checkProperty(run, "run_end", std::string("2006-11-21T09:29:28"));
    checkProperty(run, "dur_secs", std::string("8697"));
    checkProperty(run, "nspectra", 32);
    checkProperty(run, "goodfrm", 417485);

    checkProperty(run, "sample_temp", 200.0);
    checkProperty(run, "sample_magn_field", 5.0);
  }
  

private:
  LoadMuonNexus1 nxLoad,nxload2,nxload3;
  std::string outputSpace;
  std::string entryName;
  std::string inputFile;
  std::string inputFile2;

  template<typename T>
  void checkProperty(const Run& run, const std::string& property, const T& expectedValue)
  {
    if ( run.hasProperty(property) )
    {
      T propertyValue;

      try
      {
        propertyValue = run.getPropertyValueAsType<T>(property);
      }
      catch(...)
      {
        TS_FAIL("Unexpected property type: " + property);
        return;
      }

      TSM_ASSERT_EQUALS("Property value mismatch: " + property, propertyValue, expectedValue);
    }
    else
    {
      TS_FAIL("No property: " + property);
    }
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMuonNexus1TestPerformance : public CxxTest::TestSuite
{
public:
  void testDefaultLoad()
  {
    LoadMuonNexus1 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006475.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT( loader.execute() );
  }
};

#endif /*LOADMUONNEXUS1TEST_H_*/
