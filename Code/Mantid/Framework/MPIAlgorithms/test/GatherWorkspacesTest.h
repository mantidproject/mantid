#ifndef GATHERWORKSPACESTEST_H_
#define GATHERWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMPIAlgorithms/GatherWorkspaces.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;

class GatherWorkspacesTest : public CxxTest::TestSuite
{
public:
  static GatherWorkspacesTest *createSuite() { return new GatherWorkspacesTest(); }
  static void destroySuite(GatherWorkspacesTest *suite) { delete suite; }

  GatherWorkspacesTest()
  {
    // Create the Framework manager so that MPI gets initialized
    API::FrameworkManager::Instance();
  }

  void testTheBasics()
  {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_EQUALS( gatherer.name(), "GatherWorkspaces" );
    TS_ASSERT_EQUALS( gatherer.version(), 1 );
    TS_ASSERT_EQUALS( gatherer.category(), "MPI" );

    TS_ASSERT_THROWS_NOTHING( gatherer.initialize() );
    TS_ASSERT( gatherer.isInitialized() );
  }

  void testRootMustHaveInputWorkspace()
  {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_THROWS_NOTHING( gatherer.initialize() );
    TS_ASSERT_THROWS_NOTHING( gatherer.setProperty("OutputWorkspace","something") );
    // Haven't set InputWorkspace and this will be the root process, so it should complain
    TS_ASSERT_THROWS( gatherer.execute(), std::runtime_error );
    TS_ASSERT( ! gatherer.isExecuted() );
  }

  void testExecute()
  {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_THROWS_NOTHING( gatherer.initialize() );
    // Create a small workspace
    API::MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::Create2DWorkspace154(1,5);

    TS_ASSERT_THROWS_NOTHING( gatherer.setProperty("InputWorkspace",inWS) );
    gatherer.setChild(true); // Make a child algorithm to keep the result out of the ADS

    TS_ASSERT( gatherer.execute() );
    API::MatrixWorkspace_const_sptr outWS = gatherer.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS( inWS->size(), outWS->size() );
    for (int i=0; i < 5; ++i)
    {
      TS_ASSERT_EQUALS( inWS->readX(0)[i], outWS->readX(0)[i] );
      TS_ASSERT_EQUALS( inWS->readY(0)[i], outWS->readY(0)[i] );
      TS_ASSERT_EQUALS( inWS->readE(0)[i], outWS->readE(0)[i] );
    }

    TS_ASSERT( outWS->spectraMap().nElements() == 0 );
    TS_ASSERT_EQUALS( inWS->getBaseInstrument(), outWS->getBaseInstrument() );
  }

  // TODO: Work out a way of testing under MPI because absent that the test is not very interesting
};

#endif /*GATHERWORKSPACESTEST_H_*/
