#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDSplitBox.h"
#include <iomanip>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDEvents
{

  //-----------------------------------------------------------------------------------------------
  /** Default constructor
   */
  TMDE(
  MDEventWorkspace)::MDEventWorkspace()
  : m_BoxController(new BoxController(nd))
  {
    // First box is at depth 0, and has this default boxController
    data = new MDBox<MDE, nd>(m_BoxController, 0);
  }

  //-----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(
  MDEventWorkspace)::~MDEventWorkspace()
  {
    delete data;
  }


  //-----------------------------------------------------------------------------------------------
  /** Perform initialization after dimensions (and others) have been set.
   * This sets the size of the box.
   */
  TMDE(
  void MDEventWorkspace)::initialize()
  {
    if (dimensions.size() != nd)
      throw std::runtime_error("MDEventWorkspace::initialize() called with an incorrect number of dimensions set. Use addDimension() first to add the right number of dimension info objects.");
    if (isGridBox())
        throw std::runtime_error("MDEventWorkspace::initialize() called on a MDEventWorkspace containing a MDGridBox. You should call initialize() before adding any events!");
    for (size_t d=0; d<nd; d++)
      data->setExtents(d, dimensions[d]->getMinimum(), dimensions[d]->getMaximum());
  }

  //-----------------------------------------------------------------------------------------------
  /** Get the data type (id) of the workspace */
  TMDE(
  const std::string MDEventWorkspace)::id() const
  {
    std::ostringstream out;
    out << "MDEventWorkspace<MDEvent," << getNumDims() << ">";
    return out.str();
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this workspace */
  TMDE(
  size_t MDEventWorkspace)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this workspace */
  TMDE(
  size_t MDEventWorkspace)::getNPoints() const
  {
    return data->getNPoints();
  }


  //-----------------------------------------------------------------------------------------------
  /** Get a vector of the minimum extents that still contain all the events in the workspace.
   *
   * @param depth :: recursion depth to which to search. This will determine the resolution
   *        to which the extents will be found.
   * @return a vector of the minimum extents that still contain all the events in the workspace.
   */
  TMDE(
  std::vector<Mantid::Geometry::MDDimensionExtents> MDEventWorkspace)::getMinimumExtents(size_t depth)
  {
    std::vector<Mantid::Geometry::MDDimensionExtents> out(nd);
    std::vector<IMDBox<MDE,nd>*> boxes;
    // Get all the end (leaf) boxes
    this->data->getBoxes(boxes, depth, true);
    typename std::vector<IMDBox<MDE,nd>*>::iterator it;
    typename std::vector<IMDBox<MDE,nd>*>::iterator it_end = boxes.end();
    for (it = boxes.begin(); it != boxes.end(); it++)
    {
      IMDBox<MDE,nd>* box = *it;
      if (box->getSignal() > 0)
      {
        for (size_t d=0; d<nd; d++)
        {
          Mantid::Geometry::MDDimensionExtents & x = box->getExtents(d);
          if (x.max > out[d].max) out[d].max = x.max;
          if (x.min < out[d].min) out[d].min = x.min;
        }
      }
    }
    return out;
  }


  //-----------------------------------------------------------------------------------------------
  /// Returns some information about the box controller, to be displayed in the GUI, for example
  TMDE(
  std::vector<std::string> MDEventWorkspace)::getBoxControllerStats() const
  {
    std::vector<std::string> out;
    std::ostringstream mess;
    size_t mem;
    mem = (this->m_BoxController->getTotalNumMDBoxes() * sizeof(MDBox<MDE,nd>)) / 1024;
    mess << m_BoxController->getTotalNumMDBoxes() << " MDBoxes (" << mem << " kB)";
    out.push_back(mess.str()); mess.str("");

    mem = (this->m_BoxController->getTotalNumMDGridBoxes() * sizeof(MDGridBox<MDE,nd>)) / 1024;
    mess << m_BoxController->getTotalNumMDGridBoxes() << " MDGridBoxes (" << mem << " kB)";
    out.push_back(mess.str()); mess.str("");

    mess << "Avg recursion depth: " << m_BoxController->getAverageDepth();
    out.push_back(mess.str()); mess.str("");

    mess << "Recursion Coverage %: ";
    const std::vector<size_t> & num = m_BoxController->getNumMDBoxes();
    const std::vector<double> & max = m_BoxController->getMaxNumMDBoxes();
    for (size_t i=0; i<num.size(); i++)
    {
      if (i > 0) mess << ", ";
      double pct = (double(num[i]) / double(max[i] * 100));
      if (pct > 0 && pct < 1e-2) mess << std::scientific; else mess << std::fixed;
      mess << std::setprecision(2) << pct;
    }
    out.push_back(mess.str()); mess.str("");
    return out;
  }

  //-----------------------------------------------------------------------------------------------
  /** @returns the number of bytes of memory used by the workspace. */
  TMDE(
  size_t MDEventWorkspace)::getMemorySize() const
  {
//    std::cout << "sizeof(MDE) " << sizeof(MDE) << std::endl;
//    std::cout << "sizeof(MDBox<MDE,nd>) " << sizeof(MDBox<MDE,nd>) << std::endl;
//    std::cout << "sizeof(MDGridBox<MDE,nd>) " << sizeof(MDGridBox<MDE,nd>) << std::endl;
    // Add up the events and the MDBoxes contained.
    size_t total = this->getNPoints() * sizeof(MDE);
    total += this->m_BoxController->getTotalNumMDBoxes() * sizeof(MDBox<MDE,nd>);
    total += this->m_BoxController->getTotalNumMDGridBoxes() * sizeof(MDGridBox<MDE,nd>);
    return total;
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a single event to this workspace. Automatic splitting is not performed after adding
   * (call splitAllIfNeeded).
   *
   * @param event :: event to add.
   */
  TMDE(
  void MDEventWorkspace)::addEvent(const MDE & event)
  {
    data->addEvent(event);
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a vector of MDEvents to the workspace.
   *
   * @param events :: const ref. to a vector of events; they will be copied into the
   *        MDBox'es contained within.
   */
  TMDE(
  void MDEventWorkspace)::addEvents(const std::vector<MDE> & events)
  {
    data->addEvents(events);
  }

  //-----------------------------------------------------------------------------------------------
  /** Split the contained MDBox into a MDGridBox or MDSplitBox, if it is not
   * that already.
   */
  TMDE(
  void MDEventWorkspace)::splitBox()
  {
    // Want MDGridBox
    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
    if (!gridBox)
    {
      // Track how many MDBoxes there are in the overall workspace
      this->m_BoxController->trackNumBoxes(data->getDepth());
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(data);
      if (!box) throw
          std::runtime_error("MDEventWorkspace::splitBox() expected its data to be a MDBox* to split to MDGridBox.");
      gridBox = new MDGridBox<MDE,nd>(box);
      data = gridBox;
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Goes through all the sub-boxes and splits them if they contain
   * enough events to be worth it.
   *
   * @param ts :: optional ThreadScheduler * that will be used to parallelize
   *        recursive splitting. Set to NULL to do it serially. */
  TMDE(
  void MDEventWorkspace)::splitAllIfNeeded(Kernel::ThreadScheduler * ts)
  {
    data->splitAllIfNeeded(ts);
  }

  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache of # of points, signal, and error.
   * NOTE: This is performed in parallel using a threadpool.
   *  */
  TMDE(
  void MDEventWorkspace)::refreshCache()
  {
    // Function is overloaded and recursive; will check all sub-boxes
    data->refreshCache();
    //TODO ThreadPool
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a large number of events to this MDEventWorkspace.
   * This will use a ThreadPool/OpenMP to allocate events in parallel.
   *
   * @param events :: vector of events to be copied.
   * @param prog :: optional Progress object to report progress back to GUI/algorithms.
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  void MDEventWorkspace)::addManyEvents(const std::vector<MDE> & events, Mantid::Kernel::ProgressBase * prog)
  {
    // Always split the MDBox into a grid box
    this->splitBox();
    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);

    // Get some parameters that should optimize task allocation.
    size_t eventsPerTask, numTasksPerBlock;
    this->m_BoxController->getAddingEventsParameters(eventsPerTask, numTasksPerBlock);

    // Set up progress report, if any
    if (prog)
    {
      size_t numTasks = events.size()/eventsPerTask;
      prog->setNumSteps( int( numTasks + numTasks/numTasksPerBlock ));
    }

    // Where we are in the list of events
    size_t event_index = 0;
    while (event_index < events.size())
    {
      //Since the costs are not known ahead of time, use a simple FIFO buffer.
      ThreadScheduler * ts = new ThreadSchedulerFIFO();
      // Create the threadpool
      ThreadPool tp(ts);

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
        typename MDGridBox<MDE,nd>::AddEventsTask * task;
        task = new typename MDGridBox<MDE,nd>::AddEventsTask(gridBox, events, start_at, stop_at, prog) ;
        ts->push( task );

        if (breakout) break;
      }

      // Finish all threads.
//      std::cout << "Starting block ending at index " << event_index << " of " << events.size() << std::endl;
      Timer tim;
      tp.joinAll();
//      std::cout << "... block took " << tim.elapsed() << " secs.\n";

      //Create a threadpool for splitting.
      ThreadScheduler * ts_splitter = new ThreadSchedulerFIFO();
      ThreadPool tp_splitter(ts_splitter);

      //Now, shake out all the sub boxes and split those if needed
//      std::cout << "\nStarting splitAllIfNeeded().\n";
      if (prog) prog->report("Splitting MDBox'es.");

      gridBox->splitAllIfNeeded(ts_splitter);
      tp_splitter.joinAll();
//      std::cout << "\n... splitAllIfNeeded() took " << tim.elapsed() << " secs.\n";
    }

    // Refresh the counts, now that we are all done.
    this->refreshCache();
  }




}//namespace MDEvents

}//namespace Mantid

