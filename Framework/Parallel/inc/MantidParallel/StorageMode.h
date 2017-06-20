#ifndef MANTID_PARALLEL_STORAGEMODE_H_
#define MANTID_PARALLEL_STORAGEMODE_H_

#include "MantidParallel/DllConfig.h"

#include <map>
#include <string>

namespace Mantid {
namespace Parallel {

/** Storage mode used for a Workspace in an MPI build.

  Cloned: There is a copy (clone) of the Workspace on each rank.
  Distributed: Each rank holds parts of the Workspace (spectra).
  MasterOnly: The master/root rank has the Workspace.

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
enum class StorageMode { Cloned, Distributed, MasterOnly };

MANTID_PARALLEL_DLL std::string toString(StorageMode mode);
MANTID_PARALLEL_DLL std::string
toString(const std::map<std::string, StorageMode> &map);

} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_STORAGEMODE_H_ */
