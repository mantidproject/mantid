#ifndef MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_
#define MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_

#include <vector>
#include <string>

#include "MantidParallel/IO/EventsListsShmemStorage.h"

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Parallel {

/** MultiProcessEventLoader : TODO: DESCRIPTION

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_PARALLEL_DLL MultiProcessEventLoader {
public:

private:
public:
  MultiProcessEventLoader(unsigned int numPixels, unsigned int numProcesses, unsigned int numThreads);
  void load(const std::string &filename,
            const std::string &groupName,
            const std::vector<std::string> &bankNames,
            const std::vector<int32_t> &bankOffsets,
            std::vector<std::vector<Types::Event::TofEvent> *> eventLists) const;
private:
  static std::vector<std::string> GenerateSegmentsName(unsigned procNum);
  static std::string GenerateStoragename();
  static std::string GenerateTimeBasedPrefix();

  void assembleFromShared(std::vector<std::vector<Mantid::Types::Event::TofEvent> *> &result) const;
private:
  unsigned numPixels;
  unsigned numProcesses;
  unsigned numThreads;
  std::vector<std::string> segmentNames;
  std::string storageName;
};

} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_ */