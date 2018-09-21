#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidTypes/Event/TofEvent.h"
#include <memory>

namespace Mantid {
namespace Parallel {
namespace IO {

/// Constructor
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
    std::cout << ex.what() << "\n";
    std::rethrow_if_nested(ex);
  }
}

/// Destructor
EventsListsShmemStorage::~EventsListsShmemStorage() {
  if (destroyShared)
    ip::shared_memory_object::remove(m_segmentName.c_str());
}

/// Reserves memory for ToF events in given pixel and chunk
void EventsListsShmemStorage::reserve(std::size_t chunkN, std::size_t pixelN,
                                      std::size_t size) {
  assert(m_chunks);
  if (chunkN >= m_chunks->size())
    throw std::invalid_argument(std::string("Number of chunks is ") +
        std::to_string(m_chunks->size()) +
        ", asked for index " + std::to_string(chunkN));

  if (pixelN >= m_chunks->at(chunkN).size())
    throw std::invalid_argument(std::string("Number of pixels is ") +
        std::to_string(m_chunks->at(chunkN).size()) +
        ", asked for index " + std::to_string(pixelN));

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
