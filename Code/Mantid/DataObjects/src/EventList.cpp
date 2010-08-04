#include <stdexcept>
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/Exception.h"

using std::ostream;
using std::runtime_error;
using std::size_t;
using std::vector;

namespace Mantid
{
namespace DataObjects
{
  using Kernel::Exception::NotImplementedError;

  //==========================================================================
  /// --------------------- TofEvent stuff ----------------------------------
  //==========================================================================
  /** Constructor, specifying the time of flight and the frame id
   * @param time_of_flight time of flight, in nanoseconds
   * @param frameid frame id, integer
   */
  TofEvent::TofEvent(const double time_of_flight, const std::size_t frameid)
  {
	  this->time_of_flight = time_of_flight;
	  this->frame_index = frameid;
  }

  /** Constructor, copy from another TofEvent object
   * @param rhs Other TofEvent to copy.
   */
  TofEvent::TofEvent(const TofEvent& rhs)
  {
    this->time_of_flight = rhs.time_of_flight;
    this->frame_index = rhs.frame_index;
  }

  /// Empty constructor
  TofEvent::TofEvent()
  {
    this->time_of_flight = 0;
    this->frame_index = 0;
  }

  /// Destructor
  TofEvent::~TofEvent()
  {
  }

  /** Copy from another TofEvent object
   * @param rhs Other TofEvent to copy.
   */
  TofEvent& TofEvent::operator=(const TofEvent& rhs)
  {
	  this->time_of_flight = rhs.time_of_flight;
	  this->frame_index = rhs.frame_index;
	  return *this;
  }

  /// Return the time of flight, as a double, in nanoseconds.
  double TofEvent::tof() const
  {
	  return this->time_of_flight;
  }

  /// Return the frame id
  size_t TofEvent::frame() const
  {
	  return this->frame_index;
  }

  /** Output a string representation of the event to a stream
   * @param os Stream
   * @param event Event to output to the stream
   */
  ostream& operator<<(ostream &os, const TofEvent &event)
  {
    os << event.time_of_flight << "," << event.frame_index;
    return os;
  }


  //==========================================================================
  /// --------------------- TofEvent Comparators ----------------------------------
  //==========================================================================
  /** Compare two events' TOF, return true if e1 should be before e2.
   * @param e1 first event
   * @param e2 second event
   *  */
  bool compareEventTof(TofEvent e1, TofEvent e2)
  {
    return (e1.tof() < e2.tof());
  }

  /** Compare two events' FRAME id, return true if e1 should be before e2.
  * @param e1 first event
  * @param e2 second event
  *  */
  bool compareEventFrame(TofEvent e1, TofEvent e2)
  {
    return (e1.frame() < e2.frame());
  }



  //==========================================================================
  // ---------------------- EventList stuff ----------------------------------
  //==========================================================================

  // --- Constructors -------------------------------------------------------------------

  /// Constructor (empty)
  EventList::EventList()
  {
    this->order = UNSORTED;
  }

  /** Constructor copying from an existing event list
   * @param rhs EventList object to copy*/
  EventList::EventList(const EventList& rhs)
  {
    //Call the copy operator to do the job,
    this->operator=(rhs);
  }

  /** Constructor, taking a vector of events.
   * @param events Vector of TofEvent's */
  EventList::EventList(const std::vector<TofEvent> &events)
  {
    this->events.assign(events.begin(), events.end());
    this->order = UNSORTED;
  }

  /// Destructor
  EventList::~EventList()
  {
  }

  // --------------------------------------------------------------------------
  // --- Operators -------------------------------------------------------------------

  /** Copy into this event list from another
   * @param rhs We will copy all the events from that into this object.*/
  EventList& EventList::operator=(const EventList& rhs)
  {
    //Copy all data from the rhs.
    this->events.assign(rhs.events.begin(), rhs.events.end());
    this->refX = rhs.refX;
    this->order = rhs.order;
    return *this;
  }

  /** Append an event to the histogram.
   * @param event TofEvent to add at the end of the list.*/
  EventList& EventList::operator+=(const TofEvent &event)
  {
    this->events.push_back(event);
    this->order = UNSORTED;    
    return *this;
  }

