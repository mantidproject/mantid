// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidParallel/IO/MultiProcessEventLoader.h"
#include "MantidTypes/Event/TofEvent.h"

using namespace Mantid::Parallel::IO;
using namespace Mantid::Types;

int main(int argc, char **argv) {
  const std::string segmentName(argv[1]);
  const std::string storageName(argv[2]);
  //  unsigned procId = std::atoi(argv[3]);
  unsigned firstEvent = std::atoi(argv[4]);
  unsigned upperEvent = std::atoi(argv[5]);
  unsigned numPixels = std::atoi(argv[6]);
  std::size_t size = std::atoll(argv[7]);
  const std::string fileName(argv[8]);
  const std::string groupName(argv[9]);
  const bool precalcEvents = std::atoi(argv[10]);

  std::vector<std::string> bankNames;
  std::vector<int32_t> bankOffsets;
  for (int i = 11; i < argc; i += 2) {
    bankNames.emplace_back(argv[i]);
    bankOffsets.emplace_back(std::atoi(argv[i + 1]));
  }

  EventsListsShmemStorage storage(segmentName, storageName, size, 1, numPixels);
  try {
    MultiProcessEventLoader::fillFromFile(storage, fileName, groupName, bankNames, bankOffsets, firstEvent, upperEvent,
                                          precalcEvents);
  } catch (...) {
    return 1;
  }
  return 0;
}
