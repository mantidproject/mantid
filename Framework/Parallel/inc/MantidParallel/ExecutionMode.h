#ifndef MANTID_PARALLEL_EXECUTIONMODE_H_
#define MANTID_PARALLEL_EXECUTIONMODE_H_

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
enum class ExecutionMode {
  Invalid,
  Serial,
  Identical,
  Distributed,
  MasterOnly
};

MANTID_PARALLEL_DLL ExecutionMode
getCorrespondingExecutionMode(StorageMode storageMode);

MANTID_PARALLEL_DLL std::string toString(ExecutionMode mode);

} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_EXECUTIONMODE_H_ */
