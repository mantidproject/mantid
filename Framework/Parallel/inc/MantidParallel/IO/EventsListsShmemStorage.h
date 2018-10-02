#ifndef MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGE_H_
#define MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGE_H_

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/EventsListsShmemManager.h"
#include <vector>

namespace ip = boost::interprocess;

namespace Mantid {
namespace Parallel {
namespace IO {

/** EventsListsShmemStorage : NOT an !!!OWNER of shared memory!!!
  Shared memory segments are detached and can be fined by name.
  The concern of this class is allocating shared memory and
  naming segments, all other operations are on the base class:
  EventsListsShmemManager.

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
class MANTID_PARALLEL_DLL EventsListsShmemStorage
    : public EventsListsShmemManager {
public:
  EventsListsShmemStorage(const std::string &segmentName,
                          const std::string &elName, size_t size,
                          size_t chunksCnt, size_t pixelsCount);
  virtual ~EventsListsShmemStorage() = default;

  void reserve(std::size_t chunkN, std::size_t pixelN, std::size_t size);

  MANTID_PARALLEL_DLL friend std::ostream &
  operator<<(std::ostream &os, const EventsListsShmemStorage &storage);
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGE_H_ */