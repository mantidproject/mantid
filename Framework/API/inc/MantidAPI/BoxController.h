// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IBoxControllerIO.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <numeric>
#include <optional>
#include <vector>

namespace Mantid {
namespace API {

/** This class is used by MDBox and MDGridBox in order to intelligently
 * determine optimal behavior. It informs:
 *  - When a MDBox needs to split into a MDGridBox.
 *  - How the splitting will occur.
 *  - When a MDGridBox should use Tasks to parallelize adding events
 *
 * @author Janik Zikovsky
 * @date Feb 21, 2011
 */
class MANTID_API_DLL BoxController {
public:
  //-----------------------------------------------------------------------------------
  /** Constructor
   *
   * @param nd :: number of dimensions
   */
  BoxController(size_t nd)
      : nd(nd), m_maxId(0), m_SplitThreshold(1024), m_splitTopInto(std::nullopt), m_numSplit(1), m_numTopSplit(1),
        m_fileIO(std::shared_ptr<API::IBoxControllerIO>()) {
    // TODO: Smarter ways to determine all of these values
    m_maxDepth = 5;
    m_numEventsAtMax = 0;
    m_addingEvents_eventsPerTask = 1000;
    m_significantEventsNumber = 10000000;
    m_addingEvents_numTasksPerBlock = Kernel::ThreadPool::getNumPhysicalCores() * 5;
    m_splitInto.resize(this->nd, 1);
    resetNumBoxes();
  }

  virtual ~BoxController();
  // create new box controller from the existing one
  virtual BoxController *clone() const;
  /// Serialize
  std::string toXMLString() const;

  /// De-serializing XML
  void fromXMLString(const std::string &xml);

  /// Equality operator
  bool operator==(const BoxController &other) const;

  //-----------------------------------------------------------------------------------
  /** Get # of dimensions
   * @return # of dimensions
   */
  size_t getNDims() const { return nd; }

  //-----------------------------------------------------------------------------------
  /** @return the next available box Id.
   * Call when creating a MDBox to give it an ID. */
  size_t getNextId() { return m_maxId++; }

  //-----------------------------------------------------------------------------------
  /** @return the maximum (not-inclusive) ID number anywhere in the workspace.
   */
  size_t getMaxId() const { return m_maxId; }

  //-----------------------------------------------------------------------------------
  /** Set the new maximum ID number anywhere in the workspace.
   * Should only be called when loading a file.
   * @param newMaxId value to set the newMaxId to
   */
  void setMaxId(size_t newMaxId) { m_maxId = newMaxId; }

  //-----------------------------------------------------------------------------------
  /** @return the mutex for avoiding simultaneous assignments of box Ids. */
  inline std::mutex &getIdMutex() { return m_idMutex; }

  //-----------------------------------------------------------------------------------
  /** Return true if the MDBox should split, given :
   *
   * @param numPoints :: # of points in the box
   * @param depth :: recursion depth of the box
   * @return bool, true if it should split
   */
  bool willSplit(size_t numPoints, size_t depth) const {
    return (numPoints > m_SplitThreshold) && (depth < m_maxDepth);
  }

  //-----------------------------------------------------------------------------------
  /** Return the splitting threshold, in # of events */
  size_t getSplitThreshold() const { return m_SplitThreshold; }

  /** Set the splitting threshold
   * @param threshold :: # of points at which the MDBox splits
   */
  void setSplitThreshold(size_t threshold) { m_SplitThreshold = threshold; }

  //-----------------------------------------------------------------------------------
  /** Return into how many to split along a dimension
   *
   * @param dim :: index of the dimension to split
   * @return the dimension will be split into this many even boxes.
   */
  size_t getSplitInto(size_t dim) const {
    //      if (dim >= nd)
    //        throw std::invalid_argument("BoxController::setSplitInto() called
    //        with too high of a dimension index.");
    return m_splitInto[dim];
  }

