// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/Chunker.h"
#include "MantidParallel/IO/PulseTimeGenerator.h"
#include "MantidTypes/Core/DateAndTime.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** Partition the event_time_offset and event_id entries and combine them with
  pulse time information obtained from PulseTimeGenerator. Partitioning is to
  obtain a separate vector of events. Currently a
  round-robin partitioning scheme is hard-coded.

  @author Simon Heybrock
  @date 2017
*/
namespace detail {
template <class TimeOffsetType> struct Event {
  int32_t index; // local spectrum index
  TimeOffsetType tof;
  Types::Core::DateAndTime pulseTime;
};
} // namespace detail

template <class TimeOffsetType> class AbstractEventDataPartitioner {
public:
  using Event = detail::Event<TimeOffsetType>;
  AbstractEventDataPartitioner(const int numWorkers) : m_numWorkers(numWorkers) {}
  virtual ~AbstractEventDataPartitioner() = default;

  /** Partition given data.
   *
   * @param partitioned output vector of data for each partition
   * @param globalSpectrumIndex list of spectrum indices
   * @param eventTimeOffset list TOF values, same length as globalSpectrumIndex
   * @param range defines start and end of data for lookup in PulseTimeGenerator
   */
  virtual void partition(std::vector<std::vector<Event>> &partitioned, const int32_t *globalSpectrumIndex,
                         const TimeOffsetType *eventTimeOffset, const Chunker::LoadRange &range) = 0;

  virtual Types::Core::DateAndTime next() = 0;
  virtual void setEventOffset(const size_t event) = 0;

protected:
  const int m_numWorkers;
};

template <class IndexType, class TimeZeroType, class TimeOffsetType>
class EventDataPartitioner : public AbstractEventDataPartitioner<TimeOffsetType> {
public:
  using Event = detail::Event<TimeOffsetType>;
  EventDataPartitioner(const int numWorkers, PulseTimeGenerator<IndexType, TimeZeroType> &&gen)
      : AbstractEventDataPartitioner<TimeOffsetType>(numWorkers), m_pulseTimes(std::move(gen)) {}

  void partition(std::vector<std::vector<Event>> &partitioned, const int32_t *globalSpectrumIndex,
                 const TimeOffsetType *eventTimeOffset, const Chunker::LoadRange &range) override;

  Types::Core::DateAndTime next() override { return m_pulseTimes.next(); }
  void setEventOffset(const size_t event) override { m_pulseTimes.seek(event); };

private:
  PulseTimeGenerator<IndexType, TimeZeroType> m_pulseTimes;
};

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventDataPartitioner<IndexType, TimeZeroType, TimeOffsetType>::partition(
    std::vector<std::vector<Event>> &partitioned, const int32_t *globalSpectrumIndex,
    const TimeOffsetType *eventTimeOffset, const Chunker::LoadRange &range) {
  for (auto &item : partitioned)
    item.clear();
  const auto workers = AbstractEventDataPartitioner<TimeOffsetType>::m_numWorkers;
  partitioned.resize(workers);

  m_pulseTimes.seek(range.eventOffset);
  for (size_t event = 0; event < range.eventCount; ++event) {
    // Currently this supports only a hard-coded round-robin partitioning.
    int eventPartition = globalSpectrumIndex[event] % workers;
    auto index = globalSpectrumIndex[event] / workers;
    partitioned[eventPartition].emplace_back(
        detail::Event<TimeOffsetType>{index, eventTimeOffset[event], m_pulseTimes.next()});
  }
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
