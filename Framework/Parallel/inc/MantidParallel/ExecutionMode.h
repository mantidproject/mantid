// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/StorageMode.h"

#include <string>

namespace Mantid {
namespace Parallel {

/** Execution mode used for an Algorithm in an MPI build.

  Invalid: Indicates a state where execution is not possible.
  Serial: Serial execution (non-MPI build or MPI build with single rank).
  Identical: Independent execution in the same way on each rank.
  Distributed: Distributed execution, may involve communication.
  MasterOnly: Execution only on the master rank.

  @author Simon Heybrock
  @date 2017
*/
enum class ExecutionMode { Invalid, Serial, Identical, Distributed, MasterOnly };

MANTID_PARALLEL_DLL ExecutionMode getCorrespondingExecutionMode(StorageMode storageMode);

MANTID_PARALLEL_DLL std::string toString(ExecutionMode mode);

} // namespace Parallel
} // namespace Mantid
