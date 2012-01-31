#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
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
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidMDEvents/MDSplitBox.h"
#include <iomanip>
#include <functional>
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidKernel/Memory.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

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
  /** Copy constructor
   */
  TMDE(
  MDEventWorkspace)::MDEventWorkspace(const MDEventWorkspace<MDE,nd> & other)
  : IMDEventWorkspace(other),
    m_BoxController( new BoxController(*other.m_BoxController) )
  {
    const MDBox<MDE,nd> * mdbox = dynamic_cast<const MDBox<MDE,nd> *>(other.data);
    const MDGridBox<MDE,nd> * mdgridbox = dynamic_cast<const MDGridBox<MDE,nd> *>(other.data);
    if (mdbox)
      data = new MDBox<MDE, nd>(*mdbox);
    else if (mdgridbox)
      data = new MDGridBox<MDE, nd>(*mdgridbox);
    else
      throw std::runtime_error("MDEventWorkspace::copy_ctor(): unexpected data box type found.");
  }

  //-----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(
  MDEventWorkspace)::~MDEventWorkspace()
  {
    delete data;
	m_BoxController->closeFile();
  }


  //-----------------------------------------------------------------------------------------------
  /** Perform initialization after m_dimensions (and others) have been set.
   * This sets the size of the box.
   */
  TMDE(
  void MDEventWorkspace)::initialize()
  {
    if (m_dimensions.size() != nd)
      throw std::runtime_error("MDEventWorkspace::initialize() called with an incorrect number of m_dimensions set. Use addDimension() first to add the right number of dimension info objects.");
    if (isGridBox())
        throw std::runtime_error("MDEventWorkspace::initialize() called on a MDEventWorkspace containing a MDGridBox. You should call initialize() before adding any events!");
    for (size_t d=0; d<nd; d++)
      data->setExtents(d, m_dimensions[d]->getMinimum(), m_dimensions[d]->getMaximum());
  }

  //-----------------------------------------------------------------------------------------------
  /** Get the data type (id) of the workspace */
  TMDE(
  const std::string MDEventWorkspace)::id() const
  {
    std::ostringstream out;
    out << "MDEventWorkspace<" << MDE::getTypeName() << "," << getNumDims() << ">";
    return out.str();
  }


  //-----------------------------------------------------------------------------------------------
  /** Get the data type (id) of the events in the workspace.
   * @return a string, either "MDEvent" or "MDLeanEvent"
   */
  TMDE(
  std::string MDEventWorkspace)::getEventTypeName() const
  {
    return MDE::getTypeName();
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
  uint64_t MDEventWorkspace)::getNPoints() const
  {
    return data->getNPoints();
  }

  //-----------------------------------------------------------------------------------------------
  /** Recurse box structure down to a minimum depth.
   *
   * This will split all boxes so that all MDBoxes are at the depth indicated.
   * 0 = no splitting, 1 = one level of splitting, etc.
   *
   * WARNING! This should ONLY be called before adding any events to a workspace.
   *
   * WARNING! Be careful using this since it can quickly create a huge
   * number of boxes = (SplitInto ^ (MinRercursionDepth * NumDimensions))
   *
   * @param minDepth :: minimum recursion depth.
   * @throw std::runtime_error if there is not enough memory for the boxes.
   */
  TMDE(
  void MDEventWorkspace)::setMinRecursionDepth(size_t minDepth)
  {
    BoxController_sptr bc = this->getBoxController();
    double numBoxes = pow(double(bc->getNumSplit()), double(minDepth));
    double memoryToUse = numBoxes * double(sizeof(MDBox<MDE,nd>)) / 1024.0;
    MemoryStats stats;
    if (double(stats.availMem()) < memoryToUse)
    {
      std::ostringstream mess;
      mess << "Not enough memory available for the given MinRecursionDepth! "
           << "MinRecursionDepth is set to " << minDepth << ", which would create " << numBoxes << " boxes using " <<  memoryToUse << " kB of memory."
           << " You have " << stats.availMem() << " kB available." << std::endl;
      throw std::runtime_error(mess.str());
    }

    for (size_t depth = 1; depth < minDepth; depth++)
    {
      // Get all the MDGridBoxes in the workspace
      std::vector<IMDBox<MDE,nd>*> boxes;
      boxes.clear();
      this->getBox()->getBoxes(boxes, depth-1, false);
      for (size_t i=0; i<boxes.size(); i++)
      {
        IMDBox<MDE,nd> * box = boxes[i];
        MDGridBox<MDE,nd>* gbox = dynamic_cast<MDGridBox<MDE,nd>*>(box);
        if (gbox)
        {
          // Split ALL the contents.
          for (size_t j=0; j<gbox->getNumChildren(); j++)
            gbox->splitContents(j, NULL);
        }
      }
    }
  }


  //-----------------------------------------------------------------------------------------------
  /// Set the number of bins in each dimension to something corresponding to the estimated resolution of the finest binning
  TMDE(
  void MDEventWorkspace)::estimateResolution()
  {
    size_t realDepth = 0;
    std::vector<size_t> numMD = m_BoxController->getNumMDBoxes();
    for (size_t i=0; i<numMD.size(); i++)
      if (numMD[i] > 0) realDepth = i;

    for (size_t d=0; d<nd; d++)
    {
      size_t finestSplit = 1;
      for (size_t i=0; i<realDepth; i++)
        finestSplit *= m_BoxController->getSplitInto(d);
      IMDDimension_sptr dim = m_dimensions[d];
      // Set the number of bins
      dim->setRange( finestSplit, dim->getMinimum(), dim->getMaximum());
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Creates a new iterator pointing to the first cell (box) in the workspace
   * @param function :: Optional MDImplicitFunction limiting the iterator
   */
  TMDE(
  Mantid::API::IMDIterator*  MDEventWorkspace)::createIterator(Mantid::Geometry::MDImplicitFunction * function) const
  {
    // TODO: Should this be leaf only? Depends on most common use case
    return new MDBoxIterator<MDE,nd>(data, 10000, true, function);
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the (normalized) signal at a given coordinates
   *
   * @param coords :: nd-sized array of coordinates
   * @return the normalized signal of the box at the given coordinates. NaN if out of bounds
   */
  TMDE(
  signal_t MDEventWorkspace)::getSignalAtCoord(const coord_t * coords) const
  {
    // Do an initial bounds check
    for (size_t d=0; d<nd; d++)
    {
      coord_t x = coords[d];
      MDDimensionExtents & extents = data->getExtents(d);
      if (x < extents.min || x >= extents.max)
        return std::numeric_limits<signal_t>::quiet_NaN();
    }
    // If you got here, then the point is in the workspace.
    const IMDBox<MDE,nd> * box = data->getBoxAtCoord(coords);
    if (box)
      return box->getSignalNormalized();
    else
      return std::numeric_limits<signal_t>::quiet_NaN();
  }



  //-----------------------------------------------------------------------------------------------
  /** Get a vector of the minimum extents that still contain all the events in the workspace.
   *
   * @param depth :: recursion depth to which to search. This will determine the resolution
   *        to which the extents will be found.
   * @return a vector of the minimum extents that still contain all the events in the workspace.
   *         If the workspace is empty, then this will be the size of the overall workspace
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
    for (it = boxes.begin(); it != it_end; ++it)
    {
      IMDBox<MDE,nd>* box = *it;
      if (box->getNPoints() > 0)
      {
        for (size_t d=0; d<nd; d++)
        {
          Mantid::Geometry::MDDimensionExtents & x = box->getExtents(d);
          if (x.max > out[d].max) out[d].max = x.max;
          if (x.min < out[d].min) out[d].min = x.min;
        }
      }
    }

    // Fix any missing dimensions (for empty workspaces)
    for (size_t d=0; d<nd; d++)
    {
      if (out[d].min > out[d].max)
      {
        out[d].min = this->getDimension(d)->getMinimum();
        out[d].max = this->getDimension(d)->getMaximum();
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

//    mess << "Avg recursion depth: " << m_BoxController->getAverageDepth();
//    out.push_back(mess.str()); mess.str("");
//
//    mess << "Recursion Coverage %: ";
//    const std::vector<size_t> & num = m_BoxController->getNumMDBoxes();
//    const std::vector<double> & max = m_BoxController->getMaxNumMDBoxes();
//    for (size_t i=0; i<num.size(); i++)
//    {
//      if (i > 0) mess << ", ";
//      double pct = (double(num[i]) / double(max[i] * 100));
//      if (pct > 0 && pct < 1e-2) mess << std::scientific; else mess << std::fixed;
//      mess << std::setprecision(2) << pct;
//    }
//    out.push_back(mess.str()); mess.str("");

    if (m_BoxController->getFile())
    {
      mess << "File backed: ";
      double avail = double(m_BoxController->getDiskBuffer().getWriteBufferSize() * sizeof(MDE)) / (1024*1024);
      double used = double(m_BoxController->getDiskBuffer().getWriteBufferUsed() * sizeof(MDE)) / (1024*1024);
      mess << "Write buffer: " << used << " of " << avail << " MB. ";
      out.push_back(mess.str()); mess.str("");

      mess << "File";
      if (this->fileNeedsUpdating())
        mess << " (needs updating)";
      mess << ": " << this->m_BoxController->getFilename();
      out.push_back(mess.str()); mess.str("");
    }
    else
    {
      mess << "Not file backed.";
      out.push_back(mess.str()); mess.str("");
    }

    return out;
  }


  //-----------------------------------------------------------------------------------------------
  /** Comparator for sorting IMDBox'es by ID */
  template <typename BOXTYPE>
  bool SortBoxesByID(const BOXTYPE& a, const BOXTYPE& b)
  {
    return a->getId() < b->getId();
  }


  //-----------------------------------------------------------------------------------------------
  /** Create a table of data about the boxes contained */
  TMDE(
  Mantid::API::ITableWorkspace_sptr MDEventWorkspace)::makeBoxTable(size_t start, size_t num)
  {
    CPUTimer tim;
    UNUSED_ARG(start);
    UNUSED_ARG(num);
    // Boxes to show
    std::vector<IMDBox<MDE,nd>*> boxes;
    std::vector<IMDBox<MDE,nd>*> boxes_filtered;
    this->getBox()->getBoxes(boxes, 1000, false);

    bool withPointsOnly = true;
    if (withPointsOnly)
    {
      boxes_filtered.reserve(boxes.size());
      for (size_t i=0; i<boxes.size(); i++)
        if (boxes[i]->getNPoints() > 0)
          boxes_filtered.push_back(boxes[i]);
    }
    else
      boxes_filtered = boxes;

    // Now sort by ID
    typedef IMDBox<MDE,nd> * ibox_t;
    std::sort(boxes_filtered.begin(), boxes_filtered.end(), SortBoxesByID<ibox_t> );


    // Create the table
    int numRows = int(boxes_filtered.size());
    TableWorkspace_sptr ws(new TableWorkspace(numRows));
    ws->addColumn("int", "ID");
    ws->addColumn("int", "Depth");
    ws->addColumn("int", "# children");
    ws->addColumn("int", "File Pos.");
    ws->addColumn("int", "File Size");
    ws->addColumn("int", "EventVec Size");
    ws->addColumn("str", "OnDisk?");
    ws->addColumn("str", "InMemory?");
    ws->addColumn("str", "Changes?");
    ws->addColumn("str", "Extents");

    for (int i=0; i<int(boxes_filtered.size()); i++)
    {
      IMDBox<MDE,nd>* box = boxes_filtered[i];
      int col = 0;
      ws->cell<int>(i, col++) = int(box->getId());;
      ws->cell<int>(i, col++) = int(box->getDepth());
      ws->cell<int>(i, col++) = int(box->getNumChildren());
      ws->cell<int>(i, col++) = int(box->getFilePosition());
      MDBox<MDE,nd>* mdbox = dynamic_cast<MDBox<MDE,nd>*>(box);
      ws->cell<int>(i, col++) = mdbox ? int(mdbox->getFileNumEvents()) : 0;
      ws->cell<int>(i, col++) = mdbox ? int(mdbox->getEventVectorSize()) : -1;
      if (mdbox)
      {
        ws->cell<std::string>(i, col++) = (mdbox->getOnDisk() ? "yes":"no");
        ws->cell<std::string>(i, col++) = (mdbox->getInMemory() ? "yes":"no");
        ws->cell<std::string>(i, col++) = std::string(mdbox->dataAdded() ? "Added ":"") + std::string(mdbox->dataModified() ? "Modif.":"") ;
      }
      else
      {
        ws->cell<std::string>(i, col++) = "-";
        ws->cell<std::string>(i, col++) = "-";
        ws->cell<std::string>(i, col++) = "-";
      }
      ws->cell<std::string>(i, col++) = box->getExtentsStr();
    }
    std::cout << tim << " to create the MDBox data table." << std::endl;
    return ws;
  }

  //-----------------------------------------------------------------------------------------------
  /** @returns the number of bytes of memory used by the workspace. */
  TMDE(
  size_t MDEventWorkspace)::getMemorySize() const
  {
//    std::cout << "sizeof(MDE) " << sizeof(MDE) << std::endl;
//    std::cout << "sizeof(MDBox<MDE,nd>) " << sizeof(MDBox<MDE,nd>) << std::endl;
//    std::cout << "sizeof(MDGridBox<MDE,nd>) " << sizeof(MDGridBox<MDE,nd>) << std::endl;
    size_t total = 0;
    if (this->m_BoxController->getFile())
    {
      // File-backed workspace
      // How much is in the cache?
      total = this->m_BoxController->getDiskBuffer().getWriteBufferUsed() * sizeof(MDE);
    }
    else
    {
      // All the events
      total = this->getNPoints() * sizeof(MDE);
    }
    // The MDBoxes are always in memory
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
  size_t MDEventWorkspace)::addEvents(const std::vector<MDE> & events)
  {
    return data->addEvents(events);
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



  //-----------------------------------------------------------------------------------------------
  /** Obtain coordinates for a line plot through a MDWorkspace.
   * Cross the workspace from start to end points, recording the signal along the line.
   * Sets the x,y vectors to the histogram bin boundaries and counts
   *
   * @param start :: coordinates of the start point of the line
   * @param end :: coordinates of the end point of the line
   * @param normalize :: how to normalize the signal
   * @param x :: is set to the boundaries of the bins, relative to start of the line.
   * @param y :: is set to the normalized signal for each bin. Length = length(x) - 1
   */
  TMDE(
  void MDEventWorkspace)::getLinePlot(const Mantid::Kernel::VMD & start, const Mantid::Kernel::VMD & end,
      Mantid::API::MDNormalization normalize, std::vector<coord_t> & x, std::vector<signal_t> & y)
  {
    UNUSED_ARG(start);UNUSED_ARG(end);UNUSED_ARG(normalize);UNUSED_ARG(x);UNUSED_ARG(y);
    throw std::runtime_error("MDEventWorkspace::getLinePlot() not yet implemented.");
  }


}//namespace MDEvents

}//namespace Mantid

