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
  /** Performs template argument deduction and uses local call to fabricate an adapter instance of Point3D. Templated function may have a more suitable location.
   */
  template <typename AdapteeType>
  Mantid::API::Point3D* makePoint3D(const AdapteeType& adaptee)
  {

    //Local adapter.
    class PointAdapter : public Mantid::API::Point3D
    {
    private:
      AdapteeType m_adaptee;
    public:
      PointAdapter(const AdapteeType& adaptee) : m_adaptee(adaptee)
      {
      }
      virtual double getX() const 
      {
        return m_adaptee.m_min[0] + ((m_adaptee.m_max[0] - m_adaptee.m_min[0]) /2);
      }
      virtual double getY() const
      {
        return m_adaptee.m_min[1] + ((m_adaptee.m_max[1] - m_adaptee.m_min[1]) /2);
      }
      virtual double getZ() const
      {
        return m_adaptee.m_min[2] + ((m_adaptee.m_max[2] - m_adaptee.m_min[2]) /2);
      }
    };
    return new PointAdapter(adaptee);
  }

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
  /** Returns the number of bytes of memory
   * used by the workspace. */
  TMDE(
  size_t MDEventWorkspace)::getMemorySize() const
  {
//    std::cout << "sizeof(MDE) " << sizeof(MDE) << std::endl;
//    std::cout << "sizeof(MDBox<MDE,nd>) " << sizeof(MDBox<MDE,nd>) << std::endl;
//    std::cout << "sizeof(MDGridBox<MDE,nd>) " << sizeof(MDGridBox<MDE,nd>) << std::endl;
    // Add up the events and the MDBoxes contained.
    return this->getNPoints() * sizeof(MDE) +
        this->m_BoxController->getTotalNumMDBoxes() * sizeof(MDBox<MDE,nd>) +
        this->m_BoxController->getTotalNumMDGridBoxes() * sizeof(MDGridBox<MDE,nd>);
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
    if (this->m_BoxController->getBinarySplit())
    {
      // Want to split into MDSplitBox
      MDSplitBox<MDE,nd> * splitBox = dynamic_cast<MDSplitBox<MDE,nd> *>(data);
      if (!splitBox)
      {
        // Track how many MDBoxes there are in the overall workspace
        this->m_BoxController->trackNumBoxes(data->getDepth());
        MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(data);
        if (!box) throw
            std::runtime_error("MDEventWorkspace::splitBox() expected its data to be a MDBox* to split to MDSplitBox.");
        splitBox = new MDSplitBox<MDE,nd>(box);
        data = splitBox;
      }
    }
    else
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





  //-----------------------------------------------------------------------------------------------
  /** Bin a MDEventWorkspace into a dense histogram in a MDHistoWorkspace, using the MDBox's
   * centerpointBin routine.
   * You must give 4 dimensions, with names matching those in the MDEventWorkspace.
   *
   * @param dimX :: X dimension binning parameters
   * @param dimY :: Y dimension binning parameters
   * @param dimZ :: Z dimension binning parameters
   * @param dimT :: T (time) dimension binning parameters
   * @param implicitFunction :: a ImplicitFunction defining which bins to calculate. NULL if all bins will be calculated.
   * @param prog :: Progress reporter, can be NULL.
   * @return a shared ptr to MDHistoWorkspace created.
   */
  TMDE(
  IMDWorkspace_sptr MDEventWorkspace)::centerpointBinToMDHistoWorkspace(Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY,
      Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::Geometry::MDHistoDimension_sptr dimT,
      Mantid::API::ImplicitFunction * implicitFunction, Mantid::Kernel::ProgressBase * prog) const
  {
    bool DODEBUG = true;
    CPUTimer tim;

    // Create the dense histogram. This allocates the memory
    MDHistoWorkspace_sptr ws(new MDHistoWorkspace(dimX, dimY, dimZ, dimT));

    if (DODEBUG) std::cout << tim << " to create the MDHistoWorkspace.\n";

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
      if (binDimensionsIn[i]->getNBins() == 0)
        throw std::runtime_error("Dimension " + binDimensionsIn[i]->getName() + " was set to have 0 bins. Cannot continue.");

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
    // Number of output binning dimensions found
    size_t numBD = binDimensions.size();

    if (numBD == 0)
      throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");

    if (DODEBUG) std::cout << tim << " to cache the binning results.\n";

    //Since the costs are not known ahead of time, use a simple FIFO buffer.
    ThreadScheduler * ts = new ThreadSchedulerFIFO();

    // Create the threadpool with: all CPUs, a progress reporter
    ThreadPool tp(ts, 0, prog);

    // Big efficiency gain is obtained by grouping a few bins per task.
    size_t binsPerTask = 100;

    // For progress reporting, the approx  # of tasks
    if (prog)
      prog->setNumSteps( int(ws->getNPoints() / binsPerTask) );

    // This is the limit to loop over in each dimension
    size_t * index_max = Utils::nestedForLoopSetUp(numBD);
    for (size_t bd=0; bd<numBD; bd++) index_max[bd] = binDimensions[bd]->getNBins();
    // Cache a calculation to convert indices x,y,z,t into a linear index.
    size_t * index_maker = Utils::nestedForLoopSetUpIndexMaker(numBD, index_max);

    int numPoints = int(ws->getNPoints());
    // Run in OpenMP with dynamic scheduling and a smallish chunk size (binsPerTask)
    PRAGMA_OMP(parallel for schedule(dynamic, binsPerTask))
    for (int i=0; i < numPoints; i++)
    {
      size_t linear_index = size_t(i);
      // nd >= numBD in all cases so this is safe.
      size_t index[nd];

      // Get the index at each dimension for this bin.
      Utils::nestedForLoopGetIndicesFromLinearIndex(numBD, linear_index, index_maker, index_max, index);

      // Construct the bin and its coordinates
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
      }
      bin.m_index = linear_index;

      // Check if the bin is in the ImplicitFunction (if any)
      bool binContained = true;
      if (implicitFunction)
      {
        binContained = implicitFunction->evaluate(makePoint3D(bin));
      }

      if (binContained)
      {
        // Array of bools set to true when a dimension is fully contained (binary splitting only)
        bool fullyContained[nd];
        for (size_t d=0; d<nd; d++)
          fullyContained[d] = false;

        // This will recursively bin into the sub grids
        data->centerpointBin(bin, fullyContained);

        // Save the data into the dense histogram
        ws->setSignalAt(linear_index, bin.m_signal);
        ws->setErrorAt(linear_index, bin.m_errorSquared);
      }

    } // (for each linear index)


    if (DODEBUG) std::cout << tim << " to fill up the ThreadPool with all the tasks\n";

    // OK, all that was just to fill the ThreadScheduler
    // So now we actually run these.
    tp.joinAll();

    if (DODEBUG) std::cout << tim << " to run all the tasks\n";

    delete index_max;
    delete index_maker;
    return ws;
  }






