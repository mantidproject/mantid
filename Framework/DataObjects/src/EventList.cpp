// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspaceMRU.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Unit.h"

#ifdef _MSC_VER
// qualifier applied to function type has no meaning; ignored
#pragma warning(disable : 4180)
#endif
#include "tbb/parallel_sort.h"
#ifdef _MSC_VER
#pragma warning(default : 4180)
#endif

#include <cfloat>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

using std::ostream;
using std::runtime_error;
using std::size_t;
using std::vector;

namespace Mantid {
namespace DataObjects {
using Types::Core::DateAndTime;
using Types::Event::TofEvent;
using namespace Mantid::API;

namespace {

const double SEC_TO_NANO = 1.e9;

/**
 * Calculate the corrected full time in nanoseconds
 * @param event : The event with pulse time and time-of-flight
 * @param tofFactor : Time of flight coefficient factor
 * @param tofShift : Tof shift in seconds
 * @return Corrected full time at sample in Nanoseconds.
 */
template <typename EventType>
int64_t calculateCorrectedFullTime(const EventType &event,
                                   const double tofFactor,
                                   const double tofShift) {
  return event.pulseTime().totalNanoseconds() +
         static_cast<int64_t>(tofFactor * (event.tof() * 1.0E3) +
                              (tofShift * 1.0E9));
}

/**
 * Type for comparing events in terms of time at sample
 */
template <typename EventType>
class CompareTimeAtSample
    : public std::binary_function<EventType, EventType, bool> {
private:
  const double m_tofFactor;
  const double m_tofShift;

public:
  CompareTimeAtSample(const double tofFactor, const double tofShift)
      : m_tofFactor(tofFactor), m_tofShift(tofShift) {}

  /**
   * Compare two events based on the time they arrived at the sample.
   * Coefficient is used to provide scaling.
   * For elastic scattering coefficient is L1 / (L1 + L2)
   * @param e1 :: first event to compare
   * @param e2 :: second event to compare
   * @param coefficient :: scaling coefficient
   * @return True if first event evaluates to be < second event, otherwise false
   */
  bool operator()(const EventType &e1, const EventType &e2) const {
    const auto tAtSample1 =
        calculateCorrectedFullTime(e1, m_tofFactor, m_tofShift);
    const auto tAtSample2 =
        calculateCorrectedFullTime(e2, m_tofFactor, m_tofShift);
    return (tAtSample1 < tAtSample2);
  }
};
} // namespace
//==========================================================================
/// --------------------- TofEvent Comparators
/// ----------------------------------
//==========================================================================
/** Compare two events' FRAME id, return true if e1 should be before e2.
 * @param e1 :: first event
 * @param e2 :: second event
 *  */
bool compareEventPulseTime(const TofEvent &e1, const TofEvent &e2) {
  return (e1.pulseTime() < e2.pulseTime());
}

/** Compare two events' FRAME id, return true if e1 should be before e2.
 *  Assuming that if e1's pulse time is earlier than e2's, then e1 must be
 * earlier regardless TOF value
 * @param e1 :: first event
 * @param e2 :: second event
 *  */
bool compareEventPulseTimeTOF(const TofEvent &e1, const TofEvent &e2) {

  if (e1.pulseTime() < e2.pulseTime()) {
    return true;
  } else if ((e1.pulseTime() == e2.pulseTime()) && (e1.tof() < e2.tof())) {
    return true;
  }

  return false;
}

// comparator for pulse time with tolerance
struct comparePulseTimeTOFDelta {
  explicit comparePulseTimeTOFDelta(const Types::Core::DateAndTime &start,
                                    const double seconds)
      : startNano(start.totalNanoseconds()),
        deltaNano(static_cast<int64_t>(seconds * SEC_TO_NANO)) {}

  bool operator()(const TofEvent &e1, const TofEvent &e2) {
    // get the pulse times converted into bin number from start time
    const int64_t e1Pulse =
        (e1.pulseTime().totalNanoseconds() - startNano) / deltaNano;
    const int64_t e2Pulse =
        (e2.pulseTime().totalNanoseconds() - startNano) / deltaNano;

    // compare with the calculated bin information
    if (e1Pulse < e2Pulse) {
      return true;
    } else if ((e1Pulse == e2Pulse) && (e1.tof() < e2.tof())) {
      return true;
    }

    return false;
  }

