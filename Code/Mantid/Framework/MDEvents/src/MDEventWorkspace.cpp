#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ProgressBase.h"

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
  /** Split the contained MDBox into a MDGridBox, if it is not
   * that already.
   */
  TMDE(
  void MDEventWorkspace)::splitBox()
  {
    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
    if (!gridBox)
    {
      // Track how many MDBoxes there are in the overall workspace
      this->m_BoxController->trackNumBoxes(data->getDepth());

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
      throw std::runtime_error("MDEventWorkspace::splitAllIfNeeded called on a MDBox (call splitBox() first).");
    gridBox->splitAllIfNeeded(ts);
  }
  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache of # of points, signal, and error */
  TMDE(
  void MDEventWorkspace)::refreshCache()
  {
    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
    if (!gridBox)
    {
      // Cache only makes sense on a grid box. Just split it right now.
      splitBox();
      gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
    }
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



  // =============================================================================================
  /** Class for adding up events into a single bin.
   * Multitudes of these will be created by binToMDHistoWorkspace() to do a
   * dense histogram bin.
   */
  TMDE_CLASS
  class MDEventWorkspaceCenterpointBinTask : public Task
  {
  public:
    /** Constructor
     *
     * @param inBox :: pointer to the MDBox containing MDEvents
     * @param outWS :: pointer to the MDHistoWorkspace containing the result
     * @param bin :: MDBin object describing the bounds to bin into.
     * @return
     */
    MDEventWorkspaceCenterpointBinTask(IMDBox<MDE,nd> * inBox,
        MDHistoWorkspace_sptr outWS, MDBin<MDE,nd> bin) :
      Task(), inBox(inBox), outWS(outWS), bin(bin)
    {
    }

    /** Run the binning and store the result */
    void run()
    {
      // This will recursively bin into the sub grids
      inBox->centerpointBin(bin);
      // Save the data into the dense histogram
      outWS->setSignalAt(bin.m_index, bin.m_signal);
      outWS->setErrorAt(bin.m_index, bin.m_errorSquared);
    }

  private:
    /// pointer to the MDBox containing MDEvents
    IMDBox<MDE,nd> * inBox;
    /// pointer to the MDHistoWorkspace containing the result
    MDHistoWorkspace_sptr outWS;
    /// MDBin object describing the bounds to bin into.
    MDBin<MDE,nd> bin;
  };


  //-----------------------------------------------------------------------------------------------
  /** Bin a MDEventWorkspace into a dense histogram in a MDHistoWorkspace, using the MDBox's
   * centerpointBin routine.
   * You must give 4 dimensions, with names matching those in the MDEventWorkspace.
   *
   * @param dimX :: X dimension binning parameters
   * @param dimY :: Y dimension binning parameters
   * @param dimZ :: Z dimension binning parameters
   * @param dimT :: T (time) dimension binning parameters
   * @param prog :: Progress reporter.
   * @return a shared ptr to MDHistoWorkspace created.
   */
  TMDE(
  IMDWorkspace_sptr MDEventWorkspace)::centerpointBinToMDHistoWorkspace(Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY,
      Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::Geometry::MDHistoDimension_sptr dimT, Mantid::Kernel::ProgressBase * prog) const
  {
    bool DODEBUG = false;
    Timer tim;

    // Create the dense histogram. This allocates the memory
    MDHistoWorkspace_sptr ws(new MDHistoWorkspace(dimX, dimY, dimZ, dimT));

    if (DODEBUG) std::cout << tim.elapsed() << " sec to create the MDHistoWorkspace.\n";

    // Make it into a vector of dimensions to which to bin.
    std::vector<MDHistoDimension_sptr> binDimensionsIn;
    binDimensionsIn.push_back(dimX);
    binDimensionsIn.push_back(dimY);
    binDimensionsIn.push_back(dimZ);
    binDimensionsIn.push_back(dimT);

    // Thin it down for invalid dimensions
    std::vector<MDHistoDimension_sptr> binDimensions;
    std::vector<size_t> dimensionToBinFrom;
    for (size_t i = 0; i < binDimensionsIn.size(); ++i)
    {
      try {
        size_t dim_index = this->getDimensionIndexByName(binDimensionsIn[i]->getName());
        dimensionToBinFrom.push_back(dim_index);
        binDimensions.push_back(binDimensionsIn[i]);
      }
      catch (std::runtime_error & e)
      {
        // The dimension was not found, so we are not binning across it. TODO: Log message?
        if (binDimensionsIn[i]->getNBins() > 1)
            throw std::runtime_error("Dimension " + binDimensionsIn[i]->getName() + " was not found in the MDEventWorkspace and has more than one bin! Cannot continue.");
      }
    }
    // Number of input binning dimensions found
    size_t numBD = binDimensions.size();

    // Cache a calculation to convert indices x,y,z,t into a linear index.
    std::vector<size_t> linear_index_maker(numBD,1);
    for (size_t i=1; i < numBD; i++)
      linear_index_maker[i] = linear_index_maker[i-1]*binDimensions[i]->getNBins();

    if (DODEBUG) std::cout << tim.elapsed() << " sec to cache the binning results.\n";

    //Since the costs are not known ahead of time, use a simple FIFO buffer.
    ThreadScheduler * ts = new ThreadSchedulerFIFO();
    // Create the threadpool with: all CPUs, a progress reporter
    ThreadPool tp(ts, 0, prog);
    // Start the threadpool, and allow the threads to wait up to 1.0 seconds for tasks to come in.
    //tp.start(1.0);

    // For progress reporting, the # of tasks = the number of bins in the output workspace
    if (prog)
      prog->setNumSteps( ws->getNPoints() );

    // Now we need to do a nested loop in the N dimensions across which we bin

    // Index of the loop in each binning dimension, starting at 0.
    size_t * index = Utils::nestedForLoopSetUp(numBD);
    // This is the limit to loop over in each dimension
    size_t * index_max = Utils::nestedForLoopSetUp(numBD);
    for (size_t bd=0; bd<numBD; bd++) index_max[bd] = binDimensions[bd]->getNBins();

    // --- Unrolled Nested Loop ----
    bool allDone = false;
    while (!allDone)
    {
      // --- Create a MDBin object for this bit ---
      // (this index is where it lands in the MDHistoWorkspace.
      size_t linear_index = 0;
      MDBin<MDE,nd> bin;
      for (size_t bd=0; bd<numBD; bd++)
      {
        // Index in this binning dimension (i_x, i_y, etc.)
        size_t idx = index[bd];
        // Dimension in the MDEventWorkspace
        size_t d = dimensionToBinFrom[bd];
        // Corresponding extents
        bin.m_min[d] = binDimensions[bd]->getX(idx);
        bin.m_max[d] = binDimensions[bd]->getX(idx+1);
        // Count the linear_index
        linear_index += linear_index_maker[bd] * idx;
      }
      bin.m_index = linear_index;

      // Create the task and schedule it
      ts->push(  new MDEventWorkspaceCenterpointBinTask<MDE,nd>(data, ws, bin) );

      // --- Increment index in each dimension -----
      allDone = Utils::nestedForLoopIncrement(numBD, index, index_max);

    } // While !alldone

    if (DODEBUG) std::cout << tim.elapsed() << " sec to fill up the ThreadPool with all the tasks\n";

    // OK, all that was just to fill the ThreadScheduler
    // So now we actually run these.
    tp.joinAll();

    if (DODEBUG) std::cout << tim.elapsed() << " sec to run all the tasks\n";

    delete index_max;
    delete index;
    return ws;
  }


}//namespace MDEvents

}//namespace Mantid

