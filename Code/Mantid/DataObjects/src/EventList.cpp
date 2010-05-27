#include <stdexcept>
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/Exception.h"

using std::runtime_error;
using std::size_t;
using std::vector;

namespace Mantid
{
namespace DataObjects
{
  using Kernel::Exception::NotImplementedError;

  /// --------------------- TofEvent stuff ----------------------------------
  TofEvent::TofEvent(const double time_of_flight, const size_t frameid)
  {
	  this->time_of_flight = time_of_flight;
	  this->frame_index = frameid;
  }

  TofEvent::TofEvent(const TofEvent& rhs)
  {
    this->time_of_flight = rhs.time_of_flight;
    this->frame_index = rhs.frame_index;
  }

  TofEvent::TofEvent()
  {
    this->time_of_flight = 0;
    this->frame_index = 0;
  }

  TofEvent::~TofEvent()
  {
  }

  TofEvent& TofEvent::operator=(const TofEvent& rhs) {
	  this->time_of_flight = rhs.time_of_flight;
	  this->frame_index = rhs.frame_index;
	  return *this;
  }

  double TofEvent::tof()
  {
	  return this->time_of_flight;
  }

  size_t TofEvent::frame()
  {
	  return this->frame_index;
  }

  /// --------------------- TofEvent Comparators ----------------------------------
  /** Compare two events' TOF, return true if e1 should be before e2. */
  bool compareEventTof(TofEvent e1, TofEvent e2)
  {
    return (e1.tof() < e2.tof());
  }

  /** Compare two events' FRAME id, return true if e1 should be before e2. */
  bool compareEventFrame(TofEvent e1, TofEvent e2)
  {
    return (e1.frame() < e2.frame());
  }





  /// --------------------- EventList stuff ----------------------------------

  EventList::EventList()
  {
    this->order = UNSORTED;
  }

  EventList::EventList(const EventList& rhs)
  {
	  this->events.assign(rhs.events.begin(), rhs.events.end());
	  this->order = rhs.order;
  }

  EventList::EventList(const vector<TofEvent> &events)
  {
    this->events.assign(events.begin(), events.end());
    this->order = UNSORTED;
  }

  EventList::~EventList()
  {
  }

  EventList& EventList::operator=(const EventList& rhs)
  {
    this->events.assign(rhs.events.begin(), rhs.events.end());
    this->order = UNSORTED;
    return *this;
  }


  EventList& EventList::operator+=(const TofEvent &event)
  {
    this->events.push_back(event);
    this->order = UNSORTED;
    return *this;
  }

  EventList& EventList::operator+=(const std::vector<TofEvent> & events)
  {
    this->events.insert(this->events.end(), events.begin(), events.end());
    this->order = UNSORTED;
    return *this;
  }

  std::vector<TofEvent> EventList::getEvents()
  {
    return this->events;
  }


  void EventList::sort(const EventSortType order)
  {
    if (order == UNSORTED)
    {
      return; // don't bother doing anything
    }
    else if (order == TOF_SORT)
    {
      this->sortTof();
    }
    else if (order == FRAME_SORT)
    {
      this->sortFrame();
    }
    else
    {
      throw runtime_error("Invalid sort type in EventList::sort(EventSortType)");
    }
  }

  void EventList::sortTof()
  {
    if (this->order == TOF_SORT)
    {
      return; // nothing to do
    }
    //Perform sort.
    std::sort(events.begin(), events.end(), compareEventTof);
    //Save the order to avoid unnecessary re-sorting.
    this->order = TOF_SORT;
  }

  void EventList::sortFrame()
  {
    if (this->order == FRAME_SORT)
    {
      return; // nothing to do
    }
    //Perform sort.
    std::sort(events.begin(), events.end(), compareEventFrame);
    //Save the order to avoid unnecessary re-sorting.
    this->order = FRAME_SORT;
  }

  void EventList::setX(const RCtype::ptr_type& X)
  {
    this->emptyCache();
    this->refX = X;
  }

    const EventList::StorageType& EventList::dataX() const
    {
      return *(this->refX);
    }
    const EventList::StorageType& EventList::dataY()
    {
      this->generateHistogram();
      return *(this->refY);
    }

    const EventList::StorageType& EventList::dataE()
    {
      this->generateHistogram();
      return *(this->refE);
    }


  void EventList::emptyCache()
  {
    this->refX.access().clear();
    this->refY.access().clear();
    this->refE.access().clear();
  }

  void EventList::generateHistogram()
  {
    throw NotImplementedError("EventList::generateHistogram() is not implemented");
  }

} /// namespace DataObjects
} /// namespace Mantid
