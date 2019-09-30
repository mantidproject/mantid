// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidParallel/IO/EventsListsShmemManager.h"
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
  m_segment = std::make_unique<ip::managed_shared_memory>(
      ip::open_only, m_segmentName.c_str());
  m_allocatorInstance =
      std::make_unique<VoidAllocator>(m_segment->get_segment_manager());
  m_chunks = m_segment->find<Chunks>(m_chunksName.c_str()).first;
  if (!m_chunks)
    throw std::invalid_argument("No event lists found.");
}

EventsListsShmemManager::EventsListsShmemManager(const std::string &segmentName,
                                                 const std::string &elName,
                                                 int /*unused*/)
    : m_segmentName(segmentName), m_chunksName(elName), m_chunks(nullptr) {}

/// Appends ToF event to given pixel in given chunk of shared storage
void EventsListsShmemManager::appendEvent(std::size_t chunkN, std::size_t listN,
                                          const Types::Event::TofEvent &event) {
  assert(m_chunks);
  if (chunkN >= m_chunks->size())
    throw std::invalid_argument(std::string("Number of chunks is ") +
                                std::to_string(m_chunks->size()) +
                                ", asked for index " + std::to_string(chunkN));

  if (listN >= m_chunks->at(chunkN).size())
    throw std::invalid_argument(std::string("Number of pixels is ") +
                                std::to_string(m_chunks->at(chunkN).size()) +
                                ", asked for index " + std::to_string(listN));

  auto &list = m_chunks->at(chunkN).at(listN);
  list.emplace_back(event);
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
