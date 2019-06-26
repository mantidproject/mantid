// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/Events.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include <cmath>
#include <functional>
#include <stdexcept>

using std::ostream;
using std::size_t;

namespace Mantid {
namespace DataObjects {
using Types::Event::TofEvent;

//==========================================================================
/// --------------------- WeightedEvent stuff ------------------------------
//==========================================================================

/** Constructor, tof only:
 * @param time_of_flight: tof in microseconds.
 */
WeightedEvent::WeightedEvent(double time_of_flight)
    : TofEvent(time_of_flight), m_weight(1.0), m_errorSquared(1.0) {}

/** Constructor, full:
 * @param tof: tof in microseconds.
 * @param pulsetime: absolute pulse time
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEvent::WeightedEvent(double tof,
                             const Mantid::Types::Core::DateAndTime pulsetime,
                             double weight, double errorSquared)
    : TofEvent(tof, pulsetime), m_weight(static_cast<float>(weight)),
      m_errorSquared(static_cast<float>(errorSquared)) {}

/** Constructor, full:
 * @param tof: tof in microseconds.
 * @param pulsetime: absolute pulse time
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEvent::WeightedEvent(double tof,
                             const Mantid::Types::Core::DateAndTime pulsetime,
                             float weight, float errorSquared)
    : TofEvent(tof, pulsetime), m_weight(weight), m_errorSquared(errorSquared) {
}

/** Constructor, copy from a TofEvent object but add weights
 * @param rhs: TofEvent to copy into this.
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEvent::WeightedEvent(const TofEvent &rhs, double weight,
                             double errorSquared)
    : TofEvent(rhs.m_tof, rhs.m_pulsetime),
      m_weight(static_cast<float>(weight)),
      m_errorSquared(static_cast<float>(errorSquared)) {}

/** Constructor, copy from a TofEvent object but add weights
 * @param rhs: TofEvent to copy into this.
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEvent::WeightedEvent(const TofEvent &rhs, float weight,
                             float errorSquared)
    : TofEvent(rhs.m_tof, rhs.m_pulsetime), m_weight(weight),
      m_errorSquared(errorSquared) {}

/** Constructor, copy from another WeightedEvent object
 * @param rhs: source TofEvent
 */
WeightedEvent::WeightedEvent(const TofEvent &rhs)
    : TofEvent(rhs.m_tof, rhs.m_pulsetime), m_weight(1.0), m_errorSquared(1.0) {
}

/// Empty constructor
WeightedEvent::WeightedEvent()
    : TofEvent(), m_weight(1.0), m_errorSquared(1.0) {}

/** Comparison operator.
 * @param rhs :: event to which we are comparing.
 * @return true if all elements of this event are identical
 *  */
bool WeightedEvent::operator==(const WeightedEvent &rhs) const {
  return (this->m_tof == rhs.m_tof) && (this->m_pulsetime == rhs.m_pulsetime) &&
         (this->m_weight == rhs.m_weight) &&
         (this->m_errorSquared == rhs.m_errorSquared);
}

/**
 * Compare two events within the specified tolerance
 *
 * @param rhs the other TofEvent to compare
 * @param tolTof the tolerance of a difference in m_tof.
 * @param tolWeight the tolerance of a difference in m_weight
 * and m_errorSquared.
 * @param tolPulse the tolerance of a difference in m_pulsetime
 * in nanoseconds.
 *
 * @return True if the are the same within the specifed tolerances
 */
bool WeightedEvent::equals(const WeightedEvent &rhs, const double tolTof,
                           const double tolWeight,
                           const int64_t tolPulse) const {
  if (std::fabs(this->m_tof - rhs.m_tof) > tolTof)
    return false;
  if (std::fabs(this->m_weight - rhs.m_weight) > tolWeight)
    return false;
  if (std::fabs(this->m_errorSquared - rhs.m_errorSquared) > tolWeight)
    return false;
  // then it is just if the pulse-times are equal
  return (this->m_pulsetime.equals(rhs.m_pulsetime, tolPulse));
}

/** Output a string representation of the event to a stream
 * @param os :: Stream
 * @param event :: WeightedEvent to output to the stream
 */
ostream &operator<<(ostream &os, const WeightedEvent &event) {
  os << event.m_tof << "," << event.m_pulsetime.toSimpleString() << " (W"
     << event.m_weight << " +- " << event.error() << ")";
  return os;
}

//==========================================================================
/// --------------------- WeightedEventNoTime stuff ------------------------
//==========================================================================

/** Constructor, tof only:
 * @param time_of_flight: tof in microseconds.
 */
WeightedEventNoTime::WeightedEventNoTime(double time_of_flight)
    : m_tof(time_of_flight), m_weight(1.0), m_errorSquared(1.0) {}

/** Constructor, full:
 * @param tof: tof in microseconds.
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEventNoTime::WeightedEventNoTime(double tof, double weight,
                                         double errorSquared)
    : m_tof(tof), m_weight(static_cast<float>(weight)),
      m_errorSquared(static_cast<float>(errorSquared)) {}

/** Constructor, full:
 * @param tof: tof in microseconds.
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEventNoTime::WeightedEventNoTime(double tof, float weight,
                                         float errorSquared)
    : m_tof(tof), m_weight(weight), m_errorSquared(errorSquared) {}

/** Constructor that ignores a time:
 * @param tof: tof in microseconds.
 * @param pulsetime: an ignored pulse time.
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEventNoTime::WeightedEventNoTime(
    double tof, const Mantid::Types::Core::DateAndTime /*unused*/,
    double weight, double errorSquared)
    : m_tof(tof), m_weight(static_cast<float>(weight)),
      m_errorSquared(static_cast<float>(errorSquared)) {}

/** Constructor that ignores a time:
 * @param tof: tof in microseconds.
 * @param pulsetime: an ignored pulse time.
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEventNoTime::WeightedEventNoTime(
    double tof, const Mantid::Types::Core::DateAndTime /*unused*/, float weight,
    float errorSquared)
    : m_tof(tof), m_weight(weight), m_errorSquared(errorSquared) {}

