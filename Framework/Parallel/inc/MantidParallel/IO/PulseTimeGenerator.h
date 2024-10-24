// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"
#include "MantidTypes/Core/DateAndTime.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** Generator for pulse times based in input from an NXevent_data entry from a
  Nexus file. Used to generate a sequence of pulse times for a series of events
  by doing a lookup of the event in the event_index field and returning the
  corresponding pulse time obtained from event_time_zero combined with the
  optional offset parameter.

  @author Simon Heybrock
  @date 2017
*/

namespace detail {
constexpr char second[] = "second";
constexpr char microsecond[] = "microsecond";
constexpr char nanosecond[] = "nanosecond";

template <class TimeZeroType>
double scaleFromUnit(const std::string &unit,
                     typename std::enable_if<std::is_floating_point<TimeZeroType>::value>::type * = nullptr) {
  if (unit == second)
    return 1.0;
  if (unit == microsecond)
    return 1e-6;
  if (unit == nanosecond)
    return 1e-9;
  throw std::runtime_error("PulseTimeGenerator: unsupported unit `" + unit + "` for event_time_zero");
}

template <class TimeZeroType>
int64_t scaleFromUnit(const std::string &unit,
                      typename std::enable_if<std::is_integral<TimeZeroType>::value>::type * = nullptr) {
  if (unit == nanosecond)
    return 1;
  throw std::runtime_error("PulseTimeGenerator: unsupported unit `" + unit + "` for event_time_zero");
}

/// Convert any int or float type to corresponding 64 bit type needed for
/// passing into DateAndTime.
template <class T> struct IntOrFloat64Bit {
  using type = int64_t;
};
template <> struct IntOrFloat64Bit<float> {
  using type = double;
};
template <> struct IntOrFloat64Bit<double> {
  using type = double;
};
} // namespace detail

template <class IndexType, class TimeZeroType> class PulseTimeGenerator {
public:
  PulseTimeGenerator() = default;

  /// Constructor based on entries in NXevent_data.
  PulseTimeGenerator(std::vector<IndexType> event_index, std::vector<TimeZeroType> event_time_zero,
                     const std::string &event_time_zero_unit, const int64_t event_time_zero_offset)
      : m_index(std::move(event_index)), m_timeZero(std::move(event_time_zero)),
        m_timeZeroScale(detail::scaleFromUnit<TimeZeroType>(event_time_zero_unit)),
        m_timeZeroOffset(event_time_zero_offset) {}

  /// Seek to given event index.
  void seek(const size_t event) {
    if (m_index.size() == 0)
      throw std::runtime_error("Empty event index in PulseTimeGenerator");
    if (static_cast<IndexType>(event) < m_event)
      m_pulse = 0;
    m_event = static_cast<IndexType>(event);
    for (; m_pulse < m_index.size() - 1; ++m_pulse)
      if (m_event < m_index[m_pulse + 1])
        break;
    m_pulseTime = getPulseTime(m_timeZeroOffset, m_timeZero[m_pulse]);
  }

  /// Return pulse time for next event, and advance. Must call seek() first, at
  /// least once.
  Types::Core::DateAndTime next() {
    while (m_pulse < m_index.size() - 1 && m_event == m_index[m_pulse + 1]) {
      ++m_pulse;
      m_pulseTime = getPulseTime(m_timeZeroOffset, m_timeZero[m_pulse]);
    }
    ++m_event;

    return m_pulseTime;
  }

private:
  Types::Core::DateAndTime getPulseTime(const Types::Core::DateAndTime &offset, const TimeZeroType &eventTimeZero) {
    return offset + m_timeZeroScale * static_cast<typename detail::IntOrFloat64Bit<TimeZeroType>::type>(eventTimeZero);
  }

  IndexType m_event{0};
  size_t m_pulse{0};
  Types::Core::DateAndTime m_pulseTime;
  std::vector<IndexType> m_index;
  std::vector<TimeZeroType> m_timeZero;
  typename detail::IntOrFloat64Bit<TimeZeroType>::type m_timeZeroScale;
  Types::Core::DateAndTime m_timeZeroOffset;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid
