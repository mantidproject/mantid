#ifndef MANTID_PARALLEL_NXEVENTDATASOURCE_H_
#define MANTID_PARALLEL_NXEVENTDATASOURCE_H_

#include <vector>

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/EventDataPartitioner.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** Abstract base class for sources of NXevent_data. For files this is
  subclassed in NXEventDataLoader. The base class exists for testing purposes
  and potentially for supporting event streams in the future.

  @author Simon Heybrock
  @date 2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
template <class TimeOffsetType> class NXEventDataSource {
public:
  virtual ~NXEventDataSource() = default;

  virtual std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>>
  setBankIndex(const size_t bank) = 0;

  virtual void readEventID(int32_t *event_id, size_t start,
                           size_t count) const = 0;
  virtual void readEventTimeOffset(TimeOffsetType *event_time_offset,
                                   size_t start, size_t count) const = 0;
  virtual std::string readEventTimeOffsetUnit() const = 0;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_NXEVENTDATASOURCE_H_ */
