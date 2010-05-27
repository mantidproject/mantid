#ifndef MANTID_DATAOBJECTS_EVENTLIST_H_
#define MANTID_DATAOBJECTS_EVENTLIST_H_ 1

#ifdef _WIN32 /* _WIN32 */
typedef unsigned uint32_t;
#include <time.h>
#else
#include <stdint.h> //MG 15/09/09: Required for gcc4.4
#endif
#include <cstddef>
#include <vector>
#include "MantidAPI/MatrixWorkspace.h" // get MantidVec declaration
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/System.h"

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
  /** Constructor, specifying the time of flight and the frame id */
  TofEvent(double time_of_flight, const std::size_t frameid);

  /** Constructor, copy from another TofEvent object */
  TofEvent(const TofEvent&);

  /** Empty constructor, copy from another TofEvent object */
  TofEvent();

  /** Copy into this object from another */
  TofEvent& operator=(const TofEvent&);
  virtual ~TofEvent();

  /** Return the time of flight, as an int, in nanoseconds.*/
  double tof();

  /** Return the frame id */
  std::size_t frame();

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

  EventList();
  /** Constructor copying from an existing event list */
  EventList(const EventList&);

  /** Constructor, taking a vector of events */
  EventList(const std::vector<TofEvent> &);

  /** Copy into this event list from another */
  EventList& operator=(const EventList&);
  virtual ~EventList();

  /** Append an event to the histogram. */
  EventList& operator+=(const TofEvent&);

  /** Append a list of events to the histogram. */
  EventList& operator+=(const std::vector<TofEvent>&);

  /** Return the list of TofEvents contained. */
  std::vector<TofEvent> getEvents();

  void sort(const EventSortType);
  void sortTof();
  void sortFrame();

  /**
   * Set the x-component for the histogram view. This will not cause the
   * histogram to be calculated.
   */
  void setX(const RCtype::ptr_type& X);

  /** Returns the x data const. */
  virtual const StorageType& dataX() const;

  /** Returns the y data const. */
  virtual const StorageType& dataY();

  /** Returns the error data const. */
  virtual const StorageType& dataE();

  /** Delete the cached version of the histogram data. */
  void emptyCache();

private:
  std::vector<TofEvent> events;
  EventSortType order;
  /** Cached version of the x axis. */
  RCtype refX;
  /** Cached version of the counts. */
  RCtype refY;
  /** Cached version of the uncertainties. */
  RCtype refE;

  void generateHistogram();
};

} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTLIST_H_
