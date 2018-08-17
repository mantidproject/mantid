#include <memory>
#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidTypes/Event/TofEvent.h"

namespace Mantid {
namespace Parallel {

EventsListsShmemStorage::EventsListsShmemStorage(const std::string& segmentName, const std::string& elName, size_t size)
    : EventsListsShmemManager(segmentName, elName, 0), m_segment(ip::create_only, m_segmentName.c_str(), size)
{
  m_allocatorInstance = std::make_shared<VoidAllocator >(m_segment.get_segment_manager());
  m_eventLists = m_segment.construct<EventLists>(m_eventListsName.c_str())(*m_allocatorInstance.get());
  for (unsigned i = 0; i < 10; ++i) {
    m_eventLists->emplace_back(*m_allocatorInstance.get());
    m_eventLists->rbegin()->mutex =
        m_segment.construct<ip::interprocess_mutex>(("_mutex" + std::to_string(i)).c_str())();
  }

}

EventsListsShmemStorage::~EventsListsShmemStorage() {
  m_segment.destroy<EventLists>(m_eventListsName.c_str());
  ip::shared_memory_object::remove(m_segmentName.c_str());
}

std::ostream &operator<<(std::ostream &os,
                         const EventsListsShmemStorage &storage) {
  os << static_cast<const EventsListsShmemManager &>(storage);
  return os;
}

} // namespace Parallel
} // namespace Mantid
