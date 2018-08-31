#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidParallel/IO/MultiProcessEventLoader.h"
#include "MantidTypes/Event/TofEvent.h"

using namespace Mantid::Parallel::IO;
using namespace Mantid::Types;

int main(int argc, char **argv) {
  const std::string segmentName(argv[1]);
  const std::string storageName(argv[2]);
  unsigned procId = std::atoi(argv[3]);
  std::size_t firstEvent = std::atoll(argv[4]);
  std::size_t upperEvent = std::atoll(argv[5]);
  unsigned numPixels = std::atoi(argv[6]);
  std::size_t size = std::atoll(argv[7]);
  const std::string fileName(argv[8]);
  const std::string groupName(argv[9]);

  std::vector<std::string> bankNames;
  std::vector<int32_t> bankOffsets;
  for (unsigned i = 10; i < argc; i += 2) {
    bankNames.emplace_back(argv[i]);
    bankOffsets.emplace_back(std::atoi(argv[i + 1]));
  }

  EventsListsShmemStorage storage(segmentName, storageName, size, 1, numPixels,
                                  false);

  MultiProcessEventLoader::fillFromFile(storage, fileName, groupName, bankNames,
                                        bankOffsets, firstEvent, upperEvent);

  return 0;
}