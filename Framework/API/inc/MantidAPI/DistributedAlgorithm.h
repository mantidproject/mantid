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

/** Base class for algorithms that treat all spectra independently, i.e., we can
  trivially parallelize over the spectra without changes. The assumption is that
  we have one input and one output workspace. The storage mode is just
  propagated from input to output. When a specific algorithm is determined to be
  trivially parallel (this is a manual process), the only required change to add
  MPI support is to inherit from this class instead of Algorithm. Inheriting
  from DistributedAlgorithm instead of from Algorithm provides the necessary
  overriden method(s) to allow running an algorithm with MPI. This works under
  the following conditions:
  1. The algorithm must input workspaces with compatible storage modes.
  StorageMode::Distributed is not compatible with StorageMode::MasterOnly, but
  all combinations with StorageMode::Cloned are considered compatible.
  2. No output files may be written since filenames would clash.
  Algorithms that do not modify spectra in a workspace may also use this base
  class to support MPI. For example, modifications of the instrument are handled
  in a identical manner on all MPI ranks, without requiring changes to the
  algorithm, other than setting the correct execution mode via the overloads
  provided by DistributedAlgorithm.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_API_DLL DistributedAlgorithm : public Algorithm {
protected:
  Parallel::ExecutionMode
  getParallelExecutionMode(const std::map<std::string, Parallel::StorageMode> &storageModes) const override;
};

} // namespace API
} // namespace Mantid
