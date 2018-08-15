#include <memory>
#include "MantidParallel/IO/EventsListsShmemStorage.h"

namespace Mantid {
namespace Parallel {

EventsListsShmemStorage::EventsListsShmemStorage(const std::string& segmentName, const std::string& elName, size_t size)
    : EventsListsShmemManager(segmentName, elName, 0), m_segment(ip::create_only, m_segmentName.c_str(), size)
{
  m_allocatorInstance = std::make_shared<VoidAllocator >(m_segment.get_segment_manager());
  m_eventLists = m_segment.construct<EventLists>(m_eventListsName.c_str())(*m_allocatorInstance.get());
}

EventsListsShmemStorage::~EventsListsShmemStorage() {
  m_segment.destroy<EventLists>(m_eventListsName.c_str());
  ip::shared_memory_object::remove(m_segmentName.c_str());
}

} // namespace Parallel
} // namespace Mantid
