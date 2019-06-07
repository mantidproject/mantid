// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

MANTID_PARALLEL_DLL void
load(const std::string &filename, const std::string &groupName,
     const std::vector<std::string> &bankNames,
     const std::vector<int32_t> &bankOffsets,
     std::vector<std::vector<Types::Event::TofEvent> *> eventLists,
     bool precalcEvents);

} // namespace EventLoader

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_EVENTLOADER_H_ */
