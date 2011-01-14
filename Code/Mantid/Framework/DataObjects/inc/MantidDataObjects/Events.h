#ifndef MANTID_DATAOBJECTS_EVENTS_H_
#define MANTID_DATAOBJECTS_EVENTS_H_

#ifdef _WIN32 /* _WIN32 */
#include <time.h>
#endif
#include <cstddef>
#include <iostream>
#include <vector>
#include "MantidAPI/MatrixWorkspace.h" // get MantidVec declaration
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include <set>

namespace Mantid
{
namespace DataObjects
{



//==========================================================================================
/** Info about a single neutron detection event:
 *
 *  - the time of flight of the neutron (can be converted to other units)
 *  - the absolute time of the pulse at which it was produced
 */
class DLLExport TofEvent {

  /// EventList has the right to mess with TofEvent.
  friend class EventList;
  friend class WeightedEvent;
  friend class tofGreaterOrEqual;
  friend class tofGreater;

protected:
  /** The units of the time of flight index in nanoseconds. This is relative to the
   * start of the pulse (stored in pulse_time.
   * EXCEPT: After AlignDetectors is run, this is converted to dSpacing, in Angstroms^-1
   * @return a double
   * */
  double m_tof;

  /**
   * The absolute time of the start of the pulse that generated this event.
   * This is saved as the number of ticks (1 nanosecond if boost is compiled
   * for nanoseconds) since a specified epoch: we use the GPS epoch of Jan 1, 1990.
   *
   * 64 bits gives 1 ns resolution up to +- 292 years around 1990. Should be enough.
   * @return a DateAndTime
   */
  Mantid::Kernel::DateAndTime m_pulsetime;

public:
  /// Constructor, specifying only the time of flight
  TofEvent(double tof);

  /// Constructor, specifying the time of flight and the frame id
  TofEvent(double tof, const Mantid::Kernel::DateAndTime pulsetime);

  /// Constructor, copy from another TofEvent object
  TofEvent(const TofEvent&);

  /// Empty constructor
  TofEvent();

  /// Destructor
  ~TofEvent();

  /// Copy from another TofEvent object
  TofEvent& operator=(const TofEvent&rhs);

  double operator()() const;
  bool operator==(const TofEvent & rhs) const;
  bool operator<(const TofEvent & rhs) const;
  bool operator<(const double rhs_tof) const;
  bool operator>(const TofEvent & rhs) const;


  /// Return the time of flight, as a double, in nanoseconds.
  double tof() const;

  /// Return the frame id
  Mantid::Kernel::DateAndTime pulseTime() const;

  /// Return the weight of the event - exactly 1.0 always
  double weight()
  {
    return 1.0;
  }

  /// Output a string representation of the event to a stream
  friend std::ostream& operator<<(std::ostream &os, const TofEvent &event);
};




//==========================================================================================
/** Info about a single neutron detection event, including a weight and error value:
 *
 *  - the time of flight of the neutron (can be converted to other units)
 *  - the absolute time of the pulse at which it was produced
 *  - weight of the neutron (float, can be
 */
class DLLExport WeightedEvent : public TofEvent {

  /// EventList has the right to mess with WeightedEvent.
  friend class EventList;
  friend class tofGreaterOrEqual;
  friend class tofGreater;

private:

  /// The weight of this neutron.
  float m_weight;

  /// The SQUARE of the error that this neutron contributes.
  float m_errorSquared;

public:
  /// Constructor, specifying only the time of flight
  WeightedEvent(double time_of_flight);

  /// Constructor, full
  WeightedEvent(double time_of_flight, const Mantid::Kernel::DateAndTime pulsetime, float weight, float errorSquared);

  WeightedEvent(const TofEvent&, float weight, float errorSquared);

  WeightedEvent(const WeightedEvent&);

  WeightedEvent(const TofEvent&);

  WeightedEvent();

  ~WeightedEvent();

  /// Copy from another WeightedEvent object
  WeightedEvent& operator=(const WeightedEvent & rhs);

  bool operator==(const WeightedEvent & other) const;

  /// Return the weight of the neutron, as a double (it is saved as a float).
  double weight() const;

  /// Return the error of the neutron, as a double (it is saved as a float).
  double error() const;

  /// Return the squared error of the neutron, as a double
  double errorSquared() const;

  /// Output a string representation of the event to a stream
  friend std::ostream& operator<<(std::ostream &os, const WeightedEvent &event);


};









//==========================================================================================
/** Info about a single neutron detection event, including a weight and error value,
 * but excluding the pulsetime to save memory:
 *
 *  - the time of flight of the neutron (can be converted to other units)
 *  - the absolute time of the pulse at which it was produced
 *  - weight of the neutron (float, can be
 */
class DLLExport WeightedEventNoTime {

  /// EventList has the right to mess with this
  friend class EventList;
  friend class tofGreaterOrEqual;
  friend class tofGreater;

private:

  /// The time of flight of this neutron
  float m_tof;

  /// The weight of this neutron.
  float m_weight;

  /// The SQUARE of the error that this neutron contributes.
  float m_errorSquared;

public:
  /// Constructor, specifying only the time of flight
  WeightedEventNoTime(double time_of_flight);

  /// Constructor, full
  WeightedEventNoTime(double time_of_flight, float weight, float errorSquared);

  WeightedEventNoTime(const TofEvent&, float weight, float errorSquared);

  WeightedEventNoTime(const WeightedEventNoTime&);

  WeightedEventNoTime(const WeightedEvent&);

  WeightedEventNoTime(const TofEvent&);

  WeightedEventNoTime();

  ~WeightedEventNoTime();

  /// Copy from another WeightedEventNoTime object
  WeightedEventNoTime& operator=(const WeightedEventNoTime & rhs);

  bool operator==(const WeightedEventNoTime & other) const;

  /// Return the weight of the neutron, as a double (it is saved as a float).
  double weight() const
  {
    return m_weight;
  }

  /// Return the error of the neutron, as a double (it is saved as a float).
  double error() const
  {
    return std::sqrt( double(m_errorSquared) );
  }

  /// Return the squared error of the neutron, as a double
  double errorSquared() const
  {
    return m_errorSquared;
  }

  /// Output a string representation of the event to a stream
  friend std::ostream& operator<<(std::ostream &os, const WeightedEvent &event);


};




} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTS_H_
