#ifndef MANTID_API_SERIALALGORITHM_H_
#define MANTID_API_SERIALALGORITHM_H_

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
class MANTID_API_DLL SerialAlgorithm : public API::Algorithm {
protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SERIALALGORITHM_H_ */
