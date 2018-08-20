
#include <MantidParallel/IO/EventsListsShmemManager.h>

#include "MantidParallel/IO/EventsListsShmemManager.h"
#include "MantidTypes/Event/TofEvent.h"

namespace Mantid {
namespace Parallel {

EventsListsShmemManager::EventsListsShmemManager(const std::string &segmentName,
                                                 const std::string &elName)
    : m_segmentName(segmentName), m_eventListsName(elName),
      m_eventLists(nullptr) {
  m_segment = std::make_shared<ip::managed_shared_memory>(
      ip::open_only, m_segmentName.c_str());
  m_allocatorInstance =
      std::make_shared<VoidAllocator>(m_segment->get_segment_manager());
  m_eventLists = m_segment->find<EventLists>(m_eventListsName.c_str()).first;
  if (!m_eventLists)
    throw("No event lists found.");
  std::cout << (*this) << "\n";
}

EventsListsShmemManager::~EventsListsShmemManager() {}

EventsListsShmemManager::EventsListsShmemManager(const std::string &segmentName,
                                                 const std::string &elName, int)
    : m_segmentName(segmentName), m_eventListsName(elName),
      m_eventLists(nullptr) {}

void EventsListsShmemManager::AppendEvent(std::size_t listN,
                                          const Types::Event::TofEvent &event) {
  if (!m_eventLists)
    throw ("No event lists found.");
  auto &list = m_eventLists->at(listN);
  list.emplace_back(event);
}

std::ostream &operator<<(std::ostream &os,
                         const EventsListsShmemManager &manager) {
  os << "m_segmentName: " << manager.m_segmentName
     << " m_eventListsName: " << manager.m_eventListsName << "\n";

  for (auto &list : *manager.m_eventLists) {
    for (auto &event : list)
      os << event.tof() << "\n";
  }
  os << "\n\n";

  return os;
}

} // namespace Parallel
} // namespace Mantid
