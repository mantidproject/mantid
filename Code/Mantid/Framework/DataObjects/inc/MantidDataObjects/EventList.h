#ifndef MANTID_DATAOBJECTS_EVENTLIST_H_
#define MANTID_DATAOBJECTS_EVENTLIST_H_ 1

#ifdef _WIN32 /* _WIN32 */
#include <time.h>
#endif
#include <cstddef>
#include <iostream>
#include <vector>
#include "MantidAPI/MatrixWorkspace.h" // get MantidVec declaration
#include "MantidAPI/IEventWorkspace.h" // get EventType declaration
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidDataObjects/Events.h"
#include <set>

namespace Mantid
{
namespace DataObjects
{





/// How the event list is sorted.
enum EventSortType {UNSORTED, TOF_SORT, PULSETIME_SORT};

//==========================================================================================
/** @class Mantid::DataObjects::EventList

    A class for holding :
      - a list of neutron detection events (TofEvent or WeightedEvent).
      - a list of associated detector ID's.

    This class can switch from holding regular TofEvent's (implied weight of 1.0)
    or WeightedEvent (where each neutron can have a non-1 weight).
    This is done transparently.

    @author Janik Zikovsky, SNS ORNL
    @date 4/02/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/

class DLLExport EventList
{
private:


  //==========================================================================
  /** Unary function for searching the event list.
   * Returns true if the event's TOF is >= a value
   * @param event the event being checked.
   */
  template <class T>
  class tofGreaterOrEqual: std::unary_function<T, double>
  {
    /// Comparison variable
    double m_value;
  public:
    /// Constructor: save the value
    tofGreaterOrEqual(double value): m_value(value)
    {  }
    /// () operator: return true if event.tof >= value
    bool operator()(T event)
    {
        return event.m_tof >= m_value;
    }
  };


  //==========================================================================
  /** Unary function for searching the event list.
   * Returns true if the event's TOF is > a value
   * @param event the event being checked.
   */
  template <class T>
  class tofGreater: std::unary_function<T, double>
  {
    /// Comparison variable
    double m_value;
  public:
    /// Constructor: save the value
    tofGreater(double value): m_value(value)
    {  }
    /// () operator: return true if event.tof > value
    bool operator()(T event)
    {
        return event.m_tof > m_value;
    }
  };





public:

  EventList();

  EventList(const EventList&rhs);

  EventList(const std::vector<TofEvent> &events);

  virtual ~EventList();

  EventList& operator=(const EventList&);

  EventList& operator+=(const TofEvent& event);

  EventList& operator+=(const std::vector<TofEvent>& more_events);

  EventList& operator+=(const WeightedEvent& event);

  EventList& operator+=(const std::vector<WeightedEvent>& more_events);

  EventList& operator+=(const std::vector<WeightedEventNoTime> & more_events);

  EventList& operator+=(const EventList& more_events);

  template<class T1, class T2>
  static void minusHelper(std::vector<T1> & events, const std::vector<T2> & more_events);
  EventList& operator-=(const EventList& more_events);

  bool operator==(const EventList& rhs) const;
  bool operator!=(const EventList& rhs) const;

  void addEventQuickly(const TofEvent &event);

  void addEventQuickly(const WeightedEvent &event);

  Mantid::API::EventType getEventType() const;

  void switchTo(Mantid::API::EventType newType);

//  bool hasWeights() const;

  WeightedEvent getEvent(size_t event_number);

  std::vector<TofEvent>& getEvents();
  const std::vector<TofEvent>& getEvents() const;

  std::vector<WeightedEvent>& getWeightedEvents();
  const std::vector<WeightedEvent>& getWeightedEvents() const;

  std::vector<WeightedEventNoTime>& getWeightedEventsNoTime();
  const std::vector<WeightedEventNoTime>& getWeightedEventsNoTime() const;

  void addDetectorID(const int detID);

  bool hasDetectorID(const int detID) const;

  std::set<int>& getDetectorIDs();
  const std::set<int>& getDetectorIDs() const;

  void clear();
  void clearUnused();

  void reserve(size_t num);

  void sort(const EventSortType order) const;

  void sortTof() const;
  void sortTof2() const;
  void sortTof4() const;

  void sortPulseTime() const;

  bool isSortedByTof() const;


  void setX(const MantidVecPtr::ptr_type& X);

  void setX(const MantidVecPtr& X);

  void setX(const MantidVec& X);

  virtual const MantidVec& dataX() const;

