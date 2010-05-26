#include "MantidDataObjects/EventList.h"
#include "MantidKernel/Exception.h"

using std::size_t;
using std::vector;

namespace Mantid
{
namespace DataObjects
{
  using Kernel::Exception::NotImplementedError;

  /// --------------------- TofEvent stuff
  TofEvent::TofEvent(const size_t time_of_flight, const size_t frameid)
  {
	  this->time_of_flight = time_of_flight;
	  this->frame_index = frameid;
  }

  TofEvent::TofEvent(const TofEvent& rhs)
  {
	  this->time_of_flight = rhs.time_of_flight;
	  this->frame_index = rhs.frame_index;
  }

  TofEvent::~TofEvent()
  {
  }

  TofEvent& TofEvent::operator=(const TofEvent& rhs) {
	  this->time_of_flight = rhs.time_of_flight;
	  this->frame_index = rhs.frame_index;
	  return *this;
  }

  size_t TofEvent::tof()
  {
	  return this->time_of_flight;
  }

  size_t TofEvent::frame()
  {
	  return this->frame_index;
  }

  /// --------------------- TofEvent stuff
  EventList::EventList()
  {
  }

  EventList::EventList(const EventList& rhs)
  {
	  this->events.assign(rhs.events.begin(), rhs.events.end());
  }

  EventList::EventList(const vector<TofEvent> &events)
  {
    this->events.assign(events.begin(), events.end());
  }

  EventList::~EventList()
  {
  }

  EventList& EventList::operator+=(const TofEvent &event)
  {
    this->events.push_back(event);
    return *this;
  }
  EventList& EventList::operator+=(const std::vector<TofEvent> & events)
  {
    this->events.insert(this->events.end(), events.begin(), events.end());
    return *this;
  }

} /// namespace DataObjects
} /// namespace Mantid
