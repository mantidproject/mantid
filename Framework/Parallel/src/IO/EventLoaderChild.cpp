#include <iostream>
#include "MantidParallel/IO/EventsListsShmemManager.h"
#include "MantidTypes/Event/TofEvent.h"

using namespace Mantid::Parallel;
using namespace Mantid::Types;

int main(int argc, char **argv)
// argv: shared memory segment name, event list storage name
{
  std::cout << "Nexus loader process started...\n";
  if (argc != 3)
    return 1;
  const std::string segmentName(argv[1]);
  const std::string storageName(argv[2]);

  std::cout << "Constructing loader...";
  EventsListsShmemManager shmemManager(segmentName, storageName);
  std::cout << "succeed.\n";

  for (unsigned i = 0; i < 10; ++i)
    shmemManager.AppendEvent(i, Event::TofEvent(i + 0.5));

  std::cout << shmemManager << "\n";

  return 0;
}