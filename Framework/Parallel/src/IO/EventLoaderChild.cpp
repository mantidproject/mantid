#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidTypes/Event/TofEvent.h"
#include <iostream>

using namespace Mantid::Parallel;
using namespace Mantid::Types;

int main(int argc, char **argv)
// argv: shared memory segment name, event list storage name
{
  std::cout << "Nexus loader process started with arguments: ";
  for (unsigned i = 1; i < argc; ++i)
    std::cout << argv[i] << " ";
  std::cout << "\n";

  if (argc != 7)
    return 1;
  const std::string segmentName(argv[1]);
  const std::string storageName(argv[2]);
  unsigned procId = std::atoi(argv[3]);
  std::size_t numEvents = std::atoi(argv[4]);
  unsigned numPixels = std::atoi(argv[5]);
  std::size_t size = std::atoi(argv[6]);

  std::cout << "Constructing loader" << segmentName << " " << storageName << " "
            << procId << " " << numEvents << " " << numPixels << "...";
  EventsListsShmemStorage storage(segmentName, storageName, size, 1, numPixels,
                                  false);
  std::cout << "succeed.\n";

  EventsListsShmemManager::appendEventsDeterm(numEvents, numPixels, 0, storage);

  return 0;
}