  int64_t startNano;
  int64_t deltaNano;
};

/// Constructor (empty)
// EventWorkspace is always histogram data and so is thus EventList
EventList::EventList()
    : m_histogram(HistogramData::Histogram::XMode::BinEdges,
                  HistogramData::Histogram::YMode::Counts),
      eventType(TOF), order(UNSORTED), mru(nullptr) {}

/** Constructor with a MRU list
 * @param mru :: pointer to the MRU of the parent EventWorkspace
 * @param specNo :: the spectrum number for the event list
 */
EventList::EventList(EventWorkspaceMRU *mru, specnum_t specNo)
    : IEventList(specNo), m_histogram(HistogramData::Histogram::XMode::BinEdges,
                                      HistogramData::Histogram::YMode::Counts),
      eventType(TOF), order(UNSORTED), mru(mru) {}

/** Constructor copying from an existing event list
 * @param rhs :: EventList object to copy*/
EventList::EventList(const EventList &rhs)
    : IEventList(rhs), m_histogram(rhs.m_histogram), mru{nullptr} {
  // Note that operator= also assigns m_histogram, but the above use of the copy
  // constructor avoid a memory allocation and is thus faster.
  this->operator=(rhs);
}

/** Constructor, taking a vector of events.
 * @param events :: Vector of TofEvent's */
EventList::EventList(const std::vector<TofEvent> &events)
    : m_histogram(HistogramData::Histogram::XMode::BinEdges,
                  HistogramData::Histogram::YMode::Counts),
      eventType(TOF), mru(nullptr) {
  this->events.assign(events.begin(), events.end());
  this->eventType = TOF;
  this->order = UNSORTED;
}

/** Constructor, taking a vector of events.
 * @param events :: Vector of WeightedEvent's */
EventList::EventList(const std::vector<WeightedEvent> &events)
    : m_histogram(HistogramData::Histogram::XMode::BinEdges,
                  HistogramData::Histogram::YMode::Counts),
      mru(nullptr) {
  this->weightedEvents.assign(events.begin(), events.end());
  this->eventType = WEIGHTED;
  this->order = UNSORTED;
}

/** Constructor, taking a vector of events.
 * @param events :: Vector of WeightedEventNoTime's */
EventList::EventList(const std::vector<WeightedEventNoTime> &events)
    : m_histogram(HistogramData::Histogram::XMode::BinEdges,
                  HistogramData::Histogram::YMode::Counts),
      mru(nullptr) {
  this->weightedEventsNoTime.assign(events.begin(), events.end());
  this->eventType = WEIGHTED_NOTIME;
  this->order = UNSORTED;
}

/// Destructor
EventList::~EventList() {
  // Note: These two lines do not seem to have an effect on releasing memory
  //  at least on Linux. (Memory usage seems to increase event after deleting
  //  EventWorkspaces.
  //  Therefore, for performance, they are kept commented:
  clear();

  // this->events.clear();
  // std::vector<TofEvent>().swap(events); //Trick to release the vector memory.
}

/// Copy data from another EventList, via ISpectrum reference.
void EventList::copyDataFrom(const ISpectrum &source) {
  source.copyDataInto(*this);
}

/// Used by copyDataFrom for dynamic dispatch for its `source`.
void EventList::copyDataInto(EventList &sink) const {
  sink.m_histogram = m_histogram;
  sink.events = events;
  sink.weightedEvents = weightedEvents;
  sink.weightedEventsNoTime = weightedEventsNoTime;
  sink.eventType = eventType;
  sink.order = order;
}

/// Used by Histogram1D::copyDataFrom for dynamic dispatch for `other`.
void EventList::copyDataInto(Histogram1D &sink) const {
  sink.setHistogram(histogram());
}

// --------------------------------------------------------------------------
/** Create an EventList from a histogram. This converts bins to weighted
 * events.
 * Any existing events are cleared.
 *
 * @param inSpec :: ISpectrum ptr to histogram data.
 * @param GenerateZeros :: if true, generate event(s) for empty bins
 * @param GenerateMultipleEvents :: if true, create several evenly-spaced fake
 *events inside the bin
 * @param MaxEventsPerBin :: max number of events to generate in one bin, if
 *GenerateMultipleEvents
 */
void EventList::createFromHistogram(const ISpectrum *inSpec, bool GenerateZeros,
                                    bool GenerateMultipleEvents,
                                    int MaxEventsPerBin) {
  // Fresh start
  this->clear(true);

  // Get the input histogram
  const MantidVec &X = inSpec->readX();
  const MantidVec &Y = inSpec->readY();
  const MantidVec &E = inSpec->readE();
  if (Y.size() + 1 != X.size())
    throw std::runtime_error(
        "Expected a histogram (X vector should be 1 longer than the Y vector)");

  // Copy detector IDs and spectra
  this->copyInfoFrom(*inSpec);
  // We need weights but have no way to set the time. So use weighted, no time
  this->switchTo(WEIGHTED_NOTIME);
  if (GenerateZeros)
    this->weightedEventsNoTime.reserve(Y.size());

  for (size_t i = 0; i < X.size() - 1; i++) {
    double weight = Y[i];
    if ((weight != 0.0 || GenerateZeros) && std::isfinite(weight)) {
      double error = E[i];
      // Also check that the error is not a bad number
      if (std::isfinite(error)) {
        if (GenerateMultipleEvents) {
          // --------- Multiple events per bin ----------
          double errorSquared = error * error;
          // Find how many events to fake
          double val = weight / E[i];
          val *= val;
          // Convert to int with slight rounding up. This is to avoid rounding
          // errors
          auto numEvents = int(val + 0.2);
          if (numEvents < 1)
            numEvents = 1;
          if (numEvents > MaxEventsPerBin)
            numEvents = MaxEventsPerBin;
          // Scale the weight and error for each
          weight /= numEvents;
          errorSquared /= numEvents;

          // Spread the TOF. e.g. 2 events = 0.25, 0.75.
          double tofStep = (X[i + 1] - X[i]) / (numEvents);
          for (size_t j = 0; j < size_t(numEvents); j++) {
            double tof = X[i] + tofStep * (0.5 + double(j));
            // Create and add the event
            // TODO: try emplace_back() here.
            weightedEventsNoTime.emplace_back(tof, weight, errorSquared);
          }
        } else {
          // --------- Single event per bin ----------
          // TOF = midpoint of the bin
          double tof = (X[i] + X[i + 1]) / 2.0;
          // Error squared is carried in the event
          double errorSquared = E[i];
          errorSquared *= errorSquared;
          // Create and add the event
          weightedEventsNoTime.emplace_back(tof, weight, errorSquared);
        }
      } // error is nont NAN or infinite
    }   // weight is non-zero, not NAN, and non-infinite
  }     // (each bin)

  // Set the X binning parameters
  this->setX(inSpec->ptrX());

  // Manually set that this is sorted by TOF, since it is. This will make it
  // "threadSafe" in other algos.
  this->setSortOrder(TOF_SORT);
}

// --------------------------------------------------------------------------
// --- Operators
// -------------------------------------------------------------------

/** Copy into this event list from another
 * @param rhs :: We will copy all the events from that into this object.
 * @return reference to this
 * */
EventList &EventList::operator=(const EventList &rhs) {
  // Note that we are NOT copying the MRU pointer.
  IEventList::operator=(rhs);
  m_histogram = rhs.m_histogram;
  events = rhs.events;
  weightedEvents = rhs.weightedEvents;
  weightedEventsNoTime = rhs.weightedEventsNoTime;
  eventType = rhs.eventType;
  order = rhs.order;
  return *this;
}

// --------------------------------------------------------------------------
/** Append an event to the histogram.
 * @param event :: TofEvent to add at the end of the list.
 * @return reference to this
 * */
EventList &EventList::operator+=(const TofEvent &event) {

  switch (this->eventType) {
  case TOF:
    // Simply push the events
    this->events.push_back(event);
    break;

  case WEIGHTED:
    this->weightedEvents.emplace_back(event);
    break;

  case WEIGHTED_NOTIME:
    this->weightedEventsNoTime.emplace_back(event);
    break;
  }

  this->order = UNSORTED;
  return *this;
}

// --------------------------------------------------------------------------
/** Append a list of events to the histogram.
 * The internal event list will switch to the required type.
 *
 * @param more_events :: A vector of events to append.
 * @return reference to this
 * */
EventList &EventList::operator+=(const std::vector<TofEvent> &more_events) {
  switch (this->eventType) {
  case TOF:
    // Simply push the events
    this->events.insert(this->events.end(), more_events.begin(),
                        more_events.end());
    break;

  case WEIGHTED:
    // Add default weights to all the un-weighted incoming events from the list.
    // and append to the list
    this->weightedEvents.reserve(this->weightedEvents.size() +
                                 more_events.size());
    for (const auto &event : more_events) {
      this->weightedEvents.emplace_back(event);
    }
    break;

  case WEIGHTED_NOTIME:
    // Add default weights to all the un-weighted incoming events from the list.
    // and append to the list
    this->weightedEventsNoTime.reserve(this->weightedEventsNoTime.size() +
                                       more_events.size());
    for (const auto &more_event : more_events)
      this->weightedEventsNoTime.emplace_back(more_event);
    break;
  }

  this->order = UNSORTED;
  return *this;
}

// --------------------------------------------------------------------------
/** Append a WeightedEvent to the histogram.
 * Note: The whole list will switch to weights (a possibly lengthy operation)
 *  if it did not have weights before.
 *
 * @param event :: WeightedEvent to add at the end of the list.
 * @return reference to this
 * */
EventList &EventList::operator+=(const WeightedEvent &event) {
  this->switchTo(WEIGHTED);
  this->weightedEvents.push_back(event);
  this->order = UNSORTED;
  return *this;
}

// --------------------------------------------------------------------------
/** Append a list of events to the histogram.
 * Note: The whole list will switch to weights (a possibly lengthy operation)
 *  if it did not have weights before.
 *
 * @param more_events :: A vector of events to append.
 * @return reference to this
 * */
EventList &EventList::
operator+=(const std::vector<WeightedEvent> &more_events) {
  switch (this->eventType) {
  case TOF:
    // Need to switch to weighted
    this->switchTo(WEIGHTED);
    // Fall through to the insertion!

  case WEIGHTED:
    // Append the two lists
    this->weightedEvents.insert(weightedEvents.end(), more_events.begin(),
                                more_events.end());
    break;

  case WEIGHTED_NOTIME:
    // Add default weights to all the un-weighted incoming events from the list.
    // and append to the list
    this->weightedEventsNoTime.reserve(this->weightedEventsNoTime.size() +
                                       more_events.size());
    for (const auto &event : more_events) {
      this->weightedEventsNoTime.emplace_back(event);
    }
    break;
  }

  this->order = UNSORTED;
  return *this;
}

// --------------------------------------------------------------------------
/** Append a list of events to the histogram.
 * Note: The whole list will switch to weights (a possibly lengthy operation)
 *  if it did not have weights before.
 *
 * @param more_events :: A vector of events to append.
 * @return reference to this
 * */
EventList &EventList::
operator+=(const std::vector<WeightedEventNoTime> &more_events) {
  switch (this->eventType) {
  case TOF:
  case WEIGHTED:
    // Need to switch to weighted with no time
    this->switchTo(WEIGHTED_NOTIME);
    // Fall through to the insertion!

  case WEIGHTED_NOTIME:
    // Simple appending of the two lists
    this->weightedEventsNoTime.insert(weightedEventsNoTime.end(),
                                      more_events.begin(), more_events.end());
    break;
  }

  this->order = UNSORTED;
  return *this;
}

// --------------------------------------------------------------------------
/** Append another EventList to this event list.
 * The event lists are concatenated, and a union of the sets of detector ID's is
 *done.
 * Switching of event types may occur if the two are different.
 *
 * @param more_events :: Another EventList.
 * @return reference to this
 * */
EventList &EventList::operator+=(const EventList &more_events) {
  // We'll let the += operator for the given vector of event lists handle it
  switch (more_events.getEventType()) {
  case TOF:
    this->operator+=(more_events.events);
    break;

  case WEIGHTED:
    this->operator+=(more_events.weightedEvents);
    break;

  case WEIGHTED_NOTIME:
    this->operator+=(more_events.weightedEventsNoTime);
    break;
  }

  // No guaranteed order
  this->order = UNSORTED;
  // Do a union between the detector IDs of both lists
  addDetectorIDs(more_events.getDetectorIDs());

  return *this;
}

// --------------------------------------------------------------------------
/** SUBTRACT another EventList from this event list.
 * The event lists are concatenated, but the weights of the incoming
 *    list are multiplied by -1.0.
 *
 * @tparam T1, T2 :: TofEvent, WeightedEvent or WeightedEventNoTime
 * @param events :: The event vector being changed.
 * @param more_events :: Another event vector being subtracted from this.
 * @return reference to this
 * */
template <class T1, class T2>
void EventList::minusHelper(std::vector<T1> &events,
                            const std::vector<T2> &more_events) {
  // Make the end vector big enough in one go (avoids repeated re-allocations).
  events.reserve(events.size() + more_events.size());
  /* In the event of subtracting in place, calling the end() vector would make
   * it point at the wrong place
   * Using it caused a segault, Ticket #2306.
   * So we cache the end (this speeds up too).
   */
  for (const auto &ev : more_events) {
    // We call the constructor for T1. In the case of WeightedEventNoTime, the
    // pulse time will just be ignored.
    events.emplace_back(ev.tof(), ev.pulseTime(), ev.weight() * (-1.0),
                        ev.errorSquared());
  }
}

// --------------------------------------------------------------------------
/** SUBTRACT another EventList from this event list.
 * The event lists are concatenated, but the weights of the incoming
 *    list are multiplied by -1.0.
 *
 * @param more_events :: Another EventList.
 * @return reference to this
 * */
EventList &EventList::operator-=(const EventList &more_events) {
  if (this == &more_events) {
    // Special case, ticket #3844 part 2.
    // When doing this = this - this,
    // simply clear the input event list. Saves memory!
    this->clearData();
    return *this;
  }

  // We'll let the -= operator for the given vector of event lists handle it
  switch (this->getEventType()) {
  case TOF:
    this->switchTo(WEIGHTED);
    // Fall through

  case WEIGHTED:
    switch (more_events.getEventType()) {
    case TOF:
      minusHelper(this->weightedEvents, more_events.events);
      break;
    case WEIGHTED:
      minusHelper(this->weightedEvents, more_events.weightedEvents);
      break;
    case WEIGHTED_NOTIME:
      // TODO: Should this throw?
      minusHelper(this->weightedEvents, more_events.weightedEventsNoTime);
      break;
    }
    break;

  case WEIGHTED_NOTIME:
    switch (more_events.getEventType()) {
    case TOF:
      minusHelper(this->weightedEventsNoTime, more_events.events);
      break;
    case WEIGHTED:
      minusHelper(this->weightedEventsNoTime, more_events.weightedEvents);
      break;
    case WEIGHTED_NOTIME:
      minusHelper(this->weightedEventsNoTime, more_events.weightedEventsNoTime);
      break;
    }
    break;
  }

  // No guaranteed order
  this->order = UNSORTED;

  // NOTE: What to do about detector ID's?
  return *this;
}

// --------------------------------------------------------------------------
/** Equality operator between EventList's
 * @param rhs :: other EventList to compare
 * @return :: true if equal.
 */
bool EventList::operator==(const EventList &rhs) const {
  if (this->getNumberEvents() != rhs.getNumberEvents())
    return false;
  if (this->eventType != rhs.eventType)
    return false;
  // Check all event lists; The empty ones will compare equal
  if (events != rhs.events)
    return false;
  if (weightedEvents != rhs.weightedEvents)
    return false;
  if (weightedEventsNoTime != rhs.weightedEventsNoTime)
    return false;
  return true;
}

/** Inequality comparator
 * @param rhs :: other EventList to compare
 * @return :: true if not equal.
 */
bool EventList::operator!=(const EventList &rhs) const {
  return (!this->operator==(rhs));
}

bool EventList::equals(const EventList &rhs, const double tolTof,
                       const double tolWeight, const int64_t tolPulse) const {
  // generic checks
  if (this->getNumberEvents() != rhs.getNumberEvents())
    return false;
  if (this->eventType != rhs.eventType)
    return false;

  // loop over the events
  size_t numEvents = this->getNumberEvents();
  switch (this->eventType) {
  case TOF:
    for (size_t i = 0; i < numEvents; ++i) {
      if (!this->events[i].equals(rhs.events[i], tolTof, tolPulse))
        return false;
    }
    break;
  case WEIGHTED:
    for (size_t i = 0; i < numEvents; ++i) {
      if (!this->weightedEvents[i].equals(rhs.weightedEvents[i], tolTof,
                                          tolWeight, tolPulse))
        return false;
    }
    break;
  case WEIGHTED_NOTIME:
    for (size_t i = 0; i < numEvents; ++i) {
      if (!this->weightedEventsNoTime[i].equals(rhs.weightedEventsNoTime[i],
                                                tolTof, tolWeight))
        return false;
    }
    break;
  default:
    break;
  }

  // anything that gets this far is equal within tolerances
  return true;
}

// -----------------------------------------------------------------------------------------------
/** Return the type of Event vector contained within.
 * @return :: a EventType value.
 */
EventType EventList::getEventType() const { return eventType; }

// -----------------------------------------------------------------------------------------------
/** Switch the EventList to use the given EventType (TOF, WEIGHTED, or
 * WEIGHTED_NOTIME)
 */
void EventList::switchTo(EventType newType) {
  switch (newType) {
  case TOF:
    if (eventType != TOF)
      throw std::runtime_error("EventList::switchTo() called on an EventList "
                               "with weights to go down to TofEvent's. This "
                               "would remove weight information and therefore "
                               "is not possible.");
    break;

  case WEIGHTED:
    switchToWeightedEvents();
    break;

  case WEIGHTED_NOTIME:
    switchToWeightedEventsNoTime();
    break;
  }
  // Make sure to free memory
  this->clearUnused();
}

// -----------------------------------------------------------------------------------------------
/** Switch the EventList to use WeightedEvents instead
 * of TofEvent.
 */
void EventList::switchToWeightedEvents() {
  switch (eventType) {
  case WEIGHTED:
    // Do nothing; it already is weighted
    return;

  case WEIGHTED_NOTIME:
    throw std::runtime_error("EventList::switchToWeightedEvents() called on an "
                             "EventList with WeightedEventNoTime's. It has "
                             "lost the pulse time information and can't go "
                             "back to WeightedEvent's.");
    break;

  case TOF:
    weightedEventsNoTime.clear();
    // Convert and copy all TofEvents to the weightedEvents list.
    this->weightedEvents.assign(events.cbegin(), events.cend());
    // Get rid of the old events
    events.clear();
    eventType = WEIGHTED;
    break;
  }
}

// -----------------------------------------------------------------------------------------------
/** Switch the EventList to use WeightedEventNoTime's instead
 * of TofEvent.
 */
void EventList::switchToWeightedEventsNoTime() {
  switch (eventType) {
  case WEIGHTED_NOTIME:
    // Do nothing if already there
    return;

  case TOF: {
    // Convert and copy all TofEvents to the weightedEvents list.
    this->weightedEventsNoTime.assign(events.cbegin(), events.cend());
    // Get rid of the old events
    events.clear();
    weightedEvents.clear();
    eventType = WEIGHTED_NOTIME;
  } break;

  case WEIGHTED: {
    // Convert and copy all TofEvents to the weightedEvents list.
    this->weightedEventsNoTime.assign(weightedEvents.cbegin(),
                                      weightedEvents.cend());
    // Get rid of the old events
    events.clear();
    weightedEvents.clear();
    eventType = WEIGHTED_NOTIME;
  } break;
  }
}

// ==============================================================================================
// --- Testing functions (mostly)
// ---------------------------------------------------------------
// ==============================================================================================

/** Return the given event in the list.
 * Handles the different types of events by converting to WeightedEvent (the
 * most general type).
 * @param event_number :: the index of the event to retrieve
 * @return a WeightedEvent
 */
WeightedEvent EventList::getEvent(size_t event_number) {
  switch (eventType) {
  case TOF:
    return WeightedEvent(events[event_number]);
  case WEIGHTED:
    return weightedEvents[event_number];
  case WEIGHTED_NOTIME:
    return WeightedEvent(weightedEventsNoTime[event_number].tof(), 0,
                         weightedEventsNoTime[event_number].weight(),
                         weightedEventsNoTime[event_number].errorSquared());
  }
  throw std::runtime_error("EventList: invalid event type value was found.");
}

// ==============================================================================================
// --- Handling the event list
// -------------------------------------------------------------------
// ==============================================================================================

/** Return the const list of TofEvents contained.
 * NOTE! This should be used for testing purposes only, as much as possible. The
 *EventList
 * may contain weighted events, requiring use of getWeightedEvents() instead.
 *
 * @return a const reference to the list of non-weighted events
 * */
const std::vector<TofEvent> &EventList::getEvents() const {
  if (eventType != TOF)
    throw std::runtime_error("EventList::getEvents() called for an EventList "
                             "that has weights. Use getWeightedEvents() or "
                             "getWeightedEventsNoTime().");
  return this->events;
}

/** Return the list of TofEvents contained.
 * NOTE! This should be used for testing purposes only, as much as possible. The
 *EventList
 * may contain weighted events, requiring use of getWeightedEvents() instead.
 *
 * @return a reference to the list of non-weighted events
 * */
std::vector<TofEvent> &EventList::getEvents() {
  if (eventType != TOF)
    throw std::runtime_error("EventList::getEvents() called for an EventList "
                             "that has weights. Use getWeightedEvents() or "
                             "getWeightedEventsNoTime().");
  return this->events;
}

/** Return the list of WeightedEvent contained.
 * NOTE! This should be used for testing purposes only, as much as possible. The
 *EventList
 * may contain un-weighted events, requiring use of getEvents() instead.
 *
 * @return a reference to the list of weighted events
 * */
std::vector<WeightedEvent> &EventList::getWeightedEvents() {
  if (eventType != WEIGHTED)
    throw std::runtime_error("EventList::getWeightedEvents() called for an "
                             "EventList not of type WeightedEvent. Use "
                             "getEvents() or getWeightedEventsNoTime().");
  return this->weightedEvents;
}

/** Return the list of WeightedEvent contained.
 * NOTE! This should be used for testing purposes only, as much as possible. The
 *EventList
 * may contain un-weighted events, requiring use of getEvents() instead.
 *
 * @return a const reference to the list of weighted events
 * */
const std::vector<WeightedEvent> &EventList::getWeightedEvents() const {
  if (eventType != WEIGHTED)
    throw std::runtime_error("EventList::getWeightedEvents() called for an "
                             "EventList not of type WeightedEvent. Use "
                             "getEvents() or getWeightedEventsNoTime().");
  return this->weightedEvents;
}

/** Return the list of WeightedEvent contained.
 * NOTE! This should be used for testing purposes only, as much as possible.
 *
 * @return a reference to the list of weighted events
 * */
std::vector<WeightedEventNoTime> &EventList::getWeightedEventsNoTime() {
  if (eventType != WEIGHTED_NOTIME)
    throw std::runtime_error("EventList::getWeightedEvents() called for an "
                             "EventList not of type WeightedEventNoTime. Use "
                             "getEvents() or getWeightedEvents().");
  return this->weightedEventsNoTime;
}

/** Return the list of WeightedEventNoTime contained.
 * NOTE! This should be used for testing purposes only, as much as possible.
 *
 * @return a const reference to the list of weighted events
 * */
const std::vector<WeightedEventNoTime> &
EventList::getWeightedEventsNoTime() const {
  if (eventType != WEIGHTED_NOTIME)
    throw std::runtime_error("EventList::getWeightedEventsNoTime() called for "
                             "an EventList not of type WeightedEventNoTime. "
                             "Use getEvents() or getWeightedEvents().");
  return this->weightedEventsNoTime;
}

/** Clear the list of events and any
 * associated detector ID's.
 * */
void EventList::clear(const bool removeDetIDs) {
  if (mru)
    mru->deleteIndex(this);
  this->events.clear();
  std::vector<TofEvent>().swap(this->events); // STL Trick to release memory
  this->weightedEvents.clear();
  std::vector<WeightedEvent>().swap(
      this->weightedEvents); // STL Trick to release memory
  this->weightedEventsNoTime.clear();
  std::vector<WeightedEventNoTime>().swap(
      this->weightedEventsNoTime); // STL Trick to release memory
  if (removeDetIDs)
    this->clearDetectorIDs();
}

/** Clear any unused event lists (the ones that do not
 * match the currently used type).
 * Memory is freed.
 * */
void EventList::clearUnused() {
  if (eventType != TOF) {
    this->events.clear();
    std::vector<TofEvent>().swap(this->events); // STL Trick to release memory
  }
  if (eventType != WEIGHTED) {
    this->weightedEvents.clear();
    std::vector<WeightedEvent>().swap(
        this->weightedEvents); // STL Trick to release memory
  }
  if (eventType != WEIGHTED_NOTIME) {
    this->weightedEventsNoTime.clear();
    std::vector<WeightedEventNoTime>().swap(
        this->weightedEventsNoTime); // STL Trick to release memory
  }
}

/// Mask the spectrum to this value. Removes all events.
void EventList::clearData() { this->clear(false); }

/** Sets the MRU list for this event list
 *
 * @param newMRU :: new MRU for the workspace containing this EventList
 */
void EventList::setMRU(EventWorkspaceMRU *newMRU) { mru = newMRU; }

/** Reserve a certain number of entries in the (NOT-WEIGHTED) event list. Do NOT
 *call
 * on weighted events!
 *
 * Calls std::vector<>::reserve() in order to pre-allocate the length of the
 *event list vector.
 *
 * @param num :: number of events that will be in this EventList
 */
void EventList::reserve(size_t num) { this->events.reserve(num); }

// ==============================================================================================
// --- Sorting functions -----------------------------------------------------
// ==============================================================================================

// --------------------------------------------------------------------------
/** Sort events by TOF or Frame
 * @param order :: Order by which to sort.
 * */
void EventList::sort(const EventSortType order) const {
  if (order == UNSORTED) {
    return; // don't bother doing anything. Why did you ask to unsort?
  } else if (order == TOF_SORT) {
    this->sortTof();
  } else if (order == PULSETIME_SORT) {
    this->sortPulseTime();
  } else if (order == PULSETIMETOF_SORT) {
    this->sortPulseTimeTOF();
  } else if (order == PULSETIMETOF_DELTA_SORT) {
    throw std::invalid_argument("sorting by pulse time with delta requires "
                                "extra parameters. Use sortPulseTimeTOFDelta "
                                "instead.");
  } else if (order == TIMEATSAMPLE_SORT) {
    throw std::invalid_argument("sorting by time at sample requires extra "
                                "parameters. Use sortTimeAtSample instead.");
  } else {
    throw runtime_error("Invalid sort type in EventList::sort(EventSortType)");
  }
}

// --------------------------------------------------------------------------
/** Manually set the event list sort order value. No actual sorting takes place.
 * SHOULD ONLY BE USED IN TESTS or if you know what you are doing.
 * @param order :: sort order to set.
 */
void EventList::setSortOrder(const EventSortType order) const {
  this->order = order;
}

// --------------------------------------------------------------------------
/** Sort events by TOF in one thread */
void EventList::sortTof() const {
  if (this->order == TOF_SORT)
    return; // nothing to do

  // Avoid sorting from multiple threads
  std::lock_guard<std::mutex> _lock(m_sortMutex);
  // If the list was sorted while waiting for the lock, return.
  if (this->order == TOF_SORT)
    return;

  switch (eventType) {
  case TOF:
    tbb::parallel_sort(events.begin(), events.end());
    break;
  case WEIGHTED:
    tbb::parallel_sort(weightedEvents.begin(), weightedEvents.end());
    break;
  case WEIGHTED_NOTIME:
    tbb::parallel_sort(weightedEventsNoTime.begin(),
                       weightedEventsNoTime.end());
    break;
  }
  // Save the order to avoid unnecessary re-sorting.
  this->order = TOF_SORT;
}

// --------------------------------------------------------------------------
/**
 * Sort events by time at sample
 * @param tofFactor : For the elastic case, L1 / (L1 + L2)
 * @param tofShift : Tof offset in Seconds
 * @param forceResort : If the tofFactor, or tofShift are different
 *   from a previous run of the same sort type, you need to trigger a full
 * resort using forceResort = true. False by default.
 */
void EventList::sortTimeAtSample(const double &tofFactor,
                                 const double &tofShift,
                                 bool forceResort) const {
  // Check pre-cached sort flag.
  if (this->order == TIMEATSAMPLE_SORT && !forceResort)
    return;

  // Avoid sorting from multiple threads
  std::lock_guard<std::mutex> _lock(m_sortMutex);
  // If the list was sorted while waiting for the lock, return.
  if (this->order == TIMEATSAMPLE_SORT && !forceResort)
    return;

  // Perform sort.
  switch (eventType) {
  case TOF: {
    CompareTimeAtSample<TofEvent> comparitor(tofFactor, tofShift);
    tbb::parallel_sort(events.begin(), events.end(), comparitor);
  } break;
  case WEIGHTED: {
    CompareTimeAtSample<WeightedEvent> comparitor(tofFactor, tofShift);
    tbb::parallel_sort(weightedEvents.begin(), weightedEvents.end(),
                       comparitor);
  } break;
  case WEIGHTED_NOTIME: {
    CompareTimeAtSample<WeightedEventNoTime> comparitor(tofFactor, tofShift);
    tbb::parallel_sort(weightedEventsNoTime.begin(), weightedEventsNoTime.end(),
                       comparitor);
  } break;
  }
  // Save the order to avoid unnecessary re-sorting.
  this->order = TIMEATSAMPLE_SORT;
}

// --------------------------------------------------------------------------
/** Sort events by Frame */
void EventList::sortPulseTime() const {
  if (this->order == PULSETIME_SORT)
    return; // nothing to do

  // Avoid sorting from multiple threads
  std::lock_guard<std::mutex> _lock(m_sortMutex);
  // If the list was sorted while waiting for the lock, return.
  if (this->order == PULSETIME_SORT)
    return;

  // Perform sort.
  switch (eventType) {
  case TOF:
    tbb::parallel_sort(events.begin(), events.end(), compareEventPulseTime);
    break;
  case WEIGHTED:
    tbb::parallel_sort(weightedEvents.begin(), weightedEvents.end(),
                       compareEventPulseTime);
    break;
  case WEIGHTED_NOTIME:
    // Do nothing; there is no time to sort
    break;
  }
  // Save the order to avoid unnecessary re-sorting.
  this->order = PULSETIME_SORT;
}

/*
 * Sort events by pulse time + TOF
 * (the absolute time)
 */
void EventList::sortPulseTimeTOF() const {
  if (this->order == PULSETIMETOF_SORT)
    return; // already ordered.

  // Avoid sorting from multiple threads
  std::lock_guard<std::mutex> _lock(m_sortMutex);
  // If the list was sorted while waiting for the lock, return.
  if (this->order == PULSETIMETOF_SORT)
    return;

  switch (eventType) {
  case TOF:
    tbb::parallel_sort(events.begin(), events.end(), compareEventPulseTimeTOF);
    break;
  case WEIGHTED:
    tbb::parallel_sort(weightedEvents.begin(), weightedEvents.end(),
                       compareEventPulseTimeTOF);
    break;
  case WEIGHTED_NOTIME:
    // Do nothing; there is no time to sort
    break;
  }

  // Save
  this->order = PULSETIMETOF_SORT;
}

/**
 * Sort by the pulse time with a tolerance. The pulsetime to compare is a
 * constant binning of seconds from start. This will set the sort order to
 * UNSORTED upon completion rather than storing the call parameters.
 * @param start The absolute start time
 * @param seconds The tolerance of pulse time in seconds.
 */
void EventList::sortPulseTimeTOFDelta(const Types::Core::DateAndTime &start,
                                      const double seconds) const {
  // Avoid sorting from multiple threads
  std::lock_guard<std::mutex> _lock(m_sortMutex);

  std::function<bool(const TofEvent &, const TofEvent &)> comparator =
      comparePulseTimeTOFDelta(start, seconds);

  switch (eventType) {
  case TOF:
    tbb::parallel_sort(events.begin(), events.end(), comparator);
    break;
  case WEIGHTED:
    tbb::parallel_sort(weightedEvents.begin(), weightedEvents.end(),
                       comparator);
    break;
  case WEIGHTED_NOTIME:
    // Do nothing; there is no time to sort
    break;
  }

  this->order = UNSORTED; // so the function always re-runs
}

// --------------------------------------------------------------------------
/** Return true if the event list is sorted by TOF */
bool EventList::isSortedByTof() const { return (this->order == TOF_SORT); }

// --------------------------------------------------------------------------
/** Return the type of sorting used in this event list */
EventSortType EventList::getSortType() const { return this->order; }

// --------------------------------------------------------------------------
/** Reverse the histogram boundaries and the associated events if they are
 * sorted
 * by time-of-flight.
 * Does nothing if sorted otherwise or unsorted.
 * */
void EventList::reverse() {
  // reverse the histogram bin parameters
  MantidVec &x = dataX();
  std::reverse(x.begin(), x.end());

  // flip the events if they are tof sorted
  if (this->isSortedByTof()) {
    switch (eventType) {
    case TOF:
      std::reverse(this->events.begin(), this->events.end());
      break;
    case WEIGHTED:
      std::reverse(this->weightedEvents.begin(), this->weightedEvents.end());
      break;
    case WEIGHTED_NOTIME:
      std::reverse(this->weightedEventsNoTime.begin(),
                   this->weightedEventsNoTime.end());
      break;
    }
    // And we are still sorted! :)
  }
  // Otherwise, do nothing. If it was sorted by pulse time, then it still is
}

// --------------------------------------------------------------------------
/** Return the number of events in the list.
 * NOTE: If the events have weights, this returns the NUMBER of WeightedEvent's
 *in the
 * list, and NOT the sum of their weights (which may be two different numbers).
 *
 * @return the number of events in the list.
 *  */
size_t EventList::getNumberEvents() const {
  switch (eventType) {
  case TOF:
    return this->events.size();
  case WEIGHTED:
    return this->weightedEvents.size();
  case WEIGHTED_NOTIME:
    return this->weightedEventsNoTime.size();
  }
  throw std::runtime_error("EventList: invalid event type value was found.");
}

/**
 * Much like stl containers, returns true if there is nothing in the event list.
 */
bool EventList::empty() const {
  switch (eventType) {
  case TOF:
    return this->events.empty();
  case WEIGHTED:
    return this->weightedEvents.empty();
  case WEIGHTED_NOTIME:
    return this->weightedEventsNoTime.empty();
  }
  throw std::runtime_error("EventList: invalid event type value was found.");
}

// --------------------------------------------------------------------------
/** Memory used by this event list. Note: It reports the CAPACITY of the
 * vectors, rather than their size, since that is a more accurate
 * representation of the size used.
 *
 * @return :: the memory used by the EventList, in bytes.
 * */
size_t EventList::getMemorySize() const {
  switch (eventType) {
  case TOF:
    return this->events.capacity() * sizeof(TofEvent) + sizeof(EventList);
  case WEIGHTED:
    return this->weightedEvents.capacity() * sizeof(WeightedEvent) +
           sizeof(EventList);
  case WEIGHTED_NOTIME:
    return this->weightedEventsNoTime.capacity() * sizeof(WeightedEventNoTime) +
           sizeof(EventList);
  }
  throw std::runtime_error("EventList: invalid event type value was found.");
}

// --------------------------------------------------------------------------
/** Return the size of the histogram data.
 * @return the size of the histogram representation of the data (size of Y) **/
size_t EventList::histogram_size() const {
  size_t x_size = readX().size();
  if (x_size > 1)
    return x_size - 1;
  else
    return 0;
}

// ==============================================================================================
// --- Setting the Histogram X axis, without recalculating the histogram
// -----------------------
// ==============================================================================================

/** Deprecated, use setSharedX() instead. Set the x-component for the histogram
 * view. This will NOT cause the histogram to be calculated.
 * @param X :: The vector of doubles to set as the histogram limits.
 */
void EventList::setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) {
  m_histogram.setX(X);
  if (mru)
    mru->deleteIndex(this);
}

/** Deprecated, use mutableX() instead. Returns a reference to the x data.
 *  @return a reference to the X (bin) vector.
 */
MantidVec &EventList::dataX() {
  if (mru)
    mru->deleteIndex(this);
  return m_histogram.dataX();
}

/** Deprecated, use x() instead. Returns a const reference to the x data.
 *  @return a reference to the X (bin) vector. */
const MantidVec &EventList::dataX() const { return m_histogram.dataX(); }

/// Deprecated, use x() instead. Returns the x data const
const MantidVec &EventList::readX() const { return m_histogram.readX(); }

/// Deprecated, use sharedX() instead. Returns a pointer to the x data
Kernel::cow_ptr<HistogramData::HistogramX> EventList::ptrX() const {
  return m_histogram.ptrX();
}

/// Deprecated, use mutableDx() instead.
MantidVec &EventList::dataDx() { return m_histogram.dataDx(); }
/// Deprecated, use dx() instead.
const MantidVec &EventList::dataDx() const { return m_histogram.dataDx(); }
/// Deprecated, use dx() instead.
const MantidVec &EventList::readDx() const { return m_histogram.readDx(); }

// ==============================================================================================
// --- Return Data Vectors --------------------------------------------------
// ==============================================================================================

/** Calculates and returns a pointer to the Y histogrammed data.
 * Remember to delete your pointer after use!
 *
 * @return a pointer to a MantidVec
 */
MantidVec *EventList::makeDataY() const {
  auto Y = new MantidVec();
  MantidVec E;
  // Generate the Y histogram while skipping the E if possible.
  generateHistogram(readX(), *Y, E, true);
  return Y;
}

/** Calculates and returns a pointer to the E histogrammed data.
 * Remember to delete your pointer after use!
 *
 * @return a pointer to a MantidVec
 */
MantidVec *EventList::makeDataE() const {
  MantidVec Y;
  auto E = new MantidVec();
  generateHistogram(readX(), Y, *E);
  // Y is unused.
  return E;
}

HistogramData::Histogram EventList::histogram() const {
  HistogramData::Histogram ret(m_histogram);
  ret.setSharedY(sharedY());
  ret.setSharedE(sharedE());
  return ret;
}

HistogramData::Counts EventList::counts() const { return histogram().counts(); }

HistogramData::CountVariances EventList::countVariances() const {
  return histogram().countVariances();
}

HistogramData::CountStandardDeviations
EventList::countStandardDeviations() const {
  return histogram().countStandardDeviations();
}

HistogramData::Frequencies EventList::frequencies() const {
  return histogram().frequencies();
}

HistogramData::FrequencyVariances EventList::frequencyVariances() const {
  return histogram().frequencyVariances();
}

HistogramData::FrequencyStandardDeviations
EventList::frequencyStandardDeviations() const {
  return histogram().frequencyStandardDeviations();
}

const HistogramData::HistogramY &EventList::y() const {
  if (!mru)
    throw std::runtime_error(
        "'EventList::y()' called with no MRU set. This is not allowed.");

  return *sharedY();
}
const HistogramData::HistogramE &EventList::e() const {
  if (!mru)
    throw std::runtime_error(
        "'EventList::e()' called with no MRU set. This is not allowed.");

  return *sharedE();
}
Kernel::cow_ptr<HistogramData::HistogramY> EventList::sharedY() const {
  // This is the thread number from which this function was called.
  int thread = PARALLEL_THREAD_NUMBER;

  Kernel::cow_ptr<HistogramData::HistogramY> yData(nullptr);

  // Is the data in the mrulist?
  if (mru) {
    mru->ensureEnoughBuffersY(thread);
    yData = mru->findY(thread, this);
  }

  if (!yData) {
    MantidVec Y;
    MantidVec E;
    this->generateHistogram(readX(), Y, E);

    // Create the MRU object
    yData = Kernel::make_cow<HistogramData::HistogramY>(std::move(Y));

    // Lets save it in the MRU
    if (mru) {
      mru->insertY(thread, yData, this);
      auto eData = Kernel::make_cow<HistogramData::HistogramE>(std::move(E));
      mru->ensureEnoughBuffersE(thread);
      mru->insertE(thread, eData, this);
    }
  }
  return yData;
}
Kernel::cow_ptr<HistogramData::HistogramE> EventList::sharedE() const {
  // This is the thread number from which this function was called.
  int thread = PARALLEL_THREAD_NUMBER;

  Kernel::cow_ptr<HistogramData::HistogramE> eData(nullptr);

  // Is the data in the mrulist?
  if (mru) {
    mru->ensureEnoughBuffersE(thread);
    eData = mru->findE(thread, this);
  }

  if (!eData) {
    // Now use that to get E -- Y values are generated from another function
    MantidVec Y_ignored;
    MantidVec E;
    this->generateHistogram(readX(), Y_ignored, E);
    eData = Kernel::make_cow<HistogramData::HistogramE>(std::move(E));

    // Lets save it in the MRU
    if (mru)
      mru->insertE(thread, eData, this);
  }
  return eData;
}
/** Look in the MRU to see if the Y histogram has been generated before.
 * If so, return that. If not, calculate, cache and return it.
 *
 * @return reference to the Y vector.
 */
const MantidVec &EventList::dataY() const {
  if (!mru)
    throw std::runtime_error(
        "'EventList::dataY()' called with no MRU set. This is not allowed.");

  // WARNING: The Y data of sharedY() is stored in MRU, returning reference fine
  // as long as it stays there.
  return sharedY()->rawData();
}

/** Look in the MRU to see if the E histogram has been generated before.
 * If so, return that. If not, calculate, cache and return it.
 *
 * @return reference to the E vector.
 */
const MantidVec &EventList::dataE() const {
  if (!mru)
    throw std::runtime_error(
        "'EventList::dataE()' called with no MRU set. This is not allowed.");

  // WARNING: The E data of sharedE() is stored in MRU, returning reference fine
  // as long as it stays there.
  return sharedE()->rawData();
}

namespace {
inline double calcNorm(const double errorSquared) {
  if (errorSquared == 0.)
    return 0;
  else if (errorSquared == 1.)
    return 1.;
  else
    return 1. / std::sqrt(errorSquared);
}
} // namespace

// --------------------------------------------------------------------------
/** Compress the event list by grouping events with the same TOF.
 *
 * @param events :: input event list.
 * @param out :: output WeightedEventNoTime vector.
 * @param tolerance :: how close do two event's TOF have to be to be considered
 *the same.
 */

template <class T>
inline void
EventList::compressEventsHelper(const std::vector<T> &events,
                                std::vector<WeightedEventNoTime> &out,
                                double tolerance) {
  // Clear the output. We can't know ahead of time how much space to reserve :(
  out.clear();
  // We will make a starting guess of 1/20th of the number of input events.
  out.reserve(events.size() / 20);

  // The last TOF to which we are comparing.
  double lastTof = std::numeric_limits<double>::lowest();
  // For getting an accurate average TOF
  double totalTof = 0;
  int num = 0;
  // Carrying weight, error, and normalization
  double weight = 0;
  double errorSquared = 0;
  double normalization = 0.;

  for (auto it = events.cbegin(); it != events.cend(); it++) {
    if ((it->m_tof - lastTof) <= tolerance) {
      // Carry the error and weight
      weight += it->weight();
      errorSquared += it->errorSquared();
      // Track the average tof
      num++;
      const double norm = calcNorm(it->errorSquared());
      normalization += norm;
      totalTof += it->m_tof * norm;
    } else {
      // We exceeded the tolerance
      // Create a new event with the average TOF and summed weights and
      // squared errors.
      if (num == 1) {
        // last time-of-flight is the only one contributing
        out.emplace_back(lastTof, weight, errorSquared);
      } else if (num > 1) {
        out.emplace_back(totalTof / normalization, weight, errorSquared);
      }
      // Start a new combined object
      num = 1;
      const double norm = calcNorm(it->errorSquared());
      normalization = norm;
      totalTof = it->m_tof * norm;
      weight = it->weight();
      errorSquared = it->errorSquared();
      lastTof = it->m_tof;
    }
  }

  // Put the last event in there too with the average TOF and summed weights and
  // squared errors.
  if (num == 1) {
    // last time-of-flight is the only one contributing
    out.emplace_back(lastTof, weight, errorSquared);
  } else if (num > 1) {
    out.emplace_back(totalTof / normalization, weight, errorSquared);
  }

  // If you have over-allocated by more than 5%, reduce the size.
  size_t excess_limit = out.size() / 20;
  if ((out.capacity() - out.size()) > excess_limit) {
    out.shrink_to_fit();
  }
}

// --------------------------------------------------------------------------
/** Compress the event list by grouping events with the same TOF.
 * Performs the compression in parallel.
 *
 * @param events :: input event list.
 * @param out :: output WeightedEventNoTime vector.
 * @param tolerance :: how close do two event's TOF have to be to be considered
 *the same.
 */

template <class T>
void EventList::compressEventsParallelHelper(
    const std::vector<T> &events, std::vector<WeightedEventNoTime> &out,
    double tolerance) {
  // Create a local output vector for each thread
  int numThreads = PARALLEL_GET_MAX_THREADS;
  std::vector<std::vector<WeightedEventNoTime>> outputs(numThreads);
  // This is how many events to process in each thread.
  size_t numPerBlock = events.size() / numThreads;

  // Do each block in parallel
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int thread = 0; thread < numThreads; thread++) {
    // The local output vector
    std::vector<WeightedEventNoTime> &localOut = outputs[thread];
    // Reserve a bit of space to avoid excess copying
    localOut.clear();
    localOut.reserve(numPerBlock / 20);

    // The last TOF to which we are comparing.
    double lastTof = std::numeric_limits<double>::lowest();
    // For getting an accurate average TOF
    double totalTof = 0;
    int num = 0;
    // Carrying weight, error, and normalization
    double weight = 0;
    double errorSquared = 0;
    double normalization = 0.;

    // Separate the
    typename std::vector<T>::const_iterator it =
        events.begin() + thread * numPerBlock;
    typename std::vector<T>::const_iterator it_end =
        events.begin() + (thread + 1) * numPerBlock; // cache for speed
    if (thread == numThreads - 1)
      it_end = events.end();
    for (; it != it_end; ++it) {
      if ((it->m_tof - lastTof) <= tolerance) {
        // Carry the error and weight
        weight += it->weight();
        errorSquared += it->errorSquared();
        // Track the average tof
        num++;
        const double norm = calcNorm(it->errorSquared());
        normalization += norm;
        totalTof += it->m_tof * norm;
      } else {
        // We exceeded the tolerance
        if (num > 0) {
          // Create a new event with the average TOF and summed weights and
          // squared errors.
          localOut.emplace_back(totalTof / normalization, weight, errorSquared);
        }
        // Start a new combined object
        num = 1;
        const double norm = calcNorm(it->errorSquared());
        normalization = norm;
        totalTof = it->m_tof * norm;
        weight = it->weight();
        errorSquared = it->errorSquared();
        lastTof = it->m_tof;
      }
    }

    // Put the last event in there too.
    if (num > 0) {
      // Create a new event with the average TOF and summed weights and squared
      // errors.
      localOut.emplace_back(totalTof / normalization, weight, errorSquared);
    }
  }