  //-----------------------------------------------------------------------------------
  /** Return into how many to split along a every dimension
   *
   * @return the dimension will be split into this many even boxes,
   * for every dimension.
   */
  const std::vector<size_t> &getSplitIntoAll() const { return m_splitInto; }
  //-----------------------------------------------------------------------------------
  /** Return into how many to split along a dimension for the top level
   *
   * @return the splits in each dimesion for the top level
   */
  std::optional<std::vector<size_t>> getSplitTopInto() const {
    //      if (dim >= nd)
    //        throw std::invalid_argument("BoxController::setSplitInto() called
    //        with too high of a dimension index.");
    return m_splitTopInto;
  }

  /// Return how many boxes (total) a MDGridBox will contain.
  size_t getNumSplit() const { return m_numSplit; }

  //-----------------------------------------------------------------------------------
  /** Set the way splitting will be done
   * @param num :: amount in which to split
   * */
  void setSplitInto(size_t num) {
    m_splitInto.clear();
    m_splitInto.resize(nd, num);
    calcNumSplit();
  }

  //-----------------------------------------------------------------------------------
  /** Set the way splitting will be done
   *
   * @param dim :: dimension to set
   * @param num :: amount in which to split
   */
  void setSplitInto(size_t dim, size_t num) {
    if (dim >= nd)
      throw std::invalid_argument("BoxController::setSplitInto() called with "
                                  "too high of a dimension index.");
    m_splitInto[dim] = num;
    calcNumSplit();
  }

  //-----------------------------------------------------------------------------------
  /** Set the way splitting will be done for the top level
   *
   * @param dim :: dimension to set
   * @param num :: amount in which to split
   */
  void setSplitTopInto(size_t dim, size_t num) {
    if (dim >= nd)
      throw std::invalid_argument("BoxController::setSplitTopInto() called with "
                                  "too high of a dimension index.");
    // If the vector is not created, then create it
    if (!m_splitTopInto) {
      m_splitTopInto = std::vector<size_t>(nd, 1);
    }
    m_splitTopInto.value()[dim] = num;
    calcNumTopSplit();
  }

  //-----------------------------------------------------------------------------------
  /** When adding events, how many events per task should be done?
   *
   * @param m_addingEvents_eventsPerTask :: events per task
   */
  void setAddingEvents_eventsPerTask(size_t m_addingEvents_eventsPerTask) {
    this->m_addingEvents_eventsPerTask = m_addingEvents_eventsPerTask;
  }
  /// @return When adding events, how many events per task should be done?
  size_t getAddingEvents_eventsPerTask() const { return m_addingEvents_eventsPerTask; }

  /** When adding events, how many events tasks per block should be done?
   *
   * @param m_addingEvents_numTasksPerBlock :: tasks/block
   */
  void setAddingEvents_numTasksPerBlock(size_t m_addingEvents_numTasksPerBlock) {
    this->m_addingEvents_numTasksPerBlock = m_addingEvents_numTasksPerBlock;
  }

  /// @return When adding events, how many tasks per block should be done?
  size_t getAddingEvents_numTasksPerBlock() const { return m_addingEvents_numTasksPerBlock; }

  //-----------------------------------------------------------------------------------
  /** Get parameters for adding events to a MDGridBox, trying to optimize
   *parallel CPU use.
   *
   * @param[out] eventsPerTask :: the number of events that should be added by a
   *single task object.
   *    This should be large enough to avoid overhead without being
   *    too large, making event lists too long before splitting
   * @param[out] numTasksPerBlock :: the number of tasks (of size eventsPerTask)
   *to be allocated
   *    before the grid boxes should be re-split. Having enough parallel tasks
   *will
   *    help the CPU be used fully.
   */
  void getAddingEventsParameters(size_t &eventsPerTask, size_t &numTasksPerBlock) const {
    // TODO: Smarter values here depending on nd, etc.
    eventsPerTask = m_addingEvents_eventsPerTask;
    numTasksPerBlock = m_addingEvents_numTasksPerBlock;
  }

  //-----------------------------------------------------------------------------------
  /** @return the max recursion depth allowed for grid box splitting. */
  size_t getMaxDepth() const { return m_maxDepth; }

