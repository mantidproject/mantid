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
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidDataObjects/MDSplitBox.h"
#include <iomanip>
#include <functional>
#include <algorithm>
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/Exception.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace DataObjects {

//-----------------------------------------------------------------------------------------------
/** Default constructor
 */
TMDE(MDEventWorkspace)::MDEventWorkspace()
    : m_BoxController(new BoxController(nd)) {
  // First box is at depth 0, and has this default boxController
  data = new MDBox<MDE, nd>(m_BoxController.get(), 0);
}

//-----------------------------------------------------------------------------------------------
/** Copy constructor
 */
TMDE(MDEventWorkspace)::MDEventWorkspace(const MDEventWorkspace<MDE, nd> &other)
    : IMDEventWorkspace(other),
      m_BoxController(other.m_BoxController->clone()) {

  const MDBox<MDE, nd> *mdbox =
      dynamic_cast<const MDBox<MDE, nd> *>(other.data);
  const MDGridBox<MDE, nd> *mdgridbox =
      dynamic_cast<const MDGridBox<MDE, nd> *>(other.data);
  if (mdbox) {
    data = new MDBox<MDE, nd>(*mdbox, m_BoxController.get());
  } else if (mdgridbox) {
    data = new MDGridBox<MDE, nd>(*mdgridbox, m_BoxController.get());
  } else {
    throw std::runtime_error(
        "MDEventWorkspace::copy_ctor(): unexpected data box type found.");
  }
}

//-----------------------------------------------------------------------------------------------
/** Destructor
 */
TMDE(MDEventWorkspace)::~MDEventWorkspace() { delete data; }
/**Make workspace file backed if it has not been already file backed
 * @param fileName -- short or full file name of the file, which should be used
 * as the file back end
*/
TMDE(void MDEventWorkspace)::setFileBacked(const std::string & /*fileName*/) {
  throw Kernel::Exception::NotImplementedError(" Not yet implemented");
}
/** If the workspace was filebacked, this would clear file-backed information
 *from the workspace nodes and close the files responsible for file backing
 *
 *@param LoadFileBackedData -- if true, load all data initially backed to hdd
 *when breaking connection between the file and the workspace.
 *                             if false, data on hdd are lost if not previously
 *loaded in memory and the workspace is generally corrupted
 *                              (used in destructor)
 */
TMDE(void MDEventWorkspace)::clearFileBacked(bool LoadFileBackedData) {
  if (m_BoxController->isFileBacked()) {
    data->clearFileBacked(LoadFileBackedData);
    m_BoxController->clearFileBacked();
  }
}
//-----------------------------------------------------------------------------------------------
/** Perform initialization after m_dimensions (and others) have been set.
 * This sets the size of the box.
 */
TMDE(void MDEventWorkspace)::initialize() {
  if (m_dimensions.size() != nd)
    throw std::runtime_error("MDEventWorkspace::initialize() called with an "
                             "incorrect number of m_dimensions set. Use "
                             "addDimension() first to add the right number of "
                             "dimension info objects.");
  if (isGridBox())
    throw std::runtime_error("MDEventWorkspace::initialize() called on a "
                             "MDEventWorkspace containing a MDGridBox. You "
                             "should call initialize() before adding any "
                             "events!");
  double minSize[nd], maxSize[nd];
  for (size_t d = 0; d < nd; d++) {
    minSize[d] = m_dimensions[d]->getMinimum();
    maxSize[d] = m_dimensions[d]->getMaximum();
  }
  data->setExtents(minSize, maxSize);
}

//-----------------------------------------------------------------------------------------------
/** Get the data type (id) of the workspace */
TMDE(const std::string MDEventWorkspace)::id() const {
  std::ostringstream out;
  out << "MDEventWorkspace<" << MDE::getTypeName() << "," << getNumDims()
      << ">";
  return out.str();
}

//-----------------------------------------------------------------------------------------------
/** Get the data type (id) of the events in the workspace.
 * @return a string, either "MDEvent" or "MDLeanEvent"
 */
TMDE(std::string MDEventWorkspace)::getEventTypeName() const {
  return MDE::getTypeName();
}

//-----------------------------------------------------------------------------------------------
/** Returns the number of dimensions in this workspace */
TMDE(size_t MDEventWorkspace)::getNumDims() const { return nd; }

//-----------------------------------------------------------------------------------------------
/** Returns the total number of points (events) in this workspace */
TMDE(uint64_t MDEventWorkspace)::getNPoints() const {
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
TMDE(void MDEventWorkspace)::setMinRecursionDepth(size_t minDepth) {
  BoxController_sptr bc = this->getBoxController();
  double numBoxes = pow(double(bc->getNumSplit()), double(minDepth));
  double memoryToUse = numBoxes * double(sizeof(MDBox<MDE, nd>)) / 1024.0;
  MemoryStats stats;
  if (double(stats.availMem()) < memoryToUse) {
    std::ostringstream mess;
    mess << "Not enough memory available for the given MinRecursionDepth! "
         << "MinRecursionDepth is set to " << minDepth
         << ", which would create " << numBoxes << " boxes using "
         << memoryToUse << " kB of memory."
         << " You have " << stats.availMem() << " kB available." << std::endl;
    throw std::runtime_error(mess.str());
  }

  for (size_t depth = 1; depth < minDepth; depth++) {
    // Get all the MDGridBoxes in the workspace
    std::vector<API::IMDNode *> boxes;
    boxes.clear();
    this->getBox()->getBoxes(boxes, depth - 1, false);
    for (size_t i = 0; i < boxes.size(); i++) {
      MDGridBox<MDE, nd> *gbox = dynamic_cast<MDGridBox<MDE, nd> *>(boxes[i]);
      if (gbox) {
        // Split ALL the contents.
        for (size_t j = 0; j < gbox->getNumChildren(); j++)
          gbox->splitContents(j, NULL);
      }
    }
  }
}

//-----------------------------------------------------------------------------------------------
/** @return a vector with the size of the smallest bin in each dimension */
TMDE(std::vector<coord_t> MDEventWorkspace)::estimateResolution() const {
  size_t realDepth = 0;
  std::vector<size_t> numMD = m_BoxController->getNumMDBoxes();
  for (size_t i = 0; i < numMD.size(); i++)
    if (numMD[i] > 0)
      realDepth = i;

  std::vector<coord_t> out;
  for (size_t d = 0; d < nd; d++) {
    size_t finestSplit = 1;
    for (size_t i = 0; i < realDepth; i++)
      finestSplit *= m_BoxController->getSplitInto(d);
    IMDDimension_const_sptr dim = this->getDimension(d);
    // Calculate the bin size at the smallest split amount
    out.push_back((dim->getMaximum() - dim->getMinimum()) /
                  static_cast<coord_t>(finestSplit));
  }
  return out;
}

//-----------------------------------------------------------------------------------------------
/** Creates a new iterator pointing to the first cell (box) in the workspace
 *
 * @param suggestedNumCores :: split iterator over this many cores.
 * @param function :: Optional MDImplicitFunction limiting the iterator
 */
TMDE(std::vector<Mantid::API::IMDIterator *> MDEventWorkspace)::createIterators(
    size_t suggestedNumCores,
    Mantid::Geometry::MDImplicitFunction *function) const {
  // Get all the boxes in this workspaces
  std::vector<IMDNode *> boxes;
  // TODO: Should this be leaf only? Depends on most common use case
  if (function)
    this->data->getBoxes(boxes, 10000, true, function);
  else
    this->data->getBoxes(boxes, 10000, true);

  // Calculate the right number of cores
  size_t numCores = suggestedNumCores;
  if (!this->threadSafe())
    numCores = 1;
  size_t numElements = boxes.size();
  if (numCores > numElements)
    numCores = numElements;
  if (numCores < 1)
    numCores = 1;

  // Create one iterator per core, splitting evenly amongst spectra
  std::vector<IMDIterator *> out;
  for (size_t i = 0; i < numCores; i++) {
    size_t begin = (i * numElements) / numCores;
    size_t end = ((i + 1) * numElements) / numCores;
    if (end > numElements)
      end = numElements;
    out.push_back(new MDBoxIterator<MDE, nd>(boxes, begin, end));
  }
  return out;
}

//-----------------------------------------------------------------------------------------------
/** Returns the (normalized) signal at a given coordinates
 *
 * @param coords :: nd-sized array of coordinates
 * @param normalization :: how to normalize the signal.
 * @return the normalized signal of the box at the given coordinates. NaN if out
 *of bounds
 */
TMDE(signal_t MDEventWorkspace)::getSignalAtCoord(
    const coord_t *coords,
    const Mantid::API::MDNormalization &normalization) const {
  // Do an initial bounds check
  for (size_t d = 0; d < nd; d++) {
    coord_t x = coords[d];
    if (data->getExtents(d).outside(x))
      return std::numeric_limits<signal_t>::quiet_NaN();
  }
  // If you got here, then the point is in the workspace.
  const API::IMDNode *box = data->getBoxAtCoord(coords);
  if (box) {
    // What is our normalization factor?
    switch (normalization) {
    case NoNormalization:
      return box->getSignal();
    case VolumeNormalization:
      return box->getSignal() * box->getInverseVolume();
    case NumEventsNormalization:
      return box->getSignal() / double(box->getNPoints());
    }
    // Should not reach here
    return box->getSignal();
  } else
    return std::numeric_limits<signal_t>::quiet_NaN();
}

//-----------------------------------------------------------------------------------------------
/** Get a vector of the minimum extents that still contain all the events in the
 *workspace.
 *
 * @param depth :: recursion depth to which to search. This will determine the
 *resolution
 *        to which the extents will be found.
 * @return a vector of the minimum extents that still contain all the events in
 *the workspace.
 *         If the workspace is empty, then this will be the size of the overall
 *workspace
 */
TMDE(std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>>
         MDEventWorkspace)::getMinimumExtents(size_t depth) {
  std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> out(nd);
  std::vector<API::IMDNode *> boxes;
  // Get all the end (leaf) boxes
  this->data->getBoxes(boxes, depth, true);
  auto it = boxes.begin();
  auto it_end = boxes.end();
  for (; it != it_end; ++it) {
    API::IMDNode *box = *it;
    if (box->getNPoints() > 0) {
      for (size_t d = 0; d < nd; d++)
        box->getExtents(d).expand(out[d]);
    }
  }

  // Fix any missing dimensions (for empty workspaces)
  for (size_t d = 0; d < nd; d++) {
    if (out[d].isUndefined())
      out[d].setExtents(this->getDimension(d)->getMinimum(),
                        this->getDimension(d)->getMaximum());
  }
  return out;
}

//-----------------------------------------------------------------------------------------------
/// Returns some information about the box controller, to be displayed in the
/// GUI, for example
TMDE(std::vector<std::string> MDEventWorkspace)::getBoxControllerStats() const {
  std::vector<std::string> out;
  std::ostringstream mess;
  size_t mem;
  mem = (this->m_BoxController->getTotalNumMDBoxes() * sizeof(MDBox<MDE, nd>)) /
        1024;
  mess << m_BoxController->getTotalNumMDBoxes() << " MDBoxes (" << mem
       << " kB)";
  out.push_back(mess.str());
  mess.str("");

  mem = (this->m_BoxController->getTotalNumMDGridBoxes() *
         sizeof(MDGridBox<MDE, nd>)) /
        1024;
  mess << m_BoxController->getTotalNumMDGridBoxes() << " MDGridBoxes (" << mem
       << " kB)";
  out.push_back(mess.str());
  mess.str("");

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
  //      if (pct > 0 && pct < 1e-2) mess << std::scientific; else mess <<
  //      std::fixed;
  //      mess << std::setprecision(2) << pct;
  //    }
  //    out.push_back(mess.str()); mess.str("");

  if (m_BoxController->isFileBacked()) {
    mess << "File backed: ";
    double avail = double(m_BoxController->getFileIO()->getWriteBufferSize() *
                          sizeof(MDE)) /
                   (1024 * 1024);
    double used = double(m_BoxController->getFileIO()->getWriteBufferUsed() *
                         sizeof(MDE)) /
                  (1024 * 1024);
    mess << "Write buffer: " << used << " of " << avail << " MB. ";
    out.push_back(mess.str());
    mess.str("");

    mess << "File";
    if (this->fileNeedsUpdating())
      mess << " (needs updating)";

    mess << ": " << this->m_BoxController->getFileIO()->getFileName();
    out.push_back(mess.str());
    mess.str("");
  } else {
    mess << "Not file backed.";
    out.push_back(mess.str());
    mess.str("");
  }

  return out;
}

//-----------------------------------------------------------------------------------------------
/** Comparator for sorting MDBoxBase'es by ID */
template <typename BOXTYPE>
bool SortBoxesByID(const BOXTYPE &a, const BOXTYPE &b) {
  return a->getID() < b->getID();
}

//-----------------------------------------------------------------------------------------------
/** Create a table of data about the boxes contained */
TMDE(Mantid::API::ITableWorkspace_sptr MDEventWorkspace)::makeBoxTable(
    size_t start, size_t num) {
  CPUTimer tim;
  UNUSED_ARG(start);
  UNUSED_ARG(num);

  // Boxes to show
  std::vector<API::IMDNode *> boxes;
  std::vector<MDBoxBase<MDE, nd> *> boxes_filtered;
  this->getBox()->getBoxes(boxes, 1000, false);
  boxes_filtered.reserve(boxes.size());

  for (size_t i = 0; i < boxes.size(); i++) {
    MDBoxBase<MDE, nd> *box = dynamic_cast<MDBoxBase<MDE, nd> *>(boxes[i]);
    boxes_filtered.push_back(box);
  }

  // Now sort by ID
  typedef MDBoxBase<MDE, nd> *ibox_t;
  std::sort(boxes_filtered.begin(), boxes_filtered.end(),
            SortBoxesByID<ibox_t>);

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

  for (int i = 0; i < int(boxes_filtered.size()); i++) {
    MDBoxBase<MDE, nd> *box = boxes_filtered[i];
    int col = 0;

    ws->cell<int>(i, col++) = int(box->getID());
    ;
    ws->cell<int>(i, col++) = int(box->getDepth());
    ws->cell<int>(i, col++) = int(box->getNumChildren());

    MDBox<MDE, nd> *mdbox = dynamic_cast<MDBox<MDE, nd> *>(box);
    Kernel::ISaveable const *const pSaver(box->getISaveable());

    ws->cell<int>(i, col++) = pSaver ? int(pSaver->getFilePosition()) : -1;
    ws->cell<int>(i, col++) = pSaver ? int(pSaver->getFileSize()) : 0;
    ws->cell<int>(i, col++) = mdbox ? int(mdbox->getDataInMemorySize()) : -1;
    if (mdbox && pSaver) {
      ws->cell<std::string>(i, col++) = (pSaver->wasSaved() ? "yes" : "no");
      ws->cell<std::string>(i, col++) = (pSaver->isLoaded() ? "yes" : "no");

      bool isDataAdded = (mdbox->isDataAdded());
      ws->cell<std::string>(i, col++) =
          std::string(isDataAdded ? "Added " : "") +
          std::string(pSaver->isBusy() ? "Modif." : "");
    } else {
      ws->cell<std::string>(i, col++) = (pSaver ? "-" : "NA");
      ws->cell<std::string>(i, col++) = (pSaver ? "-" : "NA");
      ws->cell<std::string>(i, col++) = (pSaver ? "-" : "NA");
    }
    ws->cell<std::string>(i, col++) = box->getExtentsStr();
  }
  std::cout << tim << " to create the MDBox data table." << std::endl;
  return ws;
}

//-----------------------------------------------------------------------------------------------
/** @returns the number of bytes of memory used by the workspace. */
TMDE(size_t MDEventWorkspace)::getMemorySize() const {
  //    std::cout << "sizeof(MDE) " << sizeof(MDE) << std::endl;
  //    std::cout << "sizeof(MDBox<MDE,nd>) " << sizeof(MDBox<MDE,nd>) <<
  //    std::endl;
  //    std::cout << "sizeof(MDGridBox<MDE,nd>) " << sizeof(MDGridBox<MDE,nd>)
  //    << std::endl;
  size_t total = 0;
  if (this->m_BoxController->isFileBacked()) {
    // File-backed workspace
    // How much is in the cache?
    total =
        this->m_BoxController->getFileIO()->getWriteBufferUsed() * sizeof(MDE);
  } else {
    // All the events
    total = this->getNPoints() * sizeof(MDE);
  }
  // The MDBoxes are always in memory
  total += this->m_BoxController->getTotalNumMDBoxes() * sizeof(MDBox<MDE, nd>);
  total += this->m_BoxController->getTotalNumMDGridBoxes() *
           sizeof(MDGridBox<MDE, nd>);
  return total;
}

//-----------------------------------------------------------------------------------------------
/** Add a single event to this workspace. Automatic splitting is not performed
 *after adding
 * (call splitAllIfNeeded).
 *
 * @param event :: event to add.
 */
TMDE(void MDEventWorkspace)::addEvent(const MDE &event) {
  data->addEvent(event);
}

//-----------------------------------------------------------------------------------------------
/** Add a vector of MDEvents to the workspace.
 *
 * @param events :: const ref. to a vector of events; they will be copied into
 *the
 *        MDBox'es contained within.
 */
TMDE(size_t MDEventWorkspace)::addEvents(const std::vector<MDE> &events) {
  return data->addEvents(events);
}

//-----------------------------------------------------------------------------------------------
/** Split the contained MDBox into a MDGridBox or MDSplitBox, if it is not
 * that already.
 */
TMDE(void MDEventWorkspace)::splitBox() {
  // Want MDGridBox
  MDGridBox<MDE, nd> *gridBox = dynamic_cast<MDGridBox<MDE, nd> *>(data);
  if (!gridBox) {
    // Track how many MDBoxes there are in the overall workspace
    this->m_BoxController->trackNumBoxes(data->getDepth());
    MDBox<MDE, nd> *box = dynamic_cast<MDBox<MDE, nd> *>(data);
    if (!box)
      throw std::runtime_error("MDEventWorkspace::splitBox() expected its data "
                               "to be a MDBox* to split to MDGridBox.");
    gridBox = new MDGridBox<MDE, nd>(box);
    delete data;
    data = gridBox;
  }
}

//-----------------------------------------------------------------------------------------------
/** Goes through all the sub-boxes and splits them if they contain
 * enough events to be worth it.
 *
 * @param ts :: optional ThreadScheduler * that will be used to parallelize
 *        recursive splitting. Set to NULL to do it serially.
 */
TMDE(void MDEventWorkspace)::splitAllIfNeeded(Kernel::ThreadScheduler *ts) {
  data->splitAllIfNeeded(ts);
}

//-----------------------------------------------------------------------------------------------
/** Goes through the MDBoxes that were tracked by the BoxController
 * as being too large, and splits them.
 * @param ts :: optional ThreadScheduler * that will be used to parallelize
 *        recursive splitting.
 */
TMDE(void MDEventWorkspace)::splitTrackedBoxes(ThreadScheduler *ts) {
  UNUSED_ARG(ts);
  throw std::runtime_error("Not implemented yet");
  //    // Get a COPY of the vector (to avoid thread-safety issues)
  //    std::vector<void *> boxes = this->getBoxController()->getBoxesToSplit();
  //    //PRAGMA_OMP( parallel for )
  //    for (int i=0; i<int(boxes.size()); i++)
  //    {
  //      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
  //      if (box)
  //      {
  //        MDGridBox<MDE,nd> * parent = dynamic_cast<MDGridBox<MDE,nd>
  //        *>(box->getParent());
  //        if (parent)
  //        {
  //          parent->splitContents(parent->getChildIndexFromID(box->getId()),
  //          ts);
  //        }
  //      }
  //    }
}

//-----------------------------------------------------------------------------------------------
/** Refresh the cache of # of points, signal, and error.
 * NOTE: This is performed in parallel using a threadpool.
 *  */
TMDE(void MDEventWorkspace)::refreshCache() {
  // Function is overloaded and recursive; will check all sub-boxes
  data->refreshCache();
  // TODO ThreadPool
}

//  //-----------------------------------------------------------------------------------------------
//  /** Add a large number of events to this MDEventWorkspace.
//   * This will use a ThreadPool/OpenMP to allocate events in parallel.
//   *
//   * param events :: vector of events to be copied.
//   * param prog :: optional Progress object to report progress back to
//   GUI/algorithms.
//   * return the number of events that were rejected (because of being out of
//   bounds)
//   */
//  TMDE(
//  void MDEventWorkspace)::addManyEvents(const std::vector<MDE> & events,
//  Mantid::Kernel::ProgressBase * prog)
//  {
//    // Always split the MDBox into a grid box
//    this->splitBox();
//    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
//
//    // Get some parameters that should optimize task allocation.
//    size_t eventsPerTask, numTasksPerBlock;
//    this->m_BoxController->getAddingEventsParameters(eventsPerTask,
//    numTasksPerBlock);
//
//    // Set up progress report, if any
//    if (prog)
//    {
//      size_t numTasks = events.size()/eventsPerTask;
//      prog->setNumSteps( int( numTasks + numTasks/numTasksPerBlock ));
//    }
//
//    // Where we are in the list of events
//    size_t event_index = 0;
//    while (event_index < events.size())
//    {
//      //Since the costs are not known ahead of time, use a simple FIFO buffer.
//      ThreadScheduler * ts = new ThreadSchedulerFIFO();
//      // Create the threadpool
//      ThreadPool tp(ts);
//
//      // Do 'numTasksPerBlock' tasks with 'eventsPerTask' events in each one.
//      for (size_t i = 0; i < numTasksPerBlock; i++)
//      {
//        // Calculate where to start and stop in the events vector
//        bool breakout = false;
//        size_t start_at = event_index;
//        event_index += eventsPerTask;
//        size_t stop_at = event_index;
//        if (stop_at >= events.size())
//        {
//          stop_at = events.size();
//          breakout = true;
//        }
//
//        // Create a task and push it into the scheduler
//        //std::cout << "Making a AddEventsTask " << start_at << " to " <<
//        stop_at << std::endl;
//        typename MDGridBox<MDE,nd>::AddEventsTask * task;
//        task = new typename MDGridBox<MDE,nd>::AddEventsTask(gridBox, events,
//        start_at, stop_at, prog) ;
//        ts->push( task );
//
//        if (breakout) break;
//      }
//
//      // Finish all threads.
////      std::cout << "Starting block ending at index " << event_index << " of
///" << events.size() << std::endl;
//      Timer tim;
//      tp.joinAll();
////      std::cout << "... block took " << tim.elapsed() << " secs.\n";
//
//      //Create a threadpool for splitting.
//      ThreadScheduler * ts_splitter = new ThreadSchedulerFIFO();
//      ThreadPool tp_splitter(ts_splitter);
//
//      //Now, shake out all the sub boxes and split those if needed
////      std::cout << "\nStarting splitAllIfNeeded().\n";
//      if (prog) prog->report("Splitting MDBox'es.");
//
//      gridBox->splitAllIfNeeded(ts_splitter);
//      tp_splitter.joinAll();
////      std::cout << "\n... splitAllIfNeeded() took " << tim.elapsed() << "
/// secs.\n";
//    }
//
//    // Refresh the counts, now that we are all done.
//    this->refreshCache();
//  }

//-----------------------------------------------------------------------------------------------
/** Obtain coordinates for a line plot through a MDWorkspace.
 * Cross the workspace from start to end points, recording the signal along the
 *line.
 * Sets the x,y vectors to the histogram bin boundaries and counts
 *
 * @param start :: coordinates of the start point of the line
 * @param end :: coordinates of the end point of the line
 * @param normalize :: how to normalize the signal
 * @param x :: is set to the boundaries of the bins, relative to start of the
 *line.
 * @param y :: is set to the normalized signal for each bin. Length = length(x)
 *- 1
 * @param e :: vector of errors for each bin.
 */
TMDE(void MDEventWorkspace)::getLinePlot(const Mantid::Kernel::VMD &start,
                                         const Mantid::Kernel::VMD &end,
                                         Mantid::API::MDNormalization normalize,
                                         std::vector<coord_t> &x,
                                         std::vector<signal_t> &y,
                                         std::vector<signal_t> &e) const {
  // TODO: Don't use a fixed number of points later
  size_t numPoints = 200;

  VMD step = (end - start) / double(numPoints);
  double stepLength = step.norm();

  // These will be the curve as plotted
  x.clear();
  y.clear();
  e.clear();
  for (size_t i = 0; i < numPoints; i++) {
    // Coordinate along the line
    VMD coord = start + step * double(i);
    // Record the position along the line
    x.push_back(static_cast<coord_t>(stepLength * double(i)));

    // Look for the box at this coordinate
    // const MDBoxBase<MDE,nd> * box = NULL;
    const IMDNode *box = NULL;

    // Do an initial bounds check
    bool outOfBounds = false;
    for (size_t d = 0; d < nd; d++) {
      if (data->getExtents(d).outside(coord[d])) {
        outOfBounds = true;
        break;
      }
    }

    // TODO: make the logic/reuse in the following nicer.
    if (!outOfBounds) {
      box = this->data->getBoxAtCoord(coord.getBareArray());

      if (box != NULL) {
        // What is our normalization factor?
        signal_t normalizer = 1.0;
        switch (normalize) {
        case NoNormalization:
          break;
        case VolumeNormalization:
          normalizer = box->getInverseVolume();
          break;
        case NumEventsNormalization:
          normalizer = double(box->getNPoints());
          break;
        }

        // And add the normalized signal/error to the list
        y.push_back(box->getSignal() * normalizer);
        e.push_back(box->getError() * normalizer);
      } else {
        y.push_back(std::numeric_limits<double>::quiet_NaN());
        e.push_back(std::numeric_limits<double>::quiet_NaN());
      }
    } else {
      // Point is outside the workspace. Add NANs
      y.push_back(std::numeric_limits<double>::quiet_NaN());
      e.push_back(std::numeric_limits<double>::quiet_NaN());
    }
  }
  // And the last point
  x.push_back((end - start).norm());
}

/**
Setter for the masking region.
@param maskingRegion : Implicit function defining mask region.
*/
TMDE(void MDEventWorkspace)::setMDMasking(
    Mantid::Geometry::MDImplicitFunction *maskingRegion) {
  if (maskingRegion) {
    std::vector<API::IMDNode *> toMaskBoxes;

    // Apply new masks
    this->data->getBoxes(toMaskBoxes, 10000, true, maskingRegion);
    for (size_t i = 0; i < toMaskBoxes.size(); ++i) {
      toMaskBoxes[i]->mask();
    }

    delete maskingRegion;
  }
}

/**
Clears ALL existing masks off the workspace.
*/
TMDE(void MDEventWorkspace)::clearMDMasking() {
  std::vector<API::IMDNode *> allBoxes;
  // Clear old masks
  this->data->getBoxes(allBoxes, 10000, true);
  for (size_t i = 0; i < allBoxes.size(); ++i) {
    allBoxes[i]->unmask();
  }
}

/**
Set the special coordinate system (if any) to use.
@param coordinateSystem : Special coordinate system to use.
*/
TMDE(void MDEventWorkspace)::setCoordinateSystem(
    const Mantid::Kernel::SpecialCoordinateSystem coordinateSystem) {
  // If there isn't an experiment info, create one.
  if (this->getNumExperimentInfo() == 0) {
    ExperimentInfo_sptr expInfo =
        boost::shared_ptr<ExperimentInfo>(new ExperimentInfo());
    this->addExperimentInfo(expInfo);
  }
  this->getExperimentInfo(0)->mutableRun().addProperty(
      "CoordinateSystem", (int)coordinateSystem, true);
}

/**
Get the special coordinate system (if any) to use.
@return Special coordinate system if any.
*/
TMDE(Mantid::Kernel::SpecialCoordinateSystem
         MDEventWorkspace)::getSpecialCoordinateSystem() const {
  Mantid::Kernel::SpecialCoordinateSystem result = None;
  try {
    auto nInfos = this->getNumExperimentInfo();
    if (nInfos > 0) {
      Property *prop =
          this->getExperimentInfo(0)->run().getProperty("CoordinateSystem");
      PropertyWithValue<int> *p = dynamic_cast<PropertyWithValue<int> *>(prop);
      int temp = *p;
      result = (SpecialCoordinateSystem)temp;
    }
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
  }
  return result;
}

} // namespace DataObjects

} // namespace Mantid
