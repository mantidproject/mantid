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
#include "MantidMPIAlgorithms/BroadcastWorkspace.h"
#include <boost/mpi/environment.hpp>

using namespace Mantid;

class BroadcastWorkspaceTest : public CxxTest::TestSuite {
public:
  static BroadcastWorkspaceTest *createSuite() { return new BroadcastWorkspaceTest(); }
  static void destroySuite(BroadcastWorkspaceTest *suite) { delete suite; }

private:
  boost::mpi::environment env;

public:
  BroadcastWorkspaceTest() {
    // Create the Framework manager so that MPI gets initialized
    API::FrameworkManager::Instance();
  }

  void testTheBasics() {
    MPIAlgorithms::BroadcastWorkspace broadcaster;
    TS_ASSERT_EQUALS(broadcaster.name(), "BroadcastWorkspace");
    TS_ASSERT_EQUALS(broadcaster.version(), 1);
    TS_ASSERT_EQUALS(broadcaster.category(), "MPI");

    TS_ASSERT_THROWS_NOTHING(broadcaster.initialize());
    TS_ASSERT(broadcaster.isInitialized());
  }

  void testRankValidator() {
    MPIAlgorithms::BroadcastWorkspace broadcaster;
    TS_ASSERT_THROWS_NOTHING(broadcaster.initialize());
    TS_ASSERT_THROWS_NOTHING(broadcaster.setPropertyValue("OutputWorkspace", "blah"));
    TS_ASSERT_THROWS(broadcaster.setProperty("BroadcasterRank", 1), const std::invalid_argument &);
  }

  void testExecute() {
    MPIAlgorithms::BroadcastWorkspace broadcaster;
    TS_ASSERT_THROWS_NOTHING(broadcaster.initialize());
    // Create a small workspace
    API::MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace154(1, 5);

    TS_ASSERT_THROWS_NOTHING(broadcaster.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(broadcaster.setPropertyValue("OutputWorkspace", "blah"));
    broadcaster.setChild(true); // Make a child algorithm to keep the result out of the ADS

    TS_ASSERT(broadcaster.execute());
    API::MatrixWorkspace_const_sptr outWS = broadcaster.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->size(), outWS->size());
    for (int i = 0; i < 5; ++i) {
      TS_ASSERT_EQUALS(inWS->readX(0)[i], outWS->readX(0)[i]);
      TS_ASSERT_EQUALS(inWS->readY(0)[i], outWS->readY(0)[i]);
      TS_ASSERT_EQUALS(inWS->readE(0)[i], outWS->readE(0)[i]);
    }

    // TS_ASSERT_EQUALS( inWS->getAxis(0)->unit()->unitID(),
    // outWS->getAxis(0)->unit()->unitID() );
  }

  // TODO: Work out a way of testing under MPI because absent that the test is
  // not very interesting
};
