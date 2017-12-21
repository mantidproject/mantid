#ifndef MANTID_API_DISTRIBUTEDALGORITHM_H_
#define MANTID_API_DISTRIBUTEDALGORITHM_H_

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

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL DistributedAlgorithm : public Algorithm {
protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_DISTRIBUTEDALGORITHM_H_ */
