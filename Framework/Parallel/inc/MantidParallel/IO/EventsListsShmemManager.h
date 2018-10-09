// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGER_H_
#define MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGER_H_

#include "boost/interprocess/containers/vector.hpp"
#include "boost/interprocess/managed_shared_memory.hpp"
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <ostream>

#include "MantidParallel/DllConfig.h"

namespace ip = boost::interprocess;

namespace Mantid {
namespace Types {
namespace Event {
class TofEvent;
}
} // namespace Types

namespace Parallel {
namespace IO {

/** EventsListsShmemManager : Operates with event list in shared memory in
 * multiprocess environment; NOT an !!!OWNER of shared memory!!!
 * Initially the plan had been to crteate all shared memory segments in
 * main process in RAII style, but in the modern linux implementation you
 * can't create more than 1 NAMED shared memory segment in single process,
 * but we should use NAMED shared memory to fit other OS needs,
 * so in current implementation all segments are created by child processes
 * and destroyed by parent. This may be fixed in the future, then this class,
 * may be used for operating on segments from child processes.
 * EventsListsShmemManager base class for EventsListsShmemStorage,
 * that is the owner. Structure of storage:
 *    chunk_0 |pixel_0|   chunk_1 |pixel_0| ... chunk_N |pixel_0|
 *            |pixel_1|           |pixel_1|      ...    |pixel_1|
 *            ... ... ... ... ... ... ... ... ... ... ... ... ...
 *            |pixel_M|           |pixel_M|      ...    |pixel_M|
 *
 * Every chunk can partially store events for every pixel.

  @author Igor Gudich
  @date 2018
*/

using SegmentManager = ip::managed_shared_memory::segment_manager;
using VoidAllocator = ip::allocator<void, SegmentManager>;
using TofEventAllocator = ip::allocator<Types::Event::TofEvent, SegmentManager>;
using EventList = ip::vector<Types::Event::TofEvent, TofEventAllocator>;
using EventListAllocator = ip::allocator<EventList, SegmentManager>;
using EventLists = ip::vector<EventList, EventListAllocator>;
using EventListsAllocator = ip::allocator<EventLists, SegmentManager>;
using Chunks = ip::vector<EventLists, EventListsAllocator>;

class MANTID_PARALLEL_DLL EventsListsShmemManager {
public:
  // Constructor for client usage: "sets" Manager to the piece of shared memory
  // with existed GuardedEventLists in it
  EventsListsShmemManager(const std::string &segmentName,
                          const std::string &elName);

  virtual ~EventsListsShmemManager() = default;

  void appendEvent(std::size_t chunkN, std::size_t listN,
                   const Types::Event::TofEvent &event);
  template <typename InIter>
  void appendEvent(std::size_t chunkN, std::size_t listN, InIter from,
                   InIter to);

  std::size_t pixelCount() {
    return m_chunks && !m_chunks->empty() ? m_chunks->at(0).size() : 0;
  }

  MANTID_PARALLEL_DLL friend std::ostream &
  operator<<(std::ostream &os, const EventsListsShmemManager &manager);

protected:
  // Constructor for internal usage in  that just sets up the names, instance
  // for m_eventLists is defined later in derivated class constructor.
  EventsListsShmemManager(const std::string &segmentName,
                          const std::string &elName, int);

  const VoidAllocator &alloc() const;

  /// The name of shared memory segment to save the list of event
  const std::string m_segmentName;

  /// Allocator to mange shared memory
  std::unique_ptr<VoidAllocator> m_allocatorInstance;

  /// Event list shared storage name
  const std::string m_chunksName;

  /// Memory segment to store data
  std::unique_ptr<ip::managed_shared_memory> m_segment;

  /// Event list shared storage
  Chunks *m_chunks;
};

/// Appends the range of ToF events (from other container for example)
template <typename InIter>
void EventsListsShmemManager::appendEvent(std::size_t chunkN, std::size_t listN,
                                          InIter from, InIter to) {
  if (!m_chunks)
    throw("No event lists found.");
  auto &list = m_chunks->at(chunkN).at(listN);
  list.insert(list.end(), from, to);
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGER_H_ */