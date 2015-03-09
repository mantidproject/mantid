#ifndef MANTID_MDEVENTS_MERGEMDEWTEST_H_
#define MANTID_MDEVENTS_MERGEMDEWTEST_H_

#include "MantidMDAlgorithms/MergeMDFiles.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidTestHelpers/MDAlgorithmsTestHelper.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class MergeMDFilesTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    MergeMDFiles alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
   do_test_exec("");
  }

  void test_exec_fileBacked()
  {
    do_test_exec("MergeMDFilesTest_OutputWS.nxs");
  }
  
  void do_test_exec(std::string OutputFilename)
  {
    if (OutputFilename != "")
    {
       if (Poco::File(OutputFilename).exists()) Poco::File(OutputFilename).remove();
    }


    // Create a bunch of input files
    std::vector<std::vector<std::string> > filenames;
    std::vector<MDEventWorkspace3Lean::sptr> inWorkspaces;
    // how many events put into each file.
    long nFileEvents(1000);
    for (size_t i=0; i<3; i++)
    {
      std::ostringstream mess;
      mess << "MergeMDFilesTestInput" << i;
      MDEventWorkspace3Lean::sptr ws = MDAlgorithmsTestHelper::makeFileBackedMDEW(mess.str(), true,-nFileEvents);
      inWorkspaces.push_back(ws);
      filenames.push_back(std::vector<std::string>(1,ws->getBoxController()->getFilename()));
    }

    // Name of the output workspace.
    std::string outWSName("MergeMDFilesTest_OutputWS");
  
    MergeMDFiles alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Filenames", filenames) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputFilename", OutputFilename) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );

    // clean up possible rubbish from previous runs
    std::string fullName = alg.getPropertyValue("OutputFilename");
    if (fullName!="")
      if(Poco::File(fullName).exists()) Poco::File(fullName).remove();


    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    std::string actualOutputFilename = alg.getPropertyValue("OutputFilename");

    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    TS_ASSERT_EQUALS( ws->getNPoints(), 3*nFileEvents);
    MDBoxBase3Lean * box = ws->getBox();
    TS_ASSERT_EQUALS( box->getNumChildren(), 1000);

    // Every sub-box has on average 30 events (there are 1000 boxes)
    // Check that each box has at least SOMETHING
    for (size_t i=0; i<box->getNumChildren(); i++)
      TS_ASSERT_LESS_THAN( 1, box->getChild(i)->getNPoints());

    if (!OutputFilename.empty())
    {
      TS_ASSERT( ws->isFileBacked() );
      TS_ASSERT( Poco::File(actualOutputFilename).exists());
      ws->clearFileBacked(false);
      Poco::File(actualOutputFilename).remove();

    }

    // Cleanup generated input files
    for (size_t i=0; i<inWorkspaces.size(); i++)
    { 
        if(inWorkspaces[i]->getBoxController()->isFileBacked())
        {
            std::string fileName  = inWorkspaces[i]->getBoxController()->getFileIO()->getFileName();
            inWorkspaces[i]->clearFileBacked(false);
            Poco::File(fileName).remove();

        }
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

  
  }



};


#endif /* MANTID_MDEVENTS_MERGEMDEWTEST_H_ */

