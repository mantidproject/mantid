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
    this->isCacheDirty = true;
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
    this->isCacheDirty = true;
  }

  /// Destructor
  EventList::~EventList()
  {
  }

  // --- Operators -------------------------------------------------------------------

  /** Copy into this event list from another
   * @param rhs We will copy all the events from that into this object.*/
  EventList& EventList::operator=(const EventList& rhs)
  {
    //Copy all data from the rhs.
    this->events.assign(rhs.events.begin(), rhs.events.end());
    this->refX = rhs.refX;
    this->refY = rhs.refY;
    this->refE = rhs.refE;
    this->order = rhs.order;
    this->isCacheDirty = rhs.isCacheDirty;
    this->isErrorCacheDirty = rhs.isErrorCacheDirty;
    return *this;
  }

  /** Append an event to the histogram.
   * @param event TofEvent to add at the end of the list.*/
  EventList& EventList::operator+=(const TofEvent &event)
  {
    this->events.push_back(event);
    this->order = UNSORTED;    
    this->isCacheDirty = true;
    return *this;
  }

  /** Append a list of events to the histogram.
   * @param more_events A vector of events to append.*/
  EventList& EventList::operator+=(const std::vector<TofEvent> & more_events)
  {
    this->events.insert(this->events.end(), more_events.begin(), more_events.end());
    this->order = UNSORTED;    
    this->isCacheDirty = true;
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
    this->isCacheDirty = true;
    return *this;
  }


  /** Append an event to the histogram, without clearing the cache, to make it faster.
   * @param event TofEvent to add at the end of the list.
   * */
  void EventList::addEventQuickly(const TofEvent &event)
  {
    this->events.push_back(event);
  }


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
    this->isCacheDirty = true;
  }

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


  /** Return the number of events in the list. */
  size_t EventList::getNumberEvents() const
  {
    return this->events.size();
  }

  /** Return the size of the histogram representation of the data (size of Y) **/
  size_t EventList::histogram_size() const
  {
    return dataY().size();
  }

  // --- Setting the Histrogram X axis, without recalculating the cache -------------------------
  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   * @param set_xUnit :: [Optional] pointer to the Unit of the X data.
   */
  void EventList::setX(const RCtype::ptr_type& X, Unit* set_xUnit)
  {
    this->emptyCache();
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
    this->emptyCache();
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
    this->emptyCache();
    this->refX.access()=X;
    if (!(set_xUnit == NULL))
      this->xUnit = set_xUnit;
  }


  // --- Return Data Vectors -------------------------------------------------------------
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

  /** Returns the Y data. */
  const EventList::StorageType& EventList::dataY() const
  {
    if (isCacheDirty)
      this->generateCountsHistogram();
    return *(this->refY);
  }

  /** Returns the E data. */
  const EventList::StorageType& EventList::dataE() const
  {
    if (isCacheDirty)
      this->generateCountsHistogram();
    if (isErrorCacheDirty)
      this->generateErrorsHistogram();
    return *(this->refE);
  }

  /** Returns a reference to the X data */
  Kernel::cow_ptr<MantidVec> EventList::getRefX() const
  {
    this->isCacheDirty = true;
    return refX;
  }

  // --- Histogram functions -------------------------------------------------
  void EventList::emptyCache() const
  {
    if (refX->size() > 0)
    {
      this->isCacheDirty = true;
      this->refX.access().clear();
    }
    if (refY->size() > 0)    
    {
      this->isCacheDirty = true;
      this->refY.access().clear();
    }
    if (refE->size() > 0)
    {
      this->isErrorCacheDirty = true;
      this->refE.access().clear();
    }
  }

  /** Delete the cached version of the CALCULATED histogram data.
   * Necessary when modifying the event list.
   * */
  void EventList::emptyCacheData() const
  {
    this->isCacheDirty = true;
    this->refY.access().clear();
    this->refE.access().clear();
  }

  /** Delete and release the memory of the calculated histogram data
   *
   */
  void EventList::releaseDataMemory() const
  {
    this->isCacheDirty = true;
    this->refY.access().clear();
    this->refE.access().clear();
    //This swapping trick is necessary to make std::vector release the allocated memory!!!
    MantidVec().swap( this->refY.access() );
    MantidVec().swap( this->refE.access() );
  }

  /** Fill a histogram given specified histogram bounds in this spectra list - replace the cached values.
   */
  void EventList::generateCountsHistogram() const
  {
      const StorageType& X = *refX;
      StorageType& Y = refY.access();
      generateCountsHistogram(X,Y);
  }

  /** Fill a histogram given specified histogram bounds. Does not modify
   * the eventlist (const method).
   * @param X The x bins
   * @param Y The generated counts histogram
   */
  void EventList::generateCountsHistogram(const StorageType& X, StorageType& Y) const
  {
    //We are recalculating then the errors cache is dirty
    this->isErrorCacheDirty = true;

    //For slight speed=up.
    size_t x_size = X.size();

    if (x_size <= 1)
    {
      //By default, if no histogram bins are set, simply sum up all events!
      // This is equivalent to a single bin from -inf to +inf.
      Y.resize(1, 0);
      //Set the single bin to the total # of events.
      Y[0] = this->events.size();
      //And we're done! That was easy.
      this->isCacheDirty = false;
      return;
    }

    //TODO: Should we have a smarter check for this?
    if (Y.size() != x_size-1)
    {
      //Need to redo the histogram.
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
        //Go to the next event, we've already binned this first one.
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
    }// end if (we need to re-histogram)
    this->isCacheDirty = false;
  }

  ///Generate the error histogram for this event list and replace the cache
  void EventList::generateErrorsHistogram() const
  {
      const StorageType& Y = *refY;
      StorageType& E = refE.access();
      generateErrorsHistogram(Y,E);
  }

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
 
      this->isErrorCacheDirty = false;
  }

} /// namespace DataObjects
} /// namespace Mantid
