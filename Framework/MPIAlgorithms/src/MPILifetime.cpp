// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMPIAlgorithms/MPILifetime.h"

#include <mpi.h>
#include <stdexcept>

namespace Mantid::MPIAlgorithms {
namespace {
struct MPILifetime {
  bool m_owned = false;
  MPILifetime() {
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (initialized) {
      int finalized = 0;
      MPI_Finalized(&finalized);
      if (finalized)
        throw std::runtime_error("MPI has already been finalized");
      return; // e.g. mpi4py got there first: it owns MPI_Finalize
    }
    int provided = 0;
    // Request the same thread support as mpi4py so the order of initialization does not matter
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
    m_owned = true;
  }
  ~MPILifetime() {
    int finalized = 0;
    MPI_Finalized(&finalized);
    if (m_owned && !finalized)
      MPI_Finalize();
  }
};
} // namespace

bool ensureMPIInitialized() {
  // A function-local static is initialized once, thread-safely, and is retried if the constructor throws
  static MPILifetime lifetime;
  return lifetime.m_owned;
}

} // namespace Mantid::MPIAlgorithms
