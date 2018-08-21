#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidTypes/Event/TofEvent.h"
#include <memory>

namespace Mantid {
namespace Parallel {

EventsListsShmemStorage::EventsListsShmemStorage(const std::string &segmentName,
                                                 const std::string &elName,
                                                 size_t size, size_t chunksCnt,
                                                 size_t pixelsCount,
                                                 bool destroy)
    : EventsListsShmemManager(segmentName, elName, 0), destroyShared(destroy) {
  try {
    m_segment = std::make_shared<ip::managed_shared_memory>(
        ip::create_only, m_segmentName.c_str(), size);
    m_allocatorInstance =
        std::make_shared<VoidAllocator>(m_segment->get_segment_manager());
    m_chunks = m_segment->construct<Chunks>(m_chunksName.c_str())(
        chunksCnt, EventLists(pixelsCount, EventList(alloc()), alloc()),
        alloc());
  } catch (std::exception const &ex) {
    std::cout << "Can't create the EventsListsShmemStorage: " << ex.what()
              << std::endl;
    std::rethrow_if_nested(ex);
  }

  for (auto &chunk : *m_chunks)
    for (auto &list : chunk)
      list.reserve(10000);
}

EventsListsShmemStorage::~EventsListsShmemStorage() {
  if (destroyShared)
    ip::shared_memory_object::remove(m_segmentName.c_str());
}

std::ostream &operator<<(std::ostream &os,
                         const EventsListsShmemStorage &storage) {
  os << static_cast<const EventsListsShmemManager &>(storage);
  return os;
}

} // namespace Parallel
} // namespace Mantid
