#ifndef SAVENEXUSPROCESSEDTEST_H_
#define SAVENEXUSPROCESSEDTEST_H_


//TODO: Remove this before checking in the test
//#define _WIN64



#include <fstream>
#include <cxxtest/TestSuite.h>

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidDataHandling/LoadInstrument.h"
//
#include "MantidDataHandling/LoadRaw.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidNexus/SaveNexusProcessed.h"
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidNexus/LoadNeXus.h"
#include "MantidNexus/LoadSNSEventNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::DataObjects;

class SaveNexusProcessedTest : public CxxTest::TestSuite
{
public:

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
    std::string inputFile = "HET15869.raw";
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

    //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
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

#endif /*_WIN64*/
  }



  void testExecOnMuonXml()
  {
    SaveNexusProcessed algToBeTested;

    //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
    //  std::string s;
    //std::getline(std::cin,s);

    LoadNexus nxLoad;
    std::string outputSpace,inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "emu00006473.nxs";
    nxLoad.setPropertyValue("Filename", inputFile);
    inputFile = nxLoad.getPropertyValue("Filename"); //Get the absolute path
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
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();
    algToBeTested.setRethrows(true);

    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "SaveNexusProcessedTest_testExecOnMuonXml.xml";
    //entryName = "entry4";
    dataName = "spectra";
    title = "A save of a 2D workspace from Muon file";
    algToBeTested.setPropertyValue("Filename", outputFile);
    //algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("Title", title);
    outputFile = algToBeTested.getPropertyValue("Filename");
    if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));
    //TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") );
    //TS_ASSERT( ! result.compare(entryName));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

    if(clearfiles)
    {
      if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();
    }
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(outputSpace));

#endif /*_WIN64*/
  }




  static EventWorkspace_sptr do_testExec_EventWorkspaces(std::string filename_root, EventType type,
      std::string & outputFile,  bool makeDifferentTypes, bool clearfiles)
  {
    std::vector< std::vector<int> > groups(5);
    groups[0].push_back(10);
    groups[0].push_back(11);
    groups[0].push_back(12);
    groups[1].push_back(20);
    groups[2].push_back(30);
    groups[2].push_back(31);  // Switch the event type

    groups[3].push_back(40);
    groups[4].push_back(50);

    EventWorkspace_sptr WS = WorkspaceCreationHelper::CreateGroupedEventWorkspace(groups, 100, 1.0);
    WS->getEventList(3).clear();
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
      for (int wi=0; wi < WS->getNumberHistograms(); wi++)
        WS->getEventList(wi).switchTo(type);
    }

    SaveNexusProcessed alg;
    alg.initialize();

    // Now set it...
    alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(WS));

    // specify name of file to save workspace to
    std::ostringstream mess;
    mess << filename_root << static_cast<int>(type) << ".nxs";
    outputFile = mess.str();
    std::string dataName = "spectra";
    std::string title = "A simple workspace saved in Processed Nexus format";

    alg.setPropertyValue("Filename", outputFile);
    outputFile = alg.getPropertyValue("Filename");
    alg.setPropertyValue("Title", title);

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


  void xtestExec_LoadedEventWorkspace()
  {

    //----- Now we re-load with precounting and compare memory use ----
    LoadSNSEventNexus ld2;
    std::string outws_name = "SaveNexusProcessed_Loaded";
    ld2.initialize();
    ld2.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld2.setPropertyValue("OutputWorkspace",outws_name);
    ld2.setPropertyValue("Precount", "1");
    ld2.execute();
    TS_ASSERT( ld2.isExecuted() );

    SaveNexusProcessed alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", outws_name);
    outputFile = "SaveNexusProcessed_Loaded.nxs";
    dataName = "spectra";
    title = "A simple workspace saved in Processed Nexus format";
    alg.setPropertyValue("Filename", outputFile);
    outputFile = alg.getPropertyValue("Filename");
    alg.setPropertyValue("Title", title);

    // Clear the existing file, if any
    if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    TS_ASSERT( Poco::File(outputFile).exists() );

    if (clearfiles)
      if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();
  }



private:
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