  // Clear the output. Reserve the required size
  out.clear();
  size_t numEvents = 0;
  for (int thread = 0; thread < numThreads; thread++)
    numEvents += outputs[thread].size();
  out.reserve(numEvents);

  // Re-join all the outputs
  for (int thread = 0; thread < numThreads; thread++)
    out.insert(out.end(), outputs[thread].begin(), outputs[thread].end());
}

template <class T>
inline void EventList::compressFatEventsHelper(
    const std::vector<T> &events, std::vector<WeightedEvent> &out,
    const double tolerance, const Types::Core::DateAndTime &timeStart,
    const double seconds) {
  // Clear the output. We can't know ahead of time how much space to reserve :(
  out.clear();
  // We will make a starting guess of 1/20th of the number of input events.
  out.reserve(events.size() / 20);

  // The last TOF to which we are comparing.
  double lastTof = std::numeric_limits<double>::lowest();
  // For getting an accurate average TOF
  double totalTof = 0;

  // pulsetime bin information - stored as int nanoseconds because it
  // is the implementation type for DateAndTime object
  const int64_t pulsetimeStart = timeStart.totalNanoseconds();
  const auto pulsetimeDelta = static_cast<int64_t>(seconds * SEC_TO_NANO);

  // pulsetime information
  std::vector<DateAndTime> pulsetimes; // all the times for new event
  std::vector<double> pulsetimeWeights;

  // Carrying weight and error
  double weight = 0.;
  double errorSquared = 0.;
  double tofNormalization = 0.;

  // Move up to first event that has a large enough pulsetime. This is just in
  // case someone starts from after the starttime of the run. It is expected
  // that users will normally use the default which means this will only check
  // the first event.
  auto it = events.cbegin();
  for (; it != events.cend(); ++it) {
    if (it->m_pulsetime >= timeStart)
      break;
  }

  // bin if the pulses are histogrammed
  int64_t lastPulseBin =
      (it->m_pulsetime.totalNanoseconds() - pulsetimeStart) / pulsetimeDelta;
  // loop through events and accumulate weight
  for (; it != events.cend(); ++it) {
    const int64_t eventPulseBin =
        (it->m_pulsetime.totalNanoseconds() - pulsetimeStart) / pulsetimeDelta;
    if ((eventPulseBin <= lastPulseBin) &&
        (std::fabs(it->m_tof - lastTof) <= tolerance)) {
      // Carry the error and weight
      weight += it->weight();
      errorSquared += it->errorSquared();
      double norm = calcNorm(it->errorSquared());
      tofNormalization += norm;
      // Track the average tof
      totalTof += it->m_tof * norm;
      // Accumulate the pulse times
      pulsetimes.push_back(it->m_pulsetime);
      pulsetimeWeights.push_back(norm);
    } else {
      // We exceeded the tolerance
      if (!pulsetimes.empty()) {
        // Create a new event with the average TOF and summed weights and
        // squared errors. 1 event used doesn't need to average
        if (pulsetimes.size() == 1) {
          out.emplace_back(lastTof, pulsetimes.front(), weight, errorSquared);
        } else {
          out.emplace_back(totalTof / tofNormalization,
                           Kernel::DateAndTimeHelpers::averageSorted(
                               pulsetimes, pulsetimeWeights),
                           weight, errorSquared);
        }
      }
      // Start a new combined object
      double norm = calcNorm(it->errorSquared());
      totalTof = it->m_tof * norm;
      weight = it->weight();
      errorSquared = it->errorSquared();
      tofNormalization = norm;
      lastTof = it->m_tof;
      lastPulseBin = eventPulseBin;
      pulsetimes.clear();
      pulsetimes.push_back(it->m_pulsetime);
      pulsetimeWeights.clear();
      pulsetimeWeights.push_back(norm);
    }
  }

  // Put the last event in there too.
  if (!pulsetimes.empty()) {
    // Create a new event with the average TOF and summed weights and
    // squared errors. 1 event used doesn't need to average
    if (pulsetimes.size() == 1) {
      out.emplace_back(lastTof, pulsetimes.front(), weight, errorSquared);
    } else {
      out.emplace_back(totalTof / tofNormalization,
                       Kernel::DateAndTimeHelpers::averageSorted(
                           pulsetimes, pulsetimeWeights),
                       weight, errorSquared);
    }
  }

  // If you have over-allocated by more than 5%, reduce the size.
  size_t excess_limit = out.size() / 20;
  if ((out.capacity() - out.size()) > excess_limit) {
    out.shrink_to_fit();
  }
}

// --------------------------------------------------------------------------
/** Compress the event list by grouping events with the same
 * TOF (within a given tolerance). PulseTime is ignored.
 * The event list will be switched to WeightedEventNoTime.
 *
 * @param tolerance :: how close do two event's TOF have to be to be considered
 *the same.
 * @param destination :: EventList that will receive the compressed events. Can
 *be == this.
 */
void EventList::compressEvents(double tolerance, EventList *destination) {
  if (!this->empty()) {
    this->sortTof();
    switch (eventType) {
    case TOF:
      //      if (parallel)
      //        compressEventsParallelHelper(this->events,
      //        destination->weightedEventsNoTime, tolerance);
      //      else
      compressEventsHelper(this->events, destination->weightedEventsNoTime,
                           tolerance);
      break;

    case WEIGHTED:
      //      if (parallel)
      //        compressEventsParallelHelper(this->weightedEvents,
      //        destination->weightedEventsNoTime, tolerance);
      //      else
      compressEventsHelper(this->weightedEvents,
                           destination->weightedEventsNoTime, tolerance);

      break;

    case WEIGHTED_NOTIME:
      if (destination == this) {
        // Put results in a temp output
        std::vector<WeightedEventNoTime> out;
        //        if (parallel)
        //          compressEventsParallelHelper(this->weightedEventsNoTime,
        //          out,
        //          tolerance);
        //        else
        compressEventsHelper(this->weightedEventsNoTime, out, tolerance);
        // Put it back
        this->weightedEventsNoTime.swap(out);
      } else {
        //        if (parallel)
        //          compressEventsParallelHelper(this->weightedEventsNoTime,
        //          destination->weightedEventsNoTime, tolerance);
        //        else
        compressEventsHelper(this->weightedEventsNoTime,
                             destination->weightedEventsNoTime, tolerance);
      }
      break;
    }
  }
  // In all cases, you end up WEIGHTED_NOTIME.
  destination->eventType = WEIGHTED_NOTIME;
  // The sort is still valid!
  destination->order = TOF_SORT;
  // Empty out storage for vectors that are now unused.
  destination->clearUnused();
}

void EventList::compressFatEvents(
    const double tolerance, const Mantid::Types::Core::DateAndTime &timeStart,
    const double seconds, EventList *destination) {

  // only worry about non-empty EventLists
  if (!this->empty()) {
    switch (eventType) {
    case WEIGHTED_NOTIME:
      throw std::invalid_argument(
          "Cannot compress events that do not have pulsetime");
    case TOF:
      this->sortPulseTimeTOFDelta(timeStart, seconds);
      compressFatEventsHelper(this->events, destination->weightedEvents,
                              tolerance, timeStart, seconds);
      break;
    case WEIGHTED:
      this->sortPulseTimeTOFDelta(timeStart, seconds);
      if (destination == this) {
        // Put results in a temp output
        std::vector<WeightedEvent> out;
        compressFatEventsHelper(this->weightedEvents, out, tolerance, timeStart,
                                seconds);
        // Put it back
        this->weightedEvents.swap(out);
      } else {
        compressFatEventsHelper(this->weightedEvents,
                                destination->weightedEvents, tolerance,
                                timeStart, seconds);
      }
      break;
    }
  }
  // In all cases, you end up WEIGHTED_NOTIME.
  destination->eventType = WEIGHTED;
  // The sort order is pulsetimetof as we've compressed out the tolerance
  destination->order = PULSETIMETOF_SORT;
  // Empty out storage for vectors that are now unused.
  destination->clearUnused();
}

