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
#include "MantidKernel/System.h"

namespace Mantid
{
namespace DataObjects
{

class DLLExport TofEvent {
private:
  /** The units of the time of flight index is nanoseconds. */
  std::size_t time_of_flight;
  /**
   * The frame vector is not a member of this object, but it is necessary in
   * order to have the actual time for the data.
   */
  std::size_t frame_index;
 public:
  TofEvent(const std::size_t, const std::size_t);
  TofEvent(const TofEvent&);
  TofEvent& operator=(const TofEvent&);
  virtual ~TofEvent();

  std::size_t tof();
  std::size_t frame();

};

class DLLExport EventList
{
public:
  EventList();
  EventList(const EventList&);
  EventList(const std::vector<TofEvent> &);
  EventList& operator=(const EventList&);
  virtual ~EventList();
  /** Add an event to the histogram. */
  EventList& operator+=(const TofEvent&);
  EventList& operator+=(const std::vector<TofEvent>&);
private:
  std::vector<TofEvent> events;
};

} // DataObjects
} // Mantid
#endif /// MANTID_DATAOBJECTS_EVENTLIST_H_
