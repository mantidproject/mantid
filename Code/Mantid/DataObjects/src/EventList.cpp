#include <stdexcept>
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/Exception.h"
#include <functional>

using std::ostream;
using std::runtime_error;
using std::size_t;
using std::vector;

namespace Mantid
{
namespace DataObjects
{
using Kernel::Exception::NotImplementedError;
using Kernel::PulseTimeType;

  //==========================================================================
  /// --------------------- TofEvent stuff ----------------------------------
  //==========================================================================
  /** Constructor, specifying the time of flight and the frame id
   * @param time_of_flight time of flight, in nanoseconds
   * @param frameid frame id, integer
   */
  TofEvent::TofEvent(const double time_of_flight, const PulseTimeType pulse_time) :
              time_of_flight(time_of_flight), pulse_time(pulse_time)
  {
  }

  /** Constructor, copy from another TofEvent object
   * @param rhs Other TofEvent to copy.
   */
  TofEvent::TofEvent(const TofEvent& rhs) :
      time_of_flight(rhs.time_of_flight), pulse_time(rhs.pulse_time)
  {
  }

  /// Empty constructor
  TofEvent::TofEvent() :
            time_of_flight(0), pulse_time(0)
  {
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
	  this->pulse_time = rhs.pulse_time;
	  return *this;
  }

  /// Return the time of flight, as a double, in nanoseconds.
  double TofEvent::tof() const
  {
	  return this->time_of_flight;
  }

  /// Return the frame id
  PulseTimeType TofEvent::pulseTime() const
  {
	  return this->pulse_time;
  }

