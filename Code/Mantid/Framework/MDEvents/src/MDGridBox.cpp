#include "MantidAPI/Progress.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadSchedulerMutexes.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDGridBox.h"
#include <ostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

//===============================================================================================
//===============================================================================================
  /** Task for adding events to a MDGridBox. */
  TMDE_CLASS
  class AddEventsTask : public Task
  {
  public:
    /// Pointer to MDGridBox.
    MDGridBox<MDE, nd> * box;
    /// Reference to the MD events that will be added
    const std::vector<MDE> & events;
    /// Where to start in vector
    size_t start_at;
    /// Where to stop in vector
    size_t stop_at;
    /// Progress report
    Progress * prog;

    /** Ctor
     *
     * @param box :: Pointer to MDGridBox
     * @param events :: Reference to the MD events that will be added
     * @param start_at :: Where to start in vector
     * @param stop_at :: Where to stop in vector
     * @return
     */
    AddEventsTask(MDGridBox<MDE, nd> * box, const std::vector<MDE> & events,
                  const size_t start_at, const size_t stop_at, Progress * prog)
    : Task(),
      box(box), events(events), start_at(start_at), stop_at(stop_at), prog(prog)
    {
    }

    /// Add the events in the MDGridBox.
    void run()
    {
      box->addEvents(events, start_at, stop_at);
      if (prog)
      {
        std::ostringstream out;
        out << "Adding events " << start_at;
        prog->report(out.str());
      }
    }
  };



  //===============================================================================================
  //===============================================================================================
  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param box :: MDBox containing the events to split */
  TMDE(MDGridBox)::MDGridBox(MDBox<MDE, nd> * box) :
    IMDBox<MDE, nd>(),
    nPoints(0)
  {
    if (!box)
      throw std::runtime_error("MDGridBox::ctor(): box is NULL.");
    BoxController_sptr bc = box->getBoxController();
    if (!bc)
      throw std::runtime_error("MDGridBox::ctor(): No BoxController specified in box.");
    // Save the controller in this object.
    this->m_BoxController = bc;

    // Copy the extents
    for (size_t d=0; d<nd; d++)
      extents[d] = box->getExtents(d);
    // Copy the depth
    this->m_depth = box->getDepth();

    // Do some computation based on how many splits per each dim.
    size_t tot = 1;
    for (size_t d=0; d<nd; d++)
    {
      // Cumulative multiplier, for indexing
      splitCumul[d] = tot;
      // How many is it split?
      split[d] = bc->splitInto(d);
      tot *= split[d];
      // Length of the side of a box in this dimension
      boxSize[d] = (extents[d].max - extents[d].min) / split[d];
    }

    if (tot == 0)
      throw std::runtime_error("MDGridBox::ctor(): Invalid splitting criterion (one was zero).");

    // Create the array of MDBox contents.
    boxes.clear();
    boxes.reserve(tot);
    numBoxes = tot;

    size_t indices[nd];
    for (size_t d=0; d<nd; d++) indices[d] = 0;
    for (size_t i=0; i<tot; i++)
    {
      // Create the box
      // (Increase the depth of this box to one more than the parent (this))
      MDBox<MDE,nd> * myBox = new MDBox<MDE,nd>(bc, this->m_depth + 1);
      // Set the extents of this box.
      for (size_t d=0; d<nd; d++)
      {
        CoordType min = extents[d].min + boxSize[d] * indices[d];
        myBox->setExtents(d, min, min + boxSize[d]);
      }
      boxes.push_back(myBox);

      // Increment the indices, rolling back as needed
      indices[0]++;
      for (size_t d=0; d<nd-1; d++) //This is not run if nd=1; that's okay, you can ignore the warning
      {
        if (indices[d] >= split[d])
        {
          indices[d] = 0;
          indices[d+1]++;
        }
      }
    } // for each box

    // Now distribute the events that were in the box before
    this->addEvents(box->getEvents());
    // Copy the cached numbers from the incoming box. This is most efficient
    this->nPoints = box->getNPoints();
    this->m_signal = box->getSignal();
    this->m_errorSquared = box->getErrorSquared();
  }



  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDGridBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDGridBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(size_t MDGridBox)::getNPoints() const
  {
    //Use the cached value
    return nPoints;
  }


  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache of nPoints, signal and error,
   * by adding up all boxes (recursively).
   * MDBoxes' totals are used directly.
   */
  TMDE(
  void MDGridBox)::refreshCache()
  {
    // Clear your total
    nPoints = 0;
    this->m_signal = 0;
    this->m_errorSquared = 0;

    typename boxVector_t::iterator it;
    for (it = boxes.begin(); it != boxes.end(); it++)
    {
      IMDBox<MDE,nd> * ibox = *it;
      MDGridBox<MDE,nd> * gbox = dynamic_cast<MDGridBox<MDE,nd> *>(ibox);

      // Refresh the cache of the contents. Recursive.
      if (gbox)
        gbox->refreshCache();

      // Add up what's in there
      nPoints += ibox->getNPoints();
      this->m_signal += ibox->getSignal();
      this->m_errorSquared += ibox->getErrorSquared();
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDGridBox)::getEventsCopy()
  {
    std::vector< MDE > * out = new std::vector<MDE>();
    //Make the copy
    //out->insert(out->begin(), data.begin(), data.end());
    return out;
  }


  //-----------------------------------------------------------------------------------------------
  /** Split a box that is contained in the GridBox, at the given index,
   * into a MDGridBox.
   *
   * @param index :: index into the boxes vector.
   *        Warning: No bounds check is made, don't give stupid values!
   */
  TMDE(
  void MDGridBox)::splitContents(size_t index)
  {
    // You can only split it if it is a MDBox (not MDGridBox).
    MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>(boxes[index]);
    if (!box) return;
    // Construct the grid box
    MDGridBox<MDE, nd> * gridbox = new MDGridBox<MDE, nd>(box);
    // Delete the old ungridded box
    delete boxes[index];
    // And now we have a gridded box instead of a boring old regular box.
    boxes[index] = gridbox;
  }

  //-----------------------------------------------------------------------------------------------
  /** Goes through all the sub-boxes and splits them if they contain
   * enough events to be worth it.
   *
   * @param index :: index into the boxes vector.
   *        Warning: No bounds check is made, don't give stupid values!
   */
  TMDE(
  void MDGridBox)::splitAllIfNeeded()
  {
    for (size_t i=0; i < numBoxes; ++i)
    {
      MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
      if (box)
      {
        // Plain MD-Box. Does it need to split?
        if (this->m_BoxController->willSplit(box->getNPoints(), box->getDepth() ))
        {
          //std::cout << "Splitting box at depth " << box->getDepth() << "\n";
          // The MDBox needs to split into a grid box.
          MDGridBox<MDE, nd> * gridBox = new MDGridBox<MDE, nd>(box);
          // Replace in the array
          boxes[i] = gridBox;
          // Delete the old box
          delete box;
          // Now recursively check if this NEW grid box's contents should be split too
          gridBox->splitAllIfNeeded();
        }
      }
      else
      {
        // It should be a MDGridBox
        MDGridBox<MDE, nd> * gridBox = dynamic_cast<MDGridBox<MDE, nd>*>(boxes[i]);
        if (gridBox)
        {
          // Now recursively check if this old grid box's contents should be split too
          gridBox->splitAllIfNeeded();
        }
      }
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a single MDEvent to the grid box. If the boxes
   * contained within are also gridded, this will recursively push the event
   * down to the deepest level.
   * Warning! No bounds checking is done (for performance). It must
   * be known that the event is within the bounds of the grid box before adding.
   *
   * Note! nPoints, signal and error must be re-calculated using refreshCache()
   * after all events have been added.
   *
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  inline void MDGridBox)::addEvent( const MDE & event)
  {
    size_t index = 0;
    for (size_t d=0; d<nd; d++)
    {
      CoordType x = event.getCenter(d);
      int i = int((x - extents[d].min) / boxSize[d]);
      // NOTE: No bounds checking is done (for performance).
      //if (i < 0 || i >= int(split[d])) return;

      // Accumulate the index
      index += (i * splitCumul[d]);
    }

//    // Keep the running total of signal, error, and # of points
//    if (trackTotals)
//    {
//      statsMutex.lock();
//      this->m_signal += event.getSignal();
//      this->m_errorSquared += event.getErrorSquared();
//      ++this->nPoints;
//      statsMutex.unlock();
//    }

    // Add it to the contained box, and return how many bads where there
    boxes[index]->addEvent(event);
  }



  //-----------------------------------------------------------------------------------------------
  /** Add several events.
   *
   * @param events :: vector of events to be copied.
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDGridBox)::addEvents(const std::vector<MDE> & events)
  {
    return this->addEvents(events, 0, events.size());
  }

  //-----------------------------------------------------------------------------------------------
  /** Add several events, starting and stopping at particular point in a vector.
   *
   * NOTE: You must call refreshCache() after you are done, to calculate the
   *  nPoints, signal and error.
   *
   * @param events :: vector of events to be copied.
   * @param start_at :: begin at this index in the array
   * @param stop_at :: stop at this index in the array
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDGridBox)::addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at)
  {
    size_t numBad = 0;
    // --- Go event by event and add them ----
    typename std::vector<MDE>::const_iterator it = events.begin() + start_at;
    typename std::vector<MDE>::const_iterator it_end = events.begin() + stop_at;
    for (; it != it_end; it++)
    {
      //Check out-of-bounds-ness
      bool badEvent = false;
      for (size_t d=0; d<nd; d++)
      {
        double x = it->getCenter(d);
        if ((x < extents[d].min) || (x >= extents[d].max))
        {
          badEvent = true;
          break;
        }
      }

      if (badEvent)
        // Event was out of bounds. Count it
        ++numBad;
      else
        // Event was in bounds; add it
        addEvent(*it);
    }

    return numBad;
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a large number of events to this MDGridBox.
   * This will use a ThreadPool/OpenMP to allocate events in parallel. Therefore, it should
   * only be called on the highest level MDGridBox in a MDEventWorkspace.
   *
   * @param events :: vector of events to be copied.
   * @param prog :: optional Progress object to report progress back to GUI/algorithms.
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDGridBox)::addManyEvents(const std::vector<MDE> & events, Mantid::API::Progress * prog)
  {
    // Get some parameters that should optimize task allocation.
    size_t eventsPerTask, numTasksPerBlock;
    this->m_BoxController->getAddingEventsParameters(eventsPerTask, numTasksPerBlock);

    // Set up progress report, if any
    if (prog)
    {
      size_t numTasks = events.size()/eventsPerTask;
      prog->setNumSteps( numTasks + numTasks/numTasksPerBlock);
    }

    // Where we are in the list of events
    size_t event_index = 0;
    while (event_index < events.size())
    {
      //Since the costs are not known ahead of time, use a simple FIFO buffer.
      ThreadScheduler * ts = new ThreadSchedulerFIFO();
      // Create the threadpool
      ThreadPool tp(ts, 1);

      // Do 'numTasksPerBlock' tasks with 'eventsPerTask' events in each one.
      for (size_t i = 0; i < numTasksPerBlock; i++)
      {
        // Calculate where to start and stop in the events vector
        bool breakout = false;
        size_t start_at = event_index;
        event_index += eventsPerTask;
        size_t stop_at = event_index;
        if (stop_at >= events.size())
        {
          stop_at = events.size();
          breakout = true;
        }

        // Create a task and push it into the scheduler
        //std::cout << "Making a AddEventsTask " << start_at << " to " << stop_at << std::endl;
        ts->push( new AddEventsTask<MDE,nd>(this, events, start_at, stop_at, prog) );

        if (breakout) break;
      }

      // Finish all threads.
//      std::cout << "Starting block ending at index " << event_index << " of " << events.size() << std::endl;
      Timer tim;
      tp.joinAll();
//      std::cout << "... block took " << tim.elapsed() << " secs.\n";

      //Now, shake out all the sub boxes and split those if needed
//      std::cout << "Starting splitAllIfNeeded().\n";
      if (prog) prog->report("Splitting MDBox'es.");
      this->splitAllIfNeeded();
//      std::cout << "... splitAllIfNeeded() took " << tim.elapsed() << " secs.\n";
    }

    // Refresh the counts, now that we are all done.
    this->refreshCache();

    // Done!
    return 0;
  }



}//namespace MDEvents

}//namespace Mantid

