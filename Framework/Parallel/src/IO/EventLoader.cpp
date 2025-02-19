// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidParallel/IO/EventLoader.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidParallel/IO/EventLoaderHelpers.h"
#include "MantidParallel/IO/MultiProcessEventLoader.h"
#include "MantidParallel/IO/NXEventDataLoader.h"

#include <H5Cpp.h>
#include <thread>

namespace Mantid::Parallel::IO::EventLoader {

/** Return a map from any one event ID in a bank to the bank index.
 *
 * For every bank there is one map entry, i.e., this is NOT a mapping from all
 * IDs in a bank to the bank. The returned map will not contain an entry for
 * banks that contain no events. */
std::unordered_map<int32_t, size_t> makeAnyEventIdToBankMap(const std::string &filename, const std::string &groupName,
                                                            const std::vector<std::string> &bankNames) {
  std::unordered_map<int32_t, size_t> idToBank;
  H5::FileAccPropList access_plist;
  access_plist.setFcloseDegree(H5F_CLOSE_STRONG);
  H5::H5File file(filename, H5F_ACC_RDONLY, access_plist);
  H5::Group group = file.openGroup(groupName);
  for (size_t i = 0; i < bankNames.size(); ++i) {
    try {
      int32_t eventId;
      detail::read<int32_t>(&eventId, group, bankNames[i] + "/event_id", 0, 1);
      idToBank[eventId] = i;
    } catch (const std::out_of_range &) {
      // No event in file, do not add to map.
    }
  }
  return idToBank;
}

/// Load events from given banks into event lists.
void load(const std::string &filename, const std::string &groupname, const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets, const std::vector<std::vector<Types::Event::TofEvent> *> &eventLists,
          bool precalcEvents) {
  auto concurencyNumber = PARALLEL_GET_MAX_THREADS;
  auto numThreads = std::max<int>(concurencyNumber / 2, 1);
  auto numProceses = std::max<int>(concurencyNumber / 2, 1);
  std::string executableName = Kernel::ConfigService::Instance().getPropertiesDir() + "MantidNexusParallelLoader";

  MultiProcessEventLoader loader(static_cast<unsigned>(eventLists.size()), numProceses, numThreads, executableName,
                                 precalcEvents);
  loader.load(filename, groupname, bankNames, bankOffsets, eventLists);
}

} // namespace Mantid::Parallel::IO::EventLoader
