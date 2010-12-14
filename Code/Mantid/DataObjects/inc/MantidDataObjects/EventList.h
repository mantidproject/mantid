#ifndef MANTID_DATAOBJECTS_EVENTLIST_H_
#define MANTID_DATAOBJECTS_EVENTLIST_H_ 1

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

  EventList& operator+=(const EventList& more_events);

  EventList& operator-=(const EventList& more_events);

  bool operator==(const EventList& rhs) const;
  bool operator!=(const EventList& rhs) const;

  void addEventQuickly(const TofEvent &event);

  void addEventQuickly(const WeightedEvent &event);


  bool hasWeights() const;

  void switchToWeightedEvents();

  std::vector<TofEvent>& getEvents();
  const std::vector<TofEvent>& getEvents() const;

  std::vector<WeightedEvent>& getWeightedEvents();
  const std::vector<WeightedEvent>& getWeightedEvents() const;

  void addDetectorID(const int detID);

  bool hasDetectorID(const int detID) const;

  std::set<int>& getDetectorIDs();
  const std::set<int>& getDetectorIDs() const;

  void clear();

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

  long int getMemorySize() const;

  virtual size_t histogram_size() const;

  void generateCountsHistogram(const MantidVec& X, MantidVec& Y) const;

  void generateErrorsHistogram(const MantidVec& Y, MantidVec& E) const;

  void generateHistogram(const MantidVec& X, MantidVec& Y, MantidVec& E) const;

  void generateHistogramsForWeights(const MantidVec& X, MantidVec& Y, MantidVec& E) const;

  double integrate(const double minX, const double maxX, const bool entireRange) const;

  void convertTof(const double factor, const double offset=0.);

  void scaleTof(const double factor);

  void addTof(const double offset);

  void maskTof(const double tofMin, const double tofMax);

//  virtual MantidVec * getTofs() const;
  void getTofs(std::vector<double>& tofs) const;

  void setTofs(const MantidVec& T);

  void reverse();

  void filterByPulseTime(Kernel::DateAndTime start, Kernel::DateAndTime stop, EventList & output) const;

  void filterInPlace(Kernel::TimeSplitterType splitter);

  void splitByTime(Kernel::TimeSplitterType splitter, std::vector< EventList * > outputs) const;

  void multiply(const double value);
  void multiply(const double value, const double error);
  EventList& operator*=(const double value);

  void multiply(const MantidVec & X, const MantidVec & Y, const MantidVec & E);

  void divide(const double value);
  void divide(const double value, const double error);
  EventList& operator/=(const double value);

  void divide(const MantidVec & X, const MantidVec & Y, const MantidVec & E);

private:
  ///List of TofEvent (no weights).
  mutable std::vector<TofEvent> events;

  ///List of WeightedEvent's
  mutable std::vector<WeightedEvent> weightedEvents;

  ///Flag indicating whether the events have the weights (use weightedEvents) or not (use events)
  bool has_weights;

  /// Last sorting order
  mutable EventSortType order;

  /// Cached version of the x axis.
  mutable MantidVecPtr refX;

  /// Set of the detector IDs associated with this EventList
  std::set<int> detectorIDs;

  void convertTof_onList(const double factor, const double offset);

  std::vector<WeightedEvent>::iterator findFirstWeightedEvent(const double seek_tof) const;

  std::vector<TofEvent>::iterator findFirstEvent(const double seek_tof) const;

};

} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTLIST_H_
