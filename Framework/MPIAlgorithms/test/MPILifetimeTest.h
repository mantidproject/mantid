// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidMPIAlgorithms/BroadcastWorkspace.h"
#include "MantidMPIAlgorithms/GatherWorkspaces.h"
#include "MantidMPIAlgorithms/MPILifetime.h"

#include <cstdlib>
#include <mpi.h>

using namespace Mantid;

namespace {
bool isMPIInitialized() {
  int initialized = 0;
  MPI_Initialized(&initialized);
  return initialized != 0;
}

void finalizeMPIAtExit() {
  int finalized = 0;
  MPI_Finalized(&finalized);
  if (!finalized)
    MPI_Finalize();
}
} // namespace

/** MPI can only be initialized once per process, so this suite has its own test executable and its tests depend on
 * running in the order they are written.
 */
class MPILifetimeTest : public CxxTest::TestSuite {
public:
  static MPILifetimeTest *createSuite() {
    // Loads the plugins, which must not initialize MPI
    API::FrameworkManager::Instance();
    return new MPILifetimeTest();
  }

  static void destroySuite(MPILifetimeTest *suite) { delete suite; }

  void test_creating_algorithms_does_not_initialize_MPI() {
    TS_ASSERT(!isMPIInitialized());

    MPIAlgorithms::BroadcastWorkspace broadcaster;
    TS_ASSERT_THROWS_NOTHING(broadcaster.initialize());
    MPIAlgorithms::GatherWorkspaces gatherer;
    TS_ASSERT_THROWS_NOTHING(gatherer.initialize());

    TS_ASSERT(!isMPIInitialized());
  }

  void test_MPI_initialized_elsewhere_is_left_to_its_owner() {
    // Mimic mpi4py, which initializes MPI on import and finalizes it in an atexit handler
    int provided = 0;
    TS_ASSERT_EQUALS(MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided), MPI_SUCCESS);
    std::atexit(finalizeMPIAtExit);

    TS_ASSERT(!MPIAlgorithms::ensureMPIInitialized());
    // Repeated calls are harmless
    TS_ASSERT(!MPIAlgorithms::ensureMPIInitialized());
    TS_ASSERT(isMPIInitialized());
  }
};
