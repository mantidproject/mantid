#ifndef MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_
#define MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_


#include <string>
#include <vector>
//#include "H5Cpp.h"

#include "MantidParallel/IO/EventLoaderHelpers.h"
#include "MantidParallel/IO/EventsListsShmemStorage.h"

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
  MultiProcessEventLoader(unsigned int numPixels, unsigned int numProcesses,
                          unsigned int numThreads, const std::string &binary);
  void
  load(const std::string &filename, const std::string &groupname,
       const std::vector<std::string> &bankNames,
       const std::vector<int32_t> &bankOffsets,
       std::vector<std::vector<Types::Event::TofEvent> *> eventLists) const;

  static void fillFromFile(EventsListsShmemStorage &storage,
                           const std::string &filename,
                           const std::string &groupname,
                           const std::vector<std::string> &bankNames,
                           const std::vector<int32_t> &bankOffsets,
                           unsigned from, unsigned to);

private:
  static std::vector<std::string> GenerateSegmentsName(unsigned procNum);
  static std::string GenerateStoragename();
  static std::string GenerateTimeBasedPrefix();

  template<typename T>
  static void loadFromGroup(EventsListsShmemStorage &storage,
                            const H5::Group &group,
                            const std::vector<std::string> &bankNames,
                            const std::vector<int32_t> &bankOffsets,
                            unsigned from, unsigned to);

  void assembleFromShared(
      std::vector<std::vector<Mantid::Types::Event::TofEvent> *> &result) const;

private:
  unsigned m_numPixels;
  unsigned m_numProcesses;
  unsigned m_numThreads;
  std::string m_binaryToLaunch;
  std::vector<std::string> m_segmentNames;
  std::string m_storageName;
};

template<typename T>
void MultiProcessEventLoader::loadFromGroup(
    EventsListsShmemStorage &storage, const H5::Group &instrument,
    const std::vector<std::string> &bankNames,
    const std::vector<int32_t> &bankOffsets, unsigned from, unsigned to) {
  std::vector<int32_t> eventId;
  std::vector<T> eventTimeOffset;

  std::size_t eventCounter{0};
  bool isLastBank = false;
  auto bankSizes = EventLoader::readBankSizes(instrument, bankNames);
  IO::NXEventDataLoader<T> loader(1, instrument, bankNames);
  for (unsigned bankIdx = 0; bankIdx < bankNames.size(); ++bankIdx) {
    auto part = loader.setBankIndex(bankIdx);
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

      std::size_t cnt = finish - start;

      eventTimeOffset.resize(cnt);
      loader.readEventTimeOffset(eventTimeOffset.data(), start, cnt);
      eventId.resize(cnt);
      loader.readEventID(eventId.data(), start, cnt);

      detail::eventIdToGlobalSpectrumIndex(eventId.data(), cnt, bankOffsets[bankIdx]);

      for (unsigned i = 0; i < eventId.size(); ++i) {
        try {
          storage.AppendEvent(0, eventId[i],
                              {(double) eventTimeOffset[i], part->next()});
        } catch (std::exception const &ex) {
          std::cerr << "Can't Append event with id =  " << eventId[i]
                    << "  bankId: " << bankIdx << ": " << ex.what()
                    << "\n";
          std::rethrow_if_nested(ex);
        }
      }
    }
  }
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_ */