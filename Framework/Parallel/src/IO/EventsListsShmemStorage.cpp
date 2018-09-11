#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidTypes/Event/TofEvent.h"
#include <memory>

namespace Mantid {
namespace Parallel {
namespace IO {

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
    std::rethrow_if_nested(ex);
  }
}

EventsListsShmemStorage::~EventsListsShmemStorage() {
  if (destroyShared)
    ip::shared_memory_object::remove(m_segmentName.c_str());
}

void EventsListsShmemStorage::reserve(std::size_t chunkN, std::size_t pixelN, std::size_t size) {
  m_chunks->at(chunkN)[pixelN].reserve(size);
}

std::ostream &operator<<(std::ostream &os,
                         const EventsListsShmemStorage &storage) {
  os << static_cast<const EventsListsShmemManager &>(storage);
  return os;
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
