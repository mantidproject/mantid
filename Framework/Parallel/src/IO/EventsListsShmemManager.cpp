#include "MantidParallel/IO/EventsListsShmemManager.h"

namespace Mantid {
namespace Parallel {

EventsListsShmemManager::EventsListsShmemManager(const std::string &segmentName,
                                                 const std::string &elName)
    : m_segmentName(segmentName), m_eventListsName (elName), m_eventLists(nullptr) {
  ip::managed_shared_memory segment(ip::open_only, m_segmentName.c_str());
  m_allocatorInstance = std::make_shared<VoidAllocator >(segment.get_segment_manager());
  m_eventLists = segment.find<EventLists>(m_eventListsName.c_str()).first;
  if(!m_eventLists)
    throw("No event lists found.");
}

EventsListsShmemManager::EventsListsShmemManager(const std::string &segmentName,
                                                 const std::string &elName, int)
    : m_segmentName(segmentName), m_eventListsName (elName), m_eventLists(nullptr) {}

} // namespace Parallel
} // namespace Mantid
