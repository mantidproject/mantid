// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDFramesToSpecialCoordinateSystem.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/WarningSuppressions.h"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <ostream>

// Test for gcc 4.4
#if __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 4 || (__GNUC_MINOR__ == 4 && __GNUC_PATCHLEVEL__ > 0)))
GNU_DIAG_OFF("strict-aliasing")
#endif

namespace Mantid {
namespace DataObjects {

namespace {
Kernel::Logger logger("MDEventWorkspace");
}

//-----------------------------------------------------------------------------------------------
/** Constructor
 */

TMDE(MDEventWorkspace)::MDEventWorkspace(Mantid::API::MDNormalization preferredNormalization,
                                         Mantid::API::MDNormalization preferredNormalizationHisto)
    : API::IMDEventWorkspace(), m_BoxController(new API::BoxController(nd)), data(),
      m_displayNormalization(preferredNormalization), m_displayNormalizationHisto(preferredNormalizationHisto),
      m_coordSystem(Kernel::None) {
  // First box is at depth 0, and has this default boxController
  data = std::make_unique<MDBox<MDE, nd>>(m_BoxController.get(), 0);
}

//-----------------------------------------------------------------------------------------------
/** Copy constructor
 */
TMDE(MDEventWorkspace)::MDEventWorkspace(const MDEventWorkspace<MDE, nd> &other)
    : IMDEventWorkspace(other), m_BoxController(other.m_BoxController->clone()), data(),
      m_displayNormalization(other.m_displayNormalization),
      m_displayNormalizationHisto(other.m_displayNormalizationHisto), m_coordSystem(other.m_coordSystem) {

  const MDBox<MDE, nd> *mdbox = dynamic_cast<const MDBox<MDE, nd> *>(other.data.get());
  const MDGridBox<MDE, nd> *mdgridbox = dynamic_cast<const MDGridBox<MDE, nd> *>(other.data.get());
  if (mdbox) {
    data = std::make_unique<MDBox<MDE, nd>>(*mdbox, m_BoxController.get());
  } else if (mdgridbox) {
    data = std::make_unique<MDGridBox<MDE, nd>>(*mdgridbox, m_BoxController.get());
  } else {
    throw std::runtime_error("MDEventWorkspace::copy_ctor(): unexpected data box type found.");
  }
}

/**Make workspace file backed if it has not been already file backed
 * @param fileName -- short or full file name of the file, which should be used
 * as the file back end
 */
TMDE(void MDEventWorkspace)::setFileBacked(const std::string & /*fileName*/) {
  throw Kernel::Exception::NotImplementedError(" Not yet implemented");
}
/*
 * Set filebacked on the contained box
 */
TMDE(void MDEventWorkspace)::setFileBacked() { this->getBox()->setFileBacked(); }
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
  out << "MDEventWorkspace<" << MDE::getTypeName() << "," << getNumDims() << ">";
  return out.str();
}

//-----------------------------------------------------------------------------------------------
/** Get the data type (id) of the events in the workspace.
 * @return a string, either "MDEvent" or "MDLeanEvent"
 */
TMDE(std::string MDEventWorkspace)::getEventTypeName() const { return MDE::getTypeName(); }

//-----------------------------------------------------------------------------------------------
/** Returns the number of dimensions in this workspace */
TMDE(size_t MDEventWorkspace)::getNumDims() const { return nd; }

//-----------------------------------------------------------------------------------------------
/** Returns the total number of points (events) in this workspace */
TMDE(uint64_t MDEventWorkspace)::getNPoints() const { return data->getNPoints(); }

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
  API::BoxController_sptr bc = this->getBoxController();
  double numBoxes = pow(double(bc->getNumSplit()), double(minDepth));
  double memoryToUse = numBoxes * double(sizeof(MDBox<MDE, nd>)) / 1024.0;
  Kernel::MemoryStats stats;
  if (double(stats.availMem()) < memoryToUse) {
    std::ostringstream mess;
    mess << "Not enough memory available for the given MinRecursionDepth! "
         << "MinRecursionDepth is set to " << minDepth << ", which would create " << numBoxes << " boxes using "
         << memoryToUse << " kB of memory."
         << " You have " << stats.availMem() << " kB available.\n";
    throw std::runtime_error(mess.str());
  }

