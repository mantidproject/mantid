// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
                                                 size_t pixelsCount)
    : EventsListsShmemManager(segmentName, elName, 0) {
  try {
    boost::interprocess::permissions perm;
    perm.set_unrestricted();
    m_segment = std::make_unique<ip::managed_shared_memory>(
        ip::create_only, m_segmentName.c_str(), size, nullptr, perm);
    m_allocatorInstance =
        std::make_unique<VoidAllocator>(m_segment->get_segment_manager());
    m_chunks = m_segment->construct<Chunks>(m_chunksName.c_str())(
        chunksCnt, EventLists(pixelsCount, EventList(alloc()), alloc()),
        alloc());
  } catch (std::exception const &ex) {
    std::cout << ex.what() << "\n";
    std::rethrow_if_nested(ex);
  }
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
