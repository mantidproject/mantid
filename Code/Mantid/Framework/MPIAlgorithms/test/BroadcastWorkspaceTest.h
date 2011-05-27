#ifndef BROADCASTWORKSPACETEST_H_
#define BROADCASTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMPIAlgorithms/BroadcastWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;

class BroadcastWorkspaceTest : public CxxTest::TestSuite
{
public:
  static BroadcastWorkspaceTest *createSuite() { return new BroadcastWorkspaceTest(); }
  static void destroySuite(BroadcastWorkspaceTest *suite) { delete suite; }

  BroadcastWorkspaceTest()
  {
    // Create the Framework manager so that MPI gets initialized
    API::FrameworkManager::Instance();
  }

  void testTheBasics()
  {
    MPIAlgorithms::BroadcastWorkspace broadcaster;
    TS_ASSERT_EQUALS( broadcaster.name(), "BroadcastWorkspace" );
    TS_ASSERT_EQUALS( broadcaster.version(), 1 );
    TS_ASSERT_EQUALS( broadcaster.category(), "MPI" );

    TS_ASSERT_THROWS_NOTHING( broadcaster.initialize() );
    TS_ASSERT( broadcaster.isInitialized() );
  }

  void testRankValidator()
  {
    MPIAlgorithms::BroadcastWorkspace broadcaster;
    TS_ASSERT_THROWS_NOTHING( broadcaster.initialize() );
    TS_ASSERT_THROWS_NOTHING( broadcaster.setPropertyValue("OutputWorkspace","blah") );
    TS_ASSERT_THROWS( broadcaster.setProperty("BroadcasterRank",1), std::invalid_argument );
  }

  void testExecute()
  {
    MPIAlgorithms::BroadcastWorkspace broadcaster;
    TS_ASSERT_THROWS_NOTHING( broadcaster.initialize() );
    // Create a small workspace
    API::MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::Create2DWorkspace154(1,5);

    TS_ASSERT_THROWS_NOTHING( broadcaster.setProperty("InputWorkspace",inWS) );
    TS_ASSERT_THROWS_NOTHING( broadcaster.setPropertyValue("OutputWorkspace","blah") );
    broadcaster.setChild(true); // Make a child algorithm to keep the result out of the ADS

    TS_ASSERT( broadcaster.execute() );
    API::MatrixWorkspace_const_sptr outWS = broadcaster.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS( inWS->size(), outWS->size() );
    for (int i=0; i < 5; ++i)
    {
      TS_ASSERT_EQUALS( inWS->readX(0)[i], outWS->readX(0)[i] );
      TS_ASSERT_EQUALS( inWS->readY(0)[i], outWS->readY(0)[i] );
      TS_ASSERT_EQUALS( inWS->readE(0)[i], outWS->readE(0)[i] );
    }

    //TS_ASSERT_EQUALS( inWS->getAxis(0)->unit()->unitID(), outWS->getAxis(0)->unit()->unitID() );
  }

  // TODO: Work out a way of testing under MPI because absent that the test is not very interesting
};

#endif /*BROADCASTWORKSPACETEST_H_*/
