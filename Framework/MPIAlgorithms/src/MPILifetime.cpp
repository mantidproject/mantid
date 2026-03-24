// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <mpi.h>

namespace {
struct MPILifetime {
  MPILifetime() {
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (!initialized)
      MPI_Init(nullptr, nullptr);
  }
  ~MPILifetime() {
    // dlclose is never called by Mantid so this never runs,
    // but MPI_Finalize is safe to call at process exit anyway
    int finalized = 0;
    MPI_Finalized(&finalized);
    if (!finalized)
      MPI_Finalize();
  }
};
static MPILifetime mpi_lifetime;
} // namespace
