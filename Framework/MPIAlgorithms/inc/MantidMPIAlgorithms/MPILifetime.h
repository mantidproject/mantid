// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMPIAlgorithms/DllConfig.h"

namespace Mantid::MPIAlgorithms {

/** Initialize MPI on first use rather than when the plugin library is loaded.
 *
 * Loading the plugin (e.g. by importing mantid) must not start MPI. If MPI is already running, for example because
 * mpi4py was imported first, it is left alone and its owner remains responsible for finalizing it. Otherwise MPI is
 * initialized here and finalized when the process exits. Safe to call repeatedly and from multiple threads.
 *
 * @return true if Mantid initialized MPI and will finalize it
 * @throws std::runtime_error if MPI has already been finalized
 */
MANTID_MPIALGORITHMS_DLL bool ensureMPIInitialized();

} // namespace Mantid::MPIAlgorithms
