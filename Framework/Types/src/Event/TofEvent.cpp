#include "MantidTypes/Event/TofEvent.h"

using std::ostream;

namespace Mantid {
namespace Types {
using Core::DateAndTime;
namespace Event {
//==========================================================================
/// --------------------- TofEvent stuff ----------------------------------
//==========================================================================
/** Constructor, specifying the time of flight only
 * @param tof :: time of flight, in microseconds
 */
TofEvent::TofEvent(const double tof) : m_tof(tof), m_pulsetime(0) {}

/** Constructor, specifying the time of flight and the frame id
 * @param tof :: time of flight, in microseconds
 * @param pulsetime :: absolute pulse time of the neutron.
 */
TofEvent::TofEvent(const double tof, const DateAndTime pulsetime)
    : m_tof(tof), m_pulsetime(pulsetime) {}

/// Empty constructor
TofEvent::TofEvent() : m_tof(0), m_pulsetime(0) {}

/** Comparison operator.
 * @param rhs: the other TofEvent to compare.
 * @return true if the TofEvent's are identical.*/
bool TofEvent::operator==(const TofEvent &rhs) const {
  return (this->m_tof == rhs.m_tof) && (this->m_pulsetime == rhs.m_pulsetime);
}

/** < comparison operator, using the TOF to do the comparison.
 * @param rhs: the other TofEvent to compare.
 * @return true if this->m_tof < rhs.m_tof*/
bool TofEvent::operator<(const TofEvent &rhs) const {
  return (this->m_tof < rhs.m_tof);
}

/** < comparison operator, using the TOF to do the comparison.
 * @param rhs: the other TofEvent to compare.
 * @return true if this->m_tof < rhs.m_tof*/
bool TofEvent::operator>(const TofEvent &rhs) const {
  return (this->m_tof > rhs.m_tof);
}

/** < comparison operator, using the TOF to do the comparison.
 * @param rhs_tof: the other time of flight to compare.
 * @return true if this->m_tof < rhs.m_tof*/
bool TofEvent::operator<(const double rhs_tof) const {
  return (this->m_tof < rhs_tof);
}

/**
 * Compare two events within the specified tolerance
 *
 * @param rhs the other TofEvent to compare
 * @param tolTof the tolerance of a difference in m_tof.
 * @param tolPulse the tolerance of a difference in m_pulsetime
 * in nanoseconds.
 *
 * @return True if the are the same within the specifed tolerances
 */
bool TofEvent::equals(const TofEvent &rhs, const double tolTof,
                      const int64_t tolPulse) const {
  // compare m_tof
  if (std::fabs(this->m_tof - rhs.m_tof) > tolTof)
    return false;
  // then it is just if the pulse-times are equal
  return (this->m_pulsetime.equals(rhs.m_pulsetime, tolPulse));
}

/** Output a string representation of the event to a stream
 * @param os :: Stream
 * @param event :: TofEvent to output to the stream
 */
ostream &operator<<(ostream &os, const TofEvent &event) {
  os << event.m_tof << "," << event.m_pulsetime.toSimpleString();
  return os;
}
} // namespace Event
} // namespace Types
} // namespace Mantid