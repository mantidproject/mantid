#ifndef MANTID_PARALLEL_EVENTDATASINK_H_
#define MANTID_PARALLEL_EVENTDATASINK_H_

#include <vector>

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/Chunker.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** Abstract base class for sinks of event data. Subclassed in EventParser. The
base class exists for testing purposes.

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
template <class IndexType, class TimeZeroType, class TimeOffsetType>
class EventDataSink {
public:
  virtual ~EventDataSink() = default;

  virtual void setPulseInformation(std::vector<IndexType> event_index,
                                   std::vector<TimeZeroType> event_time_zero);
  virtual void startAsync(int32_t *event_id_start,
                          const TimeOffsetType *event_time_offset_start,
                          const Chunker::LoadRange &range);
  virtual void wait() const;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_EVENTDATASINK_H_ */