  /** Sets the max recursion depth allowed for grid box splitting.
   * NOTE! This resets numMDBoxes stats!
   * @param value :: the max depth for splitting
   *  */
  void setMaxDepth(size_t value) {
    m_maxDepth = value;
    resetNumBoxes();
  }

  /// The number of events that triggers box splitting
  size_t getSignificantEventsNumber() const { return m_significantEventsNumber; }

  //-----------------------------------------------------------------------------------
  /** Determine when would be a good time to split MDBoxes into MDGridBoxes.
   * This is to be called while adding events. Splitting boxes too frequently
   * would be a slow-down, but keeping the boxes split at an earlier stage
   * should help scalability for later adding, so there is a balance to get.
   *
   * @param nEventsInOutput :: How many events are currently in workspace in
   *memory;
   * @param eventsAdded     :: How many events were added since the last split?
   * @param numMDBoxes      :: How many un-split MDBoxes are there (total) in
   *the workspace
   * @return true if the boxes should get split.
   */
  bool shouldSplitBoxes(size_t nEventsInOutput, size_t eventsAdded, size_t numMDBoxes) const {
    // Avoid divide by zero
    if (numMDBoxes == 0)
      return false;
    // Performance depends pretty strongly on WHEN you split the boxes.
    // This is an empirically-determined way to optimize the splitting calls.
    // Split when adding 1/16^th as many events as are already in the output,
    //  (because when the workspace gets very large you should split less often)
    // But no more often than every 10 million events.
    const size_t comparisonPoint = std::max(nEventsInOutput / 16, m_significantEventsNumber);
    if (eventsAdded > comparisonPoint)
      return true;

    // Return true if the average # of events per box is big enough to split.
    return ((eventsAdded / numMDBoxes) > m_SplitThreshold);
  }
  //-----------------------------------------------------------------------------------

  void clearBoxesCounter(size_t depth) {
    std::lock_guard<std::mutex> lock(m_mutexNumMDBoxes);
    m_numMDBoxes[depth] = 0;
  }

  void clearGridBoxesCounter(size_t depth) {
    std::lock_guard<std::mutex> lock(m_mutexNumMDBoxes);
    m_numMDGridBoxes[depth] = 0;
  }
  void incGridBoxesCounter(size_t depth, size_t inc = 1) {
    std::lock_guard<std::mutex> lock(m_mutexNumMDBoxes);
    m_numMDGridBoxes[depth] += inc;
  }

  void incBoxesCounter(size_t depth, size_t inc = 1) {
    std::lock_guard<std::mutex> lock(m_mutexNumMDBoxes);
    m_numMDBoxes[depth] += inc;
  }

  /** Call to track the number of MDBoxes are contained in the MDEventWorkspace
   * This should be called when a MDBox gets split into a MDGridBox.
   * The number of MDBoxes at [depth] is reduced by one
   * The number of MDBoxes at [depth+1] is increased by however many the
   *splitting gives.
   * Also tracks the number of MDGridBoxes.
   *
   * @param depth :: the depth of the MDBox that is being split into MDGrid
   *boxes.
   */
  void trackNumBoxes(size_t depth) {
    std::lock_guard<std::mutex> lock(m_mutexNumMDBoxes);
    if (m_numMDBoxes[depth] > 0) {
      m_numMDBoxes[depth]--;
    }
    m_numMDGridBoxes[depth]++;

    // We need to account for optional top level splitting
    if (depth == 0 && m_splitTopInto) {

      const auto &splitTopInto = m_splitTopInto.value();
      size_t numSplitTop =
          std::accumulate(splitTopInto.cbegin(), splitTopInto.cend(), size_t{1}, std::multiplies<size_t>());
      m_numMDBoxes[depth + 1] += numSplitTop;
    } else {
      m_numMDBoxes[depth + 1] += m_numSplit;
    }
  }

  /** Return the vector giving the number of MD Boxes as a function of depth */
  const std::vector<size_t> &getNumMDBoxes() const { return m_numMDBoxes; }

