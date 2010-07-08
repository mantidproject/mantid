#ifndef MANTID_DATAOBJECTS_EVENTLIST_H_
#define MANTID_DATAOBJECTS_EVENTLIST_H_ 1

#ifdef _WIN32 /* _WIN32 */
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;
#include <time.h>
#else
#include <stdint.h> //MG 15/09/09: Required for gcc4.4
#endif
#include <cstddef>
#include <iostream>
#include <vector>
#include "MantidAPI/MatrixWorkspace.h" // get MantidVec declaration
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::Kernel::Unit;

namespace Mantid
{
namespace DataObjects
{

//==========================================================================================
/** Info about a single event: the time of flight of the neutron, and the frame id
 * in which it was detected.
 */
class DLLExport TofEvent {
private:
  /** The units of the time of flight index in nanoseconds. */
  double time_of_flight;

  /**
   * The frame vector is not a member of this object, but it is necessary in
   * order to have the actual time for the data.
   */
  std::size_t frame_index;

 public:
  /// Constructor, specifying the time of flight and the frame id
  TofEvent(double time_of_flight, const std::size_t frameid);

  /// Constructor, copy from another TofEvent object
  TofEvent(const TofEvent&);

  /// Empty constructor
  TofEvent();

  /// Destructor
  virtual ~TofEvent();

  /// Copy from another TofEvent object
  TofEvent& operator=(const TofEvent&rhs);

  /// Return the time of flight, as a double, in nanoseconds.
  double tof() const;

  /// Return the frame id
  std::size_t frame() const;

  /// Output a string representation of the event to a stream
  friend std::ostream& operator<<(std::ostream &os, const TofEvent &event);
};

//==========================================================================================
/** A list of TofEvent objects, corresponding to all the events that were measured on a pixel.
 *
 */
enum EventSortType {UNSORTED, TOF_SORT, FRAME_SORT};

class DLLExport EventList
{
public:
  /// The data storage type used internally in a Histogram1D
  typedef MantidVec StorageType;
  /// Data Store: NOTE:: CHANGED TO BREAK THE WRONG USEAGE OF SHARED_PTR
  typedef Kernel::cow_ptr<StorageType > RCtype;

  /// Constructor (empty)
  EventList();
  /** Constructor copying from an existing event list */
  EventList(const EventList&rhs);

  /** Constructor, taking a vector of events */
  EventList(const std::vector<TofEvent> &events);

  /// Destructor
  virtual ~EventList();

  /** Copy into this event list from another */
  EventList& operator=(const EventList&);

  /** Append an event to the histogram. */
  EventList& operator+=(const TofEvent& event);

  /** Append a list of events to the histogram. */
  EventList& operator+=(const std::vector<TofEvent>& more_events);

  /** Append a list of events to the histogram. */
  EventList& operator+=(const EventList& more_events);

  /** Append an event to the histogram, without clearing the cache, to make it faster. */
  void addEventQuickly(const TofEvent &event);

  /** Return the list of TofEvents contained. */
  std::vector<TofEvent>& getEvents();
  const std::vector<TofEvent>& getEvents() const;

  /** Clear the list of events */
  void clear();

  /** Sort events by TOF or Frame */
  void sort(const EventSortType order) const;
  /** Sort events by TOF */
  void sortTof() const;
  /** Sort events by Frame */
  void sortFrame() const;

  /** Set the x-component for the histogram view. */
  void setX(const RCtype::ptr_type& X, Unit* set_xUnit = NULL);

  /** Set the x-component for the histogram view. */
  void setX(const RCtype& X, Unit* set_xUnit = NULL);

  /** Set the x-component for the histogram view. */
  void setX(const StorageType& X, Unit* set_xUnit = NULL);


  /** Returns the x data. */
  virtual const StorageType& dataX() const;

  /** Returns the y data. */
  virtual const StorageType& dataY() const;

  /** Returns the e data. */
  virtual const StorageType& dataE() const;

  /** Returns a reference to the X data */
  Kernel::cow_ptr<MantidVec> getRefX() const;

  /** Return the number of events in the list. */
  virtual std::size_t getNumberEvents() const;

  /** Return the size of the histogram representation of the data (size of Y) **/
  virtual size_t histogram_size() const;

  /** Delete the cached version of the histogram data. */
  void emptyCache() const;

  void generateCountsHistogram(const StorageType& X, StorageType& Y) const;
  void generateErrorsHistogram(const StorageType& Y, StorageType& E) const;

private:
  ///List of events.
  mutable std::vector<TofEvent> events;

  /** Pointer to unit of the x-axis of the histogram */
  Mantid::Kernel::Unit *xUnit;

  /// Last sorting order
  mutable EventSortType order;
  /** Cached version of the x axis. */
  mutable RCtype refX;
  /** Cached version of the counts. */
  mutable RCtype refY;
  /** Cached version of the uncertainties. */
  mutable RCtype refE;

  mutable bool isCacheDirty;
  mutable bool isErrorCacheDirty;

  
  void generateCountsHistogram() const;
  void generateErrorsHistogram() const;

  /** Delete the cached version of the CALCULATED histogram data. */
  void emptyCacheData();

};

} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTLIST_H_
