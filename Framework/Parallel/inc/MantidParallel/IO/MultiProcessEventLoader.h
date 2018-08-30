#ifndef MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_
#define MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_

#include <vector>
#include <string>
#include <H5Cpp.h>

#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidParallel/IO/EventLoaderHelpers.h"

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Parallel {
namespace IO {

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
  static void fillFromFile(EventsListsShmemStorage &storage,
                           const std::string &filename,
                           const std::string &groupname,
                           const std::vector<std::string> &bankNames,
                           const std::vector<int32_t> &bankOffsets,
                           unsigned from, unsigned to);
  template<typename T>
  static void loadFromGroup(EventsListsShmemStorage &storage,
                            const H5::Group &group,
                            const std::vector<std::string> &bankNames,
                            const std::vector<int32_t> &bankOffsets,
                            unsigned from, unsigned to
  );

  void assembleFromShared(std::vector<std::vector<Mantid::Types::Event::TofEvent> *> &result) const;
private:
  unsigned numPixels;
  unsigned numProcesses;
  unsigned numThreads;
  std::vector<std::string> segmentNames;
  std::string storageName;
};

template<typename T>
void MultiProcessEventLoader::loadFromGroup(EventsListsShmemStorage &storage,
                                            const H5::Group &instrument,
                                            const std::vector<std::string> &bankNames,
                                            const std::vector<int32_t> &bankOffsets,
                                            unsigned from, unsigned to
) {
  std::vector<int32_t> eventId;
  std::vector<int32_t> eventTimeOffset;

  std::size_t eventCounter{0};
  bool isLastBank = false;
  auto bankSizes = EventLoader::readBankSizes(instrument, bankNames);
  IO::NXEventDataLoader<T> loader(1, instrument, bankNames);
  for (unsigned bankIdx = 0; bankIdx < bankNames.size(); ++bankIdx) {
    auto part = loader.SetBankIndex(bankIdx);
    auto count = bankSizes[bankIdx];

    if (isLastBank)
      break;

    bool isFirstBank = (eventCounter < from);
    eventCounter += count;

    if (eventCounter > from) {
      size_t start{0};
      size_t finish{count};
      if (isFirstBank) {
        start = from - eventCounter + count;
      } else if (eventCounter > to) {
        finish = to - eventCounter + count;
        isLastBank = true;
      }

      auto cnt = finish - start;
      loader.readEventTimeOffset(eventTimeOffset, start, cnt);
      loader.readEventID(eventId, start, cnt);

      auto &eventIndex = loader.eventIndex();
      auto &eventTimeZero = loader.eventTimeZero();

      std::size_t eventIdOffset = bankOffsets[bankIdx];

      detail::eventIdToGlobalSpectrumIndex(eventId.data(), cnt, eventIdOffset);

      size_t start_pulse = 0;
      size_t end_pulse = 0;
      for (size_t pulse = 0; pulse < eventIndex.size(); ++pulse) {
        size_t count =
            (pulse != eventIndex.size() - 1 ? eventIndex[pulse + 1]
                                            : eventTimeOffset.size()) -
                eventIndex[pulse];

        if (start >= eventIndex[pulse] && start < eventIndex[pulse] + count)
          start_pulse = pulse;

        if (finish > eventIndex[pulse] && finish <= eventIndex[pulse] + count)
          end_pulse = pulse + 1;
      }

      for (size_t pulse = start_pulse; pulse < end_pulse; ++pulse) {
        size_t begin = std::max<unsigned>(start, eventIndex[pulse]) - start;

        size_t end =
            std::min<unsigned>(finish, (pulse != eventIndex.size() - 1
                                        ? eventIndex[pulse + 1]
                                        : eventTimeOffset.size())) -
                start;

        for (size_t i = begin; i < end; ++i) {
          auto id = eventId[i];
          try {
            storage.AppendEvent(
                0, id, {(double) eventTimeOffset[i], eventTimeZero[pulse]});
          } catch (std::exception const &ex) {
            std::cout << "Can't Append event with id =  " << id
                      << "  bankId: " << bankIdx << ": " << ex.what()
                      << std::endl;
            std::rethrow_if_nested(ex);
          }
        }
      }
    }
  }
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_ */