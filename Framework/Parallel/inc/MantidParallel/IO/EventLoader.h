#ifndef MANTID_PARALLEL_EVENTLOADER_H_
#define MANTID_PARALLEL_EVENTLOADER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Types {
namespace Event {
class TofEvent;
}
} // namespace Types
namespace Parallel {
class Communicator;
namespace IO {

/** Loader for event data from Nexus files with parallelism based on multiple
  processes (MPI) for performance.

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
namespace EventLoader {
MANTID_PARALLEL_DLL std::unordered_map<int32_t, size_t>
makeAnyEventIdToBankMap(const std::string &filename,
                        const std::string &groupName,
                        const std::vector<std::string> &bankNames);
MANTID_PARALLEL_DLL void
load(const Communicator &communicator, const std::string &filename,
     const std::string &groupName, const std::vector<std::string> &bankNames,
     const std::vector<int32_t> &bankOffsets,
     std::vector<std::vector<Types::Event::TofEvent> *> eventLists);
} // namespace EventLoader

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_EVENTLOADER_H_ */
