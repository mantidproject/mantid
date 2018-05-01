#ifndef MANTID_DATAHANDLING_PARALLELEVENTLOADER_H_
#define MANTID_DATAHANDLING_PARALLELEVENTLOADER_H_

#include <string>
#include <vector>

#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataObjects {
class EventWorkspace;
}
namespace DataHandling {

/** Loader for event data from Nexus files with parallelism based on multiple
  processes (MPI) for performance. This class provides integration of the low
  level loader component Parallel::IO::EventLoader with higher level concepts
  such as DataObjects::EventWorkspace and the instrument.

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
class MANTID_DATAHANDLING_DLL ParallelEventLoader {
public:
  static void load(DataObjects::EventWorkspace &ws, const std::string &filename,
                   const std::string &groupName,
                   const std::vector<std::string> &bankNames,
                   const bool eventIDIsSpectrumNumber);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_PARALLELEVENTLOADER_H_ */
