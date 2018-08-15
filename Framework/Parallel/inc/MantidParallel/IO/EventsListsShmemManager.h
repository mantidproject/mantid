#ifndef MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGER_H_
#define MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGER_H_

#include "boost/interprocess/containers/vector.hpp"
#include "boost/interprocess/managed_shared_memory.hpp"
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/offset_ptr.hpp>

#include "MantidParallel/DllConfig.h"

namespace ip = boost::interprocess;

namespace Mantid {
namespace Types {
namespace Event {
class TofEvent;
}
} // namespace Types

namespace Parallel {

/** EventsListsShmemManager : Operates with event list in shared memory in
  multiprocess environment; NOT an !!!OWNER of shared memory!!!

  @author Igor Gudich
  @date 2018

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_PARALLEL_DLL EventsListsShmemManager {
public:
  // Constructor for client usage: "sets" Manager to the piece of shared memory
  // with existed GuardedEventLists in it
  EventsListsShmemManager(const std::string &segmentName,
                          const std::string &elName);
  ~EventsListsShmemManager();

protected:
  using SegmentManager = ip::managed_shared_memory::segment_manager;
  using VoidAllocator = ip::allocator<void, SegmentManager>;
  using TofEventAllocator =
      ip::allocator<Types::Event::TofEvent, SegmentManager>;
  using EventList = ip::vector<Types::Event::TofEvent, TofEventAllocator>;
  using EventListPtr = ip::offset_ptr<EventList>;
  using EventListPtrAllocator = ip::allocator<EventListPtr, SegmentManager>;

  // Structure to control access to every event list
  struct GuardedEventList {
    EventListPtr eventList;
    ip::interprocess_mutex mutex;
  };

  using GuardedEventListAllocator =
      ip::allocator<GuardedEventList, SegmentManager>;
  using EventLists = ip::vector<GuardedEventList, GuardedEventListAllocator>;

  // Constructor for internal usage in  that just sets up the names, instance
  // for m_eventLists is defined later in derivated class constructor.
  EventsListsShmemManager(const std::string &segmentName,
                          const std::string &elName, int);

  /// The name of shared memory segment to save the list of event
  const std::string m_segmentName;

  /// Allocator to mange shared memory
  std::shared_ptr<VoidAllocator> m_allocatorInstance;

  /// Event list shared storage name
  const std::string m_eventListsName;

  /// Event list shared storage
  EventLists *m_eventLists;
};

} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGER_H_ */