  for (size_t depth = 1; depth < minDepth; depth++) {
    // Get all the MDGridBoxes in the workspace
    std::vector<API::IMDNode *> boxes;
    boxes.clear();
    this->getBox()->getBoxes(boxes, depth - 1, false);
    for (const auto box : boxes) {
      MDGridBox<MDE, nd> *gbox = dynamic_cast<MDGridBox<MDE, nd> *>(box);
      if (gbox) {
        // Split ALL the contents.
        for (size_t j = 0; j < gbox->getNumChildren(); j++)
          gbox->splitContents(j, nullptr);
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

  auto splitTop = m_BoxController->getSplitTopInto();
  std::vector<coord_t> out;
  for (size_t d = 0; d < nd; d++) {
    size_t finestSplit = 1;
    size_t i = 0;
    if (splitTop) {
      finestSplit *= splitTop.value()[d];
      i = 1;
    }
    for (; i < realDepth; i++)
      finestSplit *= m_BoxController->getSplitInto(d);
    Geometry::IMDDimension_const_sptr dim = this->getDimension(d);
    // Calculate the bin size at the smallest split amount
    out.emplace_back((dim->getMaximum() - dim->getMinimum()) / static_cast<coord_t>(finestSplit));
  }
  return out;
}

//-----------------------------------------------------------------------------------------------
/** Creates a new iterator pointing to the first cell (box) in the workspace
 *
 * @param suggestedNumCores :: split iterator over this many cores.
 * @param function :: Optional MDImplicitFunction limiting the iterator
 */
TMDE(std::vector<std::unique_ptr<Mantid::API::IMDIterator>> MDEventWorkspace)::createIterators(
    size_t suggestedNumCores, Mantid::Geometry::MDImplicitFunction *function) const {
  // Get all the boxes in this workspaces
  std::vector<API::IMDNode *> boxes;
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
  std::vector<std::unique_ptr<API::IMDIterator>> out;
  for (size_t i = 0; i < numCores; i++) {
    size_t begin = (i * numElements) / numCores;
    size_t end = ((i + 1) * numElements) / numCores;
    if (end > numElements)
      end = numElements;
    out.emplace_back(std::make_unique<MDBoxIterator<MDE, nd>>(boxes, begin, end));
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
TMDE(signal_t MDEventWorkspace)::getSignalAtCoord(const coord_t *coords,
                                                  const Mantid::API::MDNormalization &normalization) const {
  if (!isInBounds(coords)) {
    return std::numeric_limits<signal_t>::quiet_NaN();
  }
  // If you got here, then the point is in the workspace.
  const API::IMDNode *box = data->getBoxAtCoord(coords);
  return getNormalizedSignal(box, normalization);
}

TMDE(signal_t MDEventWorkspace)::getNormalizedSignal(const API::IMDNode *box,
                                                     const Mantid::API::MDNormalization &normalization) const {
  if (box) {
    // What is our normalization factor?
    switch (normalization) {
    case API::NoNormalization:
      return box->getSignal();
    case API::VolumeNormalization:
      return box->getSignal() * box->getInverseVolume();
    case API::NumEventsNormalization:
      return box->getSignal() / static_cast<double>(box->getNPoints());
    default:
      return box->getSignal();
    }
  } else
    return std::numeric_limits<signal_t>::quiet_NaN();
}

TMDE(signal_t MDEventWorkspace)::getNormalizedError(const API::IMDNode *box,
                                                    const Mantid::API::MDNormalization &normalization) const {
  if (box) {
    // What is our normalization factor?
    switch (normalization) {
    case Mantid::API::NoNormalization:
      return box->getError();
    case Mantid::API::VolumeNormalization:
      return box->getError() * box->getInverseVolume();
    case Mantid::API::NumEventsNormalization:
      return box->getError() / static_cast<double>(box->getNPoints());
    default:
      return box->getError();
    }
  } else
    return std::numeric_limits<signal_t>::quiet_NaN();
}

TMDE(bool MDEventWorkspace)::isInBounds(const coord_t *coords) const {
  for (size_t d = 0; d < nd; d++) {
    coord_t x = coords[d];
    if (data->getExtents(d).outside(x))
      return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------
/** Get the signal at a particular coordinate in the workspace
 * or return 0 if masked
 *
 * @param coords :: numDimensions-sized array of the coordinates to look at
 * @param normalization : Normalisation to use.
 * @return the (normalized) signal at a given coordinates.
 *         NaN if outside the range of this workspace
 */
TMDE(signal_t MDEventWorkspace)::getSignalWithMaskAtCoord(const coord_t *coords,
                                                          const Mantid::API::MDNormalization &normalization) const {
  if (!isInBounds(coords)) {
    return std::numeric_limits<signal_t>::quiet_NaN();
  }
  // Check if masked
  const API::IMDNode *box = data->getBoxAtCoord(coords);
  if (!box)
    return API::MDMaskValue;
  if (box->getIsMasked()) {
    return API::MDMaskValue;
  }
  return getNormalizedSignal(box, normalization);
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
TMDE(std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> MDEventWorkspace)::getMinimumExtents(
    size_t depth) const {
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
      out[d].setExtents(this->getDimension(d)->getMinimum(), this->getDimension(d)->getMaximum());
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
  mem = (this->m_BoxController->getTotalNumMDBoxes() * sizeof(MDBox<MDE, nd>)) / 1024;
  mess << m_BoxController->getTotalNumMDBoxes() << " MDBoxes (" << mem << " kB)";
  out.emplace_back(mess.str());
  mess.str("");

  mem = (this->m_BoxController->getTotalNumMDGridBoxes() * sizeof(MDGridBox<MDE, nd>)) / 1024;
  mess << m_BoxController->getTotalNumMDGridBoxes() << " MDGridBoxes (" << mem << " kB)";
  out.emplace_back(mess.str());
  mess.str("");

  //    mess << "Avg recursion depth: " << m_BoxController->getAverageDepth();
  //    out.emplace_back(mess.str()); mess.str("");
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
  //    out.emplace_back(mess.str()); mess.str("");

  if (m_BoxController->isFileBacked()) {
    mess << "File backed: ";
    double avail = double(m_BoxController->getFileIO()->getWriteBufferSize() * sizeof(MDE)) / (1024 * 1024);
    double used = double(m_BoxController->getFileIO()->getWriteBufferUsed() * sizeof(MDE)) / (1024 * 1024);
    mess << "Write buffer: " << used << " of " << avail << " MB. ";
    out.emplace_back(mess.str());
    mess.str("");

    mess << "File";
    if (this->fileNeedsUpdating())
      mess << " (needs updating)";

    mess << ": " << this->m_BoxController->getFileIO()->getFileName();
    out.emplace_back(mess.str());
    mess.str("");
  } else {
    mess << "Not file backed.";
    out.emplace_back(mess.str());
    mess.str("");
  }

  return out;
}

//-----------------------------------------------------------------------------------------------
/** Comparator for sorting MDBoxBase'es by ID */
template <typename BOXTYPE> bool SortBoxesByID(const BOXTYPE &a, const BOXTYPE &b) { return a->getID() < b->getID(); }

//-----------------------------------------------------------------------------------------------
/** Create a table of data about the boxes contained */
TMDE(Mantid::API::ITableWorkspace_sptr MDEventWorkspace)::makeBoxTable(size_t start, size_t num) {
  Kernel::CPUTimer tim;
  UNUSED_ARG(start);
  UNUSED_ARG(num);

  // Boxes to show
  std::vector<API::IMDNode *> boxes;
  std::vector<MDBoxBase<MDE, nd> *> boxes_filtered;
  this->getBox()->getBoxes(boxes, 1000, false);
  boxes_filtered.reserve(boxes.size());

  std::transform(boxes.cbegin(), boxes.cend(), std::back_inserter(boxes_filtered),
                 [](const auto box) { return dynamic_cast<MDBoxBase<MDE, nd> *>(box); });

  // Now sort by ID
  using ibox_t = MDBoxBase<MDE, nd> *;
  std::sort(boxes_filtered.begin(), boxes_filtered.end(), SortBoxesByID<ibox_t>);

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
          std::string(isDataAdded ? "Added " : "") + std::string(pSaver->isBusy() ? "Modif." : "");
    } else {
      ws->cell<std::string>(i, col++) = (pSaver ? "-" : "NA");
      ws->cell<std::string>(i, col++) = (pSaver ? "-" : "NA");
      ws->cell<std::string>(i, col++) = (pSaver ? "-" : "NA");
    }
    ws->cell<std::string>(i, col++) = box->getExtentsStr();
  }
  logger.information() << tim << " to create the MDBox data table.\n";
  return ws;
}

//-----------------------------------------------------------------------------------------------
/** @returns the number of bytes of memory used by the workspace. */
TMDE(size_t MDEventWorkspace)::getMemorySize() const {
  size_t total = 0;
  if (this->m_BoxController->isFileBacked()) {
    // File-backed workspace
    // How much is in the cache?
    total = this->m_BoxController->getFileIO()->getWriteBufferUsed() * sizeof(MDE);
  } else {
    // All the events
    total = this->getNPoints() * sizeof(MDE);
  }
  // The MDBoxes are always in memory
  total += this->m_BoxController->getTotalNumMDBoxes() * sizeof(MDBox<MDE, nd>);
  total += this->m_BoxController->getTotalNumMDGridBoxes() * sizeof(MDGridBox<MDE, nd>);
  return total;
}

//-----------------------------------------------------------------------------------------------
/** Add a single event to this workspace. Automatic splitting is not performed
 *after adding
 * (call splitAllIfNeeded).
 *
 * @param event :: event to add.
 */
TMDE(size_t MDEventWorkspace)::addEvent(const MDE &event) { return data->addEvent(event); }

//-----------------------------------------------------------------------------------------------
/** Add a vector of MDEvents to the workspace.
 *
 * @param events :: const ref. to a vector of events; they will be copied into
 *the
 *        MDBox'es contained within.
 */
TMDE(size_t MDEventWorkspace)::addEvents(const std::vector<MDE> &events) { return data->addEvents(events); }

//-----------------------------------------------------------------------------------------------
/** Split the contained MDBox into a MDGridBox or MDSplitBox, if it is not
 * that already.
 */
TMDE(void MDEventWorkspace)::splitBox() {
  // Want MDGridBox
  MDGridBox<MDE, nd> *gridBox = dynamic_cast<MDGridBox<MDE, nd> *>(data.get());
  if (!gridBox) {
    // Track how many MDBoxes there are in the overall workspace
    this->m_BoxController->trackNumBoxes(data->getDepth());
    MDBox<MDE, nd> *box = dynamic_cast<MDBox<MDE, nd> *>(data.get());
    if (!box)
      throw std::runtime_error("MDEventWorkspace::splitBox() expected its data "
                               "to be a MDBox* to split to MDGridBox.");
    auto tempGridBox = std::make_unique<MDGridBox<MDE, nd>>(box);

    data = std::move(tempGridBox);
  }
}

//-----------------------------------------------------------------------------------------------
/** Goes through all the sub-boxes and splits them if they contain
 * enough events to be worth it.
 *
 * @param ts :: optional ThreadScheduler * that will be used to parallelize
 *        recursive splitting. Set to NULL to do it serially.
 */
TMDE(void MDEventWorkspace)::splitAllIfNeeded(Kernel::ThreadScheduler *ts) { data->splitAllIfNeeded(ts); }

//-----------------------------------------------------------------------------------------------
/** Goes through the MDBoxes that were tracked by the BoxController
 * as being too large, and splits them.
 * @param ts :: optional ThreadScheduler * that will be used to parallelize
 *        recursive splitting.
 */
TMDE(void MDEventWorkspace)::splitTrackedBoxes(Kernel::ThreadScheduler *ts) {
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

//----------------------------------------------------------------------------------------------
/** Get ordered list of positions-along-the-line that lie halfway between points
 *where the line crosses box boundaries
 *
 * @param start :: coords of the start of the line
 * @param end :: coords of the end of the line
 * @param num_d :: number of dimensions
 * @param dir :: vector of the direction of the line
 * @param length :: the length of the line
 * @returns :: ordered set of halfway points between box crossings
 */
TMDE(std::set<coord_t> MDEventWorkspace)::getBoxBoundaryBisectsOnLine(const Mantid::Kernel::VMD &start,
                                                                      const Mantid::Kernel::VMD &end,
                                                                      const size_t num_d,
                                                                      const Mantid::Kernel::VMD &dir,
                                                                      const coord_t length) const {
  std::set<coord_t> mid_points;

  // Get the smallest box size along each dimension
  // We'll assume all boxes are this size and filter out any boundaries
  // which are not real later (by checking for unique box IDs)
  std::vector<coord_t> smallest_box_sizes = this->estimateResolution();

  // Next, we go through each dimension and see where the box boundaries
  // intersect the line.
  for (size_t d = 0; d < num_d; d++) {
    auto line_start = start[d];
    auto line_end = end[d];
    auto box_size = smallest_box_sizes[d];
    auto dir_current_dim = dir[d];

    // +1 to get the last box
    size_t num_boundaries = static_cast<size_t>(ceil(std::abs(line_end - line_start) / box_size)) + 1;

    // If the line has some component in this dimension then look for boundaries
    // it crosses
    if (dir_current_dim != 0.0) {
      getBoundariesInDimension(start, dir, num_boundaries, length, dir_current_dim, box_size, mid_points);
    }
  }
  return mid_points;
}

//----------------------------------------------------------------------------------------------
/** Insert positions-along-the-line that lie halfway between points
 * where the line crosses box boundaries in a single dimension of the workspace
 * into ordered set
 *
 * @param start :: coords of the start of the line
 * @param dir :: vector of the direction of the line
 * @param num_boundaries :: maximum number of boundary crossings possible if all
 *boxes are the size of the smallest box
 * @param length :: the length of the line
 * @param dir_current_dim :: component of the line direction in this dimension
 * @param box_size :: the minimum box size in this dimension
 * @param mid_points :: ordered set of halfway points between box crossings
 */
TMDE(void MDEventWorkspace)::getBoundariesInDimension(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &dir,
                                                      const size_t num_boundaries, const coord_t length,
                                                      const coord_t dir_current_dim, const coord_t box_size,
                                                      std::set<coord_t> &mid_points) const {
  auto lastPos = start;
  coord_t lastLinePos = 0;
  coord_t previousLinePos = 0;
  coord_t line_pos_of_box_centre = 0;
  const API::IMDNode *box = nullptr;
  auto last_id = std::numeric_limits<size_t>::max();

  for (size_t i = 1; i <= num_boundaries; i++) {
    size_t current_id = std::numeric_limits<size_t>::max();
    // Position along the line
    coord_t this_x = static_cast<coord_t>(i) * box_size;
    auto linePos = static_cast<coord_t>(this_x / fabs(dir_current_dim));
    // Full position
    auto pos = start + (dir * linePos);

    // Get box using midpoint as using boundary would be ambiguous
    auto middle = (pos + lastPos) * 0.5;
    box = this->data->getBoxAtCoord(middle.getBareArray());
    lastPos = pos;
    if (box != nullptr) {
      current_id = box->getID();
      // Make sure we get the last box
    } else if (i == num_boundaries) {
      current_id = std::numeric_limits<size_t>::max();
    } else
      continue;
    // If we haven't already a point for this box...
    // This filters out the extra boundaries that don't really exist that
    // we gained by assuming all boxes are the size of the smallest box
    if ((current_id != last_id && i != 1)) {
      // Check line position is within limits of the line and not too close to
      // previous position
      if (line_pos_of_box_centre >= 0 && line_pos_of_box_centre <= length && fabs(linePos - lastLinePos) > 1e-5) {
        mid_points.insert(line_pos_of_box_centre);
      }
      lastLinePos = previousLinePos;
    }
    line_pos_of_box_centre = static_cast<coord_t>((linePos + lastLinePos) * 0.5);
    previousLinePos = linePos;

    last_id = current_id;
  }
}

//-----------------------------------------------------------------------------------------------
/** Obtain coordinates for a line plot through a MDWorkspace.
 * Cross the workspace from start to end points, recording the signal along the
 * line halfway between each bin boundary that the line crosses
 *
 * @param start :: coordinates of the start point of the line
 * @param end :: coordinates of the end point of the line
 * @param normalize :: how to normalize the signal
 * @param x :: mid points between positions where the line crosses box
 *boundaries
 * @param y :: signal of the box in which corresponding x position lies
 * @param e :: error of the box in which corresponding x position lies
 */
TMDE(API::IMDWorkspace::LinePlot MDEventWorkspace)
::getLinePlot(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &end,
              Mantid::API::MDNormalization normalize) const {
  auto num_dims = this->getNumDims();
  if (start.getNumDims() != num_dims)
    throw std::runtime_error("Start point must have the same number of "
                             "dimensions as the workspace.");
  if (end.getNumDims() != num_dims)
    throw std::runtime_error("End point must have the same number of dimensions as the workspace.");

  // Unit-vector of the direction
  Mantid::Kernel::VMD dir = end - start;
  const auto length = dir.normalize();

  const std::set<coord_t> mid_points = getBoxBoundaryBisectsOnLine(start, end, num_dims, dir, length);

  LinePlot line;

  if (mid_points.empty()) {
    makeSinglePointWithNaN(line.x, line.y, line.e);
    return line;
  } else {

    for (const auto &line_pos : mid_points) {
      // This position in coordinates of the workspace is
      Mantid::Kernel::VMD ws_pos = start + (dir * line_pos);

      if (isInBounds(ws_pos.getBareArray())) {
        auto box = this->data->getBoxAtCoord(ws_pos.getBareArray());

        // If the box is not masked then record the signal and error here
        if (box && !box->getIsMasked()) {
          line.x.emplace_back(line_pos);
          signal_t signal = this->getNormalizedSignal(box, normalize);
          if (std::isinf(signal)) {
            // The plotting library (qwt) doesn't like infs.
            signal = std::numeric_limits<signal_t>::quiet_NaN();
          }
          line.y.emplace_back(signal);
          line.e.emplace_back(this->getNormalizedError(box, normalize));
        }
      }
    }
  }

  // If everything was masked
  if (line.x.empty()) {
    makeSinglePointWithNaN(line.x, line.y, line.e);
  }
  return line;
}

/**
Setter for the masking region.
@param maskingRegion : Implicit function defining mask region.
*/
TMDE(void MDEventWorkspace)::setMDMasking(std::unique_ptr<Mantid::Geometry::MDImplicitFunction> maskingRegion) {
  if (maskingRegion) {
    std::vector<API::IMDNode *> toMaskBoxes;

    // Apply new masks
    this->data->getBoxes(toMaskBoxes, 10000, true, maskingRegion.get());
    for (const auto box : toMaskBoxes) {
      box->clear();
      box->clearFileBacked(false);
      box->mask();
    }
  }
}

/**
Clears ALL existing masks off the workspace.
*/
TMDE(void MDEventWorkspace)::clearMDMasking() {
  std::vector<API::IMDNode *> allBoxes;
  // Clear old masks
  this->data->getBoxes(allBoxes, 10000, true);
  for (const auto box : allBoxes) {
    box->unmask();
  }
}

/**
Get the coordinate system (if any) to use.
@return An enumeration specifying the coordinate system if any.
*/
TMDE(Kernel::SpecialCoordinateSystem MDEventWorkspace)::getSpecialCoordinateSystem() const {
  MDFramesToSpecialCoordinateSystem converter;
  auto coordinatesFromMDFrames = converter(this);
  auto coordinates = m_coordSystem;

  if (coordinatesFromMDFrames) {
    coordinates = coordinatesFromMDFrames.value();
  }
  return coordinates;
}

/**
Set the coordinate system (if any) to use.
@param coordSystem : Coordinate system to use.
*/
TMDE(void MDEventWorkspace)::setCoordinateSystem(const Kernel::SpecialCoordinateSystem coordSystem) {
  m_coordSystem = coordSystem;
}

/**
  Set the display normalization for any subsequently generated histoworkspaces.
  @param preferredNormalization : Display normalization preference to pass on to
  generated histo workspaces.
*/
TMDE(void MDEventWorkspace)::setDisplayNormalizationHisto(
    const Mantid::API::MDNormalization preferredNormalizationHisto) {
  m_displayNormalizationHisto = preferredNormalizationHisto;
}

/**
Return the preferred normalization preference for subsequent histoworkspaces.
*/
TMDE(API::MDNormalization MDEventWorkspace)::displayNormalizationHisto() const { return m_displayNormalizationHisto; }

/**
  Set the display normalization
  @param preferredNormalization : Display normalization preference.
*/
TMDE(void MDEventWorkspace)::setDisplayNormalization(const Mantid::API::MDNormalization preferredNormalization) {
  m_displayNormalization = preferredNormalization;
}

/**
Return the preferred normalization to use for visualization.
*/
TMDE(API::MDNormalization MDEventWorkspace)::displayNormalization() const { return m_displayNormalization; }

} // namespace DataObjects

} // namespace Mantid
