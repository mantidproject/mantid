#ifndef MANTID_DATAOBJECTS_EVENTLIST_H_
#define MANTID_DATAOBJECTS_EVENTLIST_H_ 1

#ifdef _WIN32 /* _WIN32 */
#include <time.h>
#endif
#include "MantidAPI/IEventList.h"
#include "MantidAPI/IEventWorkspace.h" // get EventType declaration
#include "MantidAPI/MatrixWorkspace.h" // get MantidVec declaration
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/EventWorkspaceMRU.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSplitter.h"
#include <cstddef>
#include <iosfwd>
#include <set>
#include <vector>
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace DataObjects {

/// How the event list is sorted.
enum EventSortType {
  UNSORTED,
  TOF_SORT,
  PULSETIME_SORT,
  PULSETIMETOF_SORT,
  TIMEATSAMPLE_SORT
};

//==========================================================================================
/** @class Mantid::DataObjects::EventList

    A class for holding :
      - a list of neutron detection events (TofEvent or WeightedEvent).
      - a list of associated detector ID's.

    This class can switch from holding regular TofEvent's (implied weight of
   1.0)
    or WeightedEvent (where each neutron can have a non-1 weight).
    This is done transparently.

    @author Janik Zikovsky, SNS ORNL
    @date 4/02/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/

class DLLExport EventList : public Mantid::API::IEventList {
public:
  EventList();

  EventList(EventWorkspaceMRU *mru, specid_t specNo);

  EventList(const EventList &rhs);

  EventList(const std::vector<TofEvent> &events);

  EventList(const std::vector<WeightedEvent> &events);

  EventList(const std::vector<WeightedEventNoTime> &events);

  virtual ~EventList();

  void createFromHistogram(const ISpectrum *spec, bool GenerateZeros,
                           bool GenerateMultipleEvents, int MaxEventsPerBin);

  EventList &operator=(const EventList &);

  EventList &operator+=(const TofEvent &event);

  EventList &operator+=(const std::vector<TofEvent> &more_events);

  EventList &operator+=(const WeightedEvent &event);

  EventList &operator+=(const std::vector<WeightedEvent> &more_events);

  EventList &operator+=(const std::vector<WeightedEventNoTime> &more_events);

  EventList &operator+=(const EventList &more_events);

  EventList &operator-=(const EventList &more_events);

  bool operator==(const EventList &rhs) const;
  bool operator!=(const EventList &rhs) const;
  bool equals(const EventList &rhs, const double tolTof, const double tolWeight,
              const int64_t tolPulse) const;

  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it
   *faster.
   * NOTE: Only call this on a un-weighted event list!
   *
   * @param event :: TofEvent to add at the end of the list.
   * */
  inline void addEventQuickly(const TofEvent &event) {
    this->events.push_back(event);
    this->order = UNSORTED;
  }

  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it
   * faster.
   * @param event :: WeightedEvent to add at the end of the list.
   * */
  inline void addEventQuickly(const WeightedEvent &event) {
    this->weightedEvents.push_back(event);
    this->order = UNSORTED;
  }

  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it
   * faster.
   * @param event :: WeightedEventNoTime to add at the end of the list.
   * */
  inline void addEventQuickly(const WeightedEventNoTime &event) {
    this->weightedEventsNoTime.push_back(event);
    this->order = UNSORTED;
  }

  Mantid::API::EventType getEventType() const;

  void switchTo(Mantid::API::EventType newType);

  WeightedEvent getEvent(size_t event_number);

  std::vector<TofEvent> &getEvents();
  const std::vector<TofEvent> &getEvents() const;

  std::vector<WeightedEvent> &getWeightedEvents();
  const std::vector<WeightedEvent> &getWeightedEvents() const;

  std::vector<WeightedEventNoTime> &getWeightedEventsNoTime();
  const std::vector<WeightedEventNoTime> &getWeightedEventsNoTime() const;

  void clear(const bool removeDetIDs = true);
  void clearUnused();

  void lockData() const;
  void unlockData() const;

  void setMRU(EventWorkspaceMRU *newMRU);

  EventWorkspaceMRU *getMRU();

  void clearData();

  void reserve(size_t num);

  void sort(const EventSortType order) const;

  void setSortOrder(const EventSortType order) const;

  void sortTof() const;
  void sortTof2() const;
  void sortTof4() const;

  void sortPulseTime() const;
  void sortPulseTimeTOF() const;
  void sortTimeAtSample(const double &tofFactor, const double &tofShift,
                        bool forceResort = false) const;

  bool isSortedByTof() const;

  EventSortType getSortType() const;

  // X-vector accessors. These reset the MRU for this spectrum
  void setX(const MantidVecPtr::ptr_type &X);

  void setX(const MantidVecPtr &X);

  void setX(const MantidVec &X);

  MantidVec &dataX();
  const MantidVec &dataX() const;
  const MantidVec &constDataX() const;

  // TODO: This overload will probably be needed in a future to work with Event
  // data properly
  // std::pair<double,double> getXDataRange()const;

  /// Disallowed data accessors - can't modify Y/E on a EventList
  void setData(const MantidVec & /*Y*/) {
    throw std::runtime_error("EventList: cannot set Y or E data directly.");
  }
  /// Disallowed data accessors - can't modify Y/E on a EventList
  void setData(const MantidVec & /*Y*/, const MantidVec & /*E*/) {
    throw std::runtime_error("EventList: cannot set Y or E data directly.");
  }
  /// Disallowed data accessors - can't modify Y/E on a EventList
  void setData(const MantidVecPtr & /*Y*/) {
    throw std::runtime_error("EventList: cannot set Y or E data directly.");
  }
  /// Disallowed data accessors - can't modify Y/E on a EventList
  void setData(const MantidVecPtr & /*Y*/, const MantidVecPtr & /*E*/) {
    throw std::runtime_error("EventList: cannot set Y or E data directly.");
  }
  /// Disallowed data accessors - can't modify Y/E on a EventList
  void setData(const MantidVecPtr::ptr_type & /*Y*/) {
    throw std::runtime_error("EventList: cannot set Y or E data directly.");
  }
  /// Disallowed data accessors - can't modify Y/E on a EventList
  void setData(const MantidVecPtr::ptr_type & /*Y*/,
               const MantidVecPtr::ptr_type & /*E*/) {
    throw std::runtime_error("EventList: cannot set Y or E data directly.");
  }

  /// Disallowed data accessors - can't modify Y/E on a EventList
  MantidVec &dataY() {
    throw std::runtime_error(
        "EventList: non-const access to Y data is not possible.");
  }
  /// Disallowed data accessors - can't modify Y/E on a EventList
  MantidVec &dataE() {
    throw std::runtime_error(
        "EventList: non-const access to E data is not possible.");
  }

  // Allowed data accessors - read-only Y/E histogram VIEWS of an event list
  /// Return a read-only Y histogram view of an event list
  const MantidVec &dataY() const { return constDataY(); }
  /// Return a read-only E histogram view of an event list
  const MantidVec &dataE() const { return constDataE(); }

  const MantidVec &constDataY() const;
  const MantidVec &constDataE() const;

  MantidVec *makeDataY() const;
  MantidVec *makeDataE() const;

  std::size_t getNumberEvents() const;
  bool empty() const;

  size_t getMemorySize() const;

  virtual size_t histogram_size() const;

  void compressEvents(double tolerance, EventList *destination,
                      bool parallel = false);
  // get EventType declaration
  void generateHistogram(const MantidVec &X, MantidVec &Y, MantidVec &E,
                         bool skipError = false) const;
  void generateHistogramPulseTime(const MantidVec &X, MantidVec &Y,
                                  MantidVec &E, bool skipError = false) const;
  void generateHistogramTimeAtSample(const MantidVec &X, MantidVec &Y,
                                     MantidVec &E, const double &tofFactor,
                                     const double &tofOffset,
                                     bool skipError = false) const;

  void integrate(const double minX, const double maxX, const bool entireRange,
                 double &sum, double &error) const;

  double integrate(const double minX, const double maxX,
                   const bool entireRange) const;

  void convertTof(const double factor, const double offset = 0.);

  void scaleTof(const double factor);

  void addTof(const double offset);

  void addPulsetime(const double seconds);

  void maskTof(const double tofMin, const double tofMax);

  void getTofs(std::vector<double> &tofs) const;
  double getTofMin() const;
  double getTofMax() const;
  Mantid::Kernel::DateAndTime getPulseTimeMax() const;
  Mantid::Kernel::DateAndTime getPulseTimeMin() const;
  Mantid::Kernel::DateAndTime getTimeAtSampleMax(const double &tofFactor,
                                                 const double &tofOffset) const;
  Mantid::Kernel::DateAndTime getTimeAtSampleMin(const double &tofFactor,
                                                 const double &tofOffset) const;

  std::vector<double> getTofs() const;

  /// Return the list of event weight  values
  std::vector<double> getWeights() const;
  /// Return the list of event weight values
  void getWeights(std::vector<double> &weights) const;

  /// Return the list of event weight  error values
  std::vector<double> getWeightErrors() const;
  /// Return the list of event weight error values
  void getWeightErrors(std::vector<double> &weightErrors) const;

  std::vector<Mantid::Kernel::DateAndTime> getPulseTimes() const;

  void setTofs(const MantidVec &tofs);

  void reverse();

  void filterByPulseTime(Kernel::DateAndTime start, Kernel::DateAndTime stop,
                         EventList &output) const;

  void filterByTimeAtSample(Kernel::DateAndTime start, Kernel::DateAndTime stop,
                            double tofFactor, double tofOffset,
                            EventList &output) const;

  void filterInPlace(Kernel::TimeSplitterType &splitter);

  void splitByTime(Kernel::TimeSplitterType &splitter,
                   std::vector<EventList *> outputs) const;

  void splitByFullTime(Kernel::TimeSplitterType &splitter,
                       std::map<int, EventList *> outputs, bool docorrection,
                       double toffactor, double tofshift) const;

  /// Split ...
  std::string splitByFullTimeMatrixSplitter(
      const std::vector<int64_t> &vectimes, const std::vector<int> &vecgroups,
      std::map<int, EventList *> vec_outputEventList, bool docorrection,
      double toffactor, double tofshift) const;

  /// Split events by pulse time
  void splitByPulseTime(Kernel::TimeSplitterType &splitter,
                        std::map<int, EventList *> outputs) const;

  void multiply(const double value, const double error = 0.0);
  EventList &operator*=(const double value);

  void multiply(const MantidVec &X, const MantidVec &Y, const MantidVec &E);

  void divide(const double value, const double error = 0.0);
  EventList &operator/=(const double value);

  void divide(const MantidVec &X, const MantidVec &Y, const MantidVec &E);

  void convertUnitsViaTof(Mantid::Kernel::Unit *fromUnit,
                          Mantid::Kernel::Unit *toUnit);
  void convertUnitsQuickly(const double &factor, const double &power);

private:
  /// List of TofEvent (no weights).
  mutable std::vector<TofEvent> events;

  /// List of WeightedEvent's
  mutable std::vector<WeightedEvent> weightedEvents;

  /// List of WeightedEvent's
  mutable std::vector<WeightedEventNoTime> weightedEventsNoTime;

  /// What type of event is in our list.
  Mantid::API::EventType eventType;

  /// Last sorting order
  mutable EventSortType order;

  /// MRU lists of the parent EventWorkspace
  mutable EventWorkspaceMRU *mru;

  /// Mutex that is locked while sorting an event list
  mutable Mantid::Kernel::Mutex m_sortMutex;

  /// Lock out deletion of items in the MRU
  mutable bool m_lockedMRU;

  template <class T>
  static typename std::vector<T>::const_iterator
  findFirstEvent(const std::vector<T> &events, const double seek_tof);

  template <class T>
  static typename std::vector<T>::const_iterator
  findFirstPulseEvent(const std::vector<T> &events, const double seek_tof);

  template <class T>
  typename std::vector<T>::const_iterator
  findFirstTimeAtSampleEvent(const std::vector<T> &events,
                             const double seek_time, const double &tofFactor,
                             const double &tofOffset) const;

  template <class T>
  static typename std::vector<T>::iterator
  findFirstEvent(std::vector<T> &events, const double seek_tof);

  void generateCountsHistogram(const MantidVec &X, MantidVec &Y) const;

  void generateCountsHistogramPulseTime(const MantidVec &X, MantidVec &Y) const;

  void generateCountsHistogramTimeAtSample(const MantidVec &X, MantidVec &Y,
                                           const double &tofFactor,
                                           const double &tofOffset) const;

  void generateErrorsHistogram(const MantidVec &Y, MantidVec &E) const;

  void switchToWeightedEvents();
  void switchToWeightedEventsNoTime();

  // helper functions are all internal to simplify the code
  template <class T1, class T2>
  static void minusHelper(std::vector<T1> &events,
                          const std::vector<T2> &more_events);
  template <class T>
  static void compressEventsHelper(const std::vector<T> &events,
                                   std::vector<WeightedEventNoTime> &out,
                                   double tolerance);
  template <class T>
  void compressEventsParallelHelper(const std::vector<T> &events,
                                    std::vector<WeightedEventNoTime> &out,
                                    double tolerance);
  template <class T>
  static void histogramForWeightsHelper(const std::vector<T> &events,
                                        const MantidVec &X, MantidVec &Y,
                                        MantidVec &E);
  template <class T>
  static void integrateHelper(std::vector<T> &events, const double minX,
                              const double maxX, const bool entireRange,
                              double &sum, double &error);
  template <class T>
  static double integrateHelper(std::vector<T> &events, const double minX,
                                const double maxX, const bool entireRange);
  template <class T>
  void convertTofHelper(std::vector<T> &events, const double factor,
                        const double offset);
  template <class T>
  void addPulsetimeHelper(std::vector<T> &events, const double seconds);
  template <class T>
  static std::size_t maskTofHelper(std::vector<T> &events, const double tofMin,
                                   const double tofMax);
  template <class T>
  static void getTofsHelper(const std::vector<T> &events,
                            std::vector<double> &tofs);
  template <class T>
  static void getWeightsHelper(const std::vector<T> &events,
                               std::vector<double> &weights);
  template <class T>
  static void getWeightErrorsHelper(const std::vector<T> &events,
                                    std::vector<double> &weightErrors);
  template <class T>
  static void
  getPulseTimesHelper(const std::vector<T> &events,
                      std::vector<Mantid::Kernel::DateAndTime> &times);
  template <class T>
  static void setTofsHelper(std::vector<T> &events,
                            const std::vector<double> &tofs);
  template <class T>
  static void
  filterByPulseTimeHelper(std::vector<T> &events, Kernel::DateAndTime start,
                          Kernel::DateAndTime stop, std::vector<T> &output);
  template <class T>
  static void
  filterByTimeAtSampleHelper(std::vector<T> &events, Kernel::DateAndTime start,
                             Kernel::DateAndTime stop, double tofFactor,
                             double tofOffset, std::vector<T> &output);
  template <class T>
  void filterInPlaceHelper(Kernel::TimeSplitterType &splitter,
                           typename std::vector<T> &events);
  template <class T>
  void splitByTimeHelper(Kernel::TimeSplitterType &splitter,
                         std::vector<EventList *> outputs,
                         typename std::vector<T> &events) const;
  template <class T>
  void splitByFullTimeHelper(Kernel::TimeSplitterType &splitter,
                             std::map<int, EventList *> outputs,
                             typename std::vector<T> &events, bool docorrection,
                             double toffactor, double tofshift) const;
  /// Split events by pulse time
  template <class T>
  void splitByPulseTimeHelper(Kernel::TimeSplitterType &splitter,
                              std::map<int, EventList *> outputs,
                              typename std::vector<T> &events) const;
  template <class T>
  std::string splitByFullTimeVectorSplitterHelper(
      const std::vector<int64_t> &vectimes, const std::vector<int> &vecgroups,
      std::map<int, EventList *> outputs, typename std::vector<T> &events,
      bool docorrection, double toffactor, double tofshift) const;
  template <class T>
  static void multiplyHelper(std::vector<T> &events, const double value,
                             const double error = 0.0);
  template <class T>
  static void multiplyHistogramHelper(std::vector<T> &events,
                                      const MantidVec &X, const MantidVec &Y,
                                      const MantidVec &E);
  template <class T>
  static void divideHistogramHelper(std::vector<T> &events, const MantidVec &X,
                                    const MantidVec &Y, const MantidVec &E);
  template <class T>
  void convertUnitsViaTofHelper(typename std::vector<T> &events,
                                Mantid::Kernel::Unit *fromUnit,
                                Mantid::Kernel::Unit *toUnit);
  template <class T>
  void convertUnitsQuicklyHelper(typename std::vector<T> &events,
                                 const double &factor, const double &power);
};

// Methods overloaded to get event vectors.
DLLExport void getEventsFrom(EventList &el, std::vector<TofEvent> *&events);
DLLExport void getEventsFrom(const EventList &el,
                             std::vector<TofEvent> const *&events);
DLLExport void getEventsFrom(EventList &el,
                             std::vector<WeightedEvent> *&events);
DLLExport void getEventsFrom(const EventList &el,
                             std::vector<WeightedEvent> const *&events);
DLLExport void getEventsFrom(EventList &el,
                             std::vector<WeightedEventNoTime> *&events);
DLLExport void getEventsFrom(const EventList &el,
                             std::vector<WeightedEventNoTime> const *&events);

} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTLIST_H_