  virtual MantidVec * dataY() const;

  virtual MantidVec * dataE() const;

  Kernel::cow_ptr<MantidVec> getRefX() const;

  virtual std::size_t getNumberEvents() const;

  size_t getMemorySize() const;

  virtual size_t histogram_size() const;

  template<class T>
  static void compressEventsHelper(const std::vector<T> & events, std::vector<WeightedEventNoTime> & out, double tolerance);
  void compressEvents(double tolerance, EventList * destination);

  template<class T>
  static void histogramForWeightsHelper(const std::vector<T> & events, const MantidVec & X, MantidVec & Y, MantidVec & E);
  void generateHistogram(const MantidVec& X, MantidVec& Y, MantidVec& E, bool skipError = false) const;

  template<class T>
  static double integrateHelper(std::vector<T> & events, const double minX, const double maxX, const bool entireRange);
  double integrate(const double minX, const double maxX, const bool entireRange) const;

  template<class T>
  void convertTofHelper(std::vector<T> & events, const double factor, const double offset);
  void convertTof(const double factor, const double offset=0.);

  void scaleTof(const double factor);

  void addTof(const double offset);

  template<class T>
  static void maskTofHelper(std::vector<T> & events, const double tofMin, const double tofMax);
  void maskTof(const double tofMin, const double tofMax);

  template<class T>
  static void getTofsHelper(const std::vector<T> & events, std::vector<double> & tofs);
  void getTofs(std::vector<double>& tofs) const;

  template<class T>
  static void setTofsHelper(std::vector<T> & events, const std::vector<double> & tofs);
  void setTofs(const MantidVec& tofs);

  void reverse();

  template<class T>
  static void filterByPulseTimeHelper(std::vector<T> & events, Kernel::DateAndTime start, Kernel::DateAndTime stop, std::vector<T> & output);
  void filterByPulseTime(Kernel::DateAndTime start, Kernel::DateAndTime stop, EventList & output) const;

  template< class T >
  void filterInPlaceHelper(Kernel::TimeSplitterType & splitter, typename std::vector<T> & events);
  void filterInPlace(Kernel::TimeSplitterType & splitter);

  template< class T >
  void splitByTimeHelper(Kernel::TimeSplitterType & splitter, std::vector< EventList * > outputs, typename std::vector<T> & events) const;
  void splitByTime(Kernel::TimeSplitterType & splitter, std::vector< EventList * > outputs) const;

  template< class T>
  static void multiplyHelper(std::vector<T> & events, const double value, const double error = 0.0);
  void multiply(const double value, const double error = 0.0);
  EventList& operator*=(const double value);

  template<class T>
  static void multiplyHistogramHelper(std::vector<T> & events, const MantidVec & X, const MantidVec & Y, const MantidVec & E);
  void multiply(const MantidVec & X, const MantidVec & Y, const MantidVec & E);

  template<class T>
  static void divideHistogramHelper(std::vector<T> & events, const MantidVec & X, const MantidVec & Y, const MantidVec & E);
  void divide(const double value, const double error=0.0);
  EventList& operator/=(const double value);

  void divide(const MantidVec & X, const MantidVec & Y, const MantidVec & E);

private:
  ///List of TofEvent (no weights).
  mutable std::vector<TofEvent> events;

  ///List of WeightedEvent's
  mutable std::vector<WeightedEvent> weightedEvents;

  ///List of WeightedEvent's
  mutable std::vector<WeightedEventNoTime> weightedEventsNoTime;

  /// What type of event is in our list.
  Mantid::API::EventType eventType;

  /// Last sorting order
  mutable EventSortType order;

  /// Cached version of the x axis.
  mutable MantidVecPtr refX;

  /// Set of the detector IDs associated with this EventList
  std::set<int> detectorIDs;

  void convertTof_onList(const double factor, const double offset);

  template<class T>
  static typename std::vector<T>::const_iterator findFirstEvent(const std::vector<T> & events, const double seek_tof);

  template<class T>
  static typename std::vector<T>::iterator findFirstEvent(std::vector<T> & events, const double seek_tof);

  void generateCountsHistogram(const MantidVec& X, MantidVec& Y) const;

  void generateErrorsHistogram(const MantidVec& Y, MantidVec& E) const;

  void switchToWeightedEvents();
  void switchToWeightedEventsNoTime();

  //std::vector<TofEvent>::iterator findFirstEvent(const double seek_tof) const;

};

} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTLIST_H_