// --------------------------------------------------------------------------
/** Utility function:
 * Returns the iterator into events of the first TofEvent with
 * tof() > seek_tof
 * Will return events.end() if nothing is found!
 *
 * @param events :: event vector in which to look.
 * @param seek_tof :: tof to find (typically the first bin X[0])
 * @return iterator where the first event matching it is.
 */
template <class T>
typename std::vector<T>::const_iterator static findFirstEvent(
    const std::vector<T> &events, T seek_tof) {
  return std::find_if_not(events.cbegin(), events.cend(),
                          [seek_tof](const T &x) { return x < seek_tof; });
}

// --------------------------------------------------------------------------
/** Utility function:
 * Returns the iterator into events of the first TofEvent with
 * pulsetime() > seek_pulsetime
 * Will return events.end() if nothing is found!
 *
 * @param events :: event vector in which to look.
 * @param seek_pulsetime :: pulse time to find (typically the first bin X[0])
 * @return iterator where the first event matching it is.
 */
template <class T>
typename std::vector<T>::const_iterator
EventList::findFirstPulseEvent(const std::vector<T> &events,
                               const double seek_pulsetime) {
  auto itev = events.begin();
  auto itev_end = events.end(); // cache for speed

  // if tof < X[0], that means that you need to skip some events
  while ((itev != itev_end) &&
         (static_cast<double>(itev->pulseTime().totalNanoseconds()) <
          seek_pulsetime))
    itev++;
  // Better fix would be to use a binary search instead of the linear one used
  // here.
  return itev;
}

// --------------------------------------------------------------------------
/** Utility function:
 * Returns the iterator into events of the first TofEvent with
 * time at sample > seek_time
 * Will return events.end() if nothing is found!
 *
 * @param events :: event vector in which to look.
 * @param seek_time :: seek time to find (typically the first bin X[0]). Seek
 *time in nanoseconds.
 * @param tofFactor :: Time of flight factor
 * @param tofOffset :: Time of flight offset
 * @return iterator where the first event matching it is.
 */
template <class T>
typename std::vector<T>::const_iterator EventList::findFirstTimeAtSampleEvent(
    const std::vector<T> &events, const double seek_time,
    const double &tofFactor, const double &tofOffset) const {
  auto itev = events.cbegin();
  auto itev_end = events.cend(); // cache for speed

  // if tof < X[0], that means that you need to skip some events
  while ((itev != itev_end) && (static_cast<double>(calculateCorrectedFullTime(
                                    *itev, tofFactor, tofOffset)) < seek_time))
    itev++;
  // Better fix would be to use a binary search instead of the linear one used
  // here.
  return itev;
}

// --------------------------------------------------------------------------
/** Utility function:
 * Returns the iterator into events of the first TofEvent with
 * tof() > seek_tof
 * Will return events.end() if nothing is found!
 *
 * @param events :: event vector in which to look.
 * @param seek_tof :: tof to find (typically the first bin X[0])
 * @return iterator where the first event matching it is.
 */
template <class T>
typename std::vector<T>::iterator static findFirstEvent(std::vector<T> &events,
                                                        T seek_tof) {
  return std::find_if_not(events.begin(), events.end(),
                          [seek_tof](const T &x) { return x < seek_tof; });
}

// --------------------------------------------------------------------------
/** Generates both the Y and E (error) histograms
 * for an EventList with WeightedEvents.
 *
 * @param events: vector of events (with weights)
 * @param X: X-bins supplied
 * @param Y: counts returned
 * @param E: errors returned
 * @throw runtime_error if the EventList does not have weighted events
 */
template <class T>
void EventList::histogramForWeightsHelper(const std::vector<T> &events,
                                          const MantidVec &X, MantidVec &Y,
                                          MantidVec &E) {
  // For slight speed=up.
  size_t x_size = X.size();

  if (x_size <= 1) {
    // X was not set. Return an empty array.
    Y.resize(0, 0);
    return;
  }

  // If the sizes are the same, then the "resize" command will NOT clear the
  // original values.
  bool mustFill = (Y.size() == x_size - 1);
  // Clear the Y data, assign all to 0.
  Y.resize(x_size - 1, 0.0);
  // Clear the Error data, assign all to 0.
  // Note: Errors will be squared until the last step.
  E.resize(x_size - 1, 0.0);

  if (mustFill) {
    // We must make sure the starting point is 0.0
    std::fill(Y.begin(), Y.end(), 0.0);
    std::fill(E.begin(), E.end(), 0.0);
  }

  //---------------------- Histogram without weights
  //---------------------------------

  // Do we even have any events to do?
  if (!events.empty()) {
    // Iterate through all events (sorted by tof)
    auto itev = findFirstEvent(events, T(X[0]));
    auto itev_end = events.cend();
    // The above can still take you to end() if no events above X[0], so check
    // again.
    if (itev == itev_end)
      return;

    // Find the first bin
    size_t bin = 0;
    // The tof is greater the first bin boundary, so we need to find the first
    // bin
    double tof = itev->tof();
    while (bin < x_size - 1) {
      // Within range?
      if ((tof >= X[bin]) && (tof < X[bin + 1])) {
        // Add up the weight (convert to double before adding, to preserve
        // precision)
        Y[bin] += double(itev->m_weight);
        E[bin] += double(itev->m_errorSquared); // square of error
        break;
      }
      ++bin;
    }
    // Go to the next event, we've already binned this first one.
    ++itev;

    // Keep going through all the events
    while ((itev != itev_end) && (bin < x_size - 1)) {
      tof = itev->tof();
      while (bin < x_size - 1) {
        // Within range? Since both events and X are sorted, they are going to
        // have
        // tof >= X[bin] because the previous event was.
        if (tof < X[bin + 1]) {
          // Add up the weight (convert to double before adding, to preserve
          // precision)
          Y[bin] += double(itev->m_weight);
          E[bin] += double(itev->m_errorSquared); // square of error
          break;
        }
        ++bin;
      }
      ++itev;
    }
  } // end if (there are any events to histogram)

  // Now do the sqrt of all errors
  std::transform(E.begin(), E.end(), E.begin(),
                 static_cast<double (*)(double)>(sqrt));
}

// --------------------------------------------------------------------------
/** Generates both the Y and E (error) histograms w.r.t Pulse Time
 * for an EventList with or without WeightedEvents.
 *
 * @param X: x-bins supplied
 * @param Y: counts returned
 * @param E: errors returned
 * @param skipError: skip calculating the error. This has no effect for weighted
 *        events; you can just ignore the returned E vector.
 */
void EventList::generateHistogramPulseTime(const MantidVec &X, MantidVec &Y,
                                           MantidVec &E, bool skipError) const {
  // All types of weights need to be sorted by Pulse Time
  this->sortPulseTime();

  switch (eventType) {
  case TOF:
    // Make the single ones
    this->generateCountsHistogramPulseTime(X, Y);
    if (!skipError)
      this->generateErrorsHistogram(Y, E);
    break;

  case WEIGHTED:
    throw std::runtime_error("Cannot histogram by pulse time on Weighted "
                             "Events currently"); // This could be supported.

  case WEIGHTED_NOTIME:
    throw std::runtime_error(
        "Cannot histogram by pulse time on Weighted Events NoTime");
  }
}

/**
 * Generates both the Y and E (error) histograms w.r.t Time at sample position.
 * @param X: X - axis supplied as reference
 * @param Y: counts to fill
 * @param E: errors to fill
 * @param tofFactor : Time of flight factor. Usually L1/(L1 + L2)
 * @param tofOffset : Time of flight offset.
 * @param skipError : skip calculating the error. This has no effect for
 * weighted
 *          events; you can just ignore the returned E vector.
 */
void EventList::generateHistogramTimeAtSample(const MantidVec &X, MantidVec &Y,
                                              MantidVec &E,
                                              const double &tofFactor,
                                              const double &tofOffset,
                                              bool skipError) const {
  // All types of weights need to be sorted by time at sample
  this->sortTimeAtSample(tofFactor, tofOffset);

  switch (eventType) {
  case TOF:
    // Make the single ones
    this->generateCountsHistogramTimeAtSample(X, Y, tofFactor, tofOffset);
    if (!skipError)
      this->generateErrorsHistogram(Y, E);
    break;

  case WEIGHTED:
    throw std::runtime_error("Cannot histogram by time at sample on Weighted "
                             "Events currently"); // This could be supported.

  case WEIGHTED_NOTIME:
    throw std::runtime_error(
        "Cannot histogram by time at sample on Weighted Events NoTime");
  }
}

// --------------------------------------------------------------------------
/** Generates both the Y and E (error) histograms w.r.t TOF
 * for an EventList with or without WeightedEvents.
 *
 * @param X: x-bins supplied
 * @param Y: counts returned
 * @param E: errors returned
 * @param skipError: skip calculating the error. This has no effect for weighted
 *        events; you can just ignore the returned E vector.
 */
void EventList::generateHistogram(const MantidVec &X, MantidVec &Y,
                                  MantidVec &E, bool skipError) const {
  // All types of weights need to be sorted by TOF

  this->sortTof();

  switch (eventType) {
  case TOF:
    // Make the single ones
    this->generateCountsHistogram(X, Y);
    if (!skipError)
      this->generateErrorsHistogram(Y, E);
    break;

  case WEIGHTED:
    histogramForWeightsHelper(this->weightedEvents, X, Y, E);
    break;

  case WEIGHTED_NOTIME:
    histogramForWeightsHelper(this->weightedEventsNoTime, X, Y, E);
    break;
  }
}

// --------------------------------------------------------------------------
/** With respect to PulseTime Fill a histogram given specified histogram bounds.
 * Does not modify
 * the eventlist (const method).
 * @param X :: The x bins
 * @param Y :: The generated counts histogram
 */
void EventList::generateCountsHistogramPulseTime(const MantidVec &X,
                                                 MantidVec &Y) const {
  // For slight speed=up.
  size_t x_size = X.size();

  if (x_size <= 1) {
    // X was not set. Return an empty array.
    Y.resize(0, 0);
    return;
  }

  // Sort the events by pulsetime
  this->sortPulseTime();
  // Clear the Y data, assign all to 0.
  Y.resize(x_size - 1, 0);

  //---------------------- Histogram without weights
  //---------------------------------

  if (!this->events.empty()) {
    // Iterate through all events (sorted by pulse time)
    auto itev = findFirstPulseEvent(this->events, X[0]);
    auto itev_end = events.cend(); // cache for speed
    // The above can still take you to end() if no events above X[0], so check
    // again.
    if (itev == itev_end)
      return;

    // Find the first bin
    size_t bin = 0;

    // The tof is greater the first bin boundary, so we need to find the first
    // bin
    double pulsetime =
        static_cast<double>(itev->pulseTime().totalNanoseconds());
    while (bin < x_size - 1) {
      // Within range?
      if ((pulsetime >= X[bin]) && (pulsetime < X[bin + 1])) {
        Y[bin]++;
        break;
      }
      ++bin;
    }
    // Go to the next event, we've already binned this first one.
    ++itev;

    // Keep going through all the events
    while ((itev != itev_end) && (bin < x_size - 1)) {
      pulsetime = static_cast<double>(itev->pulseTime().totalNanoseconds());
      while (bin < x_size - 1) {
        // Within range?
        if ((pulsetime >= X[bin]) && (pulsetime < X[bin + 1])) {
          Y[bin]++;
          break;
        }
        ++bin;
      }
      ++itev;
    }
  } // end if (there are any events to histogram)
}

/** With respect to PulseTime fill a histogram given equal histogram
 *   bins.
 * Number of bins is equal to number of elements in vector Y.
 * Appends values to existing Y values.
 *
 * @param xMin :: Minimal Pulse time (in nanoseconds,
 *                i.e. DateTime->totalNanoseconds()) value to include
 *                in binning.
 * @param xMax :: Maximal Pulse time value to constrain binning by (include the
 *                times smaller than right boundary, excluding equal)
 * @param Y :: The generated counts histogram
 * @param TOF_min -- min TOF to include in histogram.
 * @param TOF_max -- max TOF to constrain values included in histogram.
 */
void EventList::generateCountsHistogramPulseTime(const double &xMin,
                                                 const double &xMax,
                                                 MantidVec &Y,
                                                 const double TOF_min,
                                                 const double TOF_max) const {

  if (this->events.empty())
    return;

  size_t nBins = Y.size();

  if (nBins == 0)
    return;

  double step = (xMax - xMin) / static_cast<double>(nBins);

  for (const TofEvent &ev : this->events) {
    double pulsetime = static_cast<double>(ev.pulseTime().totalNanoseconds());
    if (pulsetime < xMin || pulsetime >= xMax)
      continue;
    if (ev.tof() < TOF_min || ev.tof() >= TOF_max)
      continue;

    auto n_bin = static_cast<size_t>((pulsetime - xMin) / step);
    Y[n_bin]++;
  }
}

// --------------------------------------------------------------------------
/** With respect to Time at Sample, fill a histogram given specified histogram
 * bounds. Does not modify
 * the eventlist (const method).
 * @param X :: The x bins
 * @param Y :: The generated counts histogram
 * @param tofFactor :: time of flight factor
 * @param tofOffset :: time of flight offset
 */
void EventList::generateCountsHistogramTimeAtSample(
    const MantidVec &X, MantidVec &Y, const double &tofFactor,
    const double &tofOffset) const {
  // For slight speed=up.
  const size_t x_size = X.size();

  if (x_size <= 1) {
    // X was not set. Return an empty array.
    Y.resize(0, 0);
    return;
  }

  // Sort the events by pulsetime
  this->sortTimeAtSample(tofFactor, tofOffset);
  // Clear the Y data, assign all to 0.
  Y.resize(x_size - 1, 0);

  //---------------------- Histogram without weights
  //---------------------------------

  if (!this->events.empty()) {
    // Iterate through all events (sorted by pulse time)
    auto itev =
        findFirstTimeAtSampleEvent(this->events, X[0], tofFactor, tofOffset);
    std::vector<TofEvent>::const_iterator itev_end =
        events.end(); // cache for speed
    // The above can still take you to end() if no events above X[0], so check
    // again.
    if (itev == itev_end)
      return;

    // Find the first bin
    size_t bin = 0;

    auto tAtSample = static_cast<double>(
        calculateCorrectedFullTime(*itev, tofFactor, tofOffset));
    while (bin < x_size - 1) {
      // Within range?
      if ((tAtSample >= X[bin]) && (tAtSample < X[bin + 1])) {
        Y[bin]++;
        break;
      }
      ++bin;
    }
    // Go to the next event, we've already binned this first one.
    ++itev;

    // Keep going through all the events
    while ((itev != itev_end) && (bin < x_size - 1)) {
      tAtSample = static_cast<double>(
          calculateCorrectedFullTime(*itev, tofFactor, tofOffset));
      while (bin < x_size - 1) {
        // Within range?
        if ((tAtSample >= X[bin]) && (tAtSample < X[bin + 1])) {
          Y[bin]++;
          break;
        }
        ++bin;
      }
      ++itev;
    }
  } // end if (there are any events to histogram)
}

// --------------------------------------------------------------------------
/** Fill a histogram given specified histogram bounds. Does not modify
 * the eventlist (const method).
 * @param X :: The x bins
 * @param Y :: The generated counts histogram
 */
void EventList::generateCountsHistogram(const MantidVec &X,
                                        MantidVec &Y) const {
  // For slight speed=up.
  size_t x_size = X.size();

  if (x_size <= 1) {
    // X was not set. Return an empty array.
    Y.resize(0, 0);
    return;
  }

  // Sort the events by tof
  this->sortTof();
  // Clear the Y data, assign all to 0.
  Y.resize(x_size - 1, 0);

  //---------------------- Histogram without weights
  //---------------------------------

  // Do we even have any events to do?
  if (!this->events.empty()) {
    // Iterate through all events (sorted by tof) placing them in the correct
    // bin.
    auto itev = findFirstEvent(this->events, TofEvent(X[0]));
    // Go through all the events,
    for (auto itx = X.cbegin(); itev != events.end(); ++itev) {
      double tof = itev->tof();
      itx = std::find_if(itx, X.cend(),
                         [tof](const double x) { return tof < x; });
      if (itx == X.cend()) {
        break;
      }
      auto bin =
          std::max(std::distance(X.cbegin(), itx) - 1, std::ptrdiff_t{0});
      ++Y[bin];
    }
  } // end if (there are any events to histogram)
}

// --------------------------------------------------------------------------
/**
 * Generate the Error histogram for the provided counts histogram.
 * It simply returns the sqrt of the number of counts for each bin.
 *
 * @param Y :: The counts histogram
 * @param E :: The generated error histogram
 */
void EventList::generateErrorsHistogram(const MantidVec &Y,
                                        MantidVec &E) const {
  // Fill the vector for the errors, containing sqrt(count)
  E.resize(Y.size(), 0);

  // windows can get confused about std::sqrt
  std::transform(Y.begin(), Y.end(), E.begin(),
                 static_cast<double (*)(double)>(sqrt));

} //----------------------------------------------------------------------------------
/** Integrate the events between a range of X values, or all events.
 *
 * @param events :: reference to a vector of events to change.
 * @param minX :: minimum X bin to use in integrating.
 * @param maxX :: maximum X bin to use in integrating.
 * @param entireRange :: set to true to use the entire range. minX and maxX are
 *then ignored!
 * @return the integrated number of events.
 */
template <class T>
double EventList::integrateHelper(std::vector<T> &events, const double minX,
                                  const double maxX, const bool entireRange) {
  double sum(0), error(0);
  integrateHelper(events, minX, maxX, entireRange, sum, error);
  return sum;
}

/** Integrate the events between a range of X values, or all events.
 *
 * @param events :: reference to a vector of events to change.
 * @param minX :: minimum X bin to use in integrating.
 * @param maxX :: maximum X bin to use in integrating.
 * @param entireRange :: set to true to use the entire range. minX and maxX are
 *then ignored!
 * @param sum :: reference to a double to put the sum in.
 * @param error :: reference to a double to put the error in.
 */
template <class T>
void EventList::integrateHelper(std::vector<T> &events, const double minX,
                                const double maxX, const bool entireRange,
                                double &sum, double &error) {
  sum = 0;
  error = 0;
  // Nothing in the list?
  if (events.empty())
    return;

  // Iterators for limits - whole range by default
  typename std::vector<T>::iterator lowit, highit;
  lowit = events.begin();
  highit = events.end();

  // But maybe we don't want the entire range?
  if (!entireRange) {
    // If a silly range was given, return 0.
    if (maxX < minX)
      return;

    // If the first element is lower that the xmin then search for new lowit
    if (lowit->tof() < minX)
      lowit = std::lower_bound(events.begin(), events.end(), minX);
    // If the last element is higher that the xmax then search for new lowit
    if ((highit - 1)->tof() > maxX) {
      highit = std::upper_bound(lowit, events.end(), T(maxX));
    }
  }

  // Sum up all the weights
  for (auto it = lowit; it != highit; ++it) {
    sum += it->weight();
    error += it->errorSquared();
  }
  error = std::sqrt(error);
}

