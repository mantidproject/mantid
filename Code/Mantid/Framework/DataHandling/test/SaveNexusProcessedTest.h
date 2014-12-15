#ifndef SAVENEXUSPROCESSEDTEST_H_
#define SAVENEXUSPROCESSEDTEST_H_

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataHandling/LoadEventPreNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/SaveNexusProcessed.h"
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <Poco/Path.h>

#include <nexus/NeXusFile.hpp>
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <cxxtest/TestSuite.h>


using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::NeXus;

class SaveNexusProcessedTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveNexusProcessedTest *createSuite() { return new SaveNexusProcessedTest(); }
  static void destroySuite( SaveNexusProcessedTest *suite ) { delete suite; }

  SaveNexusProcessedTest()
  {
    // clearfiles - make true for SVN as dont want to leave on build server.
    // Unless the file "KEEP_NXS_FILES" exists, then clear up nxs files
    Poco::File file("KEEP_NXS_FILES");
    clearfiles = !file.exists();
  }

  void setUp()
  {

  }

  void tearDown()
  {
  }


  void testInit()
  {
    SaveNexusProcessed algToBeTested;
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }


  void testExec()
  {

    SaveNexusProcessed algToBeTested;
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(),std::runtime_error);


    // create dummy 2D-workspace
    Workspace2D_sptr localWorkspace2D = boost::dynamic_pointer_cast<Workspace2D>
    (WorkspaceFactory::Instance().create("Workspace2D",1,10,10));
    localWorkspace2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    double d = 0.0;
    for(int i = 0; i<10; ++i,d+=0.1)
    {
      localWorkspace2D->dataX(0)[i] = d;
      localWorkspace2D->dataY(0)[i] = d;
      localWorkspace2D->dataE(0)[i] = d;
    }

    AnalysisDataService::Instance().addOrReplace("testSpace", localWorkspace2D);

    // Now set it...
    // specify name of file to save workspace to
    algToBeTested.setPropertyValue("InputWorkspace", "testSpace");
    outputFile = "SaveNexusProcessedTest_testExec.nxs";
    //entryName = "test";
    dataName = "spectra";
    title = "A simple workspace saved in Processed Nexus format";
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("Filename", outputFile));
    outputFile = algToBeTested.getPropertyValue("Filename");
    //algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("Title", title);
    if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));
    //TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") )
    //TS_ASSERT( ! result.compare(entryName));

    // changed so that 1D workspaces are no longer written.
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

    if(clearfiles) Poco::File(outputFile).remove();

    AnalysisDataService::Instance().remove("testSpace");


  }



  void testExecOnLoadraw()
  {
    SaveNexusProcessed algToBeTested;
    std::string inputFile = "LOQ48127.raw";
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized() );
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "outer4";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    //
    // get workspace
    //
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    //
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();

    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "SaveNexusProcessedTest_testExecOnLoadraw.nxs";
    if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();
    //entryName = "entry4";
    dataName = "spectra";
    title = "A save of a workspace from Loadraw file";
    algToBeTested.setPropertyValue("Filename", outputFile);

    //algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("Title", title);
    algToBeTested.setPropertyValue("Append", "0");
    outputFile = algToBeTested.getPropertyValue("Filename");
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));
    //TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") );
    //TS_ASSERT( ! result.compare(entryName));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

    if(clearfiles) remove(outputFile.c_str());
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(outputSpace));

  }


  void testExecOnMuon()
  {
    SaveNexusProcessed algToBeTested;

    LoadNexus nxLoad;
    std::string outputSpace,inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "emu00006473.nxs";
    nxLoad.setPropertyValue("Filename", inputFile);
    outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT( nxLoad.isExecuted() );
    //
    // get workspace
    //
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // this would make all X's separate
    // output2D->dataX(22)[3]=0.55;
    //
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();

    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "SaveNexusProcessedTest_testExecOnMuon.nxs";
    if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();
    //entryName = "entry4";
    dataName = "spectra";
    title = "A save of a 2D workspace from Muon file";
    algToBeTested.setPropertyValue("Filename", outputFile);
    outputFile = algToBeTested.getPropertyValue("Filename");
    if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();

    //algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("Title", title);
    algToBeTested.setPropertyValue("Append", "0");

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));
    //TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") );
    //TS_ASSERT( ! result.compare(entryName));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

    // Nice idea, but confusing (seg-faulted) if algorithm doesn't clean its state
    // In reality out algorithms are only call once
    //    // try writing data again
    //    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    //    TS_ASSERT( algToBeTested.isExecuted() );
    if(clearfiles) Poco::File(outputFile).remove();
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(outputSpace));
  }

  /**
   *
   * @param filename_root :: base of the file to save
   * @param type :: event type to create
   * @param outputFile[out] :: returns the output file
   * @param makeDifferentTypes :: mix event types
   * @param clearfiles :: clear files after saving
   * @param PreserveEvents :: save as event list
   * @param CompressNexus :: compress
   * @return
   */
  static EventWorkspace_sptr do_testExec_EventWorkspaces(std::string filename_root, EventType type,
      std::string & outputFile,  bool makeDifferentTypes, bool clearfiles,
      bool PreserveEvents=true, bool CompressNexus=false)
  {
    std::vector< std::vector<int> > groups(5);
    groups[0].push_back(10);
    groups[0].push_back(11);
    groups[0].push_back(12);
    groups[1].push_back(20);
    groups[2].push_back(30);
    groups[2].push_back(31);
    groups[3].push_back(40);
    groups[4].push_back(50);

    EventWorkspace_sptr WS = WorkspaceCreationHelper::CreateGroupedEventWorkspace(groups, 100, 1.0);
    WS->getEventList(3).clear(false);
    // Switch the event type
    if (makeDifferentTypes)
    {
      WS->getEventList(0).switchTo(TOF);
      WS->getEventList(1).switchTo(WEIGHTED);
      WS->getEventList(2).switchTo(WEIGHTED_NOTIME);
      WS->getEventList(4).switchTo(WEIGHTED);
    }
    else
    {
      for (size_t wi=0; wi < WS->getNumberHistograms(); wi++)
        WS->getEventList(wi).switchTo(type);
    }

    SaveNexusProcessed alg;
    alg.initialize();

    // Now set it...
    alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<Workspace>(WS));

    // specify name of file to save workspace to
    std::ostringstream mess;
    mess << filename_root << static_cast<int>(type) << ".nxs";
    outputFile = mess.str();
    std::string dataName = "spectra";
    std::string title = "A simple workspace saved in Processed Nexus format";

    alg.setPropertyValue("Filename", outputFile);
    outputFile = alg.getPropertyValue("Filename");
    alg.setPropertyValue("Title", title);
    alg.setProperty("PreserveEvents", PreserveEvents);
    alg.setProperty("CompressNexus", CompressNexus);

    // Clear the existing file, if any
    if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    TS_ASSERT( Poco::File(outputFile).exists() );

    if(clearfiles) Poco::File(outputFile).remove();

    return WS;
  }


  void testExec_EventWorkspace_TofEvent()
  {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_", TOF, outputFile, false, clearfiles);
  }

  void testExec_EventWorkspace_WeightedEvent()
  {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_", WEIGHTED, outputFile, false, clearfiles);
  }

  void testExec_EventWorkspace_WeightedEventNoTime()
  {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_", WEIGHTED_NOTIME, outputFile, false, clearfiles);
  }

  void testExec_EventWorkspace_DifferentTypes()
  {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_DifferentTypes_", WEIGHTED_NOTIME, outputFile, true, clearfiles);
  }

  void testExec_EventWorkspace_DontPreserveEvents()
  {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_EventTo2D", TOF, outputFile, false, clearfiles, false /* DONT preserve events */);
  }
  void testExec_EventWorkspace_CompressNexus()
  {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_EventTo2D", TOF, outputFile, false, clearfiles, true /* DONT preserve events */, true /* Compress */);
  }

  void testExecSaveLabel()
  {
    SaveNexusProcessed alg;
    if ( !alg.isInitialized() ) alg.initialize();

    // create dummy 2D-workspace
    Workspace2D_sptr localWorkspace2D = boost::dynamic_pointer_cast<Workspace2D>
      (WorkspaceFactory::Instance().create("Workspace2D",1,10,10));

    //set units to be a label
    localWorkspace2D->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
    auto label = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(localWorkspace2D->getAxis(0)->unit());
    label->setLabel("Temperature","K");

    double d = 0.0;
    for(int i = 0; i<10; ++i,d+=0.1)
    {
      localWorkspace2D->dataX(0)[i] = d;
      localWorkspace2D->dataY(0)[i] = d;
      localWorkspace2D->dataE(0)[i] = d;
    }

    AnalysisDataService::Instance().addOrReplace("testSpace", localWorkspace2D);

    // Now set it...
    // specify name of file to save workspace to
    alg.setPropertyValue("InputWorkspace", "testSpace");
    outputFile = "SaveNexusProcessedTest_testExec.nxs";
    //entryName = "test";
    dataName = "spectra";
    title = "A simple workspace saved in Processed Nexus format";
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outputFile));
    outputFile = alg.getPropertyValue("Filename");
    //alg.setPropertyValue("EntryName", entryName);
    alg.setPropertyValue("Title", title);
    if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = alg.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));

    // changed so that 1D workspaces are no longer written.
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    if(clearfiles) Poco::File(outputFile).remove();

    AnalysisDataService::Instance().remove("testSpace");
  }

  void testSaveGroupWorkspace()
  {
    const std::string output_filename = "SaveNexusProcessedTest_GroupWorkspaceFile.nxs";

    // Clean out any previous instances.
    bool doesFileExist = Poco::File(output_filename).exists();
    if (doesFileExist)
    {
      Poco::File(output_filename).remove();
    }
    const int nEntries = 3;
    const int nHist = 1;
    const int nBins = 1;
    const std::string stem = "test_group_ws";
    Mantid::API::WorkspaceGroup_sptr group_ws = WorkspaceCreationHelper::CreateWorkspaceGroup(nEntries,
        nHist, nBins, stem);

    SaveNexusProcessed alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();

    alg.setProperty("Filename", output_filename);
    alg.setProperty("InputWorkspace", group_ws);
    alg.execute();

    doesFileExist = Poco::File(output_filename).exists();
    TSM_ASSERT("File should have been created", doesFileExist);
    if (doesFileExist)
    {
      Poco::File(output_filename).remove();
    }

  }

  void testSaveTableVectorColumn()
  {
    std::string outputFileName = "SaveNexusProcessedTest_testSaveTableVectorColumn.nxs";

    // Create a table which we will save
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    table->addColumn("vector_int", "IntVectorColumn");
    table->addColumn("vector_double", "DoubleVectorColumn");

    std::vector<double> d1, d2, d3;
    d1.push_back(0.5);
    d2.push_back(1.0); d2.push_back(2.5);
    d3.push_back(4.0);

    // Add some rows of different sizes
    TableRow row1 = table->appendRow(); row1 << Strings::parseRange("1")<< d1;
    TableRow row2 = table->appendRow(); row2 << Strings::parseRange("2,3") << d2;
    TableRow row3 = table->appendRow(); row3 << Strings::parseRange("4,5,6,7") << d3;

    ScopedWorkspace inputWsEntry(table);

    SaveNexusProcessed alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inputWsEntry.name());
    alg.setPropertyValue("Filename", outputFileName);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    if ( ! alg.isExecuted() )
      return; // Nothing to check

    // Get full output file path
    outputFileName = alg.getPropertyValue("Filename");

    try
    {
      NeXus::File savedNexus(outputFileName);

      savedNexus.openGroup("mantid_workspace_1", "NXentry");
      savedNexus.openGroup("table_workspace", "NXdata");

      // -- Checking int column -----

      savedNexus.openData("column_1");

      NeXus::Info columnInfo1 = savedNexus.getInfo();
      TS_ASSERT_EQUALS( columnInfo1.dims.size(), 2 );
      TS_ASSERT_EQUALS( columnInfo1.dims[0], 3 );
      TS_ASSERT_EQUALS( columnInfo1.dims[1], 4 );
      TS_ASSERT_EQUALS( columnInfo1.type, NX_INT32 );

      std::vector<int> data1;
      savedNexus.getData<int>(data1);

      TS_ASSERT_EQUALS( data1.size(), 12 );
      TS_ASSERT_EQUALS( data1[0], 1 );
      TS_ASSERT_EQUALS( data1[3], 0 );
      TS_ASSERT_EQUALS( data1[5], 3 );
      TS_ASSERT_EQUALS( data1[8], 4 );
      TS_ASSERT_EQUALS( data1[11], 7 );

      std::vector<NeXus::AttrInfo> attrInfos1 = savedNexus.getAttrInfos();
      TS_ASSERT_EQUALS( attrInfos1.size(), 6 );

      if ( attrInfos1.size() == 6 )
      {
        TS_ASSERT_EQUALS( attrInfos1[0].name, "row_size_0");
        TS_ASSERT_EQUALS( savedNexus.getAttr<int>(attrInfos1[0]), 1 );

        TS_ASSERT_EQUALS( attrInfos1[2].name, "row_size_2");
        TS_ASSERT_EQUALS( savedNexus.getAttr<int>(attrInfos1[2]), 4 );

        TS_ASSERT_EQUALS( attrInfos1[4].name, "interpret_as");
        TS_ASSERT_EQUALS( savedNexus.getStrAttr(attrInfos1[4]), "" );

        TS_ASSERT_EQUALS( attrInfos1[5].name, "name");
        TS_ASSERT_EQUALS( savedNexus.getStrAttr(attrInfos1[5]), "IntVectorColumn" );
      }

      // -- Checking double column -----

      savedNexus.openData("column_2");

      NeXus::Info columnInfo2 = savedNexus.getInfo();
      TS_ASSERT_EQUALS( columnInfo2.dims.size(), 2 );
      TS_ASSERT_EQUALS( columnInfo2.dims[0], 3 );
      TS_ASSERT_EQUALS( columnInfo2.dims[1], 2 );
      TS_ASSERT_EQUALS( columnInfo2.type, NX_FLOAT64 );

      std::vector<double> data2;
      savedNexus.getData<double>(data2);

      TS_ASSERT_EQUALS( data2.size(), 6 );
      TS_ASSERT_EQUALS( data2[0], 0.5 );
      TS_ASSERT_EQUALS( data2[3], 2.5 );
      TS_ASSERT_EQUALS( data2[5], 0.0 );

      std::vector<NeXus::AttrInfo> attrInfos2 = savedNexus.getAttrInfos();
      TS_ASSERT_EQUALS( attrInfos2.size(), 6 );

      if ( attrInfos2.size() == 6 )
      {
        TS_ASSERT_EQUALS( attrInfos2[0].name, "row_size_0");
        TS_ASSERT_EQUALS( savedNexus.getAttr<int>(attrInfos2[0]), 1 );

        TS_ASSERT_EQUALS( attrInfos2[1].name, "row_size_1");
        TS_ASSERT_EQUALS( savedNexus.getAttr<int>(attrInfos2[1]), 2 );

        TS_ASSERT_EQUALS( attrInfos2[4].name, "interpret_as");
        TS_ASSERT_EQUALS( savedNexus.getStrAttr(attrInfos2[4]), "" );

        TS_ASSERT_EQUALS( attrInfos2[5].name, "name");
        TS_ASSERT_EQUALS( savedNexus.getStrAttr(attrInfos2[5]), "DoubleVectorColumn" );
      }
    }
    catch(std::exception& e)
    {
      TS_FAIL( e.what() );
    }

    Poco::File(outputFileName).remove();
  }

  void testSaveTableColumn()
  {
    std::string outputFileName = "SaveNexusProcessedTest_testSaveTable.nxs";

    // Create a table which we will save
    auto table = boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(WorkspaceFactory::Instance().createTable());
    table->setRowCount(3);
    table->addColumn("int", "IntColumn");
    {
      auto& data = table->getColVector<int>("IntColumn");
      data[0] = 5;
      data[1] = 2;
      data[2] = 3;
    }
    table->addColumn("double", "DoubleColumn");
    {
      auto& data = table->getColVector<double>("DoubleColumn");
      data[0] = 0.5;
      data[1] = 0.2;
      data[2] = 0.3;
    }
    table->addColumn("float", "FloatColumn");
    {
      auto& data = table->getColVector<float>("FloatColumn");
      data[0] = 10.5f;
      data[1] = 10.2f;
      data[2] = 10.3f;
    }
    table->addColumn("uint", "UInt32Column");
    {
      auto& data = table->getColVector<uint32_t>("UInt32Column");
      data[0] = 15;
      data[1] = 12;
      data[2] = 13;
    }
    table->addColumn("long64", "Int64Column");
    {
      auto& data = table->getColVector<int64_t>("Int64Column");
      data[0] = 25;
      data[1] = 22;
      data[2] = 23;
    }
    table->addColumn("size_t", "SizeColumn");
    {
      auto& data = table->getColVector<size_t>("SizeColumn");
      data[0] = 35;
      data[1] = 32;
      data[2] = 33;
    }
    table->addColumn("bool", "BoolColumn");
    {
      auto& data = table->getColVector<Boolean>("BoolColumn");
      data[0] = true;
      data[1] = false;
      data[2] = true;
    }
    table->addColumn("V3D", "V3DColumn");
    {
      auto& data = table->getColVector<V3D>("V3DColumn");
      data[0] = V3D(1,2,3);
      data[1] = V3D(4,5,6);
      data[2] = V3D(7,8,9);
    }
    table->addColumn("str", "StringColumn");
    {
      auto& data = table->getColVector<std::string>("StringColumn");
      data[0] = "First row";
      data[1] = "2";
      data[2] = "";
    }

    SaveNexusProcessed alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", table);
    alg.setPropertyValue("Filename", outputFileName);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    if ( ! alg.isExecuted() )
      return; // Nothing to check

    // Get full output file path
    outputFileName = alg.getPropertyValue("Filename");

    NeXus::File savedNexus(outputFileName);

    savedNexus.openGroup("mantid_workspace_1", "NXentry");
    savedNexus.openGroup("table_workspace", "NXdata");

    {
      savedNexus.openData("column_1");
      doTestColumnInfo( savedNexus, NX_INT32, "", "IntColumn" );
      int32_t expectedData[] = { 5, 2, 3 };
      doTestColumnData( "IntColumn", savedNexus, expectedData );
    }

    {
      savedNexus.openData("column_2");
      doTestColumnInfo( savedNexus, NX_FLOAT64, "", "DoubleColumn" );
      double expectedData[] = { 0.5, 0.2, 0.3 };
      doTestColumnData( "DoubleColumn", savedNexus, expectedData );
    }

    {
      savedNexus.openData("column_3");
      doTestColumnInfo( savedNexus, NX_FLOAT32, "", "FloatColumn" );
      float expectedData[] = { 10.5f, 10.2f, 10.3f };
      doTestColumnData( "FloatColumn", savedNexus, expectedData );
    }

    {
      savedNexus.openData("column_4");
      doTestColumnInfo( savedNexus, NX_UINT32, "", "UInt32Column" );
      uint32_t expectedData[] = { 15, 12, 13 };
      doTestColumnData( "UInt32Column", savedNexus, expectedData );
    }

    {
      savedNexus.openData("column_5");
      doTestColumnInfo( savedNexus, NX_INT64, "", "Int64Column" );
      int64_t expectedData[] = { 25, 22, 23 };
      doTestColumnData( "Int64Column", savedNexus, expectedData );
    }

    {
      savedNexus.openData("column_6");
      doTestColumnInfo( savedNexus, NX_UINT64, "", "SizeColumn" );
      uint64_t expectedData[] = { 35, 32, 33 };
      doTestColumnData( "SizeColumn", savedNexus, expectedData );
    }

    {
      savedNexus.openData("column_7");
      doTestColumnInfo( savedNexus, NX_UINT8, "", "BoolColumn" );
      unsigned char expectedData[] = { 1, 0, 1 };
      doTestColumnData( "BoolColumn", savedNexus, expectedData );
    }

    {
      savedNexus.openData("column_8");
      doTestColumnInfo2( savedNexus, NX_FLOAT64, "V3D", "V3DColumn", 3 );
      double expectedData[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
      doTestColumnData( "V3DColumn",savedNexus, expectedData, 9 );
    }

    {
      savedNexus.openData("column_9");

      NeXus::Info columnInfo = savedNexus.getInfo();
      TS_ASSERT_EQUALS( columnInfo.dims.size(), 2 );
      TS_ASSERT_EQUALS( columnInfo.dims[0], 3 );
      TS_ASSERT_EQUALS( columnInfo.dims[1], 9 );
      TS_ASSERT_EQUALS( columnInfo.type, NX_CHAR );

      std::vector<NeXus::AttrInfo> attrInfos = savedNexus.getAttrInfos();
      TS_ASSERT_EQUALS( attrInfos.size(), 3 );

      if ( attrInfos.size() == 3 )
      {
        TS_ASSERT_EQUALS( attrInfos[1].name, "interpret_as");
        TS_ASSERT_EQUALS( savedNexus.getStrAttr(attrInfos[1]), "A string" );

        TS_ASSERT_EQUALS( attrInfos[2].name, "name");
        TS_ASSERT_EQUALS( savedNexus.getStrAttr(attrInfos[2]), "StringColumn" );

        TS_ASSERT_EQUALS( attrInfos[0].name, "units");
        TS_ASSERT_EQUALS( savedNexus.getStrAttr(attrInfos[0]), "N/A" );
      }

      std::vector<char> data;
      savedNexus.getData(data);
      TS_ASSERT_EQUALS( data.size(), 9 * 3 );

      std::string first( data.begin(), data.begin() + 9 );
      TS_ASSERT_EQUALS( first, "First row" );

      std::string second( data.begin() + 9, data.begin() + 18 );
      TS_ASSERT_EQUALS( second, "2        " );

      std::string third( data.begin() + 18, data.end() );
      TS_ASSERT_EQUALS( third, "         " );

    }

    savedNexus.close();
    Poco::File(outputFileName).remove();
    AnalysisDataService::Instance().clear();
  }

private:

  void doTestColumnInfo(NeXus::File& file, int type, const std::string& interpret_as, const std::string& name )
  {
      NeXus::Info columnInfo = file.getInfo();
      TSM_ASSERT_EQUALS( name, columnInfo.dims.size(), 1 );
      TSM_ASSERT_EQUALS( name, columnInfo.dims[0], 3 );
      TSM_ASSERT_EQUALS( name, columnInfo.type, type );

      std::vector<NeXus::AttrInfo> attrInfos = file.getAttrInfos();
      TSM_ASSERT_EQUALS( name, attrInfos.size(), 3 );

      if ( attrInfos.size() == 3 )
      {
        TSM_ASSERT_EQUALS( name, attrInfos[1].name, "interpret_as");
        TSM_ASSERT_EQUALS( name, file.getStrAttr(attrInfos[1]), interpret_as );

        TSM_ASSERT_EQUALS( name, attrInfos[2].name, "name");
        TSM_ASSERT_EQUALS( name, file.getStrAttr(attrInfos[2]), name );

        TSM_ASSERT_EQUALS( name, attrInfos[0].name, "units");
        TSM_ASSERT_EQUALS( name, file.getStrAttr(attrInfos[0]), "Not known" );
      }
  }

  void doTestColumnInfo2(NeXus::File& file, int type, const std::string& interpret_as, const std::string& name, int dim1 )
  {
      NeXus::Info columnInfo = file.getInfo();
      TSM_ASSERT_EQUALS( name, columnInfo.dims.size(), 2 );
      TSM_ASSERT_EQUALS( name, columnInfo.dims[0], 3 );
      TSM_ASSERT_EQUALS( name, columnInfo.dims[1], dim1 );
      TSM_ASSERT_EQUALS( name, columnInfo.type, type );

      std::vector<NeXus::AttrInfo> attrInfos = file.getAttrInfos();
      TSM_ASSERT_EQUALS( name, attrInfos.size(), 6 );

      if ( attrInfos.size() == 6 )
      {
        TSM_ASSERT_EQUALS( name, attrInfos[4].name, "interpret_as");
        TSM_ASSERT_EQUALS( name, file.getStrAttr(attrInfos[4]), interpret_as );

        TSM_ASSERT_EQUALS( name, attrInfos[5].name, "name");
        TSM_ASSERT_EQUALS( name, file.getStrAttr(attrInfos[5]), name );

        TSM_ASSERT_EQUALS( name, attrInfos[3].name, "units");
        TSM_ASSERT_EQUALS( name, file.getStrAttr(attrInfos[3]), "Not known" );
      }
  }

  template<typename T>
  void doTestColumnData( const std::string& name, NeXus::File& file, const T expectedData[], size_t len = 3 )
  {
    std::vector<T> data;
    file.getData(data);

    TSM_ASSERT_EQUALS( name, data.size(), len );
    for(size_t i = 0; i < len; ++i)
    {
      std::string mess = name + ", item #" + boost::lexical_cast<std::string>(i);
      TSM_ASSERT_EQUALS( mess, data[i], expectedData[i] );
    };
  }

  std::string outputFile;
  std::string entryName;
  std::string dataName;
  std::string title;
  Workspace2D myworkspace;

  Mantid::DataHandling::LoadRaw3 loader;
  std::string inputFile;
  std::string outputSpace;
  bool clearfiles;

};
#endif /*SAVENEXUSPROCESSEDTEST_H_*/
