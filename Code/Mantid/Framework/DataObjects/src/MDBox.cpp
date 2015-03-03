#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDBoxSaveable.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidDataObjects/MDGridBox.h"
#include <boost/math/special_functions/round.hpp>
#include <cmath>

using namespace Mantid::API;

namespace Mantid {
namespace DataObjects {

/**Destructor */
TMDE(MDBox)::~MDBox() {
  if (m_Saveable) {
    // tell disk buffer that there are no point of tracking this box any more.
    // BAD!!! TODO: make correct destructors order.
    if (this->m_BoxController) // it is destructor, in tests everything may fall
                               // apart, though it should not be issue for a
                               // worspace
    {
      if (this->m_BoxController->isFileBacked()) {
        this->m_BoxController->getFileIO()->objectDeleted(m_Saveable);
      }
    }
    delete m_Saveable;
  }
}
//-----------------------------------------------------------------------------------------------
/** Convenience Constructor/default constructor for accepting shared pointer
 * @param splitter :: shared pointer to BoxController that controls how boxes
 * split
 * @param depth :: splitting depth of the new box.
 * @param nBoxEvents :: number of events to reserve memory for.
 * @param boxID :: id for the given box
 */
TMDE(MDBox)::MDBox(API::BoxController_sptr &splitter, const uint32_t depth,
                   const size_t nBoxEvents, const size_t boxID)
    : MDBoxBase<MDE, nd>(splitter.get(), depth, boxID), m_Saveable(NULL),
      m_bIsMasked(false) {
  initMDBox(nBoxEvents);
}

//-----------------------------------------------------------------------------------------------
/** Constructor/default constructor
 * @param splitter :: BoxController that controls how boxes split
 * @param depth :: splitting depth of the new box.
 * @param nBoxEvents :: number of events to reserve memory for.
 * @param boxID :: id for the given box
 */
TMDE(MDBox)::MDBox(API::BoxController *const splitter, const uint32_t depth,
                   const size_t nBoxEvents, const size_t boxID)
    : MDBoxBase<MDE, nd>(splitter, depth, boxID), m_Saveable(NULL),
      m_bIsMasked(false) {
  initMDBox(nBoxEvents);
}

//-----------------------------------------------------------------------------------------------
/** Constructor
 * @param splitter :: BoxController that controls how boxes split
 * @param depth :: splitting depth of the new box.
 * @param extentsVector :: vector defining the extents of the box in all
 * n-dimensions
 * @param nBoxEvents :: number of events to reserve memory for.
 * @param boxID :: id for the given box
 */
TMDE(MDBox)::MDBox(
    BoxController_sptr &splitter, const uint32_t depth,
    const std::vector<
        Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector,
    const size_t nBoxEvents, const size_t boxID)
    : MDBoxBase<MDE, nd>(splitter.get(), depth, boxID, extentsVector),
      m_Saveable(NULL), m_bIsMasked(false) {
  initMDBox(nBoxEvents);
}
//-----------------------------------------------------------------------------------------------
/** Constructor
 * @param splitter :: BoxController that controls how boxes split
 * @param depth :: splitting depth of the new box.
 * @param extentsVector :: vector defining the extents
 * @param nBoxEvents :: Initial number of events to reserve memory for. If left
 * undefined, the memory will be alocated on request.
 * @param boxID :: id for the given box
 */
TMDE(MDBox)::MDBox(
    BoxController *const splitter, const uint32_t depth,
    const std::vector<
        Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector,
    const size_t nBoxEvents, const size_t boxID)
    : MDBoxBase<MDE, nd>(splitter, depth, boxID, extentsVector),
      m_Saveable(NULL), m_bIsMasked(false) {
  initMDBox(nBoxEvents);
}
/**Common part of MD box constructor */
TMDE(void MDBox)::initMDBox(const size_t nBoxEvents) {
  if (this->m_BoxController->getNDims() != nd)
    throw std::invalid_argument(
        "MDBox::ctor(): controller passed has the wrong number of dimensions.");

  if (nBoxEvents != UNDEF_SIZET)
    data.reserve(nBoxEvents);

  if (this->m_BoxController->isFileBacked())
    this->setFileBacked();
}

//-----------------------------------------------------------------------------------------------
/** Copy constructor
 * @param other: MDBox object to copy from.
 * @param otherBC - mandatory other box controller, which controls how this box
 * will split.
 */
TMDE(MDBox)::MDBox(const MDBox<MDE, nd> &other,
                   Mantid::API::BoxController *const otherBC)
    : MDBoxBase<MDE, nd>(other, otherBC), m_Saveable(NULL), data(other.data),
      m_bIsMasked(other.m_bIsMasked) {
  if (otherBC) // may be absent in some tests but generally have to be present
  {
    if (otherBC->isFileBacked())
      this->setFileBacked();
  }
}
// unhide MDBoxBase method
TMDE(size_t MDBox)::addEventsUnsafe(const std::vector<MDE> &events) {
  return MDBoxBase<MDE, nd>::addEventsUnsafe(events);
}

//-----------------------------------------------------------------------------------------------
/** Clear any points contained. */
TMDE(void MDBox)::clear() {
  // Make sure the object is not in any of the disk MRUs, and mark any space it
  // used as free
  // if (this->m_BoxController->useWriteBuffer())
  if (m_Saveable)
    this->m_BoxController->getFileIO()->objectDeleted(m_Saveable);

  // Clear all contents
  this->m_signal = 0.0;
  this->m_errorSquared = 0.0;

  this->clearDataFromMemory();
}

TMDE(Kernel::ISaveable *MDBox)::getISaveable() { return m_Saveable; }
TMDE(Kernel::ISaveable *MDBox)::getISaveable() const { return m_Saveable; }

//-----------------------------------------------------------------------------------------------
/** Clear the data[] vector ONLY but does not change the file-backed settings.
 * Used to free up the memory in a file-backed workspace without removing the
 * events from disk. */
TMDE(void MDBox)::clearDataFromMemory() {
  data.clear();
  vec_t().swap(data); // Linux trick to really free the memory
  // mark data unchanged
  if (m_Saveable) {
    m_Saveable->setLoaded(false);
    m_Saveable->setBusy(false);
    m_Saveable->clearDataChanged();
  }
}

//-----------------------------------------------------------------------------------------------
/** Returns the number of dimensions in this box */
TMDE(size_t MDBox)::getNumDims() const { return nd; }

//-----------------------------------------------------------------------------------------------
/** Returns the number of un-split MDBoxes in this box (including all children)
 * @return :: 1 always since this is just a MD Box*/
TMDE(size_t MDBox)::getNumMDBoxes() const { return 1; }

//-----------------------------------------------------------------------------------------------
/// Fill a vector with all the boxes up to a certain depth
TMDE(void MDBox)::getBoxes(std::vector<MDBoxBase<MDE, nd> *> &boxes,
                           size_t /*maxDepth*/, bool /*leafOnly*/) {
  boxes.push_back(this);
}
TMDE(void MDBox)::getBoxes(std::vector<API::IMDNode *> &boxes,
                           size_t /*maxDepth*/, bool /*leafOnly*/) {
  boxes.push_back(this);
}

//-----------------------------------------------------------------------------------------------
/// Fill a vector with all the boxes up to a certain depth
TMDE(void MDBox)::getBoxes(
    std::vector<MDBoxBase<MDE, nd> *> &boxes, size_t /*maxDepth*/,
    bool /*leafOnly*/, Mantid::Geometry::MDImplicitFunction * /*function*/) {
  boxes.push_back(this);
}
TMDE(void MDBox)::getBoxes(
    std::vector<API::IMDNode *> &boxes, size_t /*maxDepth*/, bool /*leafOnly*/,
    Mantid::Geometry::MDImplicitFunction * /*function*/) {
  boxes.push_back(this);
}

//-----------------------------------------------------------------------------------------------
/** Returns the total number of points (events) in this box either they are all
 * in memory, or on disk or partially on memory and partially on disk
 * for partially loaded object substantially relies on correct settings of
 * wasSaved and isLoaded switches of iSaveable object
*/
TMDE(uint64_t MDBox)::getNPoints() const {
  if (!m_Saveable)
    return data.size();

  if (m_Saveable->wasSaved()) {
    if (m_Saveable->isLoaded())
      return data.size();
    else // m_fileNumEvents
      return m_Saveable->getFileSize() + data.size();
  } else
    return data.size();
}

//-----------------------------------------------------------------------------------------------
/** Returns a reference to the events vector contained within.
 * VERY IMPORTANT: call MDBox::releaseEvents() when you are done accessing that
 * data.
 */
TMDE(std::vector<MDE> &MDBox)::getEvents() {
  if (!m_Saveable)
    return data;
  else {
    if (m_Saveable->wasSaved()) { // Load and concatenate the events if needed
      m_Saveable
          ->load(); // this will set isLoaded to true if not already loaded;
    }
    // The data vector is busy - can't release the memory yet
    m_Saveable->setBusy(true);
    // the non-const access to events assumes that the data will be modified;
    m_Saveable->setDataChanged();

    // Tell the to-write buffer to discard the object (when no longer busy) as
    // it has not been modified
    this->m_BoxController->getFileIO()->toWrite(m_Saveable);
    // else: do nothing if the events are already in memory.
    return data;
  }
}

TMDE(const std::vector<MDE> &MDBox)::getEvents() const {
  return getConstEvents();
}
//-----------------------------------------------------------------------------------------------
/** Returns a const reference to the events vector contained within.
 * VERY IMPORTANT: call MDBox::releaseEvents() when you are done accessing that
 * data.
 */
TMDE(const std::vector<MDE> &MDBox)::getConstEvents() const {
  if (!m_Saveable)
    return data;
  else {
    if (m_Saveable->wasSaved()) {
      // Load and concatenate the events if needed
      m_Saveable
          ->load(); // this will set isLoaded to true if not already loaded;
      // This access to data was const. Don't change the m_dataModified flag.
    }
    // The data vector is busy - can't release the memory yet
    m_Saveable->setBusy(true);

    // Tell the to-write buffer to discard the object (when no longer busy) as
    // it has not been modified
    this->m_BoxController->getFileIO()->toWrite(m_Saveable);
    // else: do nothing if the events are already in memory.
    return data;
  }
}

//-----------------------------------------------------------------------------------------------
/** For file-backed MDBoxes, this marks that the data vector is
 * no longer "busy", and so it is safe for the MRU to cache it
 * back to disk if needed.
 */
TMDE(void MDBox)::releaseEvents() {
  // Data vector is no longer busy.
  if (m_Saveable)
    m_Saveable->setBusy(false);
}

/** The method to convert events in a box into a table of
 * coodrinates/signal/errors casted into coord_t type
  *   Used to save events from plain binary file
  *   @returns coordTable -- vector of events parameters in the form signal,
 * error, [detID,rinId], eventsCoordinates....
  *   @return nColumns    -- number of parameters for each event
  */
TMDE(void MDBox)::getEventsData(std::vector<coord_t> &coordTable,
                                size_t &nColumns) const {
  double signal, errorSq;
  MDE::eventsToData(this->data, coordTable, nColumns, signal, errorSq);
  this->m_signal = static_cast<signal_t>(signal);
  this->m_errorSquared = static_cast<signal_t>(errorSq);

#ifdef MDBOX_TRACK_CENTROID
  this->calculateCentroid(this->m_centroid);
#endif
};
/** The method to convert the table of data into vector of events
 *   Used to load events from plain binary file
 *   @param coordTable -- vector of events parameters, which will be converted
 into events
                           signal error and coordinates
 */
TMDE(void MDBox)::setEventsData(const std::vector<coord_t> &coordTable) {
  MDE::dataToEvents(coordTable, this->data);
};

//-----------------------------------------------------------------------------------------------
/** Allocate and return a vector with a copy of all events contained
 */
TMDE(std::vector<MDE> *MDBox)::getEventsCopy() {
  if (m_Saveable) {
  }
  std::vector<MDE> *out = new std::vector<MDE>();
  // Make the copy
  out->insert(out->begin(), data.begin(), data.end());
  return out;
}

//-----------------------------------------------------------------------------------------------
/** Refresh the cache.
 *
 * For MDBox, if MDBOX_TRACK_SIGNAL_WHEN_ADDING is defined,
 * then the signal/error is tracked on adding, so
 * this does nothing.

 * beware, that it wrongly accumulates signal and error when part of the data is
 on file and
 * and some recent data were not saved to the HDD before adding new data
 * This actually means that refreshCache has to be called only once in events
 adding process
 */
TMDE(void MDBox)::refreshCache(Kernel::ThreadScheduler * /*ts*/) {

  // Use the cached value if it is on disk
  double signalSum(0);
  double errorSum(0);

  if (m_Saveable) {
    if (m_Saveable->wasSaved()) // There are possible problems with disk
                                // buffered events, as saving calculates
                                // averages and these averages has to be added
                                // to memory contents
    {
      if (!m_Saveable->isLoaded()) // events were saved,  averages calculated
                                   // and stored
      {
        // the partial data were not loaded from HDD but their averages should
        // be calculated when loaded. Add them
        signalSum = double(this->m_signal);
        errorSum = double(this->m_errorSquared);
      }
    }
  }
  // calculate all averages from memory
  typename std::vector<MDE>::const_iterator it_end = data.end();
  for (typename std::vector<MDE>::const_iterator it = data.begin();
       it != it_end; ++it) {
    const MDE &event = *it;
    // Convert floats to doubles to preserve precision when adding them.
    signalSum += static_cast<signal_t>(event.getSignal());
    errorSum += static_cast<signal_t>(event.getErrorSquared());
  }

  this->m_signal = signal_t(signalSum);
  this->m_errorSquared = signal_t(errorSum);
#ifdef MDBOX_TRACK_CENTROID
  this->calculateCentroid(this->m_centroid);
#endif

  /// TODO #4734: sum the individual weights of each event?
  this->m_totalWeight = static_cast<double>(this->getNPoints());
}
/// @return true if events were added to the box (using addEvent()) while the
/// rest of the event list is cached to disk
TMDE(bool MDBox)::isDataAdded() const {
  if (m_Saveable) {
    if (m_Saveable->isLoaded())
      return data.size() != m_Saveable->getFileSize();
  }
  return (data.size() != 0);
}

//-----------------------------------------------------------------------------------------------
/** Calculate the centroid of this box.
 * @param centroid [out] :: nd-sized array that will be set to the centroid.
 */
TMDE(void MDBox)::calculateCentroid(coord_t *centroid) const {
  for (size_t d = 0; d < nd; d++)
    centroid[d] = 0;

  // Signal was calculated before (when adding)
  // Keep 0.0 if the signal is null. This avoids dividing by 0.0
  if (this->m_signal == 0)
    return;

  typename std::vector<MDE>::const_iterator it_end = data.end();
  for (typename std::vector<MDE>::const_iterator it = data.begin();
       it != it_end; ++it) {
    const MDE &Evnt = *it;
    double signal = Evnt.getSignal();
    for (size_t d = 0; d < nd; d++) {
      // Total up the coordinate weighted by the signal.
      centroid[d] += Evnt.getCenter(d) * static_cast<coord_t>(signal);
    }
  }

  // Normalize by the total signal
  for (size_t d = 0; d < nd; d++) {
    centroid[d] /= coord_t(this->m_signal);
  }
}

//-----------------------------------------------------------------------------------------------
/** Calculate the statistics for each dimension of this MDBox, using
 * all the contained events
 * @param stats :: nd-sized fixed array of MDDimensionStats, reset to 0.0
 * before!
 */
TMDE(void MDBox)::calculateDimensionStats(MDDimensionStats *stats) const {
  typename std::vector<MDE>::const_iterator it_end = data.end();
  for (typename std::vector<MDE>::const_iterator it = data.begin();
       it != it_end; ++it) {
    const MDE &Evnt = *it;
    for (size_t d = 0; d < nd; d++) {
      stats[d].addPoint(Evnt.getCenter(d));
    }
  }
}

//-----------------------------------------------------------------------------------------------
/** Perform centerpoint binning of events.
 * @param bin :: MDBin object giving the limits of events to accept.
 * @param fullyContained :: optional bool array sized [nd] of which dimensions
 * are known to be fully contained (for MDSplitBox)
 */
TMDE(void MDBox)::centerpointBin(MDBin<MDE, nd> &bin,
                                 bool *fullyContained) const {
  if (fullyContained) {
    // For MDSplitBox, check if we've already found that all dimensions are
    // fully contained
    size_t d;
    for (d = 0; d < nd; ++d) {
      if (!fullyContained[d])
        break;
    }
    if (d == nd) {
      // All dimensions are fully contained, so just return the cached total
      // signal instead of counting.
      bin.m_signal += static_cast<signal_t>(this->m_signal);
      bin.m_errorSquared += static_cast<signal_t>(this->m_errorSquared);
      return;
    }
  }

  // If the box is cached to disk, you need to retrieve it
  const std::vector<MDE> &events = this->getConstEvents();
  typename std::vector<MDE>::const_iterator it = events.begin();
  typename std::vector<MDE>::const_iterator it_end = events.end();

  // For each MDLeanEvent
  for (; it != it_end; ++it) {
    // Go through each dimension
    size_t d;
    for (d = 0; d < nd; ++d) {
      // Check that the value is within the bounds given. (Rotation is for
      // later)
      coord_t x = it->getCenter(d);
      if (x < bin.m_min[d])
        break;
      if (x >= bin.m_max[d])
        break;
    }
    // If the loop reached the end, then it was all within bounds.
    if (d == nd) {
      // Accumulate error and signal (as doubles, to preserve precision)
      bin.m_signal += static_cast<signal_t>(it->getSignal());
      bin.m_errorSquared += static_cast<signal_t>(it->getErrorSquared());
    }
  }
  // it is constant access, so no saving or fiddling with the buffer is needed.
  // Events just can be dropped if necessary
  // releaseEvents
  if (m_Saveable)
    m_Saveable->setBusy(false);
}

//-----------------------------------------------------------------------------------------------
/** General (non-axis-aligned) centerpoint binning method.
 * TODO: TEST THIS!
 *
 * @param bin :: a MDBin object giving the limits, aligned with the axes of the
 *workspace,
 *        of where the non-aligned bin MIGHT be present.
 * @param function :: a ImplicitFunction that will evaluate true for any
 *coordinate that is
 *        contained within the (non-axis-aligned) bin.
 */
TMDE(void MDBox)::generalBin(
    MDBin<MDE, nd> &bin, Mantid::Geometry::MDImplicitFunction &function) const {
  UNUSED_ARG(bin);

  typename std::vector<MDE>::const_iterator it = data.begin();
  typename std::vector<MDE>::const_iterator it_end = data.end();
  // For each MDLeanEvent
  for (; it != it_end; ++it) {
    if (function.isPointContained(it->getCenter())) // HACK
    {
      // Accumulate error and signal
      bin.m_signal += static_cast<signal_t>(it->getSignal());
      bin.m_errorSquared += static_cast<signal_t>(it->getErrorSquared());
    }
  }
}

/** Integrate the signal within a sphere; for example, to perform single-crystal
 * peak integration.
 * The CoordTransform object could be used for more complex shapes, e.g.
 *"lentil" integration, as long
 * as it reduces the dimensions to a single value.
 *
 * @param radiusTransform :: nd-to-1 coordinate transformation that converts
 *from these
 *        dimensions to the distance (squared) from the center of the sphere.
 * @param radiusSquared :: radius^2 below which to integrate
 * @param[out] signal :: set to the integrated signal
 * @param[out] errorSquared :: set to the integrated squared error.
 */
TMDE(void MDBox)::integrateSphere(Mantid::API::CoordTransform &radiusTransform,
                                  const coord_t radiusSquared, signal_t &signal,
                                  signal_t &errorSquared) const {
  // If the box is cached to disk, you need to retrieve it
  const std::vector<MDE> &events = this->getConstEvents();
  typename std::vector<MDE>::const_iterator it = events.begin();
  typename std::vector<MDE>::const_iterator it_end = events.end();

  // For each MDLeanEvent
  for (; it != it_end; ++it) {
    coord_t out[nd];
    radiusTransform.apply(it->getCenter(), out);
    if (out[0] < radiusSquared) {
      signal += static_cast<signal_t>(it->getSignal());
      errorSquared += static_cast<signal_t>(it->getErrorSquared());
    }
  }
  // it is constant access, so no saving or fiddling with the buffer is needed.
  // Events just can be dropped if necessary
  // m_Saveable->releaseEvents();
  if (m_Saveable) {
    m_Saveable->setBusy(false);
  }
}

/** Integrate the signal within a sphere; for example, to perform single-crystal
 * peak integration.
 * The CoordTransform object could be used for more complex shapes, e.g.
 *"lentil" integration, as long
 * as it reduces the dimensions to a single value.
 *
 * @param radiusTransform :: nd-to-1 coordinate transformation that converts
 *from these
 *        dimensions to the distance (squared) from the center of the sphere.
 * @param radius :: radius below which to integrate
 * @param length :: length below which to integrate
 * @param[out] signal :: set to the integrated signal
 * @param[out] errorSquared :: set to the integrated squared error.
 * @param[out] signal_fit :: evaluation parameter on fit
 */
TMDE(void MDBox)::integrateCylinder(
    Mantid::API::CoordTransform &radiusTransform, const coord_t radius,
    const coord_t length, signal_t &signal, signal_t &errorSquared,
    std::vector<signal_t> &signal_fit) const {
  // If the box is cached to disk, you need to retrieve it
  const std::vector<MDE> &events = this->getConstEvents();
  typename std::vector<MDE>::const_iterator it = events.begin();
  typename std::vector<MDE>::const_iterator it_end = events.end();
  size_t numSteps = signal_fit.size();
  double deltaQ = length / static_cast<double>(numSteps - 1);

  // For each MDLeanEvent
  for (; it != it_end; ++it) {
    coord_t out[2]; // radius and length of cylinder
    radiusTransform.apply(it->getCenter(), out);
    if (out[0] < radius && std::fabs(out[1]) < 0.5 * length) {
      // add event to appropriate y channel
      size_t xchannel =
          static_cast<size_t>(std::floor(out[1] / deltaQ)) + numSteps / 2;
      if (xchannel < numSteps)
        signal_fit[xchannel] += static_cast<signal_t>(it->getSignal());

      signal += static_cast<signal_t>(it->getSignal());
      errorSquared += static_cast<signal_t>(it->getErrorSquared());
    }
  }
  // it is constant access, so no saving or fiddling with the buffer is needed.
  // Events just can be dropped if necessary
  // m_Saveable->releaseEvents();
  if (m_Saveable) {
    m_Saveable->setBusy(false);
  }
}

//-----------------------------------------------------------------------------------------------
/** Return the centroid of this box.
 */
TMDE(coord_t *MDBox)::getCentroid() const { return this->m_centroid; }

//-----------------------------------------------------------------------------------------------
/** Find the centroid of all events contained within by doing a weighted average
 * of their coordinates.
 *
 * @param radiusTransform :: nd-to-1 coordinate transformation that converts
 *from these
 *        dimensions to the distance (squared) from the center of the sphere.
 * @param radiusSquared :: radius^2 below which to integrate
 * @param[out] centroid :: array of size [nd]; its centroid will be added
 * @param[out] signal :: set to the integrated signal
 */
TMDE(void MDBox)::centroidSphere(Mantid::API::CoordTransform &radiusTransform,
                                 const coord_t radiusSquared, coord_t *centroid,
                                 signal_t &signal) const {
  // If the box is cached to disk, you need to retrieve it
  const std::vector<MDE> &events = this->getConstEvents();
  typename std::vector<MDE>::const_iterator it = events.begin();
  typename std::vector<MDE>::const_iterator it_end = events.end();

  // For each MDLeanEvent
  for (; it != it_end; ++it) {
    coord_t out[nd];
    radiusTransform.apply(it->getCenter(), out);
    if (out[0] < radiusSquared) {
      coord_t eventSignal = static_cast<coord_t>(it->getSignal());
      signal += signal_t(eventSignal);
      for (size_t d = 0; d < nd; d++)
        centroid[d] += it->getCenter(d) * eventSignal;
    }
  }
  // it is constant access, so no saving or fiddling with the buffer is needed.
  // Events just can be dropped if necessary
  if (m_Saveable)
    m_Saveable->setBusy(false);
}

//-----------------------------------------------------------------------------------------------
/** Transform the dimensions contained in this box
 * x' = x*scaling + offset.
 *
 * @param scaling :: multiply each coordinate by this value.
 * @param offset :: after multiplying, add this offset.
 */
TMDE(void MDBox)::transformDimensions(std::vector<double> &scaling,
                                      std::vector<double> &offset) {
  MDBoxBase<MDE, nd>::transformDimensions(scaling, offset);
  std::vector<MDE> &events = this->getEvents();
  typename std::vector<MDE>::iterator it;
  typename std::vector<MDE>::iterator it_end = events.end();
  for (it = events.begin(); it != it_end; ++it) {
    coord_t *center = it->getCenterNonConst();
    for (size_t d = 0; d < nd; d++)
      center[d] = (center[d] * static_cast<coord_t>(scaling[d])) +
                  static_cast<coord_t>(offset[d]);
  }
  if (m_Saveable)
    m_Saveable->setBusy(false);
}

/// Setter for masking the box
TMDE(void MDBox)::mask() { m_bIsMasked = true; }

/// Setter for unmasking the box
TMDE(void MDBox)::unmask() { m_bIsMasked = false; }
//------------------------------------------------------------------------------------------------------------------------------------------------------------

/** Create and Add several events. No bounds checking is made!
  *
  *@return number of events rejected (0 as nothing is rejected here)
  */
TMDE(size_t MDBox)::buildAndAddEvents(const std::vector<signal_t> &sigErrSq,
                                      const std::vector<coord_t> &Coord,
                                      const std::vector<uint16_t> &runIndex,
                                      const std::vector<uint32_t> &detectorId) {

  size_t nEvents = sigErrSq.size() / 2;
  size_t nExisiting = data.size();
  data.reserve(nExisiting + nEvents);
  this->m_dataMutex.lock();
  IF<MDE, nd>::EXEC(this->data, sigErrSq, Coord, runIndex, detectorId, nEvents);
  this->m_dataMutex.unlock();

  return 0;
}

/** Create event from the input data and add it to the box.
 * @param Signal  :: events signal
 * @param errorSq :: events Error squared
 * @param point :: reference to the  vector of MDEvent coordinates
 * @param runIndex  ::  run  index of the experiment the event come from
 * @param detectorId :: the ID of the  detector recoded the event
 * */
TMDE(void MDBox)::buildAndAddEvent(const signal_t Signal,
                                   const signal_t errorSq,
                                   const std::vector<coord_t> &point,
                                   uint16_t runIndex, uint32_t detectorId) {
  this->m_dataMutex.lock();
  this->data.push_back(IF<MDE, nd>::BUILD_EVENT(Signal, errorSq, &point[0],
                                                runIndex, detectorId));
  this->m_dataMutex.unlock();
}

//-----------------------------------------------------------------------------------------------
/**Create MDEvent and add it to the box, in a NON-THREAD-SAFE manner.
 * No lock is performed. This is only safe if no 2 threads will
 * try to add to the same box at the same time.
 *
 * @param Signal  :: events signal
 * @param errorSq :: events Error squared
 * @param point :: reference to the  vector of MDEvent coordinates
 * @param runIndex  ::  run  index of the experiment the event come from
 * @param detectorId :: the ID of the  detector recoded the event
 * */
TMDE(void MDBox)::buildAndAddEventUnsafe(const signal_t Signal,
                                         const signal_t errorSq,
                                         const std::vector<coord_t> &point,
                                         uint16_t runIndex,
                                         uint32_t detectorId) {
  this->data.push_back(IF<MDE, nd>::BUILD_EVENT(Signal, errorSq, &point[0],
                                                runIndex, detectorId));
}

//-----------------------------------------------------------------------------------------------
/** Add a MDLeanEvent to the box.
 * @param Evnt :: reference to a MDEvent to add.
 * */
TMDE(void MDBox)::addEvent(const MDE &Evnt) {
  this->m_dataMutex.lock();
  this->data.push_back(Evnt);

  this->m_dataMutex.unlock();
}

//-----------------------------------------------------------------------------------------------
/** Add a MDLeanEvent to the box, in a NON-THREAD-SAFE manner.
 * No lock is performed. This is only safe if no 2 threads will
 * try to add to the same box at the same time.
 *
 * @param Evnt :: reference to a MDEvent to add.
 * */
TMDE(void MDBox)::addEventUnsafe(const MDE &Evnt) {
  this->data.push_back(Evnt);
}

//-----------------------------------------------------------------------------------------------
/** Add Add all events . No bounds checking is made!
 *
 * @param events :: vector of events to be copied.
 *
 * @return the number of events that were rejected (because of being out of
 *bounds)
 */
TMDE(size_t MDBox)::addEvents(const std::vector<MDE> &events) {
  this->m_dataMutex.lock();
  typename std::vector<MDE>::const_iterator start = events.begin();
  typename std::vector<MDE>::const_iterator end = events.end();
  // Copy all the events
  this->data.insert(this->data.end(), start, end);

  this->m_dataMutex.unlock();
  return 0;
}

/**Make this box file-backed
* @param fileLocation -- the starting position of this box data are/should be
* located in the direct access file
* @param fileSize     -- the size this box data occupy in the file (in the units
* of the number of events)
* @param markSaved    -- set to true if the data indeed are physically there and
* one can indeed read then from there
*/
TMDE(void MDBox)::setFileBacked(const uint64_t fileLocation,
                                const size_t fileSize, const bool markSaved) {
  if (!m_Saveable)
    m_Saveable = new MDBoxSaveable(this);

  m_Saveable->setFilePosition(fileLocation, fileSize, markSaved);
}
/**Make this box file-backed but its place on the file is not identified yet. It
 * will be identified by the disk buffer */
TMDE(void MDBox)::setFileBacked() {
  if (!m_Saveable)
    this->setFileBacked(UNDEF_UINT64, this->getDataInMemorySize(), false);
}

/**Save the box data to specific disk position using the class, responsible for
  *the file IO.
  *
  *@param FileSaver -- the pointer to the class, responsible for File IO
  *operations
  *@param position  -- the position of the data within the class.
*/
TMDE(void MDBox)::saveAt(API::IBoxControllerIO *const FileSaver,
                         uint64_t position) const {
  if (data.empty())
    return;

  if (!FileSaver)
    throw(
        std::invalid_argument(" Needs defined file saver to save data to it"));
  if (!FileSaver->isOpened())
    throw(std::invalid_argument(
        " The data file has to be opened to use box SaveAt function"));

  std::vector<coord_t> TabledData;
  size_t nDataColumns;
  double totalSignal, totalErrSq;

  MDE::eventsToData(this->data, TabledData, nDataColumns, totalSignal,
                    totalErrSq);

  this->m_signal = static_cast<signal_t>(totalSignal);
  this->m_errorSquared = static_cast<signal_t>(totalErrSq);
#ifdef MDBOX_TRACK_CENTROID
  this->calculateCentroid(this->m_centroid);
#endif

  FileSaver->saveBlock(TabledData, position);
}

/**
 * Reserve all the memory required for loading in one step.
 *
 * @param size -- number of events to reserve for
 */
TMDE(void MDBox)::reserveMemoryForLoad(uint64_t size) {
  this->data.reserve(size);
}

/**Load the box data of specified size from the disk location provided using the
*class, respoinsible for the file IO and append them to exisiting events
 * Clear events vector first if overwriting the exisitng events is necessary.
 *
* @param FileSaver    -- the pointer to the class, responsible for file IO
* @param filePosition -- the place in the direct access file, where necessary
*data are located
* @param nEvents      -- number of events reqested to load
*/
TMDE(void MDBox)::loadAndAddFrom(API::IBoxControllerIO *const FileSaver,
                                 uint64_t filePosition, size_t nEvents) {
  if (nEvents == 0)
    return;

  if (!FileSaver)
    throw(std::invalid_argument(
        " Needs defined file saver to load data using it"));
  if (!FileSaver->isOpened())
    throw(std::invalid_argument(
        " The data file has to be opened to use box loadAndAddFrom function"));

  Poco::ScopedLock<Kernel::Mutex> _lock(this->m_dataMutex);

  std::vector<coord_t> TableData;
  FileSaver->loadBlock(TableData, filePosition, nEvents);

  // convert data to events appending new events to existing
  MDE::dataToEvents(TableData, data, false);
}
/** clear file-backed information from the box if such information exists
 *
 * @param loadDiskBackedData -- if true, load the data initially saved to HDD
 *before breaking connection between the file and memory
 *                              if false -- just forget about the data on the
 *HDD
 * not entirely fool-proof, as if the data is actually loaded is controlled by
 *isLoaded switch in ISaveable
 * and this switch has to be set up correctly
*/
TMDE(void MDBox)::clearFileBacked(bool loadDiskBackedData) {
  if (m_Saveable) {
    if (loadDiskBackedData)
      m_Saveable->load();
    // tell disk buffer that there are no point of tracking this box any more.
    this->m_BoxController->getFileIO()->objectDeleted(m_Saveable);
    delete m_Saveable;
    m_Saveable = NULL;
  }
}

} // namespace DataObjects

} // namespace Mantid
