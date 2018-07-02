#ifndef MANTID_DATAOBJECTS_EVENTS_H_
#define MANTID_DATAOBJECTS_EVENTS_H_

#ifdef _WIN32 /* _WIN32 */
#include <time.h>
#endif
#include "MantidAPI/MatrixWorkspace_fwd.h" // get MantidVec declaration
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidTypes/Event/TofEvent.h"
#include <cstddef>
#include <iosfwd>
#include <set>
#include <vector>

namespace Mantid {
// Forward declaration needed while this needs to be a friend of TofEvent (see
// below)
namespace DataHandling {
class LoadEventNexus;
}

namespace DataObjects {
//==========================================================================================
/** Info about a single neutron detection event, including a weight and error
 *value:
 *
 *  - the time of flight of the neutron (can be converted to other units)
 *  - the absolute time of the pulse at which it was produced
 *  - weight of the neutron (float, can be
 */
#pragma pack(push, 4) // Ensure the structure is no larger than it needs to
class DLLExport WeightedEvent : public Types::Event::TofEvent {

  /// EventList has the right to mess with WeightedEvent.
  friend class EventList;
  friend class WeightedEventNoTime;
  friend class tofGreaterOrEqual;
  friend class tofGreater;

public:
  /// The weight of this neutron.
  float m_weight;

  /// The SQUARE of the error that this neutron contributes.
  float m_errorSquared;

public:
  /// Constructor, specifying only the time of flight
  WeightedEvent(double time_of_flight);

  /// Constructor, full
  WeightedEvent(double tof, const Mantid::Types::Core::DateAndTime pulsetime,
                double weight, double errorSquared);
  WeightedEvent(double tof, const Mantid::Types::Core::DateAndTime pulsetime,
                float weight, float errorSquared);

  WeightedEvent(const TofEvent &, double weight, double errorSquared);
  WeightedEvent(const TofEvent &, float weight, float errorSquared);

  WeightedEvent(const TofEvent &);

  WeightedEvent();

  bool operator==(const WeightedEvent &rhs) const;
  bool equals(const WeightedEvent &rhs, const double tolTof,
              const double tolWeight, const int64_t tolPulse) const;

  double weight() const;
  double error() const;
  double errorSquared() const;

  /// Output a string representation of the event to a stream
  friend std::ostream &operator<<(std::ostream &os, const WeightedEvent &event);
};
#pragma pack(pop)

//==========================================================================================
/** Info about a single neutron detection event, including a weight and error
 *value,
 * but excluding the pulsetime to save memory:
 *
 *  - the time of flight of the neutron (can be converted to other units)
 *  - the absolute time of the pulse at which it was produced
 *  - weight of the neutron (float, can be
 */
#pragma pack(push, 4) // Ensure the structure is no larger than it needs to
class DLLExport WeightedEventNoTime {

  /// EventList has the right to mess with this
  friend class EventList;
  friend class tofGreaterOrEqual;
  friend class tofGreater;

protected:
  /// The 'x value' (e.g. time-of-flight) of this neutron
  double m_tof;

public:
  /// The weight of this neutron.
  float m_weight;

  /// The SQUARE of the error that this neutron contributes.
  float m_errorSquared;

public:
  /// Constructor, specifying only the time of flight
  WeightedEventNoTime(double time_of_flight);

  /// Constructor, full
  WeightedEventNoTime(double tof, double weight, double errorSquared);
  WeightedEventNoTime(double tof, float weight, float errorSquared);

  WeightedEventNoTime(double tof,
                      const Mantid::Types::Core::DateAndTime pulsetime,
                      double weight, double errorSquared);
  WeightedEventNoTime(double tof,
                      const Mantid::Types::Core::DateAndTime pulsetime,
                      float weight, float errorSquared);

  WeightedEventNoTime(const Types::Event::TofEvent &, double weight,
                      double errorSquared);
  WeightedEventNoTime(const Types::Event::TofEvent &, float weight,
                      float errorSquared);

  WeightedEventNoTime(const WeightedEvent &);

  WeightedEventNoTime(const Types::Event::TofEvent &);

  WeightedEventNoTime();

  bool operator==(const WeightedEventNoTime &rhs) const;

  /** < comparison operator, using the TOF to do the comparison.
   * @param rhs: the other WeightedEventNoTime to compare.
   * @return true if this->m_tof < rhs.m_tof
   */
  bool operator<(const WeightedEventNoTime &rhs) const {
    return (this->m_tof < rhs.m_tof);
  }

  /** < comparison operator, using the TOF to do the comparison.
   * @param rhs_tof: the other time of flight to compare.
   * @return true if this->m_tof < rhs.m_tof
   */
  bool operator<(const double rhs_tof) const { return (this->m_tof < rhs_tof); }

  bool equals(const WeightedEventNoTime &rhs, const double tolTof,
              const double tolWeight) const;

  double operator()() const;
  double tof() const;
  Mantid::Types::Core::DateAndTime pulseTime() const;
  double weight() const;
  double error() const;
  double errorSquared() const;

  /// Output a string representation of the event to a stream
  friend std::ostream &operator<<(std::ostream &os, const WeightedEvent &event);
};
#pragma pack(pop)

//==========================================================================================
// WeightedEvent inlined member function definitions
//==========================================================================================

/// Return the weight of the neutron, as a double (it is saved as a float).
inline double WeightedEvent::weight() const { return m_weight; }

/** @return the error of the neutron, as a double (it is saved as a float).
 *  Note: this returns the actual error; the value is saved
 *  internally as the SQUARED error, so this function calculates sqrt().
 *  For more speed, use errorSquared().
 */
inline double WeightedEvent::error() const {
  return std::sqrt(double(m_errorSquared));
}

/** @return the square of the error for this event.
 *  This is how the error is saved internally, so this is faster than error()
 */
inline double WeightedEvent::errorSquared() const { return m_errorSquared; }

//==========================================================================================
// WeightedEventNoTime inlined member function definitions
//==========================================================================================

inline double WeightedEventNoTime::operator()() const { return m_tof; }

/// Return the time-of-flight of the neutron, as a double.
inline double WeightedEventNoTime::tof() const { return m_tof; }

/** Return the pulse time; this returns 0 since this
 *  type of Event has no time associated.
 */
inline Types::Core::DateAndTime WeightedEventNoTime::pulseTime() const {
  return 0;
}

/// Return the weight of the neutron, as a double (it is saved as a float).
inline double WeightedEventNoTime::weight() const { return m_weight; }

/// Return the error of the neutron, as a double (it is saved as a float).
inline double WeightedEventNoTime::error() const {
  return std::sqrt(double(m_errorSquared));
}

/// Return the squared error of the neutron, as a double
inline double WeightedEventNoTime::errorSquared() const {
  return m_errorSquared;
}

} // namespace DataObjects
} // namespace Mantid
#endif /// MANTID_DATAOBJECTS_EVENTS_H_
