// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidMPIAlgorithms/GatherWorkspaces.h"

using namespace Mantid;

class GatherWorkspacesTest : public CxxTest::TestSuite {
public:
  static GatherWorkspacesTest *createSuite() { return new GatherWorkspacesTest(); }
  static void destroySuite(GatherWorkspacesTest *suite) { delete suite; }

private:
  boost::mpi::environment env;

public:
  GatherWorkspacesTest() { API::FrameworkManager::Instance(); }

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
    TS_ASSERT_THROWS_NOTHING(gatherer.setProperty("OutputWorkspace", "something"));
    TS_ASSERT_THROWS(gatherer.execute(), const std::runtime_error &);
    TS_ASSERT(!gatherer.isExecuted());
  }

  void testExecute() {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_THROWS_NOTHING(gatherer.initialize());
    API::MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace154(1, 5);
    TS_ASSERT_THROWS_NOTHING(gatherer.setProperty("InputWorkspace", inWS));
    gatherer.setChild(true);

    TS_ASSERT(gatherer.execute());
    API::MatrixWorkspace_const_sptr outWS = gatherer.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->size(), outWS->size());
    for (int i = 0; i < 5; ++i) {
      TS_ASSERT_EQUALS(inWS->readX(0)[i], outWS->readX(0)[i]);
      TS_ASSERT_EQUALS(inWS->readY(0)[i], outWS->readY(0)[i]);
      TS_ASSERT_EQUALS(inWS->readE(0)[i], outWS->readE(0)[i]);
    }

    TS_ASSERT_EQUALS(inWS->getInstrument()->baseInstrument(), outWS->getInstrument()->baseInstrument());
  }

  void testEvents() {
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_THROWS_NOTHING(gatherer.initialize());
    DataObjects::EventWorkspace_sptr inWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 5, true);

    TS_ASSERT_THROWS_NOTHING(gatherer.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(gatherer.setProperty("PreserveEvents", true));
    gatherer.setChild(true);

    TS_ASSERT(gatherer.execute());
    API::MatrixWorkspace_const_sptr outWS = gatherer.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->size(), outWS->size());
    for (int i = 0; i < 5; ++i) {
      printf("i: %d\n", i);
      TS_ASSERT_EQUALS(inWS->readX(0)[i], outWS->readX(0)[i]);
      TS_ASSERT_EQUALS(inWS->readY(0)[i], outWS->readY(0)[i]);
      TS_ASSERT_EQUALS(inWS->readE(0)[i], outWS->readE(0)[i]);
    }

    TS_ASSERT_EQUALS(inWS->getInstrument()->baseInstrument(), outWS->getInstrument()->baseInstrument());
  }
};
