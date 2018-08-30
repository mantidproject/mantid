#include <MantidParallel/IO/EventsListsShmemManager.h>
#include <random>
#include <sstream>

#include "MantidParallel/IO/EventsListsShmemManager.h"
#include "MantidTypes/Event/TofEvent.h"

namespace Mantid {
namespace Parallel {
namespace IO {

EventsListsShmemManager::EventsListsShmemManager(const std::string &segmentName,
                                                 const std::string &elName)
    : m_segmentName(segmentName), m_chunksName(elName), m_chunks(nullptr) {
  m_segment = std::make_shared<ip::managed_shared_memory>(
      ip::open_only, m_segmentName.c_str());
  m_allocatorInstance =
      std::make_shared<VoidAllocator>(m_segment->get_segment_manager());
  m_chunks = m_segment->find<Chunks>(m_chunksName.c_str()).first;
  if (!m_chunks)
    throw ("No event lists found.");
}

EventsListsShmemManager::~EventsListsShmemManager() {}

EventsListsShmemManager::EventsListsShmemManager(const std::string &segmentName,
                                                 const std::string &elName, int)
    : m_segmentName(segmentName), m_chunksName(elName), m_chunks(nullptr) {}

void EventsListsShmemManager::AppendEvent(std::size_t chunkN, std::size_t listN,
                                          const Types::Event::TofEvent &event) {
  if (!m_chunks)
    throw ("No event lists found.");
  auto &list = m_chunks->at(chunkN).at(listN);
  list.emplace_back(event);
}

void EventsListsShmemManager::appendEventsRandomly(
    std::size_t nE, unsigned nP, unsigned chunkId,
    EventsListsShmemManager &mngr) {
  std::random_device rand_dev;
  std::mt19937 generator(rand_dev());
  std::uniform_int_distribution<int> distr(0, nP - 1);

  for (unsigned i = 0; i < nE; ++i) {
    mngr.AppendEvent(chunkId, distr(generator),
                     Types::Event::TofEvent(i + 0.5));
  }
}

void EventsListsShmemManager::appendEventsDeterm(
    std::size_t nE, unsigned nP, unsigned chunkId,
    EventsListsShmemManager &mngr) {
  for (unsigned i = 0; i < nE; ++i) {
    mngr.AppendEvent(chunkId, i % nP, Types::Event::TofEvent(i));
  }
}

std::ostream &operator<<(std::ostream &os,
                         const EventsListsShmemManager &manager) {
  os << "m_segmentName: " << manager.m_segmentName
     << " m_eventListsName: " << manager.m_chunksName << "\n";

  for (auto &chunk : *manager.m_chunks) {
    std::stringstream ss;
    for (auto &list : chunk) {
      ss << "[ ";
      for (auto &event : list)
        ss << event.tof() << ", ";
      ss << "]\n";
    }
    os << ss.str() << "\v";
  }
  os << "\n\n";

  return os;
}

const VoidAllocator &EventsListsShmemManager::alloc() const {
  return *m_allocatorInstance.get();
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
