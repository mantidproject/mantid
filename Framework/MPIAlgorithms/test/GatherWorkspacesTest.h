#ifndef GATHERWORKSPACESTEST_H_
#define GATHERWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidMPIAlgorithms/GatherWorkspaces.h"
#include "MantidTestHelpers/HistogramDataTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;

class GatherWorkspacesTest : public CxxTest::TestSuite {
public:
  static GatherWorkspacesTest *createSuite() {
    return new GatherWorkspacesTest();
  }
  static void destroySuite(GatherWorkspacesTest *suite) { delete suite; }

  GatherWorkspacesTest() {
    // Create the Framework manager so that MPI gets initialized
    API::FrameworkManager::Instance();
  }

  void testTheBasics() {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_EQUALS(gatherer.name(), "GatherWorkspaces");
    TS_ASSERT_EQUALS(gatherer.version(), 1);
    TS_ASSERT_EQUALS(gatherer.category(), "MPI");

    TS_ASSERT_THROWS_NOTHING(gatherer.initialize());
    TS_ASSERT(gatherer.isInitialized());
  }

  void testRootMustHaveInputWorkspace() {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_THROWS_NOTHING(gatherer.initialize());
    TS_ASSERT_THROWS_NOTHING(
        gatherer.setProperty("OutputWorkspace", "something"));
    // Haven't set InputWorkspace and this will be the root process, so it
    // should complain
    TS_ASSERT_THROWS(gatherer.execute(), std::runtime_error);
    TS_ASSERT(!gatherer.isExecuted());
  }

  void testExecute() {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_THROWS_NOTHING(gatherer.initialize());
    // Create a small workspace
    API::MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspace154(1, 5);

    TS_ASSERT_THROWS_NOTHING(gatherer.setProperty("InputWorkspace", inWS));
    gatherer.setChild(
        true); // Make a child algorithm to keep the result out of the ADS

    TS_ASSERT(gatherer.execute());
    API::MatrixWorkspace_const_sptr outWS =
        gatherer.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->size(), outWS->size());
    TS_ASSERT_EQUALS(inWS->x(0), outWS->x(0));
    TS_ASSERT_EQUALS(inWS->y(0), outWS->y(0));
    TS_ASSERT_EQUALS(inWS->e(0), outWS->e(0));

    TS_ASSERT_EQUALS(inWS->getInstrument()->baseInstrument(),
                     outWS->getInstrument()->baseInstrument());
  }

  void testEvents() {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_THROWS_NOTHING(gatherer.initialize());
    // Create a small workspace
    DataObjects::EventWorkspace_sptr inWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 5,
                                                                        true);

    TS_ASSERT_THROWS_NOTHING(gatherer.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(gatherer.setProperty("PreserveEvents", true));
    gatherer.setChild(
        true); // Make a child algorithm to keep the result out of the ADS

    TS_ASSERT(gatherer.execute());
    API::MatrixWorkspace_const_sptr outWS =
        gatherer.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->size(), outWS->size());
    TS_ASSERT_EQUALS(inWS->x(0), outWS->x(0));
    TS_ASSERT_EQUALS(inWS->y(0), outWS->y(0));
    TS_ASSERT_EQUALS(inWS->e(0), outWS->e(0));

    TS_ASSERT_EQUALS(inWS->getInstrument()->baseInstrument(),
                     outWS->getInstrument()->baseInstrument());
  }

  // TODO: Work out a way of testing under MPI because absent that the test is
  // not very interesting
};

#endif /*GATHERWORKSPACESTEST_H_*/