  /** Return the vector giving the number of MD Grid Boxes as a function of
   * depth */
  const std::vector<size_t> &getNumMDGridBoxes() const { return m_numMDGridBoxes; }

  /** Return the vector giving the MAXIMUM number of MD Boxes as a function of
   * depth */
  const std::vector<double> &getMaxNumMDBoxes() const { return m_maxNumMDBoxes; }

  /** Return the total number of MD Boxes, irrespective of depth */
  size_t getTotalNumMDBoxes() const {
    return std::accumulate(m_numMDBoxes.cbegin(), m_numMDBoxes.cend(), size_t{0}, std::plus<size_t>());
  }

  /** Return the total number of MDGridBox'es, irrespective of depth */
  size_t getTotalNumMDGridBoxes() const {
    return std::accumulate(m_numMDGridBoxes.cbegin(), m_numMDGridBoxes.cend(), size_t{0}, std::plus<size_t>());
  }

  /** Return the average recursion depth of gridding.
   * */
  double getAverageDepth() const {
    double total = 0;
    double maxNumberOfFinestBoxes = m_maxNumMDBoxes.back();
    for (size_t depth = 0; depth < m_numMDBoxes.size(); depth++) {
      // Add up the number of MDBoxes at that depth, weighed by their volume in
      // units of the volume of the finest possible box.
      // I.e. a box at level 1 is 100 x bigger than a box at level 2, so it
      // counts 100x more.
      total += double(depth * m_numMDBoxes[depth]) * (maxNumberOfFinestBoxes / m_maxNumMDBoxes[depth]);
    }
    return total / maxNumberOfFinestBoxes;
  }

  /** Reset the number of boxes tracked in m_numMDBoxes */
  void resetNumBoxes() {
    std::lock_guard<std::mutex> lock(m_mutexNumMDBoxes);
    m_numMDBoxes.clear();
    m_numMDBoxes.resize(m_maxDepth + 1, 0);     // Reset to 0
    m_numMDGridBoxes.resize(m_maxDepth + 1, 0); // Reset to 0
    m_numMDBoxes[0] = 1;                        // Start at 1 at depth 0.
    resetMaxNumBoxes();                         // Also the maximums
  }

  // { return m_useWriteBuffer; }
  /// Returns if current box controller is file backed. Assumes that
  /// BC(workspace) is fileBackd if fileIO is defined;
  bool isFileBacked() const { return bool(m_fileIO); }
  /// returns the pointer to the class, responsible for fileIO operations;
  IBoxControllerIO *getFileIO() { return m_fileIO.get(); }
  /// makes box controller file based by providing class, responsible for
  /// fileIO.
  void setFileBacked(const std::shared_ptr<IBoxControllerIO> &newFileIO, const std::string &fileName = "");
  void clearFileBacked();
  //-----------------------------------------------------------------------------------
  // BoxCtrlChangesInterface *getChangesList(){return m_ChangesList;}
  // void setChangesList(BoxCtrlChangesInterface *pl){m_ChangesList=pl;}
  //-----------------------------------------------------------------------------------
  // increase the counter, calculatinb events at max;
  void rizeEventAtMax() { ++m_numEventsAtMax; }
  /// return the numner of events, which are sitting at max depth and would be
  /// split if not due to the max depth of the box they are occupying
  size_t getNumEventAtMax() const { return m_numEventsAtMax; }
  /// get range of id-s and increment box ID by this range;
  size_t claimIDRange(size_t range);

  /// the function left for compartibility with the previous bc python
  /// interface.
  std::string getFilename() const;
  /// the compartibility function -- the write buffer is always used for file
  /// based workspaces
  bool useWriteBuffer() const;

private:
  /// When you split a MDBox, it becomes this many sub-boxes
  void calcNumSplit() {
    m_numSplit = 1;
    for (size_t d = 0; d < nd; d++) {
      m_numSplit *= m_splitInto[d];
    }
    /// And this changes the max # of boxes too
    resetMaxNumBoxes();
  }

