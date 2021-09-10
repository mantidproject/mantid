// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DllConfig.h"

namespace Mantid {
namespace API {

/** Base class for algorithms that can run in parallel on all MPI ranks but not
  in a distributed fashion. A prime example are most `Load` algorithms, which,
  since they read input data from a file have no automatic way of doing so in a
  distributed manner. Creating an actual distributed workspace
  (Parallel::StorageMode::Distributed) would require a manual implementation
  taking care of setting up a workspace and partitioning it correctly.

  When a specific algorithm is determined to be parallel (this is a manual
  process), the only required change to add MPI support is to inherit from this
  class instead of Algorithm. The algorithm will then support
  Parallel::ExecutionMode::MasterOnly and Parallel::ExecutionMode::Identical,
  provided that the mode can uniquely be determined from its input workspaces.
  If there are no inputs it defaults to Parallel::ExecutionMode::Identical.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_API_DLL ParallelAlgorithm : public API::Algorithm {
protected:
  Parallel::ExecutionMode
  getParallelExecutionMode(const std::map<std::string, Parallel::StorageMode> &storageModes) const override;
};

} // namespace API
} // namespace Mantid
