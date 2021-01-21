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

/** Base class for algorithms that can only run serially
  (Parallel::ExecutionMode::MasterOnly) in an MPI run. A prime example are most
  `Save` algorithms, which, since they write to a file, cannot run in parallel
  (Parallel::ExecutionMode::Identical). By default such an algorithm also cannot
  run in a distributed manner (Parallel::ExecutionMode::Distributed) since that
  would require either gathering all data on the master rank or distributed
  writes to the same file.

  When a specific algorithm is determined to be serial (this is a manual
  process), the only required change to add "MPI support" is to inherit from
  this class instead of Algorithm. Inheriting from SerialAlgorithm instead of
  from Algorithm provides the necessary overriden method(s) to allow running an
  algorithm with MPI. This works out of the box if the algorithm has no output
  workspace. If there are output workspaces their storage mode must be set
  correctly in the algorithm.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_API_DLL SerialAlgorithm : public API::Algorithm {
protected:
  Parallel::ExecutionMode
  getParallelExecutionMode(const std::map<std::string, Parallel::StorageMode> &storageModes) const override;
};

} // namespace API
} // namespace Mantid
