#ifndef REMOVELOGSTEST_H_
#define REMOVELOGSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/RemoveLogs.h"
#include "MantidDataHandling/LoadLog.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class RemoveLogsTest : public CxxTest::TestSuite
{
public: 


  static RemoveLogsTest *createSuite() { return new RemoveLogsTest(); }
  static void destroySuite(RemoveLogsTest *suite) { delete suite; }

  RemoveLogsTest()
  {	
    //initialise framework manager to allow logging
    //Mantid::API::FrameworkManager::Instance().initialize();
  }
  void testInit()
  {
    TS_ASSERT( !remover.isInitialized() );
    TS_ASSERT_THROWS_NOTHING(remover.initialize());    
    TS_ASSERT( remover.isInitialized() );
  }

  void testExecWithSingleLogFile()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    // Path to test input file assumes Test directory checked out from SVN
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "HRP37129_ICPevent.txt") )
    inputFile = loader.getPropertyValue("Filename");

    outputSpace = "RemoveLogsTest-singleLogFile";
    TS_ASSERT_THROWS(loader.setPropertyValue("Workspace", outputSpace), std::invalid_argument)
    // Create an empty workspace and put it in the AnalysisDataService
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(outputSpace, ws));    

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(outputSpace));

    TS_ASSERT_THROWS_NOTHING(loader.execute());


    TS_ASSERT( loader.isExecuted() );    

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));

    if ( !remover.isInitialized() ) remover.initialize();
    TS_ASSERT_THROWS_NOTHING(remover.setPropertyValue("Workspace", outputSpace))
    TS_ASSERT_THROWS_NOTHING(remover.execute());


    TS_ASSERT( remover.isExecuted() );    

    // log should have been removed
    TS_ASSERT_THROWS( output->run().getLogData("HRP37129_ICPevent"), std::runtime_error);

    AnalysisDataService::Instance().remove(outputSpace);
  }


  void do_test_SNSTextFile(std::string names, std::string units, bool willFail, bool createWorkspace = true)
  {
    // Create an empty workspace and put it in the AnalysisDataService

    outputSpace = "test_SNSTextFile";
    if (createWorkspace)
    {
      Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(outputSpace, ws));
    }

    // Set up the algo
    LoadLog alg;
    alg.initialize();
    TS_ASSERT( alg.isInitialized() );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "VULCAN_furnace4208.txt") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", outputSpace) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Names", names ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Units", units ) );
    TS_ASSERT_THROWS_NOTHING(alg.execute() );
    if (willFail)
      { TS_ASSERT(!alg.isExecuted() );
        return;
      }
    else
      {TS_ASSERT(alg.isExecuted() );}

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));

    if ( !remover.isInitialized() ) remover.initialize();
    TS_ASSERT_THROWS_NOTHING(remover.setPropertyValue("Workspace", outputSpace))
    TS_ASSERT_THROWS_NOTHING(remover.execute());


    TS_ASSERT( remover.isExecuted() );    

    // logs should have been removed
    TS_ASSERT_THROWS( output->run().getLogData("Yadda"), std::runtime_error);
    TS_ASSERT_THROWS( output->run().getLogData("Temp1"), std::runtime_error);
    TS_ASSERT_THROWS( output->run().getLogData("Temp2"), std::runtime_error);
    TS_ASSERT_THROWS( output->run().getLogData("Temp3"), std::runtime_error);
    TS_ASSERT_THROWS( output->run().getLogData("Extra"), std::runtime_error);

  }

  void test_SNSTextFile_noNames_fails()
  {
    do_test_SNSTextFile("", "", true);
  }

  void test_SNSTextFile_tooFewNames_fails()
  {
    do_test_SNSTextFile("Yadda,Yadda", "", true);
  }

  void test_SNSTextFile_tooManyNames_fails()
  {
    do_test_SNSTextFile("Yadda,Yadda,Yadda,Yadda,Yadda,Yadda", "", true);
  }

  void test_SNSTextFile()
  {
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "C,K,F,Furlongs", false);
  }

  void test_SNSTextFile_noUnits()
  {
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "", false);
  }

  void test_SNSTextFile_wrongNumberOfUnits_fails()
  {
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "Dynes,Ergs", true);
  }

  void test_SNSTextFile_twice_overwrites_logs()
  {
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "C,K,F,Furlongs", false, true);
    // Dont re-create the workspace the 2nd time around.
    // Switch a unit around to make sure the new one got overwritten
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Yadda", "C,K,F,Fortnights", false, false);
  }


private:
  LoadLog loader;
  RemoveLogs remover;
  std::string inputFile;
  std::string outputSpace;
  std::string inputSpace;

};

#endif /*REMOVELOGSTEST_H_*/