  /** Output a string representation of the event to a stream
   * @param os Stream
   * @param event Event to output to the stream
   */
  ostream& operator<<(ostream &os, const TofEvent &event)
  {
    os << event.time_of_flight << "," << event.pulse_time;
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
  bool compareEventPulseTime(TofEvent e1, TofEvent e2)
  {
    return (e1.pulseTime() < e2.pulseTime());
  }

  //==========================================================================
  /** Unary function for searching the event list.
   * Returns true if the event's TOF is >= a value
   * @param event the event being checked.
   */
  class tofGreaterOrEqual: std::unary_function<TofEvent, double>
  {
    /// Comparison variable
    double m_value;
  public:
    /// Constructor: save the value
    tofGreaterOrEqual(double value): m_value(value)
    {  }
    /// () operator: return true if event.tof >= value
    bool operator()(TofEvent event)
    {
        return event.time_of_flight >= m_value;
    }
  };

  //==========================================================================
  /** Unary function for searching the event list.
   * Returns true if the event's TOF is > a value
   * @param event the event being checked.
   */
  class tofGreater: std::unary_function<TofEvent, double>
  {
    /// Comparison variable
    double m_value;
  public:
    /// Constructor: save the value
    tofGreater(double value): m_value(value)
    {  }
    /// () operator: return true if event.tof > value
    bool operator()(TofEvent event)
    {
        return event.time_of_flight > m_value;
    }
  };




  //==========================================================================
  // ---------------------- EventList stuff ----------------------------------
  //==========================================================================

  // --- Constructors -------------------------------------------------------------------

  /// Constructor (empty)
  EventList::EventList() :
      order(UNSORTED),
      detectorIDs()
  {
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
    //Copy the detector ID set
    this->detectorIDs = rhs.detectorIDs;
    return *this;
  }

  // --------------------------------------------------------------------------
  /** Append an event to the histogram.
   * @param event TofEvent to add at the end of the list.*/
  EventList& EventList::operator+=(const TofEvent &event)
  {
    this->events.push_back(event);
    this->order = UNSORTED;    
    return *this;
  }

  // --------------------------------------------------------------------------
  /** Append a list of events to the histogram.
   * @param more_events A vector of events to append.*/
  EventList& EventList::operator+=(const std::vector<TofEvent> & more_events)
  {
    this->events.insert(this->events.end(), more_events.begin(), more_events.end());
    this->order = UNSORTED;    
    return *this;
  }

  // --------------------------------------------------------------------------
  /** Append another EventList to this event list.
   * The event lists are concatenated, and a union of the sets of detector ID's is done.
   * @param more_events Another EventList.
   * */
  EventList& EventList::operator+=(const EventList& more_events)
  {
    vector<TofEvent> rel = more_events.getEvents();
    this->events.insert(this->events.end(), rel.begin(), rel.end());
    this->order = UNSORTED;
    //Do a union between the detector IDs of both lists
    std::set<int>::const_iterator it;
    for (it = more_events.detectorIDs.begin(); it != more_events.detectorIDs.end(); it++ )
      this->detectorIDs.insert( *it );

    return *this;
  }


  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it faster.
   * @param event TofEvent to add at the end of the list.
   * */
  void EventList::addEventQuickly(const TofEvent &event)
  {
    this->events.push_back(event);
  }


  // --------------------------------------------------------------------------
  /** Add a detector ID to the set of detector IDs
   *
   * @param detID detector ID to insert in set.
   */
  void EventList::addDetectorID(const int detID)
  {
    this->detectorIDs.insert( detID );
  }

  // --------------------------------------------------------------------------
  /** Return true if the given detector ID is in the list for this eventlist */
  bool EventList::hasDetectorID(const int detID) const
  {
    return ( detectorIDs.find(detID) != detectorIDs.end() );
  }

  // --------------------------------------------------------------------------
  /** Get a const reference to the detector IDs set.
   */
  const std::set<int>& EventList::getDetectorIDs() const
  {
    return this->detectorIDs;
  }

  /** Get a mutable reference to the detector IDs set.
   */
  std::set<int>& EventList::getDetectorIDs()
  {
    return this->detectorIDs;
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

  /** Clear the list of events and any
   * associated detector ID's.
   * */
  void EventList::clear()
  {
    this->events.clear();
    this->detectorIDs.clear();
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
    else if (order == PULSETIME_SORT)
    {
      this->sortPulseTime();
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
  void EventList::sortPulseTime() const
  {
    if (this->order == PULSETIME_SORT)
    {
      return; // nothing to do
    }
    //Perform sort.
    std::sort(events.begin(), events.end(), compareEventPulseTime);
    //Save the order to avoid unnecessary re-sorting.
    this->order = PULSETIME_SORT;
  }

  /** Return true if the event list is sorted by TOF */
  bool EventList::isSortedByTof() const
  {
    return (this->order == TOF_SORT);
  }

  /** Reverse the histogram boundaries and the associated events if they are sorted */
  void EventList::reverse()
  {
    // reverse the histogram bin parameters
    MantidVec x = this->refX.access();
    std::reverse(x.begin(), x.end());
    this->refX.access() = x;

    // flip the events if they are tof sorted
    if (this->isSortedByTof())
      std::reverse(this->events.begin(), this->events.end());
    else
      this->order = UNSORTED;
  }

  // --------------------------------------------------------------------------
  /** Return the number of events in the list. */
  size_t EventList::getNumberEvents() const
  {
    return this->events.size();
  }

  void EventList::allocateMoreEvents(int numEvents)
  {
    this->events.reserve( events.size() + numEvents );
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
   */
  void EventList::setX(const MantidVecPtr::ptr_type& X)
  {
    this->refX = X;
  }

  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   */
  void EventList::setX(const MantidVecPtr& X)
  {
    this->refX = X;
  }

  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   */
  void EventList::setX(const MantidVec& X)
  {
    this->refX.access()=X;
  }


  // --------------------------------------------------------------------------
  // --- Return Data Vectors --------------------------------------------------
  // Note: these will only be called from a const class; e.g
  // const EventList el;
  // el.dataX(); <<<<< this works
  // EventList el2;
  // el2.dataX(); <<<<< this throws an error.

  /** Returns the x data. */
  const MantidVec& EventList::dataX() const
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
   */
  MantidVec * EventList::dataY() const
  {
    MantidVec * Y = new MantidVec();
    generateCountsHistogram(*this->refX, *Y);
    return Y;
  }

  /** Calculates and returns a pointer to the E histogrammed data.
   * Remember to delete your pointer after use!
   */
  MantidVec * EventList::dataE() const
  {
    MantidVec Y;
    generateCountsHistogram(*this->refX, Y);
    MantidVec * E = new MantidVec();
    generateErrorsHistogram(Y, *E);
    return E;
  }


  // --------------------------------------------------------------------------
  /** Fill a histogram given specified histogram bounds. Does not modify
   * the eventlist (const method).
   * @param X The x bins
   * @param Y The generated counts histogram
   */
  void EventList::generateCountsHistogram(const MantidVec& X, MantidVec& Y) const
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

      //if tof < X[0], that means that you need to skip some events
      while ((itev != this->events.end()) && (itev->tof() < X[0]))
        itev++;

      //Find the first bin
      size_t bin=0;

      //The tof is greater the first bin boundary, so we need to find the first bin
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

  }

  // --------------------------------------------------------------------------
  /**
   * Generate the Error histogram for the provided counts histogram
   * @param Y The counts histogram
   * @param E The generated error histogram
   */
  void EventList::generateErrorsHistogram(const MantidVec& Y, MantidVec& E) const
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
   * @param factor The value to scale the time-of-flight by
   * @param offset The value to shift the time-of-flight by
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
    for (std::vector<TofEvent>::iterator iter = this->events.begin();
         iter != this->events.end(); iter++)
      iter->time_of_flight = iter->time_of_flight * factor + offset;

    // fix the histogram parameter
    MantidVec x = this->refX.access();
    for (MantidVec::iterator iter = x.begin(); iter != x.end(); ++iter)
      *iter = (*iter) * factor + offset;
    this->refX.access() = x;

    if (factor < 0.)
      this->reverse();
  }

  /**
   * Convert the units in the TofEvent's time_of_flight field to
   *  some other value, by scaling by a multiplier.
   * @param factor conversion factor (e.g. multiply TOF by this to get d-spacing)
   */
  void EventList::scaleTof(const double factor)
  {

    //Do we even have any events to do?
    if (this->events.empty())
      return;

    MantidVec x = this->refX.access();
    std::transform(x.begin(), x.end(), x.begin(),
                   std::bind2nd(std::multiplies<double>(), factor));

    //Iterate through all events
    for (std::vector<TofEvent>::iterator iter = this->events.begin();
         iter != this->events.end(); iter++)
      iter->time_of_flight *= factor;
    this->refX.access() = x;

    if (factor < 0.)
      this->reverse();

  }

  /** Add an offset to the TOF of each event in the list.
   * @param offset The value to shift the time-of-flight by
   */
  void EventList::addTof(const double offset)
  {
    if (this->events.empty())
      return;

    // iterate through all events
    std::vector<TofEvent>::iterator iter;
    for (iter = this->events.begin(); iter != this->events.end(); iter++)
      iter->time_of_flight += offset;

    // fix the histogram vector
    MantidVec x = this->refX.access();
    std::transform(x.begin(), x.end(), x.begin(),
                   std::bind2nd(std::plus<double>(), offset));
    this->refX.access() = x;
  }



  /**
   * Mask out events that have a tof between tofMin and tofMax (inclusively).
   * Events are removed from the list.
   * @param tofMin lower bound of TOF to filter out
   * @param tofMax upper bound of TOF to filter out
   */
  void EventList::maskTof(const double tofMin, const double tofMax)
  {
    if (tofMax <= tofMin)
      throw std::runtime_error("EventList::maskTof: tofMax must be > tofMin");

    //Start by sorting by tof
    this->sortTof();

    //Find the index of the first tofMin
    std::vector<TofEvent>::iterator it_first = std::find_if(this->events.begin(), this->events.end(), tofGreaterOrEqual(tofMin));
    if (it_first != events.end())
    {
      //Something was found
      //Look for the first one > tofMax
      std::vector<TofEvent>::iterator it_last = std::find_if(it_first, this->events.end(), tofGreater(tofMax));
      //it_last will either be at the end (if not found) or before it.
      //Erase this range from the vector
      events.erase(it_first, it_last);
      //Done! Sorting is still valid, no need to redo.
    }

  }

  /**
   * Get a list of the TOFs
   *
   * @return A list of the current TOFs
   */
  MantidVec * EventList::getTofs() const
  {
    MantidVec * tofs = new MantidVec();
    // iterate through all events
    std::vector<TofEvent>::iterator iter;
    for (iter = this->events.begin(); iter != this->events.end(); iter++)
    {
      tofs->push_back(iter->time_of_flight);
    }
    return tofs;
  }

  /**
   * Set a list of TOFs to the current event list. Modify the units if necessary.
   * NOTE: This function does not check if the list sizes are the same.
   *
   * @param T The vector of doubles to set the tofs to.
   */
  void EventList::setTofs(const MantidVec &T)
  {
    if (T.empty())
    {
      return;
    }
    size_t x_size = T.size();
    for (size_t i = 0; i < x_size; ++i)
    {
      events[i].time_of_flight = T[i];
    }
  }


  //------------------------------------------------------------------------------------------------
  /** Filter this EventList into an output EventList, using
   * keeping only events within the >= start and < end pulse times.
   * Detector IDs and the X axis are copied as well.
   *
   * @param start start time (absolute)
   * @param stop end time (absolute)
   * @param output reference to an event list that will be output.
   */
  void EventList::filterByPulseTime(PulseTimeType start, PulseTimeType stop, EventList & output) const
  {
    //Start by sorting the event list by pulse time.
    this->sortPulseTime();
    //Clear the output
    output.clear();
    //Copy the detector IDs
    output.detectorIDs = this->detectorIDs;
    output.refX = this->refX;

    //Iterate through all events (sorted by tof)
    std::vector<TofEvent>::iterator itev = this->events.begin();

    //Find the first event with pulse_time >= start
    while ((itev != this->events.end()) && (itev->pulse_time < start))
      itev++;

    while ((itev != this->events.end()) && (itev->pulse_time < stop))
    {
      //Copy the event into another
      const TofEvent eventCopy(*itev);
      //Add the copy to the output
      output.addEventQuickly(eventCopy);
      ++itev;
    }
  }


  //------------------------------------------------------------------------------------------------
  /** Split the event list into n outputs
   *
   * @param splitter a TimeSplitterType giving where to split
   * @param outputs a vector of where the split events will end up. The # of entries in there should
   *        be big enough to accommodate the indices.
   */
  void EventList::splitByTime(Kernel::TimeSplitterType splitter, std::vector< EventList * > outputs) const
  {
    //Start by sorting the event list by pulse time.
    this->sortPulseTime();

    //Initialize all the outputs
    int numOutputs = outputs.size();
    for (int i=0; i<numOutputs; i++)
    {
      outputs[i]->clear();
      outputs[i]->detectorIDs = this->detectorIDs;
      outputs[i]->refX = this->refX;
    }

    //Do nothing if there are no entries
    if (splitter.size() <= 0)
      return;

    //Iterate through all events (sorted by tof)
    std::vector<TofEvent>::iterator itev = this->events.begin();

    //And at the same time, iterate through the splitter
    Kernel::TimeSplitterType::iterator itspl = splitter.begin();

    PulseTimeType start, stop;
    int index;

    //This is the time of the first section. Anything before is thrown out.

    while (itspl != splitter.end())
    {
      //Get the splitting interval times and destination
      start = itspl->start();
      stop = itspl->stop();
      index = itspl->index();

      //Skip the events before the start of the time
      while ((itev != this->events.end()) && (itev->pulse_time < start))
        itev++;

      //Go through all the events that are in the interval (if any)
      while ((itev != this->events.end()) && (itev->pulse_time < stop))
      {
        //Copy the event into another
        const TofEvent eventCopy(*itev);
        if ((index >= 0) && (index < numOutputs))
        {
          EventList * myOutput = outputs[index];
          //Add the copy to the output
          myOutput->addEventQuickly(eventCopy);
        }
        ++itev;
      }

      //Go to the next interval
      itspl++;
      //But if we reached the end, then we are done.
      if (itspl==splitter.end())
        break;

      //No need to keep looping through the filter if we are out of events
      if (itev == this->events.end())
        break;

    } //Looping through entries in the splitter vector

    //Done!
  }


} /// namespace DataObjects
} /// namespace Mantid
