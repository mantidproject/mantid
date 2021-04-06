// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
template <class TimeOffsetType> class NXEventDataSource {
public:
  virtual ~NXEventDataSource() = default;

  virtual std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>> setBankIndex(const size_t bank) = 0;

  virtual void readEventID(int32_t *event_id, size_t start, size_t count) const = 0;
  virtual void readEventTimeOffset(TimeOffsetType *event_time_offset, size_t start, size_t count) const = 0;
  virtual std::string readEventTimeOffsetUnit() const = 0;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid
