#ifndef CONJOINWORKSPACESTEST_H_
#define CONJOINWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class ConjoinWorkspacesTest : public CxxTest::TestSuite
{
public:
  ConjoinWorkspacesTest() :
    ws1Name("ConjoinWorkspacesTest_grp1"), ws2Name("ConjoinWorkspacesTest_grp2")
  {
  }


  void setupWS()
  {
    IAlgorithm* loader;
    loader = new Mantid::DataHandling::LoadRaw;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", "top");
    loader->setPropertyValue("SpectrumMin","1");
    loader->setPropertyValue("SpectrumMax","10");
    TS_ASSERT_THROWS_NOTHING( loader->execute() );
    TS_ASSERT( loader->isExecuted() );
    delete loader;

    loader = new Mantid::DataHandling::LoadRaw;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", "bottom");
    loader->setPropertyValue("SpectrumMin","11");
    loader->setPropertyValue("SpectrumMax","25");
    TS_ASSERT_THROWS_NOTHING( loader->execute() );
    TS_ASSERT( loader->isExecuted() );
    delete loader;

    //Now some event workspaces
    loader = new Mantid::DataHandling::LoadEventPreNeXus;
    loader->initialize();
    loader->setPropertyValue("EventFilename", "VULCAN_2916_neutron0_event.dat");
    loader->setPropertyValue("OutputWorkspace", "vulcan0");
    TS_ASSERT_THROWS_NOTHING( loader->execute() );
    TS_ASSERT( loader->isExecuted() );
    delete loader;

    loader = new Mantid::DataHandling::LoadEventPreNeXus;
    loader->initialize();
    loader->setPropertyValue("EventFilename", "VULCAN_2916_neutron1_event.dat");
    loader->setPropertyValue("OutputWorkspace", "vulcan1");
    TS_ASSERT_THROWS_NOTHING( loader->execute() );
    TS_ASSERT( loader->isExecuted() );
    delete loader;
  }

  void testTheBasics()
  {
    conj = new ConjoinWorkspaces();
    TS_ASSERT_EQUALS( conj->name(), "ConjoinWorkspaces" );
    TS_ASSERT_EQUALS( conj->version(), 1 );
    TS_ASSERT_EQUALS( conj->category(), "General" );
    delete conj;
  }

  void testInit()
  {
    conj = new ConjoinWorkspaces();
    TS_ASSERT_THROWS_NOTHING( conj->initialize() );
    TS_ASSERT( conj->isInitialized() );
    delete conj;
  }

  //----------------------------------------------------------------------------------------------
  void testExec()
  {
    setupWS();

    conj = new ConjoinWorkspaces();
    if ( !conj->isInitialized() ) conj->initialize();

    // Get the two input workspaces for later
    MatrixWorkspace_const_sptr in1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("top"));
    MatrixWorkspace_const_sptr in2 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("bottom"));

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS( conj->execute(), std::runtime_error );
    TS_ASSERT( ! conj->isExecuted() );

    // Check it fails if input overlap
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace1","top") );
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace2","top") );
    TS_ASSERT_THROWS_NOTHING( conj->execute() );
    TS_ASSERT( ! conj->isExecuted() );

    // Now it should succeed
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace2","bottom") );
    TS_ASSERT_THROWS_NOTHING( conj->execute() );
    TS_ASSERT( conj->isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("top")) );
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 25 );
    // Check a few values
    TS_ASSERT_EQUALS( output->readX(0)[0], in1->readX(0)[0] );
    TS_ASSERT_EQUALS( output->readX(15)[444], in2->readX(5)[444] );
    TS_ASSERT_EQUALS( output->readY(3)[99], in1->readY(3)[99] );
    TS_ASSERT_EQUALS( output->readE(7)[700], in1->readE(7)[700] );
    TS_ASSERT_EQUALS( output->readY(19)[55], in2->readY(9)[55] );
    TS_ASSERT_EQUALS( output->readE(10)[321], in2->readE(0)[321] );
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(5), in1->getAxis(1)->spectraNo(5) );
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(12), in2->getAxis(1)->spectraNo(2) );

    // Check that 2nd input workspace no longer exists
    TS_ASSERT_THROWS( AnalysisDataService::Instance().retrieve("bottom"), Exception::NotFoundError );
    delete conj;
  }

  //----------------------------------------------------------------------------------------------
  void testExecMismatchedWorkspaces()
  {
    setupWS();

    // Check it fails if input overlap
    conj = new ConjoinWorkspaces();
    conj->initialize();
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace1","vulcan1") );
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace2","vulcan1") );
    conj->execute();
    TS_ASSERT( ! conj->isExecuted() );
    delete conj;

    // Check it fails if mixing event workspaces and workspace 2Ds
    conj = new ConjoinWorkspaces();
    conj->initialize();
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace1","vulcan1") );
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace2","bottom") );
    conj->execute();
    TS_ASSERT( ! conj->isExecuted() );
  }

  //----------------------------------------------------------------------------------------------
  void testExecEvent()
  {
    setupWS();

    //Save some initial data
    EventWorkspace_sptr in1, in2, out;
    in1 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("vulcan0"));
    in2 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("vulcan1"));
    int nHist1 = in1->getNumberHistograms();
    int nEvents1 = in1->getNumberEvents();
    int nHist2 = in2->getNumberHistograms();
    int nEvents2 = in2->getNumberEvents();

    // Check it runs with the two separate ones
    conj = new ConjoinWorkspaces();
    conj->initialize();
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace1","vulcan0") );
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace2","vulcan1") );
    conj->execute();
    TS_ASSERT( conj->isExecuted() );
    delete conj;

    // Get the two input workspaces for later
    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("vulcan0"));

    int nHist = out->getNumberHistograms();
    int nEvents = out->getNumberEvents();

    TS_ASSERT_EQUALS( nHist1+nHist2, nHist);
    TS_ASSERT_EQUALS( nEvents1+nEvents2, nEvents);

    TS_ASSERT( !AnalysisDataService::Instance().doesExist("vulcan1") );
  }

  //----------------------------------------------------------------------------------------------
  void testExecGroup()
  {
    setupWS();

    //Save some initial data
    EventWorkspace_sptr in1, in2, out;
    in1 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("vulcan0"));
    in2 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("vulcan1"));
    AnalysisDataService::Instance().add("grp1_1", in1);
    AnalysisDataService::Instance().add("grp2_1", in2);
    int nHist1 = in1->getNumberHistograms();
    int nEvents1 = in1->getNumberEvents();
    int nHist2 = in2->getNumberHistograms();
    int nEvents2 = in2->getNumberEvents();

    WorkspaceGroup_sptr wsSptr1 = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr1)
    {
      AnalysisDataService::Instance().add("grp1", wsSptr1);
      // Group children expected to be parentName_1,2,3 etc. grrrr.
      wsSptr1->add("grp1_1");
    }

    WorkspaceGroup_sptr wsSptr2 = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr2)
    {
      AnalysisDataService::Instance().add("grp2", wsSptr2);
      wsSptr2->add("grp2_1");
    }

    // Check it runs with the two separate ones
    conj = new ConjoinWorkspaces();
    conj->initialize();
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace1","grp1") );
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace2","grp2") );
    conj->execute();
    TS_ASSERT( conj->isExecuted() );
    delete conj;

    // Get the two input workspaces for later
    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("grp1_1"));

    int nHist = out->getNumberHistograms();
    int nEvents = out->getNumberEvents();

    TS_ASSERT_EQUALS( nHist1+nHist2, nHist);
    TS_ASSERT_EQUALS( nEvents1+nEvents2, nEvents);

    TS_ASSERT( !AnalysisDataService::Instance().doesExist("grp2") );

    // Clean up
    AnalysisDataService::Instance().remove("grp1");
  }


  void testDoCheckForOverlap()
  {
    MatrixWorkspace_sptr ws1, ws2, out;
    int numPixels = 10;
    int numBins = 20;
    ws1 = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins);
    ws2 = WorkspaceCreationHelper::CreateEventWorkspace(5, numBins);

    conj = new ConjoinWorkspaces();
    conj->initialize();
    TS_ASSERT_THROWS_NOTHING( conj->setProperty("InputWorkspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( conj->setProperty("InputWorkspace2",ws2) );
    TS_ASSERT_THROWS_NOTHING( conj->setProperty("CheckOverlapping",true) );
    conj->execute();
    TS_ASSERT( !conj->isExecuted() );
    delete conj;
  }

  void performTestNoOverlap(bool event)
  {
    MatrixWorkspace_sptr ws1, ws2, out;
    int numBins = 20;

    if (event)
    {
      ws1 = WorkspaceCreationHelper::CreateEventWorkspace2(10, numBins); //2 events per bin
      ws2 = WorkspaceCreationHelper::CreateEventWorkspace2(5, numBins);
    }
    else
    {
      ws1 = WorkspaceCreationHelper::Create2DWorkspace(10, numBins);
      ws2 = WorkspaceCreationHelper::Create2DWorkspace(5, numBins);
    }
    AnalysisDataService::Instance().addOrReplace(ws1Name, ws1);
    AnalysisDataService::Instance().addOrReplace(ws2Name, ws2);

    conj = new ConjoinWorkspaces();
    conj->initialize();
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace1",ws1Name) );
    TS_ASSERT_THROWS_NOTHING( conj->setPropertyValue("InputWorkspace2",ws2Name) );
    TS_ASSERT_THROWS_NOTHING( conj->setProperty("CheckOverlapping",false) );
    TS_ASSERT_THROWS_NOTHING( conj->execute(); )
    TS_ASSERT( conj->isExecuted() );

    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws1Name) ); )
    TS_ASSERT(out);
    if (!out) return;

    TS_ASSERT_EQUALS( out->getNumberHistograms(), 15);
    TS_ASSERT_EQUALS( out->blocksize(), numBins);

    for(int wi=0; wi<out->getNumberHistograms(); wi++)
      for(int x=0; x<out->blocksize(); x++)
        TS_ASSERT_DELTA(out->readY(wi)[x], 2.0, 1e-5);

    delete conj;
  }

  void test_DONTCheckForOverlap_Events()
  {
    performTestNoOverlap(true);
  }

  void test_DONTCheckForOverlap_2D()
  {
    performTestNoOverlap(false);
  }



private:
  ConjoinWorkspaces * conj;
  const std::string ws1Name;
  const std::string ws2Name;
};

#endif /*CONJOINWORKSPACESTEST_H_*/
