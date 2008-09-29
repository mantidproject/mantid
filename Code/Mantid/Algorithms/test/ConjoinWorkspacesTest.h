#ifndef CONJOINWORKSPACESTEST_H_
#define CONJOINWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

class ConjoinWorkspacesTest : public CxxTest::TestSuite
{
public:
  ConjoinWorkspacesTest()
  {
    IAlgorithm* loader = new Mantid::DataHandling::LoadRaw;
    loader->initialize();
    loader->setPropertyValue("Filename", "../../../../Test/Data/osi11886.raw");
    loader->setPropertyValue("OutputWorkspace", "top");
    loader->setPropertyValue("spectrum_min","1");
    loader->setPropertyValue("spectrum_max","10");
    TS_ASSERT_THROWS_NOTHING( loader->execute() )
    TS_ASSERT( loader->isExecuted() )
    delete loader;

    IAlgorithm* loader2 = new Mantid::DataHandling::LoadRaw;
    loader2->initialize();
    loader2->setPropertyValue("Filename", "../../../../Test/Data/osi11886.raw");
    loader2->setPropertyValue("OutputWorkspace", "bottom");
    loader2->setPropertyValue("spectrum_min","11");
    loader2->setPropertyValue("spectrum_max","25");
    TS_ASSERT_THROWS_NOTHING( loader2->execute() )
    TS_ASSERT( loader2->isExecuted() )
    delete loader2;
  }

  void testName()
  {
    TS_ASSERT_EQUALS( conj.name(), "ConjoinWorkspaces" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( conj.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( conj.category(), "General" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( conj.initialize() )
    TS_ASSERT( conj.isInitialized() )
  }

  void testExec()
  {
    if ( !conj.isInitialized() ) conj.initialize();

    // Get the two input workspaces for later
    Workspace_const_sptr in1 = AnalysisDataService::Instance().retrieve("top");
    Workspace_const_sptr in2 = AnalysisDataService::Instance().retrieve("bottom");

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS( conj.execute(), std::runtime_error )
    TS_ASSERT( ! conj.isExecuted() )

    // Check it fails if input overlap
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace1","top") )
    //TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace2","top") )
    //TS_ASSERT_THROWS_NOTHING( conj.execute() )
    //TS_ASSERT( ! conj.isExecuted() )

    // Now it should succeed
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace2","bottom") )
    TS_ASSERT_THROWS_NOTHING( conj.execute() )
    TS_ASSERT( conj.isExecuted() )

    Workspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("top") )
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 25 )
    // Check a few values
    TS_ASSERT_EQUALS( output->readX(0)[0], in1->readX(0)[0] )
    TS_ASSERT_EQUALS( output->readX(15)[444], in2->readX(5)[444] )
    TS_ASSERT_EQUALS( output->readY(3)[99], in1->readY(3)[99] )
    TS_ASSERT_EQUALS( output->readE(7)[700], in1->readE(7)[700] )
    TS_ASSERT_EQUALS( output->readY(19)[55], in2->readY(9)[55] )
    TS_ASSERT_EQUALS( output->readE(10)[321], in2->readE(0)[321] )
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(5), in1->getAxis(1)->spectraNo(5) )
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(12), in2->getAxis(1)->spectraNo(2) )

    // Check that 2nd input workspace no longer exists
    TS_ASSERT_THROWS( AnalysisDataService::Instance().retrieve("bottom"), Exception::NotFoundError )
  }

private:
  ConjoinWorkspaces conj;
};

#endif /*CONJOINWORKSPACESTEST_H_*/