/** Constructor, copy from a TofEvent object but add weights
 * @param rhs: TofEvent to copy into this.
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEventNoTime::WeightedEventNoTime(const TofEvent &rhs, double weight,
                                         double errorSquared)
    : m_tof(rhs.m_tof), m_weight(static_cast<float>(weight)),
      m_errorSquared(static_cast<float>(errorSquared)) {}

/** Constructor, copy from a TofEvent object but add weights
 * @param rhs: TofEvent to copy into this.
 * @param weight: weight of this neutron event.
 * @param errorSquared: the square of the error on the event
 */
WeightedEventNoTime::WeightedEventNoTime(const TofEvent &rhs, float weight,
                                         float errorSquared)
    : m_tof(rhs.m_tof), m_weight(weight), m_errorSquared(errorSquared) {}

/** Constructor, copy from a WeightedEvent object
 * @param rhs: source WeightedEvent
 */
WeightedEventNoTime::WeightedEventNoTime(const WeightedEvent &rhs)
    : m_tof(rhs.m_tof), m_weight(rhs.m_weight),
      m_errorSquared(rhs.m_errorSquared) {}

/** Constructor, copy from another TofEvent object
 * @param rhs: source TofEvent
 */
WeightedEventNoTime::WeightedEventNoTime(const TofEvent &rhs)
    : m_tof(rhs.m_tof), m_weight(1.0), m_errorSquared(1.0) {}

/// Empty constructor
WeightedEventNoTime::WeightedEventNoTime()
    : m_tof(0.0), m_weight(1.0), m_errorSquared(1.0) {}

/** Comparison operator.
 * @param rhs :: event to which we are comparing.
 * @return true if all elements of this event are identical
 *  */
bool WeightedEventNoTime::operator==(const WeightedEventNoTime &rhs) const {
  return (this->m_tof == rhs.m_tof) && (this->m_weight == rhs.m_weight) &&
         (this->m_errorSquared == rhs.m_errorSquared);
}

/**
 * Compare two events within the specified tolerance
 *
 * @param rhs the other TofEvent to compare
 * @param tolTof the tolerance of a difference in m_tof.
 * @param tolWeight the tolerance of a difference in m_weight
 * and m_errorSquared.
 *
 * @return True if the are the same within the specifed tolerances
 */
bool WeightedEventNoTime::equals(const WeightedEventNoTime &rhs,
                                 const double tolTof,
                                 const double tolWeight) const {
  if (std::fabs(this->m_tof - rhs.m_tof) > tolTof)
    return false;
  if (std::fabs(this->m_weight - rhs.m_weight) > tolWeight)
    return false;
  if (std::fabs(this->m_errorSquared - rhs.m_errorSquared) > tolWeight)
    return false;
  // then it is just if the pulse-times are equal
  return true;
}

} // namespace DataObjects
} // namespace Mantid
