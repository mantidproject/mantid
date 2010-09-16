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


/** @class Mantid::DataObjects::EventList

    A class for holding an event list for a single detector
    
    @author Janik, SNS ORNL
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

//==========================================================================================
/** Info about a single event: the time of flight of the neutron, and the frame id
 * in which it was detected.
 */
class DLLExport TofEvent {

  /// EventList has the right to mess with TofEvent.
  friend class EventList;
  friend class tofGreaterOrEqual;
  friend class tofGreater;

private:
  /** The units of the time of flight index in nanoseconds. This is relative to the
   * start of the pulse (stored in pulse_time.
   * EXCEPT: After AlignDetectors is run, this is converted to dSpacing, in Angstroms^-1 */
  double time_of_flight;

  /**
   * The absolute time of the start of the pulse that generated this event.
   * This is saved as the number of ticks (1 nanosecond if boost is compiled
   * for nanoseconds) since a specified epoch: we use the GPS epoch of Jan 1, 1990.
   *
   * 64 bits gives 1 ns resolution up to +- 292 years around 1990. Should be enough.
   */
  Mantid::Kernel::PulseTimeType pulse_time;

 public:
  /// Constructor, specifying the time of flight and the frame id
  TofEvent(double time_of_flight, const Mantid::Kernel::PulseTimeType pulsetime);

  /// Constructor, copy from another TofEvent object
  TofEvent(const TofEvent&);

  /// Empty constructor
  TofEvent();

  /// Destructor
  ~TofEvent();

  /// Copy from another TofEvent object
  TofEvent& operator=(const TofEvent&rhs);

  /// Return the time of flight, as a double, in nanoseconds.
  double tof() const;

  /// Return the frame id
  Mantid::Kernel::PulseTimeType pulseTime() const;

  /// Output a string representation of the event to a stream
  friend std::ostream& operator<<(std::ostream &os, const TofEvent &event);


};


/// How the event list is sorted.
enum EventSortType {UNSORTED, TOF_SORT, PULSETIME_SORT};

//==========================================================================================
/** A list of TofEvent objects, corresponding to all the events that were measured on a pixel.
 *
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

  EventList& operator+=(const EventList& more_events);

  void addEventQuickly(const TofEvent &event);

  std::vector<TofEvent>& getEvents();
  const std::vector<TofEvent>& getEvents() const;

  void addDetectorID(const int detID);

  bool hasDetectorID(const int detID) const;

  std::set<int>& getDetectorIDs();
  const std::set<int>& getDetectorIDs() const;

  void clear();

  void sort(const EventSortType order) const;

  void sortTof() const;

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

  void allocateMoreEvents(int numEvents);

  virtual size_t histogram_size() const;

  void generateCountsHistogram(const MantidVec& X, MantidVec& Y) const;

  void generateErrorsHistogram(const MantidVec& Y, MantidVec& E) const;

  void convertTof(const double factor, const double offset=0.);

  void scaleTof(const double factor);

  void addTof(const double offset);

  void maskTof(const double tofMin, const double tofMax);

  virtual MantidVec * getTofs() const;

  void setTofs(const MantidVec& T);

  void reverse();

  void filterByPulseTime(Kernel::PulseTimeType start, Kernel::PulseTimeType stop, EventList & output) const;

  void splitByTime(Kernel::TimeSplitterType splitter, std::vector< EventList * > outputs) const;

private:
  ///List of events.
  mutable std::vector<TofEvent> events;

  /// Last sorting order
  mutable EventSortType order;

  /** Cached version of the x axis. */
  mutable MantidVecPtr refX;

  /// Set of the detector IDs associated with this EventList
  std::set<int> detectorIDs;
};

} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTLIST_H_
