#ifndef SAVENEXUSPROCESSEDTEST_H_
#define SAVENEXUSPROCESSEDTEST_H_

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
#include "MantidAPI/AnalysisDataService.h"
#include "MantidNexus/SaveNexusProcessed.h"
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidNexus/LoadNeXus.h"
#include "MantidKernel/UnitFactory.h"
#include "Poco/File.h"

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
    //clearfiles=true;
    //



    // create dummy 2D-workspace

    std::vector<double> lVecX; for(double d=0.0; d<0.95; d=d+0.1) lVecX.push_back(d);
    std::vector<double> lVecY; for(double d=0.0; d<0.95; d=d+0.1) lVecY.push_back(d);
    std::vector<double> lVecE; for(double d=0.0; d<0.95; d=d+0.1) lVecE.push_back(d);

    Workspace2D_sptr localWorkspace2D = boost::dynamic_pointer_cast<Workspace2D>
                 (WorkspaceFactory::Instance().create("Workspace2D",1,10,10));
    localWorkspace2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    localWorkspace2D->setX(0,lVecX);
    localWorkspace2D->setData(0,lVecY, lVecE);

    AnalysisDataService::Instance().add("testSpace", localWorkspace2D);
}


  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }


  void testExec()
  {

    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(),std::runtime_error);


    // Now set it...
    // specify name of file to save workspace to
    algToBeTested.setPropertyValue("InputWorkspace", "testSpace");
    outputFile = "testOfSaveNexusProcessed.nxs";
    //entryName = "test";
    dataName = "spectra";
    title = "A simple workspace saved in Processed Nexus format";
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("FileName", outputFile));
    //algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("Title", title);
    Poco::File(outputFile).remove();

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("FileName") )
    TS_ASSERT( ! result.compare(outputFile));
    //TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") )
    //TS_ASSERT( ! result.compare(entryName));

	// changed so that 1D workspaces are no longer written.
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

    if(clearfiles) remove(outputFile.c_str());

  }
void testExecOnMuon()
  {
    LoadNexus nxLoad;
    std::string outputSpace,inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "../../../../Test/Nexus/emu00006473.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);
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
    outputFile = "testOfSaveNexusProcessed2.nxs";
    remove(outputFile.c_str());
    //entryName = "entry4";
    dataName = "spectra";
    title = "A save of a 2D workspace from Muon file";
    algToBeTested.setPropertyValue("FileName", outputFile);
    //algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("Title", title);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));
    //TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") );
    //TS_ASSERT( ! result.compare(entryName));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

	// try writing data again
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );
    if(clearfiles) remove(outputFile.c_str());
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(outputSpace));

  }

void testExecOnLoadraw()
{
    std::string inputFile = "../../../../Test/Data/HET15869.RAW";
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
    outputFile = "testSaveFromLoadraw.nxs";
    remove(outputFile.c_str());
    //entryName = "entry4";
    dataName = "spectra";
    title = "A save of a workspace from Loadraw file";
    algToBeTested.setPropertyValue("FileName", outputFile);
    //algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("Title", title);

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

void testExecOnMuonXml()
  {
    LoadNexus nxLoad;
    std::string outputSpace,inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "../../../../Test/Nexus/emu00006473.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);
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

    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "testOfSaveNexusProcessed2.xml";
    Poco::File(outputFile).remove();
    //entryName = "entry4";
    dataName = "spectra";
    title = "A save of a 2D workspace from Muon file";
    algToBeTested.setPropertyValue("FileName", outputFile);
    //algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("Title", title);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));
    //TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") );
    //TS_ASSERT( ! result.compare(entryName));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

	// try writing data again
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );
    if(clearfiles) Poco::File(outputFile).remove();
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(outputSpace));

  }

private:
  SaveNexusProcessed algToBeTested;
  std::string outputFile;
  std::string entryName;
  std::string dataName;
  std::string title;
  Workspace2D myworkspace;

  Mantid::DataHandling::LoadRaw loader;
  std::string inputFile;
  std::string outputSpace;
  bool clearfiles;

};

#endif /*SAVENEXUSPROCESSEDTEST_H_*/
