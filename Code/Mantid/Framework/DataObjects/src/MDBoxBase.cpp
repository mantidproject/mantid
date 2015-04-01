#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include <limits>
#include <boost/make_shared.hpp>

using NeXus::File;

namespace Mantid {
namespace DataObjects {

//-----------------------------------------------------------------------------------------------
/** Default constructor.
 */
TMDE(MDBoxBase)::MDBoxBase(Mantid::API::BoxController *const boxController,
                           const uint32_t depth, const size_t boxID)
    : m_signal(0.0), m_errorSquared(0.0), m_totalWeight(0.0),
      m_BoxController(boxController),
      m_inverseVolume(std::numeric_limits<coord_t>::quiet_NaN()),
      m_depth(depth), m_parent(NULL), m_fileID(boxID) {
  if (boxController) {
    // Give it a fresh ID from the controller.
    if (boxID == std::numeric_limits<
                     size_t>::max()) // Give it a fresh ID from the controller.
      this->m_fileID = boxController->getNextId();
  }
}
//-----------------------------------------------------------------------------------------------
/** Constructor with extents
 */
TMDE(MDBoxBase)::MDBoxBase(
    Mantid::API::BoxController *const boxController, const uint32_t depth,
    const size_t boxID,
    const std::vector<
        Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector)
    : m_signal(0.0), m_errorSquared(0.0), m_totalWeight(0.0),
      m_BoxController(boxController), m_inverseVolume(UNDEF_COORDT),
      m_depth(depth), m_parent(NULL), m_fileID(boxID) {
  if (boxController) {
    // Give it a fresh ID from the controller.
    if (boxID == UNDEF_SIZET) // Give it a fresh ID from the controller.
      this->m_fileID = boxController->getNextId();
  }

  // Set the extents
  if (extentsVector.size() != nd)
    throw std::invalid_argument(
        "MDBoxBase::ctor(): extentsVector.size() must be == nd.");
  for (size_t d = 0; d < nd; d++)
    this->extents[d] = extentsVector[d];

  this->calcVolume();
}

//-----------------------------------------------------------------------------------------------
/** Copy constructor. Copies the extents, depth, etc.
 * and recalculates the boxes' volume.
 * @param box :: incoming box to copy.
 * @param otherBC :: if present, other (different from the current one) box
 * controller pointer
 */
TMDE(MDBoxBase)::MDBoxBase(const MDBoxBase<MDE, nd> &box,
                           Mantid::API::BoxController *const otherBC)
    : m_signal(box.m_signal), m_errorSquared(box.m_errorSquared),
      m_totalWeight(box.m_totalWeight), m_BoxController(otherBC),
      m_inverseVolume(box.m_inverseVolume), m_depth(box.m_depth),
      m_parent(box.m_parent), m_fileID(box.m_fileID) {

  // Copy the extents
  for (size_t d = 0; d < nd; d++)
    this->extents[d] = box.extents[d];
  for (size_t d = 0; d < nd; d++)
    this->m_centroid[d] = box.m_centroid[d];
}

//---------------------------------------------------------------------------------------------------
/** Transform the dimensions contained in this box
 * x' = x*scaling + offset
 *
 * @param scaling :: multiply each coordinate by this value.
 * @param offset :: after multiplying, add this offset.
 */
TMDE(void MDBoxBase)::transformDimensions(std::vector<double> &scaling,
                                          std::vector<double> &offset) {
  for (size_t d = 0; d < nd; d++) {
    extents[d].scaleExtents(scaling[d], offset[d]);
  }
  // Re-calculate the volume of the box
  this->calcVolume();
}

//-----------------------------------------------------------------------------------------------
/** Return the vertices of corners of the box
 *
 * @return a vector of VMD objects
 */
TMDE(std::vector<Mantid::Kernel::VMD> MDBoxBase)::getVertexes() const {
  if (nd > 4)
    throw std::runtime_error("MDBoxBase::getVertexes(): At this time, cannot "
                             "return vertexes for > 4 dimensions.");
  std::vector<Mantid::Kernel::VMD> out;

  // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd
  // bits
  size_t maxVertices = 1 << nd;

  // For each vertex, increase an integeer
  for (size_t i = 0; i < maxVertices; ++i) {
    // Coordinates of the vertex
    coord_t coords[nd];
    for (size_t d = 0; d < nd; d++) {
      // Use a bit mask to look at each bit of the integer we are iterating
      // through.
      size_t mask = 1 << d;
      if ((i & mask) > 0) {
        // Bit is 1, use the max of the dimension
        coords[d] = extents[d].getMax();
      } else {
        // Bit is 0, use the min of the dimension
        coords[d] = extents[d].getMin();
      }
    } // (for each dimension)

    // Create the coordinate object and add it to the vector
    out.push_back(Mantid::Kernel::VMD(nd, coords));
  }

  return out;
}

//-----------------------------------------------------------------------------------------------
/** Return the vertices of every corner of the box, but as
 * a bare array of length numVertices * nd
 *
 * @param[out] numVertices :: returns the number of vertices in the array.
 * @return the bare array. This should be deleted by the caller!
 * */
TMDE(coord_t *MDBoxBase)::getVertexesArray(size_t &numVertices) const {
  // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd
  // bits
  numVertices = 1 << nd;

  // Allocate the array of the right size
  coord_t *out = new coord_t[nd * numVertices];

  // For each vertex, increase an integeer
  for (size_t i = 0; i < numVertices; ++i) {
    // Start point index in the output array;
    size_t outIndex = i * nd;

    // Coordinates of the vertex
    for (size_t d = 0; d < nd; d++) {
      // Use a bit mask to look at each bit of the integer we are iterating
      // through.
      size_t mask = 1 << d;
      if ((i & mask) > 0) {
        // Bit is 1, use the max of the dimension
        out[outIndex + d] = extents[d].getMax();
      } else {
        // Bit is 0, use the min of the dimension
        out[outIndex + d] = extents[d].getMin();
      }
    } // (for each dimension)
  }
  return out;
}

//-----------------------------------------------------------------------------------------------
/** Return the vertices of every corner of the box, but to a reduced
 * number of dimensions.
 *
 * Essentially, this is a projection of the vertexes from nd down to a lower
 * (or equal) number of dimensions. Redundant vertexes are NOT included.
 *
 * @param[out] numVertices :: returns the number of vertices in the array.
 * @param outDimensions :: the number of dimensions in the output array (must be
 *> 0).
 * @param maskDim :: nd-sized array of a mask, with TRUE for the dimensions to
 *KEEP.
 *        The number of TRUEs in maskDim MUST be equal to outDimensions, and
 *this is
 *        not checked for here (for performance).
 * @return the bare array, size of outDimensions. This should be deleted by the
 *caller!
 * @throw if outDimensions == 0
 * */
TMDE(coord_t *MDBoxBase)::getVertexesArray(size_t &numVertices,
                                           const size_t outDimensions,
                                           const bool *maskDim) const {
  if (outDimensions == 0)
    throw std::invalid_argument(
        "MDBoxBase::getVertexesArray(): Must have > 0 output dimensions.");

  // How many vertices does one box have? 2^numOutputDimensions
  numVertices = (size_t)1 << outDimensions;

  // Allocate the array of the right size
  coord_t *out = new coord_t[outDimensions * numVertices];

  // For each vertex, increase an integeer
  for (size_t i = 0; i < numVertices; ++i) {
    // Start point index in the output array;
    size_t outIndex = i * outDimensions;

    // The OUTPUT dimension index
    size_t outd = 0;

    // Coordinates of the vertex
    for (size_t ind = 0; ind < nd; ind++) {
      if (maskDim[ind]) {
        // Use a bit mask to look at each bit of the integer we are iterating
        // through.
        size_t mask = (size_t)1 << outd;
        if ((i & mask) > 0) {
          // Bit is 1, use the max of the dimension
          out[outIndex + outd] = extents[ind].getMax();
        } else {
          // Bit is 0, use the min of the dimension
          out[outIndex + outd] = extents[ind].getMin();
        }
        outd++;
      } // the dimensions is used in the output.
    }   // (for each INPUT dimension)
  }
  return out;
}

//-----------------------------------------------------------------------------------------------
/** Add several events, starting and stopping at particular point in a vector.
 * Bounds checking IS performed, and events outside the range are rejected.
 *
 * NOTE: You must call refreshCache() after you are done, to calculate the
 *  nPoints, signal and error.
 *
 * @param events :: vector of events to be copied.
 * @return the number of events that were rejected (because of being out of
 *bounds)
 */
TMDE(size_t MDBoxBase)::addEvents(const std::vector<MDE> &events) {
  size_t numBad = 0;
  // --- Go event by event and add them ----
  typename std::vector<MDE>::const_iterator it = events.begin();
  typename std::vector<MDE>::const_iterator it_end = events.end();
  m_dataMutex.lock();
  for (; it != it_end; ++it) {
    // Check out-of-bounds-ness
    bool badEvent = false;
    for (size_t d = 0; d < nd; d++) {
      coord_t x = it->getCenter(d);
      if (extents[d].outside(x)) {
        badEvent = true;
        break;
      }
    }

    if (badEvent)
      // Event was out of bounds. Count it
      ++numBad;
    else
      // Event was in bounds; add it
      addEventUnsafe(*it);
  }
  m_dataMutex.unlock();
  return numBad;
}

//---------------------------------------------------------------------------------------------------
/** Add all of the events contained in a vector, with:
 * - No bounds checking.
 * - No thread-safety.
 *
 * @param events :: Vector of MDEvent
 */
TMDE(size_t MDBoxBase)::addEventsUnsafe(const std::vector<MDE> &events) {
  // --- Go event by event and add them ----
  typename std::vector<MDE>::const_iterator it = events.begin();
  typename std::vector<MDE>::const_iterator it_end = events.end();
  for (; it != it_end; ++it) {
    // Check out-of-bounds-ness
    // Event was in bounds; add it
    addEventUnsafe(*it);
  }

  return 0;
}

} // namespace Mantid
} // namespace DataObjects