// --------------------------------------------------------------------------
/** Integrate the events between a range of X values, or all events.
 *
 * @param minX :: minimum X bin to use in integrating.
 * @param maxX :: maximum X bin to use in integrating.
 * @param entireRange :: set to true to use the entire range. minX and maxX are
 *then ignored!
 * @return the integrated number of events.
 */
double EventList::integrate(const double minX, const double maxX,
                            const bool entireRange) const {
  double sum(0), error(0);
  integrate(minX, maxX, entireRange, sum, error);
  return sum;
}

/** Integrate the events between a range of X values, or all events.
 *
 * @param minX :: minimum X bin to use in integrating.
 * @param maxX :: maximum X bin to use in integrating.
 * @param entireRange :: set to true to use the entire range. minX and maxX are
 *then ignored!
 * @param sum :: place holder for the resulting sum
 * @param error :: place holder for the resulting sum of errors
 * @return the integrated number of events.
 */
void EventList::integrate(const double minX, const double maxX,
                          const bool entireRange, double &sum,
                          double &error) const {
  sum = 0;
  error = 0;
  if (!entireRange) {
    // The event list must be sorted by TOF!
    this->sortTof();
  }

  // Convert the list
  switch (eventType) {
  case TOF:
    integrateHelper(this->events, minX, maxX, entireRange, sum, error);
    break;
  case WEIGHTED:
    integrateHelper(this->weightedEvents, minX, maxX, entireRange, sum, error);
    break;
  case WEIGHTED_NOTIME:
    integrateHelper(this->weightedEventsNoTime, minX, maxX, entireRange, sum,
                    error);
    break;
  default:
    throw std::runtime_error("EventList: invalid event type value was found.");
  }
}

// ==============================================================================================
// ----------- Conversion Functions (changing tof values)
// ---------------------------------------
// ==============================================================================================

/**
 * @param func Function to do the conversion.
 * @param sorting How the events are sorted after the operation. 0 = unsorted
 * (default),
 * positive = unchanged, negative = reverse.
 */
void EventList::convertTof(std::function<double(double)> func,
                           const int sorting) {
  // fix the histogram parameter
  MantidVec &x = dataX();
  transform(x.begin(), x.end(), x.begin(), func);

  // do nothing if sorting > 0
  if (sorting == 0) {
    this->setSortOrder(UNSORTED);
  } else if ((sorting < 0) && (this->getSortType() == TOF_SORT)) {
    this->reverse();
  }

  if (this->getNumberEvents() <= 0)
    return;

  // Convert the list
  switch (eventType) {
  case TOF:
    this->convertTofHelper(this->events, func);
    break;
  case WEIGHTED:
    this->convertTofHelper(this->weightedEvents, func);
    break;
  case WEIGHTED_NOTIME:
    this->convertTofHelper(this->weightedEventsNoTime, func);
    break;
  }
}

/**
 * @param events
 * @param func
 */
template <class T>
void EventList::convertTofHelper(std::vector<T> &events,
                                 std::function<double(double)> func) {
  // iterate through all events
  for (auto &ev : events)
    ev.m_tof = func(ev.m_tof);
}

// --------------------------------------------------------------------------
/**
 * Convert the time of flight by tof'=tof*factor+offset
 * @param factor :: The value to scale the time-of-flight by
 * @param offset :: The value to shift the time-of-flight by
 */
void EventList::convertTof(const double factor, const double offset) {
  // fix the histogram parameter
  auto &x = mutableX();
  x *= factor;
  x += offset;

  if ((factor < 0.) && (this->getSortType() == TOF_SORT))
    this->reverse();

  if (this->getNumberEvents() <= 0)
    return;

  // Convert the list
  switch (eventType) {
  case TOF:
    this->convertTofHelper(this->events, factor, offset);
    break;
  case WEIGHTED:
    this->convertTofHelper(this->weightedEvents, factor, offset);
    break;
  case WEIGHTED_NOTIME:
    this->convertTofHelper(this->weightedEventsNoTime, factor, offset);
    break;
  }
}

// --------------------------------------------------------------------------
/** Function to do the conversion factor work
 * on either the TofEvent list or the WeightedEvent list.
 * Does NOT reverse the event list if the factor < 0
 *
 * @param events :: reference to a vector of events to change.
 * @param factor :: multiply by this
 * @param offset :: add this
 */
template <class T>
void EventList::convertTofHelper(std::vector<T> &events, const double factor,
                                 const double offset) {
  // iterate through all events
  for (auto &event : events) {
    event.m_tof = event.m_tof * factor + offset;
  }
}

// --------------------------------------------------------------------------
/**
 * Convert the units in the TofEvent's m_tof field to
 *  some other value, by scaling by a multiplier.
 * @param factor :: conversion factor (e.g. multiply TOF by this to get
 * d-spacing)
 */
void EventList::scaleTof(const double factor) { this->convertTof(factor, 0.0); }

// --------------------------------------------------------------------------
/** Add an offset to the TOF of each event in the list.
 *
 * @param offset :: The value to shift the time-of-flight by
 */
void EventList::addTof(const double offset) { this->convertTof(1.0, offset); }

// --------------------------------------------------------------------------
/** Add an offset to the pulsetime (wall-clock time) of each event in the list.
 *
 * @param events :: reference to a vector of events to change.
 * @param seconds :: The value to shift the pulsetime by, in seconds
 */
template <class T>
void EventList::addPulsetimeHelper(std::vector<T> &events,
                                   const double seconds) {
  // iterate through all events
  for (auto &event : events) {
    event.m_pulsetime += seconds;
  }
}

// --------------------------------------------------------------------------
/** Add an offset to the pulsetime (wall-clock time) of each event in the list.
 *
 * @param seconds :: The value to shift the pulsetime by, in seconds
 */
void EventList::addPulsetime(const double seconds) {
  if (this->getNumberEvents() <= 0)
    return;

  // Convert the list
  switch (eventType) {
  case TOF:
    this->addPulsetimeHelper(this->events, seconds);
    break;
  case WEIGHTED:
    this->addPulsetimeHelper(this->weightedEvents, seconds);
    break;
  case WEIGHTED_NOTIME:
    throw std::runtime_error("EventList::addPulsetime() called on an event "
                             "list with no pulse times. You must call this "
                             "algorithm BEFORE CompressEvents.");
    break;
  }
}

// --------------------------------------------------------------------------
/** Mask out events that have a tof between tofMin and tofMax (inclusively).
 * Events are removed from the list.
 * @param events :: reference to a vector of events to change.
 * @param tofMin :: lower bound of TOF to filter out
 * @param tofMax :: upper bound of TOF to filter out
 * @returns The number of events deleted.
 */
template <class T>
std::size_t EventList::maskTofHelper(std::vector<T> &events,
                                     const double tofMin, const double tofMax) {
  // quick checks to make sure that the masking range is even in the data
  if (tofMin > events.rbegin()->tof())
    return 0;
  if (tofMax < events.begin()->tof())
    return 0;

  // Find the index of the first tofMin
  auto it_first = std::lower_bound(events.begin(), events.end(), tofMin);
  if ((it_first != events.end()) && (it_first->tof() < tofMax)) {
    // Something was found
    // Look for the first one > tofMax
    auto it_last = std::upper_bound(it_first, events.end(), T(tofMax));

    if (it_first >= it_last) {
      throw std::runtime_error("Event filter is all messed up"); // TODO
    }

    size_t tmp = (it_last - it_first);
    // it_last will either be at the end (if not found) or before it.
    // Erase this range from the vector
    events.erase(it_first, it_last);

    // Done! Sorting is still valid, no need to redo.
    return tmp; //(it_last - it_first); the iterators get invalid after erase
                //(on my machine)
  }
  return 0;
}

// --------------------------------------------------------------------------
/**
 * Mask out events that have a tof between tofMin and tofMax (inclusively).
 * Events are removed from the list.
 * @param tofMin :: lower bound of TOF to filter out
 * @param tofMax :: upper bound of TOF to filter out
 */
void EventList::maskTof(const double tofMin, const double tofMax) {
  if (tofMax <= tofMin)
    throw std::runtime_error("EventList::maskTof: tofMax must be > tofMin");

  // don't do anything with an emply list
  if (this->getNumberEvents() == 0)
    return;

  // Start by sorting by tof
  this->sortTof();

  // Convert the list
  size_t numOrig = 0;
  size_t numDel = 0;
  switch (eventType) {
  case TOF:
    numOrig = this->events.size();
    numDel = this->maskTofHelper(this->events, tofMin, tofMax);
    break;
  case WEIGHTED:
    numOrig = this->weightedEvents.size();
    numDel = this->maskTofHelper(this->weightedEvents, tofMin, tofMax);
    break;
  case WEIGHTED_NOTIME:
    numOrig = this->weightedEventsNoTime.size();
    numDel = this->maskTofHelper(this->weightedEventsNoTime, tofMin, tofMax);
    break;
  }

  if (numDel >= numOrig)
    this->clear(false);
}

// --------------------------------------------------------------------------
/** Mask out events by the condition vector.
 * Events are removed from the list.
 * @param events :: reference to a vector of events to change.
 * @param mask :: condition vector
 * @returns The number of events deleted.
 */
template <class T>
std::size_t EventList::maskConditionHelper(std::vector<T> &events,
                                           const std::vector<bool> &mask) {

  // runs through the two synchronized vectors and delete elements
  // for condition false
  auto itm = std::find(mask.begin(), mask.end(), false);
  auto first = events.begin() + (itm - mask.begin());

  if (itm != mask.end()) {
    for (auto ite = first; ++ite != events.end() && ++itm != mask.end();) {
      if (*itm != false) {
        *first++ = std::move(*ite);
      }
    }
  }

  auto n = events.end() - first;
  if (n != 0)
    events.erase(first, events.end());

  return n;
}

// --------------------------------------------------------------------------
/**
 * Mask out events by the condition vector.
 * Events are removed from the list.
 * @param mask :: condition vector
 */
void EventList::maskCondition(const std::vector<bool> &mask) {

  // mask size must match the number of events
  if (this->getNumberEvents() != mask.size())
    throw std::runtime_error("EventList::maskTof: tofMax must be > tofMin");

  // don't do anything with an emply list
  if (this->getNumberEvents() == 0)
    return;

  // Convert the list
  size_t numOrig = 0;
  size_t numDel = 0;
  switch (eventType) {
  case TOF:
    numOrig = this->events.size();
    numDel = this->maskConditionHelper(this->events, mask);
    break;
  case WEIGHTED:
    numOrig = this->weightedEvents.size();
    numDel = this->maskConditionHelper(this->weightedEvents, mask);
    break;
  case WEIGHTED_NOTIME:
    numOrig = this->weightedEventsNoTime.size();
    numDel = this->maskConditionHelper(this->weightedEventsNoTime, mask);
    break;
  }

  if (numDel >= numOrig)
    this->clear(false);
}

// --------------------------------------------------------------------------
/** Get the m_tof member of all events in a list
 *
 * @param events :: source vector of events
 * @param tofs :: vector to fill
 */
template <class T>
void EventList::getTofsHelper(const std::vector<T> &events,
                              std::vector<double> &tofs) {
  tofs.clear();
  for (auto itev = events.cbegin(); itev != events.cend(); ++itev)
    tofs.push_back(itev->m_tof);
}

/** Fill a vector with the list of TOFs
 *  @param tofs :: A reference to the vector to be filled
 */
void EventList::getTofs(std::vector<double> &tofs) const {
  // Set the capacity of the vector to avoid multiple resizes
  tofs.reserve(this->getNumberEvents());

  // Convert the list
  switch (eventType) {
  case TOF:
    this->getTofsHelper(this->events, tofs);
    break;
  case WEIGHTED:
    this->getTofsHelper(this->weightedEvents, tofs);
    break;
  case WEIGHTED_NOTIME:
    this->getTofsHelper(this->weightedEventsNoTime, tofs);
    break;
  }
}

/** Get the times-of-flight of each event in this EventList.
 *
 * @return by copy a vector of doubles of the tof() value
 */
std::vector<double> EventList::getTofs() const {
  std::vector<double> tofs;
  this->getTofs(tofs);
  return tofs;
}

// --------------------------------------------------------------------------
/** Get the weight member of all events in a list
 *
 * @param events :: source vector of events
 * @param weights :: vector to fill
 */
template <class T>
void EventList::getWeightsHelper(const std::vector<T> &events,
                                 std::vector<double> &weights) {
  weights.clear();
  weights.reserve(events.size());
  std::transform(events.cbegin(), events.cend(), std::back_inserter(weights),
                 [](const auto &event) { return event.weight(); });
}

/** Fill a vector with the list of Weights
 *  @param weights :: A reference to the vector to be filled
 */
void EventList::getWeights(std::vector<double> &weights) const {
  // Set the capacity of the vector to avoid multiple resizes
  weights.reserve(this->getNumberEvents());

  // Convert the list
  switch (eventType) {
  case WEIGHTED:
    this->getWeightsHelper(this->weightedEvents, weights);
    break;
  case WEIGHTED_NOTIME:
    this->getWeightsHelper(this->weightedEventsNoTime, weights);
    break;
  default:
    // not a weighted event type, return 1.0 for all.
    weights.assign(this->getNumberEvents(), 1.0);
    break;
  }
}

/** Get the weight of each event in this EventList.
 *
 * @return by copy a vector of doubles of the weight() value
 */
std::vector<double> EventList::getWeights() const {
  std::vector<double> weights;
  this->getWeights(weights);
  return weights;
}

// --------------------------------------------------------------------------
/** Get the weight error member of all events in a list
 *
 * @param events :: source vector of events
 * @param weightErrors :: vector to fill
 */
template <class T>
void EventList::getWeightErrorsHelper(const std::vector<T> &events,
                                      std::vector<double> &weightErrors) {
  weightErrors.clear();
  weightErrors.reserve(events.size());
  std::transform(events.cbegin(), events.cend(),
                 std::back_inserter(weightErrors),
                 [](const auto &event) { return event.error(); });
}

/** Fill a vector with the list of Weight Errors
 *  @param weightErrors :: A reference to the vector to be filled
 */
void EventList::getWeightErrors(std::vector<double> &weightErrors) const {
  // Set the capacity of the vector to avoid multiple resizes
  weightErrors.reserve(this->getNumberEvents());

  // Convert the list
  switch (eventType) {
  case WEIGHTED:
    this->getWeightErrorsHelper(this->weightedEvents, weightErrors);
    break;
  case WEIGHTED_NOTIME:
    this->getWeightErrorsHelper(this->weightedEventsNoTime, weightErrors);
    break;
  default:
    // not a weighted event type, return 1.0 for all.
    weightErrors.assign(this->getNumberEvents(), 1.0);
    break;
  }
}

/** Get the weight error of each event in this EventList.
 *
 * @return by copy a vector of doubles of the weight() value
 */
std::vector<double> EventList::getWeightErrors() const {
  std::vector<double> weightErrors;
  this->getWeightErrors(weightErrors);
  return weightErrors;
}

// --------------------------------------------------------------------------
/** Get the pulsetimes of all events in a list
 *
 * @param events :: source vector of events
 * @param times :: vector to fill
 */
template <class T>
void EventList::getPulseTimesHelper(
    const std::vector<T> &events,
    std::vector<Mantid::Types::Core::DateAndTime> &times) {
  times.clear();
  times.reserve(events.size());
  std::transform(events.cbegin(), events.cend(), std::back_inserter(times),
                 [](const auto &event) { return event.pulseTime(); });
}

/** Get the pulse times of each event in this EventList.
 *
 * @return by copy a vector of DateAndTime times
 */
std::vector<Mantid::Types::Core::DateAndTime> EventList::getPulseTimes() const {
  std::vector<Mantid::Types::Core::DateAndTime> times;
  // Set the capacity of the vector to avoid multiple resizes
  times.reserve(this->getNumberEvents());

  // Convert the list
  switch (eventType) {
  case TOF:
    this->getPulseTimesHelper(this->events, times);
    break;
  case WEIGHTED:
    this->getPulseTimesHelper(this->weightedEvents, times);
    break;
  case WEIGHTED_NOTIME:
    this->getPulseTimesHelper(this->weightedEventsNoTime, times);
    break;
  }
  return times;
}

// --------------------------------------------------------------------------
/**
 * @return The minimum tof value for the list of the events.
 */
double EventList::getTofMin() const {
  // set up as the maximum available double
  double tMin = std::numeric_limits<double>::max();

  // no events is a soft error
  if (this->empty())
    return tMin;

  // when events are ordered by tof just need the first value
  if (this->order == TOF_SORT) {
    switch (eventType) {
    case TOF:
      return this->events.begin()->tof();
    case WEIGHTED:
      return this->weightedEvents.begin()->tof();
    case WEIGHTED_NOTIME:
      return this->weightedEventsNoTime.begin()->tof();
    }
  }

  // now we are stuck with a linear search
  double temp = tMin; // start with the largest possible value
  size_t numEvents = this->getNumberEvents();
  for (size_t i = 0; i < numEvents; i++) {
    switch (eventType) {
    case TOF:
      temp = this->events[i].tof();
      break;
    case WEIGHTED:
      temp = this->weightedEvents[i].tof();
      break;
    case WEIGHTED_NOTIME:
      temp = this->weightedEventsNoTime[i].tof();
      break;
    }
    if (temp < tMin)
      tMin = temp;
  }
  return tMin;
}

/**
 * @return The maximum tof value for the list of events.
 */
double EventList::getTofMax() const {
  // set up as the minimum available double
  double tMax = std::numeric_limits<double>::lowest();

  // no events is a soft error
  if (this->empty())
    return tMax;

  // when events are ordered by tof just need the first value
  if (this->order == TOF_SORT) {
    switch (eventType) {
    case TOF:
      return this->events.rbegin()->tof();
    case WEIGHTED:
      return this->weightedEvents.rbegin()->tof();
    case WEIGHTED_NOTIME:
      return this->weightedEventsNoTime.rbegin()->tof();
    }
  }

  // now we are stuck with a linear search
  size_t numEvents = this->getNumberEvents();
  double temp = tMax; // start with the smallest possible value
  for (size_t i = 0; i < numEvents; i++) {
    switch (eventType) {
    case TOF:
      temp = this->events[i].tof();
      break;
    case WEIGHTED:
      temp = this->weightedEvents[i].tof();
      break;
    case WEIGHTED_NOTIME:
      temp = this->weightedEventsNoTime[i].tof();
      break;
    }
    if (temp > tMax)
      tMax = temp;
  }
  return tMax;
}

// --------------------------------------------------------------------------
/**
 * @return The minimum tof value for the list of the events.
 */
DateAndTime EventList::getPulseTimeMin() const {
  // set up as the maximum available date time.
  DateAndTime tMin = DateAndTime::maximum();

  // no events is a soft error
  if (this->empty())
    return tMin;

  // when events are ordered by pulse time just need the first value
  if (this->order == PULSETIME_SORT) {
    switch (eventType) {
    case TOF:
      return this->events.begin()->pulseTime();
    case WEIGHTED:
      return this->weightedEvents.begin()->pulseTime();
    case WEIGHTED_NOTIME:
      return this->weightedEventsNoTime.begin()->pulseTime();
    }
  }

  // now we are stuck with a linear search
  DateAndTime temp = tMin; // start with the largest possible value
  size_t numEvents = this->getNumberEvents();
  for (size_t i = 0; i < numEvents; i++) {
    switch (eventType) {
    case TOF:
      temp = this->events[i].pulseTime();
      break;
    case WEIGHTED:
      temp = this->weightedEvents[i].pulseTime();
      break;
    case WEIGHTED_NOTIME:
      temp = this->weightedEventsNoTime[i].pulseTime();
      break;
    }
    if (temp < tMin)
      tMin = temp;
  }
  return tMin;
}

