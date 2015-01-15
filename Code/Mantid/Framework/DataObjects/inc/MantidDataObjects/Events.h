#ifndef MANTID_DATAOBJECTS_EVENTS_H_
#define MANTID_DATAOBJECTS_EVENTS_H_

#ifdef _WIN32 /* _WIN32 */
#include <time.h>
#endif
#include <cstddef>
#include <iosfwd>
#include <vector>
#include "MantidAPI/MatrixWorkspace.h" // get MantidVec declaration
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include <set>

namespace Mantid {
// Forward declaration needed while this needs to be a friend of TofEvent (see
// below)
namespace DataHandling {
class LoadEventNexus;
}

namespace DataObjects {

//==========================================================================================
/** Info about a single neutron detection event:
 *
 *  - the time of flight of the neutron (can be converted to other units)
 *  - the absolute time of the pulse at which it was produced
 */
#pragma pack(push, 4) // Ensure the structure is no larger than it needs to
class DLLExport TofEvent {

  /// EventList has the right to mess with TofEvent.
  friend class EventList;
  friend class WeightedEvent;
  friend class WeightedEventNoTime;
  friend class tofGreaterOrEqual;
  friend class tofGreater;
  friend class DataHandling::LoadEventNexus; // Needed while the ISIS hack of
                                             // spreading events out in a bin
                                             // remains

protected:
  /** The 'x value' of the event. This will be in a unit available from the
   * UnitFactory.
   *  Initially (prior to any unit conversion on the holding workspace), this
   * will have
   *  the unit of time-of-flight in microseconds.
   */
  double m_tof;

  /**
   * The absolute time of the start of the pulse that generated this event.
   * This is saved as the number of ticks (1 nanosecond if boost is compiled
   * for nanoseconds) since a specified epoch: we use the GPS epoch of Jan 1,
   *1990.
   *
   * 64 bits gives 1 ns resolution up to +- 292 years around 1990. Should be
   *enough.
   */
  Mantid::Kernel::DateAndTime m_pulsetime;

public:
  /// Constructor, specifying only the time of flight in microseconds
  TofEvent(double tof);

  /// Constructor, specifying the time of flight in microseconds and the frame
  /// id
  TofEvent(double tof, const Mantid::Kernel::DateAndTime pulsetime);

  /// Constructor, copy from another TofEvent object
  TofEvent(const TofEvent &);

  /// Empty constructor
  TofEvent();

  /// Destructor
  ~TofEvent();

  /// Copy from another TofEvent object
  TofEvent &operator=(const TofEvent &rhs);

  bool operator==(const TofEvent &rhs) const;
  bool operator<(const TofEvent &rhs) const;
  bool operator<(const double rhs_tof) const;
  bool operator>(const TofEvent &rhs) const;
  bool equals(const TofEvent &rhs, const double tolTof,
              const int64_t tolPulse) const;

  double operator()() const;
  double tof() const;
  Mantid::Kernel::DateAndTime pulseTime() const;
  double weight() const;
  double error() const;
  double errorSquared() const;

  /// Output a string representation of the event to a stream
  friend std::ostream &operator<<(std::ostream &os, const TofEvent &event);
};
#pragma pack(pop)

//==========================================================================================
/** Info about a single neutron detection event, including a weight and error
 *value:
 *
 *  - the time of flight of the neutron (can be converted to other units)
 *  - the absolute time of the pulse at which it was produced
 *  - weight of the neutron (float, can be
 */
#pragma pack(push, 4) // Ensure the structure is no larger than it needs to
class DLLExport WeightedEvent : public TofEvent {

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
  WeightedEvent(double time_of_flight,
                const Mantid::Kernel::DateAndTime pulsetime, double weight,
                double errorSquared);
  WeightedEvent(double time_of_flight,
                const Mantid::Kernel::DateAndTime pulsetime, float weight,
                float errorSquared);

  WeightedEvent(const TofEvent &, double weight, double errorSquared);
  WeightedEvent(const TofEvent &, float weight, float errorSquared);

  WeightedEvent(const WeightedEvent &);

  WeightedEvent(const TofEvent &);

  WeightedEvent();

  ~WeightedEvent();

  /// Copy from another WeightedEvent object
  WeightedEvent &operator=(const WeightedEvent &rhs);

  bool operator==(const WeightedEvent &other) const;
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
  WeightedEventNoTime(double time_of_flight, double weight,
                      double errorSquared);
  WeightedEventNoTime(double time_of_flight, float weight, float errorSquared);

  WeightedEventNoTime(double tof, const Mantid::Kernel::DateAndTime pulsetime,
                      double weight, double errorSquared);
  WeightedEventNoTime(double tof, const Mantid::Kernel::DateAndTime pulsetime,
                      float weight, float errorSquared);

  WeightedEventNoTime(const TofEvent &, double weight, double errorSquared);
  WeightedEventNoTime(const TofEvent &, float weight, float errorSquared);

  WeightedEventNoTime(const WeightedEventNoTime &);

  WeightedEventNoTime(const WeightedEvent &);

  WeightedEventNoTime(const TofEvent &);

  WeightedEventNoTime();

  ~WeightedEventNoTime();

  /// Copy from another WeightedEventNoTime object
  WeightedEventNoTime &operator=(const WeightedEventNoTime &rhs);

  bool operator==(const WeightedEventNoTime &other) const;
  bool operator<(const WeightedEventNoTime &rhs) const;
  bool operator<(const double rhs) const;
  bool equals(const WeightedEventNoTime &rhs, const double tolTof,
              const double tolWeight) const;

  double operator()() const;
  double tof() const;
  Mantid::Kernel::DateAndTime pulseTime() const;
  double weight() const;
  double error() const;
  double errorSquared() const;

  /// Output a string representation of the event to a stream
  friend std::ostream &operator<<(std::ostream &os, const WeightedEvent &event);
};
#pragma pack(pop)

//==========================================================================================
// TofEvent inlined member function definitions
//==========================================================================================

/** () operator: return the tof (X value) of the event.
 *  This is useful for std operations like comparisons and std::lower_bound
 *  @return :: double, the tof (X value) of the event.
 */
inline double TofEvent::operator()() const { return m_tof; }

/** @return The 'x value'. Despite the name, this can be in any unit in the
 * UnitFactory.
 *  If it is time-of-flight, it will be in microseconds.
 */
inline double TofEvent::tof() const { return m_tof; }

/// Return the pulse time
inline Mantid::Kernel::DateAndTime TofEvent::pulseTime() const {
  return m_pulsetime;
}

/// Return the weight of the event - exactly 1.0 always
inline double TofEvent::weight() const { return 1.0; }

/// Return the error of the event - exactly 1.0 always
inline double TofEvent::error() const { return 1.0; }

/// Return the errorSquared of the event - exactly 1.0 always
inline double TofEvent::errorSquared() const { return 1.0; }

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
inline Kernel::DateAndTime WeightedEventNoTime::pulseTime() const { return 0; }

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

} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTS_H_
