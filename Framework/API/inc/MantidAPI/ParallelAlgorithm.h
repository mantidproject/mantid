#ifndef MANTID_API_PARALLELALGORITHM_H_
#define MANTID_API_PARALLELALGORITHM_H_

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
class MANTID_API_DLL ParallelAlgorithm : public API::Algorithm {
protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_PARALLELALGORITHM_H_ */