/**
 * @return The maximum tof value for the list of events.
 */
DateAndTime EventList::getPulseTimeMax() const {
  // set up as the minimum available date time.
  DateAndTime tMax = DateAndTime::minimum();

  // no events is a soft error
  if (this->empty())
    return tMax;

  // when events are ordered by pulse time just need the first value
  if (this->order == PULSETIME_SORT) {
    switch (eventType) {
    case TOF:
      return this->events.rbegin()->pulseTime();
    case WEIGHTED:
      return this->weightedEvents.rbegin()->pulseTime();
    case WEIGHTED_NOTIME:
      return this->weightedEventsNoTime.rbegin()->pulseTime();
    }
  }

  // now we are stuck with a linear search
  size_t numEvents = this->getNumberEvents();
  DateAndTime temp = tMax; // start with the smallest possible value
  for (size_t i = 0; i < numEvents; i++) {
    switch (eventType) {
    case TOF:
      temp = this->events[i].pulseTime();
      break;
    case WEIGHTED:
      temp = this->weightedEvents[i].pulseTime();
      break;
    case WEIGHTED_NOTIME:
      temp = this->weightedEventsNoTime[i].pulseTime();
      break;
    }
    if (temp > tMax)
      tMax = temp;
  }
  return tMax;
}

void EventList::getPulseTimeMinMax(
    Mantid::Types::Core::DateAndTime &tMin,
    Mantid::Types::Core::DateAndTime &tMax) const {
  // set up as the minimum available date time.
  tMax = DateAndTime::minimum();
  tMin = DateAndTime::maximum();

  // no events is a soft error
  if (this->empty())
    return;

  // when events are ordered by pulse time just need the first/last values
  if (this->order == PULSETIME_SORT) {
    switch (eventType) {
    case TOF:
      tMin = this->events.begin()->pulseTime();
      tMax = this->events.rbegin()->pulseTime();
      return;
    case WEIGHTED:
      tMin = this->weightedEvents.begin()->pulseTime();
      tMax = this->weightedEvents.rbegin()->pulseTime();
      return;
    case WEIGHTED_NOTIME:
      tMin = this->weightedEventsNoTime.begin()->pulseTime();
      tMax = this->weightedEventsNoTime.rbegin()->pulseTime();
      return;
    }
  }

  // now we are stuck with a linear search
  size_t numEvents = this->getNumberEvents();
  DateAndTime temp = tMax; // start with the smallest possible value
  for (size_t i = 0; i < numEvents; i++) {
    switch (eventType) {
    case TOF:
      temp = this->events[i].pulseTime();
      break;
    case WEIGHTED:
      temp = this->weightedEvents[i].pulseTime();
      break;
    case WEIGHTED_NOTIME:
      temp = this->weightedEventsNoTime[i].pulseTime();
      break;
    }
    if (temp > tMax)
      tMax = temp;
    if (temp < tMin)
      tMin = temp;
  }
}

DateAndTime EventList::getTimeAtSampleMax(const double &tofFactor,
                                          const double &tofOffset) const {
  // set up as the minimum available date time.
  DateAndTime tMax = DateAndTime::minimum();

  // no events is a soft error
  if (this->empty())
    return tMax;

  // when events are ordered by time at sample just need the first value
  if (this->order == TIMEATSAMPLE_SORT) {
    switch (eventType) {
    case TOF:
      return calculateCorrectedFullTime(*(this->events.rbegin()), tofFactor,
                                        tofOffset);
    case WEIGHTED:
      return calculateCorrectedFullTime(*(this->weightedEvents.rbegin()),
                                        tofFactor, tofOffset);
    case WEIGHTED_NOTIME:
      return calculateCorrectedFullTime(*(this->weightedEventsNoTime.rbegin()),
                                        tofFactor, tofOffset);
    }
  }

  // now we are stuck with a linear search
  size_t numEvents = this->getNumberEvents();
  DateAndTime temp = tMax; // start with the smallest possible value
  for (size_t i = 0; i < numEvents; i++) {
    switch (eventType) {
    case TOF:
      temp = calculateCorrectedFullTime(this->events[i], tofFactor, tofOffset);
      break;
    case WEIGHTED:
      temp = calculateCorrectedFullTime(this->weightedEvents[i], tofFactor,
                                        tofOffset);
      break;
    case WEIGHTED_NOTIME:
      temp = calculateCorrectedFullTime(this->weightedEventsNoTime[i],
                                        tofFactor, tofOffset);
      break;
    }
    if (temp > tMax)
      tMax = temp;
  }
  return tMax;
}

DateAndTime EventList::getTimeAtSampleMin(const double &tofFactor,
                                          const double &tofOffset) const {
  // set up as the minimum available date time.
  DateAndTime tMin = DateAndTime::maximum();

  // no events is a soft error
  if (this->empty())
    return tMin;

  // when events are ordered by time at sample just need the first value
  if (this->order == TIMEATSAMPLE_SORT) {
    switch (eventType) {
    case TOF:
      return calculateCorrectedFullTime(*(this->events.begin()), tofFactor,
                                        tofOffset);
    case WEIGHTED:
      return calculateCorrectedFullTime(*(this->weightedEvents.begin()),
                                        tofFactor, tofOffset);
    case WEIGHTED_NOTIME:
      return calculateCorrectedFullTime(*(this->weightedEventsNoTime.begin()),
                                        tofFactor, tofOffset);
    }
  }

  // now we are stuck with a linear search
  size_t numEvents = this->getNumberEvents();
  DateAndTime temp = tMin; // start with the smallest possible value
  for (size_t i = 0; i < numEvents; i++) {
    switch (eventType) {
    case TOF:
      temp = calculateCorrectedFullTime(this->events[i], tofFactor, tofOffset);
      break;
    case WEIGHTED:
      temp = calculateCorrectedFullTime(this->weightedEvents[i], tofFactor,
                                        tofOffset);
      break;
    case WEIGHTED_NOTIME:
      temp = calculateCorrectedFullTime(this->weightedEventsNoTime[i],
                                        tofFactor, tofOffset);
      break;
    }
    if (temp < tMin)
      tMin = temp;
  }
  return tMin;
}

// --------------------------------------------------------------------------
/** Set a list of TOFs to the current event list.
 *
 * @param events :: source vector of events
 * @param tofs :: The vector of doubles to set the tofs to.
 */
template <class T>
void EventList::setTofsHelper(std::vector<T> &events,
                              const std::vector<double> &tofs) {
  if (tofs.empty())
    return;

  size_t x_size = tofs.size();
  if (events.size() != x_size)
    return; // should this throw an exception?

  for (size_t i = 0; i < x_size; ++i)
    events[i].m_tof = tofs[i];
}

// --------------------------------------------------------------------------
/**
 * Set a list of TOFs to the current event list. Modify the units if necessary.
 *
 * @param tofs :: The vector of doubles to set the tofs to.
 */
void EventList::setTofs(const MantidVec &tofs) {
  this->order = UNSORTED;

  // Convert the list
  switch (eventType) {
  case TOF:
    this->setTofsHelper(this->events, tofs);
    break;
  case WEIGHTED:
    this->setTofsHelper(this->weightedEvents, tofs);
    break;
  case WEIGHTED_NOTIME:
    this->setTofsHelper(this->weightedEventsNoTime, tofs);
    break;
  }
}

// ==============================================================================================
// ----------- MULTIPLY AND DIVIDE ---------------------------------------
// ==============================================================================================

//------------------------------------------------------------------------------------------------
/** Helper method for multiplying an event list by a scalar value with/without
 *error
 *
 * @param events: vector of events (with weights)
 * @param value: multiply all weights by this amount.
 * @param error: error on 'value'. Can be 0.
 * */