  /** Append a list of events to the histogram.
   * @param more_events A vector of events to append.*/
  EventList& EventList::operator+=(const std::vector<TofEvent> & more_events)
  {
    this->events.insert(this->events.end(), more_events.begin(), more_events.end());
    this->order = UNSORTED;    
    return *this;
  }

  /** Append a list of events to the histogram.
   * @param more_events Another EventList.
   * */
  EventList& EventList::operator+=(const EventList& more_events)
  {
    vector<TofEvent> rel = more_events.getEvents();
    this->events.insert(this->events.end(), rel.begin(), rel.end());
    this->order = UNSORTED;    
    return *this;
  }


  /** Append an event to the histogram, without clearing the cache, to make it faster.
   * @param event TofEvent to add at the end of the list.
   * */
  void EventList::addEventQuickly(const TofEvent &event)
  {
    this->events.push_back(event);
  }


  // --------------------------------------------------------------------------
  // --- Handling the event list -------------------------------------------------------------------
  /** Return the const list of TofEvents contained. */
  const std::vector<TofEvent>& EventList::getEvents() const
  {
    return this->events;
  }

  /** Return the list of TofEvents contained. */
  std::vector<TofEvent>& EventList::getEvents()
  {
    return this->events;
  }

  /** Clear the list of events */
  void EventList::clear()
  {
    this->events.clear();    
  }

