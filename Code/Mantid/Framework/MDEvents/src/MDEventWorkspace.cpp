#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

//-----------------------------------------------------------------------------------------------
  /** Default constructor
   */
  TMDE(
  MDEventWorkspace)::MDEventWorkspace()
  {
    data = new MDBox<MDE, nd>();
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
      data->setExtents(d, dimensions[d].getMin(), dimensions[d].getMax());
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
  /** Set the box controller that the contained GridBoxes will use
   *
   * @param controller :: BoxController_sptr
   */
  TMDE(
  void MDEventWorkspace)::setBoxController(BoxController_sptr controller)
  {
    m_BoxController = controller;
    data->setBoxController(m_BoxController);
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the number of bytes of memory
   * used by the workspace. */
  TMDE(
  size_t MDEventWorkspace)::getMemorySize() const
  {
    return this->getNPoints() * sizeof(MDE);
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
  /** Split the contained MDBox into a MDGridBox, if it is not
   * that already.
   */
  TMDE(
  void MDEventWorkspace)::splitBox()
  {
    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
    if (!gridBox)
    {
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(data);
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
    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
    if (!gridBox)
      throw std::runtime_error("MDEventWorkspace::splitAllIfNeeded called on a "
          "MDBox (call splitBox() first).");
    gridBox->splitAllIfNeeded(ts);
  }
  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache of # of points, signal, and error */
  TMDE(
  void MDEventWorkspace)::refreshCache()
  {
    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
    if (!gridBox)
      throw std::runtime_error("MDEventWorkspace::refreshCache called on a MDBox (call splitBox() first).");
    gridBox->refreshCache();
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
  void MDEventWorkspace)::addManyEvents(const std::vector<MDE> & events, Mantid::API::Progress * prog)
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
      prog->setNumSteps( numTasks + numTasks/numTasksPerBlock);
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
    gridBox->refreshCache();

  }



}//namespace MDEvents

}//namespace Mantid