//  // =============================================================================================
//  /** Task for creating MDBin's and integrating them.
//   * Multitudes of these will be created by binToMDHistoWorkspace() to do a
//   * dense histogram bin.
//   */
//  TMDE_CLASS
//  class MDEventWorkspaceCenterpointBinTask : public Task
//  {
//  public:
//    /** Constructor
//     *
//     * @param inBox :: pointer to the INPUT MDBox containing MDEvents
//     * @param outWS :: pointer to the OUTPUT MDHistoWorkspace containing the result
//     * @param binDimensions :: vector of MDHistoDimensions giving the dimension
//     *        of the OUTPUT MDHistoWorkspace.
//     * @param dimensionToBinFrom :: vector where dimensionToBinFrom[n] =
//     *        the dimension in the INPUT MDEventWorkspace that goes to the nth dimension in the OUTPUT MDHistoWorkspace.
//     * @param linearIndexStart :: Start generating bins at this linear index of the output workspace.
//     * @param linearIndexStop :: Stop generating bins before this linear index.
//     * @param index_max :: array of the maximum index in each OUTPUT dimension.
//     * @param index_maker :: linear index maker array (from Utils::nestedForLoop)
//     * @param implicitFunction :: limiting implicit function, can be NULL
//     */
//    MDEventWorkspaceCenterpointBinTask(IMDBox<MDE,nd> * inBox,
//        MDHistoWorkspace_sptr outWS,
//        const std::vector<MDHistoDimension_sptr> & binDimensions,
//        const std::vector<size_t> & dimensionToBinFrom,
//        const size_t linearIndexStart, const size_t linearIndexStop,
//        const size_t * index_max,
//        const size_t * index_maker,
//        const Mantid::API::ImplicitFunction * implicitFunction)
//    :
//      Task(), inBox(inBox), outWS(outWS),
//      binDimensions(binDimensions),
//      dimensionToBinFrom(dimensionToBinFrom),
//      linearIndexStart(linearIndexStart),
//      linearIndexStop(linearIndexStop),
//      index_max(index_max),
//      index_maker(index_maker),
//      implicitFunction(implicitFunction)
//    {
//    }
//
//    /** - Make a certain number of MDBin's
//     *  - Run the centerpointBinning.
//     *  - Store the result in the output workspace. */
//    void run()
//    {
//      // Number of output binning dimensions found
//      size_t numBD = binDimensions.size();
//      // Counter in each dimension
//      size_t * index = Utils::nestedForLoopSetUp(numBD);
//
//      // Do each bin in this task
//      for (size_t linear_index=linearIndexStart; linear_index<linearIndexStop; linear_index++)
//      {
//        // Get the index at each dimension for this bin.
//        Utils::nestedForLoopGetIndicesFromLinearIndex(numBD, linear_index, index_maker, index_max, index);
//
//        // Construct the bin and its coordinates
//        MDBin<MDE,nd> bin;
//        for (size_t bd=0; bd<numBD; bd++)
//        {
//          // Index in this binning dimension (i_x, i_y, etc.)
//          size_t idx = index[bd];
//          // Dimension in the MDEventWorkspace
//          size_t d = dimensionToBinFrom[bd];
//          // Corresponding extents
//          bin.m_min[d] = binDimensions[bd]->getX(idx);
//          bin.m_max[d] = binDimensions[bd]->getX(idx+1);
//        }
//        bin.m_index = linear_index;
//
//        // Check if the bin is in the ImplicitFunction (if any)
//        bool binContained = true;
//        if (implicitFunction)
//        {
//          binContained = implicitFunction->evaluate(makePoint3D(bin));
//        }
//
//        if (binContained)
//        {
//          // Array of bools set to true when a dimension is fully contained (binary splitting only)
//          bool fullyContained[nd];
//          for (size_t d=0; d<nd; d++)
//            fullyContained[d] = false;
//
//          // This will recursively bin into the sub grids
//          inBox->centerpointBin(bin, fullyContained);
//
//          // Save the data into the dense histogram
//          outWS->setSignalAt(linear_index, bin.m_signal);
//          outWS->setErrorAt(linear_index, bin.m_errorSquared);
//        }
//
//      } // (for each linear index)
//
//      // Cleanup
//      delete [] index;
//
//    }
//
//  private:
//    /// pointer to the MDBox containing MDEvents
//    IMDBox<MDE,nd> * inBox;
//    /// pointer to the MDHistoWorkspace containing the result
//    MDHistoWorkspace_sptr outWS;
//    const std::vector<MDHistoDimension_sptr> & binDimensions;
//    const std::vector<size_t> & dimensionToBinFrom;
//    const size_t linearIndexStart;
//    const size_t linearIndexStop;
//    const size_t * index_max;
//    const size_t * index_maker;
//    const Mantid::API::ImplicitFunction * implicitFunction;
//
//  };
//
//
//  //-----------------------------------------------------------------------------------------------
//  /** Bin a MDEventWorkspace into a dense histogram in a MDHistoWorkspace, using the MDBox's
//   * centerpointBin routine.
//   * You must give 4 dimensions, with names matching those in the MDEventWorkspace.
//   *
//   * @param dimX :: X dimension binning parameters
//   * @param dimY :: Y dimension binning parameters
//   * @param dimZ :: Z dimension binning parameters
//   * @param dimT :: T (time) dimension binning parameters
//   * @param implicitFunction :: a ImplicitFunction defining which bins to calculate. NULL if all bins will be calculated.
//   * @param prog :: Progress reporter, can be NULL.
//   * @return a shared ptr to MDHistoWorkspace created.
//   */
//  TMDE(
//  IMDWorkspace_sptr MDEventWorkspace)::centerpointBinToMDHistoWorkspace(Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY,
//      Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::Geometry::MDHistoDimension_sptr dimT,
//      Mantid::API::ImplicitFunction * implicitFunction, Mantid::Kernel::ProgressBase * prog) const
//  {
//    bool DODEBUG = true;
//    CPUTimer tim;
//
//    // Create the dense histogram. This allocates the memory
//    MDHistoWorkspace_sptr ws(new MDHistoWorkspace(dimX, dimY, dimZ, dimT));
//
//    if (DODEBUG) std::cout << tim << " to create the MDHistoWorkspace.\n";
//
//    // Make it into a vector of dimensions to which to bin.
//    std::vector<MDHistoDimension_sptr> binDimensionsIn;
//    binDimensionsIn.push_back(dimX);
//    binDimensionsIn.push_back(dimY);
//    binDimensionsIn.push_back(dimZ);
//    binDimensionsIn.push_back(dimT);
//
//    // Thin it down for invalid dimensions
//    std::vector<MDHistoDimension_sptr> binDimensions;
//    std::vector<size_t> dimensionToBinFrom;
//    for (size_t i = 0; i < binDimensionsIn.size(); ++i)
//    {
//      if (binDimensionsIn[i]->getNBins() == 0)
//        throw std::runtime_error("Dimension " + binDimensionsIn[i]->getName() + " was set to have 0 bins. Cannot continue.");
//
//      try {
//        size_t dim_index = this->getDimensionIndexByName(binDimensionsIn[i]->getName());
//        dimensionToBinFrom.push_back(dim_index);
//        binDimensions.push_back(binDimensionsIn[i]);
//      }
//      catch (std::runtime_error & e)
//      {
//        // The dimension was not found, so we are not binning across it. TODO: Log message?
//        if (binDimensionsIn[i]->getNBins() > 1)
//          throw std::runtime_error("Dimension " + binDimensionsIn[i]->getName() + " was not found in the MDEventWorkspace and has more than one bin! Cannot continue.");
//      }
//    }
//    // Number of output binning dimensions found
//    size_t numBD = binDimensions.size();
//
//    if (numBD == 0)
//      throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");
//
//    if (DODEBUG) std::cout << tim << " to cache the binning results.\n";
//
//    //Since the costs are not known ahead of time, use a simple FIFO buffer.
//    ThreadScheduler * ts = new ThreadSchedulerFIFO();
//
//    // Create the threadpool with: all CPUs, a progress reporter
//    ThreadPool tp(ts, 0, prog);
//
//    // Big efficiency gain is obtained by grouping a few bins per task.
//    size_t binsPerTask = 100;
//
//    // For progress reporting, the approx  # of tasks
//    if (prog)
//      prog->setNumSteps( int(ws->getNPoints() / binsPerTask) );
//
//    // This is the limit to loop over in each dimension
//    size_t * index_max = Utils::nestedForLoopSetUp(numBD);
//    for (size_t bd=0; bd<numBD; bd++) index_max[bd] = binDimensions[bd]->getNBins();
//    // Cache a calculation to convert indices x,y,z,t into a linear index.
//    size_t * index_maker = Utils::nestedForLoopSetUpIndexMaker(numBD, index_max);
//
//    // Set up a bunch of tasks
//    size_t numPoints = ws->getNPoints();
//    for (size_t start=0; start < numPoints; start += binsPerTask)
//    {
//      size_t stop = start + binsPerTask;
//      if (stop > numPoints) stop = numPoints;
//      ts->push(  new MDEventWorkspaceCenterpointBinTask<MDE,nd>(data, ws, binDimensions,
//                        dimensionToBinFrom, start, stop, index_max, index_maker, implicitFunction) );
//    }
//
//    if (DODEBUG) std::cout << tim << " to fill up the ThreadPool with all the tasks\n";
//
//    // OK, all that was just to fill the ThreadScheduler
//    // So now we actually run these.
//    tp.joinAll();
//
//    if (DODEBUG) std::cout << tim << " to run all the tasks\n";
//
//    delete index_max;
//    delete index_maker;
//    return ws;
//  }
//



}//namespace MDEvents

}//namespace Mantid