template <class T>
void EventList::multiplyHelper(std::vector<T> &events, const double value,
                               const double error) {
  // Square of the value's error
  double errorSquared = error * error;
  double valueSquared = value * value;

  auto itev_end = events.end();

  if (error == 0) {
    // Error-less calculation
    for (auto itev = events.begin(); itev != itev_end; itev++) {
      itev->m_errorSquared =
          static_cast<float>(itev->m_errorSquared * valueSquared);
      itev->m_weight *= static_cast<float>(value);
    }
  } else {
    // Carry the scalar error
    for (auto itev = events.begin(); itev != itev_end; itev++) {
      itev->m_errorSquared =
          static_cast<float>(itev->m_errorSquared * valueSquared +
                             errorSquared * itev->m_weight * itev->m_weight);
      itev->m_weight *= static_cast<float>(value);
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Operator to multiply the weights in this EventList by an error-less scalar.
 * Use multiply(value,error) if you wish to multiply by a real variable with an
 *error!
 *
 * The event list switches to WeightedEvent's if needed.
 * Note that if the multiplier is exactly 1.0, the list is NOT switched to
 *WeightedEvents - nothing happens.
 *
 * @param value :: multiply by this
 * @return reference to this
 */
EventList &EventList::operator*=(const double value) {
  this->multiply(value);
  return *this;
}

//------------------------------------------------------------------------------------------------
/** Multiply the weights in this event list by a scalar variable with an error;
 * though the error can be 0.0
 *
 * The event list switches to WeightedEvent's if needed.
 * Note that if the multiplier is exactly 1.0 and the error is exactly 0.0, the
 *list is NOT switched to WeightedEvents - nothing happens.
 *
 * Given:
 *  - A is the weight, variance \f$\sigma_A \f$
 *  - B is the scalar multiplier, variance \f$\sigma_B \f$
 *
 * The error propagation formula used is:
 *
 * \f[ \left(\frac{\sigma_f}{f}\right)^2 = \left(\frac{\sigma_A}{A}\right)^2 +
 *\left(\frac{\sigma_B}{B}\right)^2 + 2\frac{\sigma_A\sigma_B}{AB}\rho_{AB} \f]
 *
 * \f$ \rho_{AB} \f$ is the covariance between A and B, which we take to be 0
 *(uncorrelated variables).
 * Therefore, this reduces to:
 * \f[ \sigma_{AB}^2 = B^2 \sigma_A^2 + A^2 \sigma_B ^ 2  \f]
 *
 * In the case of no error:
 *  - The weight is simply \f$ aA \f$
 *  - The error \f$ \sigma_A \f$ becomes \f$ \sigma_{aA} = a \sigma_{A} \f$
 *
 * @param value: multiply all weights by this amount.
 * @param error: error on 'value'. Can be 0.
 */
void EventList::multiply(const double value, const double error) {
  // Do nothing if multiplying by exactly one and there is no error
  if ((value == 1.0) && (error == 0.0))
    return;

  switch (eventType) {
  case TOF:
    // Switch to weights if needed.
    this->switchTo(WEIGHTED);
    // Fall through

  case WEIGHTED:
    multiplyHelper(this->weightedEvents, value, error);
    break;

  case WEIGHTED_NOTIME:
    multiplyHelper(this->weightedEventsNoTime, value, error);
    break;
  }
}

//------------------------------------------------------------------------------------------------
/** Helper method for multiplying an event list by a histogram with error
 *
 * @param events: vector of events (with weights)
 * @param X: bins of the multiplying histogram.
 * @param Y: value to multiply the weights.
 * @param E: error on the value to multiply.
 * @throw invalid_argument if the sizes of X, Y, E are not consistent.
 * */
template <class T>
void EventList::multiplyHistogramHelper(std::vector<T> &events,
                                        const MantidVec &X, const MantidVec &Y,
                                        const MantidVec &E) {
  // Validate inputs
  if ((X.size() < 2) || (Y.size() != E.size()) || (X.size() != 1 + Y.size())) {
    std::stringstream msg;
    msg << "EventList::multiply() was given invalid size or "
           "inconsistent histogram arrays: X["
        << X.size() << "] "
        << "Y[" << Y.size() << " E[" << E.size() << "]";
    throw std::invalid_argument(msg.str());
  }

  size_t x_size = X.size();

  // Iterate through all events (sorted by tof)
  auto itev = findFirstEvent(events, T(X[0]));
  auto itev_end = events.end();
  // The above can still take you to end() if no events above X[0], so check
  // again.
  if (itev == itev_end)
    return;

  // Find the first bin
  size_t bin = 0;

  // Multiplier values
  double value;
  double error;
  double valueSquared;
  double errorSquared;

  // If the tof is greater the first bin boundary, so we need to find the first
  // bin
  double tof = itev->tof();
  while (bin < x_size - 1) {
    // Within range?
    if ((tof >= X[bin]) && (tof < X[bin + 1]))
      break; // Stop increasing bin
    ++bin;
  }

  // New bin! Find what you are multiplying!
  value = Y[bin];
  error = E[bin];
  valueSquared = value * value;
  errorSquared = error * error;

  // Keep going through all the events
  while ((itev != itev_end) && (bin < x_size - 1)) {
    tof = itev->tof();
    while (bin < x_size - 1) {
      // Event is Within range?
      if ((tof >= X[bin]) && (tof < X[bin + 1])) {
        // Process this event. Multiply and calculate error.
        itev->m_errorSquared =
            static_cast<float>(itev->m_errorSquared * valueSquared +
                               errorSquared * itev->m_weight * itev->m_weight);
        itev->m_weight *= static_cast<float>(value);
        break; // out of the bin-searching-while-loop
      }
      ++bin;
      if (bin >= x_size - 1)
        break;

      // New bin! Find what you are multiplying!
      value = Y[bin];
      error = E[bin];
      valueSquared = value * value;
      errorSquared = error * error;
    }
    ++itev;
  }
}

//------------------------------------------------------------------------------------------------
/** Multiply the weights in this event list by a histogram.
 * The event list switches to WeightedEvent's if needed.
 * NOTE: no unit checks are made (or possible to make) to compare the units of X
 *and tof() in the EventList.
 *
 * The formula used for calculating the error on the neutron weight is:
 * \f[ \sigma_{f}^2 = B^2 \sigma_A^2 + A^2 \sigma_B ^ 2  \f]
 *
 * where:
 *  * A is the weight of the event
 *  * B is the weight of the BIN that the event falls in
 *  * \f$\sigma_A\f$ is the error (not squared) of the weight of the event
 *  * \f$\sigma_B\f$ is the error (not squared) of the bin B
 *  * f is the resulting weight of the multiplied event
 *
 * @param X: bins of the multiplying histogram.
 * @param Y: value to multiply the weights.
 * @param E: error on the value to multiply.
 * @throw invalid_argument if the sizes of X, Y, E are not consistent.
 */
void EventList::multiply(const MantidVec &X, const MantidVec &Y,
                         const MantidVec &E) {
  switch (eventType) {
  case TOF:
    // Switch to weights if needed.
    this->switchTo(WEIGHTED);
    // Fall through

  case WEIGHTED:
    // Sorting by tof is necessary for the algorithm
    this->sortTof();
    multiplyHistogramHelper(this->weightedEvents, X, Y, E);
    break;

  case WEIGHTED_NOTIME:
    // Sorting by tof is necessary for the algorithm
    this->sortTof();
    multiplyHistogramHelper(this->weightedEventsNoTime, X, Y, E);
    break;
  }
}

//------------------------------------------------------------------------------------------------
/** Helper method for dividing an event list by a histogram with error
 *
 * @param events: vector of events (with weights)
 * @param X: bins of the dividing histogram.
 * @param Y: value to dividing the weights.
 * @param E: error on the value to dividing.
 * @throw invalid_argument if the sizes of X, Y, E are not consistent.
 * */
template <class T>
void EventList::divideHistogramHelper(std::vector<T> &events,
                                      const MantidVec &X, const MantidVec &Y,
                                      const MantidVec &E) {
  // Validate inputs
  if ((X.size() < 2) || (Y.size() != E.size()) || (X.size() != 1 + Y.size())) {
    std::stringstream msg;
    msg << "EventList::divide() was given invalid size or "
           "inconsistent histogram arrays: X["
        << X.size() << "] "
        << "Y[" << Y.size() << " E[" << E.size() << "]";
    throw std::invalid_argument(msg.str());
  }

  size_t x_size = X.size();

  // Iterate through all events (sorted by tof)
  auto itev = findFirstEvent(events, T(X[0]));
  auto itev_end = events.end();
  // The above can still take you to end() if no events above X[0], so check
  // again.
  if (itev == itev_end)
    return;

  // Find the first bin
  size_t bin = 0;

  // Multiplier values
  double value;
  double error;
  double valError_over_value_squared;

  // If the tof is greater the first bin boundary, so we need to find the first
  // bin
  double tof = itev->tof();
  while (bin < x_size - 1) {
    // Within range?
    if ((tof >= X[bin]) && (tof < X[bin + 1]))
      break; // Stop increasing bin
    ++bin;
  }

  // New bin! Find what you are multiplying!
  value = Y[bin];
  error = E[bin];

  // --- Division case ---
  if (value == 0) {
    value = std::numeric_limits<float>::quiet_NaN(); // Avoid divide by zero
    valError_over_value_squared = 0;
  } else
    valError_over_value_squared = error * error / (value * value);

  // Keep going through all the events
  while ((itev != events.end()) && (bin < x_size - 1)) {
    tof = itev->tof();
    while (bin < x_size - 1) {
      // Event is Within range?
      if ((tof >= X[bin]) && (tof < X[bin + 1])) {
        // Process this event. Divide and calculate error.
        double newWeight = itev->m_weight / value;
        itev->m_errorSquared = static_cast<float>(
            newWeight * newWeight *
            ((itev->m_errorSquared / (itev->m_weight * itev->m_weight)) +
             valError_over_value_squared));
        itev->m_weight = static_cast<float>(newWeight);
        break; // out of the bin-searching-while-loop
      }
      ++bin;
      if (bin >= x_size - 1)
        break;

      // New bin! Find what you are multiplying!
      value = Y[bin];
      error = E[bin];

      // --- Division case ---
      if (value == 0) {
        value = std::numeric_limits<float>::quiet_NaN(); // Avoid divide by zero
        valError_over_value_squared = 0;
      } else
        valError_over_value_squared = error * error / (value * value);
    }
    ++itev;
  }
}

//------------------------------------------------------------------------------------------------
/** Divide the weights in this event list by a histogram.
 * The event list switches to WeightedEvent's if needed.
 * NOTE: no unit checks are made (or possible to make) to compare the units of X
 *and tof() in the EventList.
 *
 * The formula used for calculating the error on the neutron weight is:
 * \f[ \sigma_{f}^2 = (A / B)^2 * (\sigma_A^2 / A^2 + \sigma_B^2 / B^2) \f]
 *
 * where:
 *  * A is the weight of the event
 *  * B is the weight of the BIN that the event falls in
 *  * \f$\sigma_A\f$ is the error (not squared) of the weight of the event
 *  * \f$\sigma_B\f$ is the error (not squared) of the bin B
 *  * f is the resulting weight of the divided event
 *
 *
 * @param X: bins of the multiplying histogram.
 * @param Y: value to multiply the weights.
 * @param E: error on the value to multiply.
 * @throw invalid_argument if the sizes of X, Y, E are not consistent.
 */
void EventList::divide(const MantidVec &X, const MantidVec &Y,
                       const MantidVec &E) {
  switch (eventType) {
  case TOF:
    // Switch to weights if needed.
    this->switchTo(WEIGHTED);
    // Fall through

  case WEIGHTED:
    // Sorting by tof is necessary for the algorithm
    this->sortTof();
    divideHistogramHelper(this->weightedEvents, X, Y, E);
    break;

  case WEIGHTED_NOTIME:
    // Sorting by tof is necessary for the algorithm
    this->sortTof();
    divideHistogramHelper(this->weightedEventsNoTime, X, Y, E);
    break;
  }
}

//------------------------------------------------------------------------------------------------
/** Operator to divide the weights in this EventList by an error-less scalar.
 * Use divide(value,error) if your scalar has an error!
 * This simply calls the equivalent function: multiply(1.0/value).
 *
 * @param value :: divide by this
 * @return reference to this
 * @throw std::invalid_argument if value == 0; cannot divide by zero.
 */
EventList &EventList::operator/=(const double value) {
  if (value == 0.0)
    throw std::invalid_argument(
        "EventList::divide() called with value of 0.0. Cannot divide by zero.");
  this->multiply(1.0 / value, 0.0);
  return *this;
}

//------------------------------------------------------------------------------------------------
/** Divide the weights in this event list by a scalar with an (optional) error.
 * The event list switches to WeightedEvent's if needed.
 * This simply calls the equivalent function: multiply(1.0/value,
 *error/(value*value)).
 *
 * @param value: divide all weights by this amount.
 * @param error: error on 'value'. Can be 0.
 * @throw std::invalid_argument if value == 0; cannot divide by zero.
 */
void EventList::divide(const double value, const double error) {
  if (value == 0.0)
    throw std::invalid_argument(
        "EventList::divide() called with value of 0.0. Cannot divide by zero.");
  // Do nothing if dividing by exactly 1.0, no error
  else if (value == 1.0 && error == 0.0)
    return;

  // We'll multiply by 1/value
  double invValue = 1.0 / value;
  // Relative error remains the same
  double invError = (error / value) * invValue;

  this->multiply(invValue, invError);
}

// ==============================================================================================
// ----------- SPLITTING AND FILTERING ---------------------------------------
// ==============================================================================================
/** Filter a vector of events into another based on pulse time.
 * TODO: Make this more efficient using STL-fu.
 * @param events :: input events
 * @param start :: start time (absolute)
 * @param stop :: end time (absolute)
 * @param output :: reference to an event list that will be output.
 */
template <class T>
void EventList::filterByPulseTimeHelper(std::vector<T> &events,
                                        DateAndTime start, DateAndTime stop,
                                        std::vector<T> &output) {
  auto itev = events.begin();
  auto itev_end = events.end();
  // Find the first event with m_pulsetime >= start
  while ((itev != itev_end) && (itev->m_pulsetime < start))
    itev++;

  while ((itev != itev_end) && (itev->m_pulsetime < stop)) {
    // Add the copy to the output
    output.push_back(*itev);
    ++itev;
  }
}

/** Filter a vector of events into another based on time at sample.
 * TODO: Make this more efficient using STL-fu.
 * @param events :: input events
 * @param start :: start time (absolute)
 * @param stop :: end time (absolute)
 * @param tofFactor :: scaling factor for tof
 * @param tofOffset :: offset for tof
 * @param output :: reference to an event list that will be output.
 */
template <class T>
void EventList::filterByTimeAtSampleHelper(std::vector<T> &events,
                                           DateAndTime start, DateAndTime stop,
                                           double tofFactor, double tofOffset,
                                           std::vector<T> &output) {
  auto itev = events.begin();
  auto itev_end = events.end();
  // Find the first event with m_pulsetime >= start
  while ((itev != itev_end) &&
         (calculateCorrectedFullTime(*itev, tofFactor, tofOffset) <
          start.totalNanoseconds()))
    itev++;

  while ((itev != itev_end) &&
         (calculateCorrectedFullTime(*itev, tofFactor, tofOffset) <
          stop.totalNanoseconds())) {
    // Add the copy to the output
    output.push_back(*itev);
    ++itev;
  }
}

//------------------------------------------------------------------------------------------------
/** Filter this EventList into an output EventList, using
 * keeping only events within the >= start and < end pulse times.
 * Detector IDs and the X axis are copied as well.
 *
 * @param start :: start time (absolute)
 * @param stop :: end time (absolute)
 * @param output :: reference to an event list that will be output.
 * @throws std::invalid_argument If output is a reference to this EventList
 */
void EventList::filterByPulseTime(DateAndTime start, DateAndTime stop,
                                  EventList &output) const {
  if (this == &output) {
    throw std::invalid_argument("In-place filtering is not allowed");
  }

  // Start by sorting the event list by pulse time.
  this->sortPulseTime();
  // Clear the output
  output.clear();
  // Has to match the given type
  output.switchTo(eventType);
  output.setDetectorIDs(this->getDetectorIDs());
  output.setHistogram(m_histogram);
  output.setSortOrder(this->order);

  // Iterate through all events (sorted by pulse time)
  switch (eventType) {
  case TOF:
    filterByPulseTimeHelper(this->events, start, stop, output.events);
    break;
  case WEIGHTED:
    filterByPulseTimeHelper(this->weightedEvents, start, stop,
                            output.weightedEvents);
    break;
  case WEIGHTED_NOTIME:
    throw std::runtime_error("EventList::filterByPulseTime() called on an "
                             "EventList that no longer has time information.");
    break;
  }
}

void EventList::filterByTimeAtSample(Types::Core::DateAndTime start,
                                     Types::Core::DateAndTime stop,
                                     double tofFactor, double tofOffset,
                                     EventList &output) const {
  if (this == &output) {
    throw std::invalid_argument("In-place filtering is not allowed");
  }

  // Start by sorting
  this->sortTimeAtSample(tofFactor, tofOffset);
  // Clear the output
  output.clear();
  // Has to match the given type
  output.switchTo(eventType);
  output.setDetectorIDs(this->getDetectorIDs());
  output.setHistogram(m_histogram);
  output.setSortOrder(this->order);

  // Iterate through all events (sorted by pulse time)
  switch (eventType) {
  case TOF:
    filterByTimeAtSampleHelper(this->events, start, stop, tofFactor, tofOffset,
                               output.events);
    break;
  case WEIGHTED:
    filterByTimeAtSampleHelper(this->weightedEvents, start, stop, tofFactor,
                               tofOffset, output.weightedEvents);
    break;
  case WEIGHTED_NOTIME:
    throw std::runtime_error("EventList::filterByTimeAtSample() called on an "
                             "EventList that no longer has full time "
                             "information.");
    break;
  }
}

//------------------------------------------------------------------------------------------------
/** Perform an in-place filtering on a vector of either TofEvent's or
 *WeightedEvent's
 *
 * @param splitter :: a TimeSplitterType where all the entries (start/end time)
 *indicate events
 *     that will be kept. Any other events will be deleted.
 * @param events :: either this->events or this->weightedEvents.
 */
template <class T>
void EventList::filterInPlaceHelper(Kernel::TimeSplitterType &splitter,
                                    typename std::vector<T> &events) {
  // Iterate through the splitter at the same time
  auto itspl = splitter.begin();
  auto itspl_end = splitter.end();
  DateAndTime start, stop;

  // Iterate for the input
  auto itev = events.begin();
  auto itev_end = events.end();

  // Iterator for the outputted list; will follow the input except when events
  // are dropped.
  auto itOut = events.begin();

  // This is the time of the first section. Anything before is thrown out.
  while (itspl != itspl_end) {
    // Get the splitting interval times and destination
    start = itspl->start();
    stop = itspl->stop();
    const int index = itspl->index();

    // Skip the events before the start of the time
    while ((itev != itev_end) && (itev->m_pulsetime < start))
      itev++;

    // Are we aligned in the input vs output?
    bool copyingInPlace = (itOut == itev);
    if (copyingInPlace) {
      while ((itev != itev_end) && (itev->m_pulsetime < stop))
        ++itev;
      // Make sure the iterators still match
      itOut = itev;
    } else {
      // Go through all the events that are in the interval (if any)
      while ((itev != itev_end) && (itev->m_pulsetime < stop)) {
        if (index >= 0) {
          // Copy the input Event to the output iterator position.
          // Strictly speaking, this is not necessary if itOut == itev; but the
          // extra check would likely
          //  slow down the filtering in the 99% of cases where itOut != itev.
          *itOut = *itev;
          // And go up a spot in the output iterator.
          ++itOut;
        }
        ++itev;
      }
    }

    // Go to the next interval
    ++itspl;
    // But if we reached the end, then we are done.
    if (itspl == itspl_end)
      break;

    // No need to keep looping through the filter if we are out of events
    if (itev == itev_end)
      break;

  } // Looping through entries in the splitter vector

  // Ok, now resize the event list to reflect the fact that it (probably) shrank
  events.resize((itOut - events.begin()));
}

//------------------------------------------------------------------------------------------------
/** Use a TimeSplitterType to filter the event list in place.
 *
 * @param splitter :: a TimeSplitterType where all the entries (start/end time)
 *indicate events
 *     that will be kept. Any other events will be deleted.
 */
void EventList::filterInPlace(Kernel::TimeSplitterType &splitter) {
  // Start by sorting the event list by pulse time.
  this->sortPulseTime();

  // Iterate through all events (sorted by pulse time)
  switch (eventType) {
  case TOF:
    filterInPlaceHelper(splitter, this->events);
    break;
  case WEIGHTED:
    filterInPlaceHelper(splitter, this->weightedEvents);
    break;
  case WEIGHTED_NOTIME:
    throw std::runtime_error("EventList::filterInPlace() called on an "
                             "EventList that no longer has time information.");
    break;
  }
}

//------------------------------------------------------------------------------------------------
/** Split the event list into n outputs, operating on a vector of either
 *TofEvent's or WeightedEvent's
 *  Only event's pulse time is used to compare with splitters.
 *  It is a faster and simple version of splitByFullTimeHelper
 *
 * @param splitter :: a TimeSplitterType giving where to split
 * @param outputs :: a vector of where the split events will end up. The # of
 *entries in there should
 *        be big enough to accommodate the indices.
 * @param events :: either this->events or this->weightedEvents.
 */
template <class T>
void EventList::splitByTimeHelper(Kernel::TimeSplitterType &splitter,
                                  std::vector<EventList *> outputs,
                                  typename std::vector<T> &events) const {
  size_t numOutputs = outputs.size();

  // Iterate through the splitter at the same time
  auto itspl = splitter.begin();
  auto itspl_end = splitter.end();
  DateAndTime start, stop;

  // Iterate through all events (sorted by tof)
  auto itev = events.begin();
  auto itev_end = events.end();

  // This is the time of the first section. Anything before is thrown out.
  while (itspl != itspl_end) {
    // Get the splitting interval times and destination
    start = itspl->start();
    stop = itspl->stop();
    const size_t index = itspl->index();

    // Skip the events before the start of the time
    while ((itev != itev_end) && (itev->m_pulsetime < start))
      itev++;

    // Go through all the events that are in the interval (if any)
    while ((itev != itev_end) && (itev->m_pulsetime < stop)) {
      // Copy the event into another
      const T eventCopy(*itev);
      if (index < numOutputs) {
        EventList *myOutput = outputs[index];
        // Add the copy to the output
        myOutput->addEventQuickly(eventCopy);
      }
      ++itev;
    }

    // Go to the next interval
    ++itspl;
    // But if we reached the end, then we are done.
    if (itspl == itspl_end)
      break;

    // No need to keep looping through the filter if we are out of events
    if (itev == itev_end)
      break;
  }
  // Done!
}

//------------------------------------------------------------------------------------------------
/** Split the event list into n outputs
 *
 * @param splitter :: a TimeSplitterType giving where to split
 * @param outputs :: a vector of where the split events will end up. The # of
 *entries in there should
 *        be big enough to accommodate the indices.
 */
void EventList::splitByTime(Kernel::TimeSplitterType &splitter,
                            std::vector<EventList *> outputs) const {
  if (eventType == WEIGHTED_NOTIME)
    throw std::runtime_error("EventList::splitByTime() called on an EventList "
                             "that no longer has time information.");

  // Start by sorting the event list by pulse time.
  this->sortPulseTime();

  // Initialize all the outputs
  size_t numOutputs = outputs.size();
  for (size_t i = 0; i < numOutputs; i++) {
    outputs[i]->clear();
    outputs[i]->setDetectorIDs(this->getDetectorIDs());
    outputs[i]->setHistogram(m_histogram);
    // Match the output event type.
    outputs[i]->switchTo(eventType);
  }

  // Do nothing if there are no entries
  if (splitter.empty())
    return;

  switch (eventType) {
  case TOF:
    splitByTimeHelper(splitter, outputs, this->events);
    break;
  case WEIGHTED:
    splitByTimeHelper(splitter, outputs, this->weightedEvents);
    break;
  case WEIGHTED_NOTIME:
    break;
  }
}

//------------------------------------------------------------------------------------------------
/** Split the event list into n outputs, operating on a vector of either
 *TofEvent's or WeightedEvent's
 *  The comparison between neutron event and splitter is based on neutron
 *event's pulse time plus
 *
 * @param splitter :: a TimeSplitterType giving where to split
 * @param outputs :: a vector of where the split events will end up. The # of
 *entries in there should
 *        be big enough to accommodate the indices.
 * @param events :: either this->events or this->weightedEvents.
 * @param docorrection :: flag to determine whether or not to apply correction
 * @param toffactor :: factor to correct TOF in formula toffactor*tof+tofshift
 * @param tofshift :: amount to shift (in SECOND) to correct TOF in formula:
 *toffactor*tof+tofshift
 */
template <class T>
void EventList::splitByFullTimeHelper(Kernel::TimeSplitterType &splitter,
                                      std::map<int, EventList *> outputs,
                                      typename std::vector<T> &events,
                                      bool docorrection, double toffactor,
                                      double tofshift) const {
  // 1. Prepare to Iterate through the splitter at the same time
  auto itspl = splitter.begin();
  auto itspl_end = splitter.end();

  // 2. Prepare to Iterate through all events (sorted by tof)
  auto itev = events.begin();
  auto itev_end = events.end();

  // 3. This is the time of the first section. Anything before is thrown out.
  while (itspl != itspl_end) {
    // Get the splitting interval times and destination
    int64_t start = itspl->start().totalNanoseconds();
    int64_t stop = itspl->stop().totalNanoseconds();
    const int index = itspl->index();

    // a) Skip the events before the start of the time
    // TODO This step can be
    EventList *myOutput = outputs[-1];
    while (itev != itev_end) {
      int64_t fulltime;
      if (docorrection)
        fulltime = calculateCorrectedFullTime(*itev, toffactor, tofshift);
      else
        fulltime = itev->m_pulsetime.totalNanoseconds() +
                   static_cast<int64_t>(itev->m_tof * 1000);
      if (fulltime < start) {
        // a1) Record to index = -1 space
        const T eventCopy(*itev);
        myOutput->addEventQuickly(eventCopy);
        itev++;
      } else {
        break;
      }
    }

    // b) Go through all the events that are in the interval (if any)
    while (itev != itev_end) {
      int64_t fulltime;
      if (docorrection)
        fulltime = itev->m_pulsetime.totalNanoseconds() +
                   static_cast<int64_t>(toffactor * itev->m_tof * 1000 +
                                        tofshift * 1.0E9);
      else
        fulltime = itev->m_pulsetime.totalNanoseconds() +
                   static_cast<int64_t>(itev->m_tof * 1000);
      if (fulltime < stop) {
        // b1) Add a copy to the output
        outputs[index]->addEventQuickly(*itev);
        ++itev;
      } else {
        break;
      }
    }

    // Go to the next interval
    ++itspl;
    // But if we reached the end, then we are done.
    if (itspl == itspl_end)
      break;

    // No need to keep looping through the filter if we are out of events
    if (itev == itev_end)
      break;
  } // END-WHILE Splitter
}

//------------------------------------------------------------------------------------------------
/** Split the event list into n outputs by event's full time (tof + pulse time)
 *
 * @param splitter :: a TimeSplitterType giving where to split
 * @param outputs :: a map of where the split events will end up. The # of
 *entries in there should
 *        be big enough to accommodate the indices.
 * @param docorrection :: a boolean to indiciate whether it is need to do
 *correction
 * @param toffactor:  a correction factor for each TOF to multiply with
 * @param tofshift:  a correction shift for each TOF to add with
 */
void EventList::splitByFullTime(Kernel::TimeSplitterType &splitter,
                                std::map<int, EventList *> outputs,
                                bool docorrection, double toffactor,
                                double tofshift) const {
  if (eventType == WEIGHTED_NOTIME)
    throw std::runtime_error("EventList::splitByTime() called on an EventList "
                             "that no longer has time information.");

  // 1. Start by sorting the event list by pulse time.
  this->sortPulseTimeTOF();

  // 2. Initialize all the outputs
  std::map<int, EventList *>::iterator outiter;
  for (outiter = outputs.begin(); outiter != outputs.end(); ++outiter) {
    EventList *opeventlist = outiter->second;
    opeventlist->clear();
    opeventlist->setDetectorIDs(this->getDetectorIDs());
    opeventlist->setHistogram(m_histogram);
    // Match the output event type.
    opeventlist->switchTo(eventType);
  }

  // Do nothing if there are no entries
  if (splitter.empty()) {
    // 3A. Copy all events to group workspace = -1
    (*outputs[-1]) = (*this);
    // this->duplicate(outputs[-1]);
  } else {
    // 3B. Split
    switch (eventType) {
    case TOF:
      splitByFullTimeHelper(splitter, outputs, this->events, docorrection,
                            toffactor, tofshift);
      break;
    case WEIGHTED:
      splitByFullTimeHelper(splitter, outputs, this->weightedEvents,
                            docorrection, toffactor, tofshift);
      break;
    case WEIGHTED_NOTIME:
      break;
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Split the event list into n outputs, operating on a vector of either
 *TofEvent's or WeightedEvent's
 *  The comparison between neutron event and splitter is based on neutron
 *event's pulse time plus
 *
 * @param vectimes :: a vector of absolute time in nanoseconds serving as
 *boundaries of splitters
 * @param vecgroups :: a vector of integer serving as the target workspace group
 *for splitters
 * @param outputs :: a vector of where the split events will end up. The # of
 *entries in there should
 *        be big enough to accommodate the indices.
 * @param vecEvents :: either this->events or this->weightedEvents.
 * @param docorrection :: flag to determine whether or not to apply correction
 * @param toffactor :: factor multiplied to TOF for correcting event time from
 *detector to sample
 * @param tofshift :: shift in SECOND to TOF for correcting event time from
 *detector to sample
 */
template <class T>
std::string EventList::splitByFullTimeVectorSplitterHelper(
    const std::vector<int64_t> &vectimes, const std::vector<int> &vecgroups,
    std::map<int, EventList *> outputs, typename std::vector<T> &vecEvents,
    bool docorrection, double toffactor, double tofshift) const {
  // Define variables for events
  // size_t numevents = events.size();
  typename std::vector<T>::iterator eviter;
  std::stringstream msgss;

  // Loop through events
  for (eviter = vecEvents.begin(); eviter != vecEvents.end(); ++eviter) {
    // Obtain time of event
    int64_t evabstimens;
    if (docorrection)
      evabstimens = eviter->m_pulsetime.totalNanoseconds() +
                    static_cast<int64_t>(toffactor * eviter->m_tof * 1000 +
                                         tofshift * 1.0E9);
    else
      evabstimens = eviter->m_pulsetime.totalNanoseconds() +
                    static_cast<int64_t>(eviter->m_tof * 1000);

    // Search in vector
    int index = static_cast<int>(
        lower_bound(vectimes.begin(), vectimes.end(), evabstimens) -
        vectimes.begin());
    int group;
    // FIXME - whether lower_bound() equal to vectimes.size()-1 should be
    // filtered out?
    if (index == 0 || index > static_cast<int>(vectimes.size() - 1)) {
      // Event is before first splitter or after last splitter.  Put to -1
      group = -1;
    } else {
      group = vecgroups[index - 1];
    }

    // Copy event to the proper group
    EventList *myOutput = outputs[group];
    if (!myOutput) {
      std::stringstream errss;
      errss << "Group " << group << " has a NULL output EventList. "
            << "\n";
      msgss << errss.str();
    } else {
      const T eventCopy(*eviter);
      myOutput->addEventQuickly(eventCopy);
    }
  }

  return (msgss.str());
}

//------------------------------------------------------------------------------------------------
/** Split the event list into n outputs, operating on a vector of either
 *TofEvent's or WeightedEvent's
 *  The comparison between neutron event and splitter is based on neutron
 *event's pulse time plus
 *
 * @param vectimes :: a vector of absolute time in nanoseconds serving as
 *boundaries of splitters
 * @param vecgroups :: a vector of integer serving as the target workspace group
 *for splitters
 * @param outputs :: a vector of where the split events will end up. The # of
 *entries in there should
 *        be big enough to accommodate the indices.
 * @param vecEvents :: either this->events or this->weightedEvents.
 * @param docorrection :: flag to determine whether or not to apply correction
 * @param toffactor :: factor multiplied to TOF for correcting event time from
 *detector to sample
 * @param tofshift :: shift in SECOND to TOF for correcting event time from
 *detector to sample
 */
template <class T>
std::string EventList::splitByFullTimeSparseVectorSplitterHelper(
    const std::vector<int64_t> &vectimes, const std::vector<int> &vecgroups,
    std::map<int, EventList *> outputs, typename std::vector<T> &vecEvents,
    bool docorrection, double toffactor, double tofshift) const {
  // Define variables for events
  // size_t numevents = events.size();
  // typename std::vector<T>::iterator eviter;
  std::stringstream msgss;

  size_t num_splitters = vecgroups.size();
  // prepare to Iterate through all events (sorted by tof)
  auto iter_events = vecEvents.begin();
  auto iter_events_end = vecEvents.end();

  // std::stringstream debug_ss;
  // debug_ss << "\nFilter events...:\n";

  for (size_t i = 0; i < num_splitters; ++i) {
    // get one splitter
    int64_t start_i64 = vectimes[i];
    int64_t stop_i64 = vectimes[i + 1];
    int group = vecgroups[i];
    // debug_ss << "working on splitter: " << i << " from " << start_i64 << " to
    // " << stop_i64 << "\n";

    // go over events
    while (iter_events != iter_events_end) {
      int64_t absolute_time;
      if (docorrection)
        absolute_time =
            iter_events->m_pulsetime.totalNanoseconds() +
            static_cast<int64_t>(toffactor * iter_events->m_tof * 1000 +
                                 tofshift * 1.0E9);
      else
        absolute_time = iter_events->m_pulsetime.totalNanoseconds() +
                        static_cast<int64_t>(iter_events->m_tof * 1000);

      // debug_ss << "  event " << iter_events - vecEvents.begin() << " abs.time
      // = " << absolute_time << "\n";

      if (absolute_time < start_i64) {
        // event occurs before the splitter. only can happen with first
        // splitter. Then ignore and move to next
        ++iter_events;
        continue;
      }

      if (absolute_time < stop_i64) {
        // in the splitter, then copy the event into another
        const T eventCopy(*iter_events);
        // Copy event to the proper group
        EventList *myOutput = outputs[group];
        if (!myOutput) {
          // there is no such group defined. quit for this group
          std::stringstream errss;
          errss << "Group " << group << " has a NULL output EventList. "
                << "\n";
          msgss << errss.str();
          throw std::runtime_error(errss.str());
        }
        // Add the copy to the output
        myOutput->addEventQuickly(eventCopy);
        ++iter_events;
      } else {
        // event occurs after the stop time, it should belonged to the next
        // splitter
        break;
      }
    } // while

    // quit the loop if there is no more event left
    if (iter_events == iter_events_end)
      break;
  } // for splitter

  // std::cout << debug_ss.str();

  return (msgss.str());
}

//----------------------------------------------------------------------------------------------
/**
 * @brief EventList::splitByFullTimeMatrixSplitter
 * @param vec_splitters_time  :: vector of splitting times
 * @param vecgroups :: vector of index group for splitters
 * @param vec_outputEventList :: vector of groups of splitted events
 * @param docorrection :: flag to do TOF correction from detector to sample
 * @param toffactor :: factor multiplied to TOF for correction
 * @param tofshift :: shift to TOF in unit of SECOND for correction
 * @return
 */
// TODO/FIXME/NOW - Consider to use vector to replace vec_outputEventList and
// have an option to ignore the un-filtered events!
std::string EventList::splitByFullTimeMatrixSplitter(
    const std::vector<int64_t> &vec_splitters_time,
    const std::vector<int> &vecgroups,
    std::map<int, EventList *> vec_outputEventList, bool docorrection,
    double toffactor, double tofshift) const {
  // Check validity
  if (eventType == WEIGHTED_NOTIME)
    throw std::runtime_error("EventList::splitByTime() called on an EventList "
                             "that no longer has time information.");

  // Start by sorting the event list by pulse time, if its flag is not set up
  // right
  sortPulseTimeTOF();

  // Initialize all the output event list
  std::map<int, EventList *>::iterator outiter;
  for (outiter = vec_outputEventList.begin();
       outiter != vec_outputEventList.end(); ++outiter) {
    EventList *opeventlist = outiter->second;
    opeventlist->clear();
    opeventlist->setDetectorIDs(this->getDetectorIDs());
    opeventlist->setHistogram(m_histogram);
    // Match the output event type.
    opeventlist->switchTo(eventType);
  }

  std::string debugmessage;

  // Do nothing if there are no entries
  if (vecgroups.empty()) {
    // Copy all events to group workspace = -1
    (*vec_outputEventList[-1]) = (*this);
    // this->duplicate(outputs[-1]);
  } else {
    // Split

    // Try to find out which filtering algorithm to use by comparing number of
    // splitters and number of events
    bool sparse_splitter = vec_splitters_time.size() < this->getNumberEvents();

    switch (eventType) {
    case TOF:
      if (sparse_splitter)
        debugmessage = splitByFullTimeSparseVectorSplitterHelper(
            vec_splitters_time, vecgroups, vec_outputEventList, this->events,
            docorrection, toffactor, tofshift);
      else
        debugmessage = splitByFullTimeVectorSplitterHelper(
            vec_splitters_time, vecgroups, vec_outputEventList, this->events,
            docorrection, toffactor, tofshift);
      break;
    case WEIGHTED:
      if (sparse_splitter)
        debugmessage = splitByFullTimeSparseVectorSplitterHelper(
            vec_splitters_time, vecgroups, vec_outputEventList,
            this->weightedEvents, docorrection, toffactor, tofshift);
      else
        debugmessage = splitByFullTimeVectorSplitterHelper(
            vec_splitters_time, vecgroups, vec_outputEventList,
            this->weightedEvents, docorrection, toffactor, tofshift);
      break;
    case WEIGHTED_NOTIME:
      debugmessage = "TOF type is weighted no time.  Impossible to split. ";
      break;
    }
  }

  return debugmessage;
}

//-------------------------------------------
//--------------------------------------------------
/** Split the event list into n outputs by each event's pulse time only
 */
template <class T>
void EventList::splitByPulseTimeHelper(Kernel::TimeSplitterType &splitter,
                                       std::map<int, EventList *> outputs,
                                       typename std::vector<T> &events) const {
  // Prepare to TimeSplitter Iterate through the splitter at the same time
  auto itspl = splitter.begin();
  auto itspl_end = splitter.end();
  Types::Core::DateAndTime start, stop;

  // Prepare to Events Iterate through all events (sorted by tof)
  auto itev = events.begin();
  auto itev_end = events.end();

  // Iterate (loop) on all splitters
  while (itspl != itspl_end) {
    // Get the splitting interval times and destination group
    start = itspl->start().totalNanoseconds();
    stop = itspl->stop().totalNanoseconds();
    const int index = itspl->index();

    // Skip the events before the start of the time and put to 'unfiltered'
    // EventList
    EventList *myOutput = outputs[-1];
    while (itev != itev_end) {
      if (itev->m_pulsetime < start) {
        // Record to index = -1 space
        const T eventCopy(*itev);
        myOutput->addEventQuickly(eventCopy);
        ++itev;
      } else {
        // Event within a splitter interval
        break;
      }
    }

    // Go through all the events that are in the interval (if any)
    while (itev != itev_end) {

      if (itev->m_pulsetime < stop) {
        outputs[index]->addEventQuickly(*itev);
        ++itev;
      } else {
        // Out of interval
        break;
      }
    }

    // Go to the next interval
    ++itspl;
    // But if we reached the end, then we are done.
    if (itspl == itspl_end)
      break;

    // No need to keep looping through the filter if we are out of events
    if (itev == itev_end)
      break;
  } // END-WHILE Splitter
}

//----------------------------------------------------------------------------------------------
/** Split the event list by pulse time
 */
void EventList::splitByPulseTime(Kernel::TimeSplitterType &splitter,
                                 std::map<int, EventList *> outputs) const {
  // Check for supported event type
  if (eventType == WEIGHTED_NOTIME)
    throw std::runtime_error("EventList::splitByTime() called on an EventList "
                             "that no longer has time information.");

  // Start by sorting the event list by pulse time.
  this->sortPulseTimeTOF();

  // Initialize all the output event lists
  std::map<int, EventList *>::iterator outiter;
  for (outiter = outputs.begin(); outiter != outputs.end(); ++outiter) {
    EventList *opeventlist = outiter->second;
    opeventlist->clear();
    opeventlist->setDetectorIDs(this->getDetectorIDs());
    opeventlist->setHistogram(m_histogram);
    // Match the output event type.
    opeventlist->switchTo(eventType);
  }

  // Split
  if (splitter.empty()) {
    // No splitter: copy all events to group workspace = -1
    (*outputs[-1]) = (*this);
  } else {
    // Split
    switch (eventType) {
    case TOF:
      splitByPulseTimeHelper(splitter, outputs, this->events);
      break;
    case WEIGHTED:
      splitByPulseTimeHelper(splitter, outputs, this->weightedEvents);
      break;
    case WEIGHTED_NOTIME:
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Split the event list by pulse time
 */
// TODO/NOW - TEST
void EventList::splitByPulseTimeWithMatrix(
    const std::vector<int64_t> &vec_times, const std::vector<int> &vec_target,
    std::map<int, EventList *> outputs) const {
  // Check for supported event type
  if (eventType == WEIGHTED_NOTIME)
    throw std::runtime_error("EventList::splitByTime() called on an EventList "
                             "that no longer has time information.");

  // Start by sorting the event list by pulse time.
  this->sortPulseTimeTOF();

  // Initialize all the output event lists
  std::map<int, EventList *>::iterator outiter;
  for (outiter = outputs.begin(); outiter != outputs.end(); ++outiter) {
    EventList *opeventlist = outiter->second;
    opeventlist->clear();
    opeventlist->setDetectorIDs(this->getDetectorIDs());
    opeventlist->setHistogram(m_histogram);
    // Match the output event type.
    opeventlist->switchTo(eventType);
  }

  // Split
  if (vec_target.empty()) {
    // No splitter: copy all events to group workspace = -1
    (*outputs[-1]) = (*this);
  } else {
    // Split
    switch (eventType) {
    case TOF:
      splitByPulseTimeWithMatrixHelper(vec_times, vec_target, outputs,
                                       this->events);
      break;
    case WEIGHTED:
      splitByPulseTimeWithMatrixHelper(vec_times, vec_target, outputs,
                                       this->weightedEvents);
      break;
    case WEIGHTED_NOTIME:
      break;
    }
  }
}

template <class T>
void EventList::splitByPulseTimeWithMatrixHelper(
    const std::vector<int64_t> &vec_split_times,
    const std::vector<int> &vec_split_target,
    std::map<int, EventList *> outputs, typename std::vector<T> &events) const {
  // Prepare to TimeSplitter Iterate through the splitter at the same time
  if (vec_split_times.size() != vec_split_target.size() + 1)
    throw std::runtime_error("Splitter time vector size and splitter target "
                             "vector size are not correct.");

  // Prepare to Events Iterate through all events (sorted by tof)
  auto itev = events.begin();
  auto itev_end = events.end();

  // Iterate (loop) on all splitters
  for (size_t i_target = 0; i_target < vec_split_target.size(); ++i_target) {
    // Get the splitting interval times and destination group
    int64_t start = vec_split_times[i_target];
    int64_t stop = vec_split_times[i_target + 1];
    const int index = vec_split_target[i_target];

    // Skip the events before the start of the time and put to 'unfiltered'
    // EventList
    EventList *myOutput = outputs[-1];
    while (itev != itev_end) {
      if (itev->m_pulsetime < start) {
        // Record to index = -1 space
        const T eventCopy(*itev);
        myOutput->addEventQuickly(eventCopy);
        ++itev;
      } else {
        // Event within a splitter interval
        break;
      }
    }

    // Go through all the events that are in the interval (if any)
    while (itev != itev_end) {

      if (itev->m_pulsetime < stop) {
        outputs[index]->addEventQuickly(*itev);
        ++itev;
      } else {
        // Out of interval
        break;
      }
    }

    // No need to keep looping through the filter if we are out of events
    if (itev == itev_end)
      break;
  } // END-WHILE Splitter
}

//--------------------------------------------------------------------------
/** Get the vector of events contained in an EventList;
 * this is overloaded by event type.
 *
 * @param el :: The EventList to retrieve
 * @param[out] events :: reference to a pointer to a vector of this type of
 *event.
 *             The pointer will be set to point to the vector.
 * @throw runtime_error if you call this on the wrong type of EventList.
 */
void getEventsFrom(EventList &el, std::vector<TofEvent> *&events) {
  events = &el.getEvents();
}
void getEventsFrom(const EventList &el, std::vector<TofEvent> const *&events) {
  events = &el.getEvents();
}

//--------------------------------------------------------------------------
/** Get the vector of events contained in an EventList;
 * this is overloaded by event type.
 *
 * @param el :: The EventList to retrieve
 * @param[out] events :: reference to a pointer to a vector of this type of
 *event.
 *             The pointer will be set to point to the vector.
 * @throw runtime_error if you call this on the wrong type of EventList.
 */
void getEventsFrom(EventList &el, std::vector<WeightedEvent> *&events) {
  events = &el.getWeightedEvents();
}
void getEventsFrom(const EventList &el,
                   std::vector<WeightedEvent> const *&events) {
  events = &el.getWeightedEvents();
}

//--------------------------------------------------------------------------
/** Get the vector of events contained in an EventList;
 * this is overloaded by event type.
 *
 * @param el :: The EventList to retrieve
 * @param[out] events :: reference to a pointer to a vector of this type of
 *event.
 *             The pointer will be set to point to the vector.
 * @throw runtime_error if you call this on the wrong type of EventList.
 */
void getEventsFrom(EventList &el, std::vector<WeightedEventNoTime> *&events) {
  events = &el.getWeightedEventsNoTime();
}
void getEventsFrom(const EventList &el,
                   std::vector<WeightedEventNoTime> const *&events) {
  events = &el.getWeightedEventsNoTime();
}

//--------------------------------------------------------------------------
/** Helper function for the conversion to TOF. This handles the different
 *  event types.
 *
 * @param events the list of events
 * @param fromUnit the unit to convert from
 * @param toUnit the unit to convert to
 */
template <class T>
void EventList::convertUnitsViaTofHelper(typename std::vector<T> &events,
                                         Mantid::Kernel::Unit *fromUnit,
                                         Mantid::Kernel::Unit *toUnit) {
  auto itev = events.begin();
  auto itev_end = events.end();
  for (; itev != itev_end; itev++) {
    // Conver to TOF
    double tof = fromUnit->singleToTOF(itev->m_tof);
    // And back from TOF to whatever
    itev->m_tof = toUnit->singleFromTOF(tof);
  }
}

//--------------------------------------------------------------------------
/** Converts the X units in each event by going through TOF.
 * Note: if the unit conversion reverses the order, use "reverse()" to flip it
 *back.
 *
 * @param fromUnit :: the Unit describing the input unit. Must be initialized.
 * @param toUnit :: the Unit describing the output unit. Must be initialized.
 */
void EventList::convertUnitsViaTof(Mantid::Kernel::Unit *fromUnit,
                                   Mantid::Kernel::Unit *toUnit) {
  // Check for initialized
  if (!fromUnit || !toUnit)
    throw std::runtime_error(
        "EventList::convertUnitsViaTof(): one of the units is NULL!");
  if (!fromUnit->isInitialized())
    throw std::runtime_error(
        "EventList::convertUnitsViaTof(): fromUnit is not initialized!");
  if (!toUnit->isInitialized())
    throw std::runtime_error(
        "EventList::convertUnitsViaTof(): toUnit is not initialized!");

  switch (eventType) {
  case TOF:
    convertUnitsViaTofHelper(this->events, fromUnit, toUnit);
    break;
  case WEIGHTED:
    convertUnitsViaTofHelper(this->weightedEvents, fromUnit, toUnit);
    break;
  case WEIGHTED_NOTIME:
    convertUnitsViaTofHelper(this->weightedEventsNoTime, fromUnit, toUnit);
    break;
  }
}

//--------------------------------------------------------------------------
/** Convert the event's TOF (x) value according to a simple output = a *
 * (input^b) relationship
 *  @param events :: templated class for the list of events
 *  @param factor :: the conversion factor a to apply
 *  @param power :: the Power b to apply to the conversion
 */
template <class T>
void EventList::convertUnitsQuicklyHelper(typename std::vector<T> &events,
                                          const double &factor,
                                          const double &power) {
  for (auto &event : events) {
    // Output unit = factor * (input) ^ power
    event.m_tof = factor * std::pow(event.m_tof, power);
  }
}

//--------------------------------------------------------------------------
/** Convert the event's TOF (x) value according to a simple output = a *
 * (input^b) relationship
 *  @param factor :: the conversion factor a to apply
 *  @param power :: the Power b to apply to the conversion
 */
void EventList::convertUnitsQuickly(const double &factor, const double &power) {
  switch (eventType) {
  case TOF:
    convertUnitsQuicklyHelper(this->events, factor, power);
    break;
  case WEIGHTED:
    convertUnitsQuicklyHelper(this->weightedEvents, factor, power);
    break;
  case WEIGHTED_NOTIME:
    convertUnitsQuicklyHelper(this->weightedEventsNoTime, factor, power);
    break;
  }
}

HistogramData::Histogram &EventList::mutableHistogramRef() {
  if (mru)
    mru->deleteIndex(this);
  return m_histogram;
}

void EventList::checkAndSanitizeHistogram(HistogramData::Histogram &histogram) {
  if (histogram.xMode() != HistogramData::Histogram::XMode::BinEdges)
    throw std::runtime_error("EventList: setting histogram with storage mode "
                             "other than BinEdges is not possible");
  if (histogram.sharedY() || histogram.sharedE())
    throw std::runtime_error("EventList: setting histogram data with non-null "
                             "Y or E data is not possible");
  // Avoid flushing of YMode: we only change X but YMode depends on events.
  if (histogram.yMode() == HistogramData::Histogram::YMode::Uninitialized)
    histogram.setYMode(m_histogram.yMode());
  if (histogram.yMode() != m_histogram.yMode())
    throw std::runtime_error("EventList: setting histogram data with different "
                             "YMode is not possible");
}

void EventList::checkWorksWithPoints() const {
  throw std::runtime_error("EventList: setting Points as X data is not "
                           "possible, only BinEdges are supported");
}

void EventList::checkIsYAndEWritable() const {
  throw std::runtime_error("EventList: Cannot set Y or E data, these data are "
                           "generated automatically based on the events");
}

} // namespace DataObjects
} // namespace Mantid