  // --------------------------------------------------------------------------
  // --- Sorting functions -------------------------------------------------------------------
  /** Sort events by TOF or Frame
   * @param order Order by which to sort.
   * */
  void EventList::sort(const EventSortType order) const
  {
    if (order == UNSORTED)
    {
      return; // don't bother doing anything. Why did you ask to unsort?
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

  /** Sort events by TOF */
  void EventList::sortTof() const
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

  /** Sort events by Frame */
  void EventList::sortFrame() const
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


  // --------------------------------------------------------------------------
  /** Return the number of events in the list. */
  size_t EventList::getNumberEvents() const
  {
    return this->events.size();
  }

  /** Return the size of the histogram representation of the data (size of Y) **/
  size_t EventList::histogram_size() const
  {
    size_t x_size = refX->size();
    if (x_size > 1)
      return x_size - 1;
    else
      return 0;
  }

  // --------------------------------------------------------------------------
  // --- Setting the Histrogram X axis, without recalculating the histogram -------------------------
  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   * @param set_xUnit :: [Optional] pointer to the Unit of the X data.
   */
  void EventList::setX(const RCtype::ptr_type& X, Unit* set_xUnit)
  {
    this->refX = X;
    if (!(set_xUnit == NULL))
      this->xUnit = set_xUnit;
  }

  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   * @param set_xUnit :: [Optional] pointer to the Unit of the X data.
   */
  void EventList::setX(const RCtype& X, Unit* set_xUnit)
  {
    this->refX = X;
    if (!(set_xUnit == NULL))
      this->xUnit = set_xUnit;
  }

  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   * @param set_xUnit :: [Optional] pointer to the Unit of the X data.
   */
  void EventList::setX(const StorageType& X, Unit* set_xUnit)
  {
    this->refX.access()=X;
    if (!(set_xUnit == NULL))
      this->xUnit = set_xUnit;
  }


  // --------------------------------------------------------------------------
  // --- Return Data Vectors --------------------------------------------------
  // Note: these will only be called from a const class; e.g
  // const EventList el;
  // el.dataX(); <<<<< this works
  // EventList el2;
  // el2.dataX(); <<<<< this throws an error.

  /** Returns the x data. */
  const EventList::StorageType& EventList::dataX() const
  {
    return *(this->refX);
  }

  /** Returns a reference to the X data */
  Kernel::cow_ptr<MantidVec> EventList::getRefX() const
  {
    return refX;
  }


  /** Calculates and returns a pointer to the Y histogrammed data.
   * Remember to delete your pointer after use!
   * */
  EventList::StorageType * EventList::dataY() const
  {
    StorageType * Y = new StorageType();
    generateCountsHistogram(*this->refX, *Y);
    return Y;
  }

  /** Calculates and returns a pointer to the E histogrammed data.
   * Remember to delete your pointer after use!
   * */
  EventList::StorageType * EventList::dataE() const
  {
    StorageType Y;
    generateCountsHistogram(*this->refX, Y);
    StorageType * E = new StorageType();
    generateErrorsHistogram(Y, *E);
    return E;
  }


  // --------------------------------------------------------------------------
  /** Fill a histogram given specified histogram bounds. Does not modify
   * the eventlist (const method).
   * @param X The x bins
   * @param Y The generated counts histogram
   */
  void EventList::generateCountsHistogram(const StorageType& X, StorageType& Y) const
  {
    //For slight speed=up.
    size_t x_size = X.size();

    if (x_size <= 1)
    {
      //X was not set. Return an empty array.
      Y.resize(0, 0);
      return;
    }

    //Do the histogram.

    //Sort the events by tof
    this->sortTof();
    //Clear the Y data, assign all to 0.
    Y.resize(x_size-1, 0);

    //Do we even have any events to do?
    if (this->events.size() > 0)
    {
      //Iterate through all events (sorted by tof)
      std::vector<TofEvent>::iterator itev = this->events.begin();

      //Find the first bin
      size_t bin=0;
      double tof = itev->tof();
      while (bin < x_size-1)
      {
        //Within range?
        if ((tof >= X[bin]) && (tof < X[bin+1]))
        {
          Y[bin]++;
          break;
        }
        ++bin;
      }
      //Go to the next event, we've alreadyconversions Map with key = pixel ID, and value =  binned this first one.
      ++itev;

      //Keep going through all the events
      while ((itev != this->events.end()) && (bin < x_size-1))
      {
        tof = itev->tof();
        while (bin < x_size-1)
        {
          //Within range?
          if ((tof >= X[bin]) && (tof < X[bin+1]))
          {
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
  ///Generate the Error histogram for the provided counts histogram
  ///@param Y The counts histogram
  ///@param E The generated error histogram
  void EventList::generateErrorsHistogram(const StorageType& Y, StorageType& E) const
  {
      // Fill the vector for the errors, containing sqrt(count)
      E.resize(Y.size(), 0);    

      // windows can get confused about std::sqrt
      typedef double (*uf)(double);
      uf dblSqrt = std::sqrt;
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt); 

  }


  // --------------------------------------------------------------------------
  /**
   * Convert the time of flight by tof'=tof*factor+offset
   */
  void EventList::convertTof(const double factor, const double offset)
  {
    if (this->events.empty())
      return;
    if (factor == 1.)
    {
      this->addTof(offset);
      return;
    }
    if (offset == 0.)
    {
      this->scaleTof(factor);
      return;
    }

    // iterate through all events
    std::vector<TofEvent>::iterator iter;
    for (iter = this->events.begin(); iter != this->events.end(); iter++)
      iter->time_of_flight = iter->time_of_flight * factor + offset;
  }

  /**
   * Convert the units in the TofEvent's contained to d-spacing.
   * WARNING: There is no check to see if you did this before! Don't be dumb
   *          and do it twice!
   * @param factor: conversion factor (multiply TOF by this to get d-spacing)
   */
  void EventList::scaleTof(const double factor)
  {

    //Do we even have any events to do?
    if (this->events.empty())
      return;

    //Iterate through all events
    std::vector<TofEvent>::iterator itev;
    for (itev= this->events.begin(); itev != this->events.end(); itev++)
    {
      itev->time_of_flight *= factor;
    }
    //The sorting of the list will be unchanged, since it is just a multiplicative factor.
  }

  void EventList::addTof(const double offset)
  {
    if (this->events.empty())
      return;

    // iterate through all events
    std::vector<TofEvent>::iterator iter;
    for (iter = this->events.begin(); iter != this->events.end(); iter++)
      iter->time_of_flight += offset;
  }

} /// namespace DataObjects
} /// namespace Mantid