  /// When you split an MDBox by force, it becomes this many sub boxes
  void calcNumTopSplit() {
    m_numTopSplit = 1;
    for (size_t d = 0; d < nd; d++) {
      m_numTopSplit *= m_splitTopInto.value()[d];
    }
    /// And this changes the max # of boxes too
    resetMaxNumBoxes();
  }

  /// Calculate the vector of the max # of MDBoxes per level.
  void resetMaxNumBoxes() {
    // Now calculate the max # of boxes
    m_maxNumMDBoxes.resize(m_maxDepth + 1, 0); // Reset to 0
    m_maxNumMDBoxes[0] = 1;
    for (size_t depth = 1; depth < m_maxNumMDBoxes.size(); depth++) {
      if (depth == 1 && m_splitTopInto) {
        m_maxNumMDBoxes[depth] = m_maxNumMDBoxes[depth - 1] * double(m_numTopSplit);
      } else {
        m_maxNumMDBoxes[depth] = m_maxNumMDBoxes[depth - 1] * double(m_numSplit);
      }
    }
  }

protected:
  /// box controller is an ws-based singleton so it should not be possible to
  /// copy it, left protected for inheritance;
  BoxController(const BoxController &other);
  BoxController &operator=(const BoxController &) = delete;

private:
  /// Number of dimensions
  size_t nd;

  /** The maximum ID number of any boxes in the workspace (not inclusive,
   * i.e. maxId = 100 means there the highest ID number is 99.  */
  size_t m_maxId;

  /// Splitting threshold
  size_t m_SplitThreshold;

  /// This empirically-determined number of events takes a noticeable time to
  /// process and triggers box splitting.
  size_t m_significantEventsNumber;

  /** Maximum splitting depth: don't go further than this many levels of
   *recursion.
   * This avoids infinite recursion and should be set to a value that gives a
   *smallest
   * box size that is a little smaller than the finest desired binning upon
   *viewing.
   *
   *RE: In fact, max depth should not be higher then
   *EPSILON(coord_t/(BIGGEST_BOX_SIZE))=max_split_size^m_maxDepth;
   */
  size_t m_maxDepth;
  /// number of events sitting in the boxes which should be split but are
  /// already split up to the max depth
  volatile size_t m_numEventsAtMax;

  /// Splitting # for all dimensions
  std::vector<size_t> m_splitInto;

  /// Splittin # for all dimensions in the top level
  std::optional<std::vector<size_t>> m_splitTopInto;

  /// When you split a MDBox, it becomes this many sub-boxes
  size_t m_numSplit;

  /// When you split a top level MDBox by force, it becomes this many sub boxes
  size_t m_numTopSplit;

  /// For adding events tasks
  size_t m_addingEvents_eventsPerTask;

  /// For adding events tasks
  size_t m_addingEvents_numTasksPerBlock;

  /// For tracking how many MDBoxes (not MDGridBoxes) are at each recursion
  /// level
  std::vector<size_t> m_numMDBoxes;

  /// For tracking how many MDGridBoxes (not MDBoxes) are at each recursion
  /// level
  std::vector<size_t> m_numMDGridBoxes;

  /// Mutex for changing the number of MD Boxes.
  std::mutex m_mutexNumMDBoxes;

  /// This is the maximum number of MD boxes there could be at each recursion
  /// level (e.g. (splitInto ^ ndims) ^ depth )
  std::vector<double> m_maxNumMDBoxes;

  /// Mutex for getting IDs
  std::mutex m_idMutex;

  // the class which does actual IO operations, including MRU support list
  std::shared_ptr<IBoxControllerIO> m_fileIO;

  /// Number of bytes in a single MDLeanEvent<> of the workspace.
  // size_t m_bytesPerEvent;
public:
};

/// Shared ptr to BoxController
using BoxController_sptr = std::shared_ptr<BoxController>;

/// Shared ptr to a const BoxController
using BoxController_const_sptr = std::shared_ptr<const BoxController>;

} // namespace API

} // namespace Mantid
