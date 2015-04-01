#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidKernel/Utils.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

namespace Mantid {
namespace DataObjects {
namespace {
size_t integerPower(const size_t base, const size_t pow) {
  size_t result = 1;
  for (size_t i = 0; i < pow; ++i) {
    result *= base;
  }
  return result;
}
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param workspace :: MDHistoWorkspace_sptr being iterated
 * @param function :: The implicit function to use. Becomes owned by this
 *object.
 * @return
 */

//----------------------------------------------------------------------------------------------
/**
 * Constructor
 * @param workspace :: MDHistoWorkspace_sptr being iterated
 * @param function :: The implicit function to use. Becomes owned by this
 * object.
 * @param beginPos :: start position
 * @param endPos :: end position
 */
MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(
    MDHistoWorkspace_const_sptr workspace,
    Mantid::Geometry::MDImplicitFunction *function, size_t beginPos,
    size_t endPos)
    : m_skippingPolicy(new SkipMaskedBins(this)) {
  this->init(workspace.get(), function, beginPos, endPos);
}

/**
 * Constructor
 * @param workspace :: MDHistoWorkspace_sptr being iterated
 * @param function :: The implicit function to use. Becomes owned by this
 * object.
 * @param beginPos
 * @param endPos
 */
MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(
    const MDHistoWorkspace *workspace,
    Mantid::Geometry::MDImplicitFunction *function, size_t beginPos,
    size_t endPos)
    : m_skippingPolicy(new SkipMaskedBins(this)) {
  this->init(workspace, function, beginPos, endPos);
}

/**
 * Constructor
 * @param workspace :: MDHistoWorkspace_sptr being iterated
 * @param skippingPolicy :: The skipping policy to use.
 * @param function :: The implicit function to use. Becomes owned by this
 * object.
 * @param beginPos :: Start position
 * @param endPos :: End position
 */
MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(
    MDHistoWorkspace_const_sptr workspace, SkippingPolicy *skippingPolicy,
    Mantid::Geometry::MDImplicitFunction *function, size_t beginPos,
    size_t endPos)
    : m_skippingPolicy(skippingPolicy) {
  this->init(workspace.get(), function, beginPos, endPos);
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param workspace :: MDHistoWorkspace_sptr being iterated
 * @param function :: The implicit function to use. Becomes owned by this
 *object.
 * @param skippingPolicy :: The skipping policy to use
 * @param beginPos :: Start position
 * @param endPos :: End position
 * @return
 */
MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(
    const MDHistoWorkspace *workspace, SkippingPolicy *skippingPolicy,
    Mantid::Geometry::MDImplicitFunction *function, size_t beginPos,
    size_t endPos)
    : m_skippingPolicy(skippingPolicy) {
  this->init(workspace, function, beginPos, endPos);
}

/**
 * Constructor helper
 * @param workspace :: MDWorkspace
 * @param function :: implicit function or NULL for none. Gains ownership of the
 * pointer.
 * @param beginPos :: Start position
 * @param endPos :: End position
 */
void
MDHistoWorkspaceIterator::init(const MDHistoWorkspace *workspace,
                               Mantid::Geometry::MDImplicitFunction *function,
                               size_t beginPos, size_t endPos) {
  m_ws = workspace;
  if (m_ws == NULL)
    throw std::invalid_argument(
        "MDHistoWorkspaceIterator::ctor(): NULL workspace given.");

  m_begin = beginPos;
  m_pos = m_begin;
  m_function = function;

  m_max = endPos;
  if (m_max > m_ws->getNPoints())
    m_max = m_ws->getNPoints();
  if (m_max < m_pos)
    throw std::invalid_argument("MDHistoWorkspaceIterator::ctor(): End point "
                                "given is before the start point.");

  m_nd = m_ws->getNumDims();
  m_center = new coord_t[m_nd];
  m_origin = new coord_t[m_nd];
  m_binWidth = new coord_t[m_nd];
  m_index = new size_t[m_nd];
  m_indexMax = new size_t[m_nd];
  m_indexMaker = new size_t[m_nd];
  Utils::NestedForLoop::SetUp(m_nd, m_index, 0);
  // Initalize all these values
  for (size_t d = 0; d < m_nd; d++) {
    IMDDimension_const_sptr dim = m_ws->getDimension(d);
    m_center[d] = 0;
    m_origin[d] = dim->getMinimum();
    m_binWidth[d] = dim->getBinWidth();
    m_indexMax[d] = dim->getNBins();
  }
  Utils::NestedForLoop::SetUpIndexMaker(m_nd, m_indexMaker, m_indexMax);

  // Initialize the current index from the start position.
  Utils::NestedForLoop::GetIndicesFromLinearIndex(m_nd, m_pos, m_indexMaker,
                                                  m_indexMax, m_index);

  // Make sure that the first iteration is at a point inside the implicit
  // function
  if (m_function) {
    // Calculate the center of the 0-th bin
    for (size_t d = 0; d < m_nd; d++)
      m_center[d] = m_origin[d] + 0.5f * m_binWidth[d];
    // Skip on if the first point is NOT contained
    if (!m_function->isPointContained(m_center))
      next();
  }

  // --- Calculate index permutations for neighbour finding vertex touching ---
  auto temp = std::vector<int64_t>(integerPower(3, m_nd), 0);
  m_permutationsVertexTouching.swap(temp);

  // --- Calculate index permutations for neighbour finding face touching ---
  temp = std::vector<int64_t>(2 * m_nd);
  m_permutationsFaceTouching.swap(temp);

  int64_t offset = 1;
  m_permutationsVertexTouching[0] = 0;
  m_permutationsVertexTouching[1] = 1;
  m_permutationsVertexTouching[2] = -1;

  m_permutationsFaceTouching[0] = -1;
  m_permutationsFaceTouching[1] = 1;

  // Figure out what possible indexes deltas to generate indexes that are next
  // to the current one.
  size_t nVertexTouchingPermutations = 3;
  for (size_t j = 1; j < m_nd; ++j) {
    offset =
        offset * static_cast<int64_t>(m_ws->getDimension(j - 1)->getNBins());
    size_t counter = nVertexTouchingPermutations;
    for (size_t k = 0; k < nVertexTouchingPermutations; k += 1, counter += 2) {
      int64_t newVariant = m_permutationsVertexTouching[k] + offset;
      m_permutationsVertexTouching[counter] = newVariant;
      m_permutationsVertexTouching[counter + 1] = (-1 * newVariant);
    }

    m_permutationsFaceTouching[j * 2] = offset;
    m_permutationsFaceTouching[(j * 2) + 1] = -offset;

    nVertexTouchingPermutations *= 3;
  }
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MDHistoWorkspaceIterator::~MDHistoWorkspaceIterator() {
  delete[] m_center;
  delete[] m_origin;
  delete[] m_binWidth;
  delete[] m_index;
  delete[] m_indexMax;
  delete[] m_indexMaker;

  if (m_function)
    delete m_function;
  m_function = NULL;
}

//----------------------------------------------------------------------------------------------
/** @return the number of points to be iterated on */
size_t MDHistoWorkspaceIterator::getDataSize() const {
  return size_t(m_max - m_begin);
}

//----------------------------------------------------------------------------------------------
/** Jump to the index^th cell.
 *  No range checking is performed, for performance reasons!
 *
 * @param index :: point to jump to. Must be 0 <= index < getDataSize().
 */
void MDHistoWorkspaceIterator::jumpTo(size_t index) {
  m_pos = uint64_t(index + m_begin);
}

//----------------------------------------------------------------------------------------------
/** @return true if the iterator is valid. Check this at the start of an
 * iteration,
 * in case the very first point is not valid.
 */
bool MDHistoWorkspaceIterator::valid() const { return (m_pos < m_max); }

//----------------------------------------------------------------------------------------------
/// Advance to the next cell. If the current cell is the last one in the
/// workspace
/// do nothing and return false.
/// @return true if you can continue iterating
bool MDHistoWorkspaceIterator::next() {
  if (m_function) {
    do {
      m_pos++;
      Utils::NestedForLoop::Increment(m_nd, m_index, m_indexMax);
      // Calculate the center
      for (size_t d = 0; d < m_nd; d++) {
        m_center[d] =
            m_origin[d] + (coord_t(m_index[d]) + 0.5f) * m_binWidth[d];
        //          std::cout << m_center[d] << ",";
      }
      //        std::cout<<std::endl;
      // Keep incrementing until you are in the implicit function
    } while (!m_function->isPointContained(m_center) && m_pos < m_max);
  } else {
    ++m_pos;
  }
  // Keep calling next if the current position is masked.
  bool ret = m_pos < m_max;
  while (m_skippingPolicy->keepGoing()) {
    ret = this->next();
    if (!ret)
      break;
  }
  // Go through every point;
  return ret;
}

//----------------------------------------------------------------------------------------------
/// Advance, skipping a certain number of cells.
/// @param skip :: how many to increase. If 1, then every point will be sampled.
bool MDHistoWorkspaceIterator::next(size_t skip) {
  m_pos += skip;

  return (m_pos < m_max);
}

//----------------------------------------------------------------------------------------------
/// Returns the normalized signal for this box
signal_t MDHistoWorkspaceIterator::getNormalizedSignal() const {
  // What is our normalization factor?
  switch (m_normalization) {
  case NoNormalization:
    return m_ws->getSignalAt(m_pos);
  case VolumeNormalization:
    return m_ws->getSignalAt(m_pos) * m_ws->getInverseVolume();
  case NumEventsNormalization:
    return m_ws->getSignalAt(m_pos) / m_ws->getNumEventsAt(m_pos);
  }
  // Should not reach here
  return std::numeric_limits<signal_t>::quiet_NaN();
}

//----------------------------------------------------------------------------------------------
/// Returns the normalized error for this box
signal_t MDHistoWorkspaceIterator::getNormalizedError() const {
  // What is our normalization factor?
  switch (m_normalization) {
  case NoNormalization:
    return m_ws->getErrorAt(m_pos);
  case VolumeNormalization:
    return m_ws->getErrorAt(m_pos) * m_ws->getInverseVolume();
  case NumEventsNormalization:
    return m_ws->getErrorAt(m_pos) / m_ws->getNumEventsAt(m_pos);
  }
  // Should not reach here
  return std::numeric_limits<signal_t>::quiet_NaN();
}

//----------------------------------------------------------------------------------------------
/// Returns the signal for this box, same as innerSignal
signal_t MDHistoWorkspaceIterator::getSignal() const {
  return m_ws->getSignalAt(m_pos);
}

/// Returns the error for this box, same as innerError
signal_t MDHistoWorkspaceIterator::getError() const {
  return m_ws->getErrorAt(m_pos);
}
//----------------------------------------------------------------------------------------------
/// Return a list of vertexes defining the volume pointed to
coord_t *MDHistoWorkspaceIterator::getVertexesArray(size_t &numVertices) const {
  // The MDHistoWorkspace takes care of this
  return m_ws->getVertexesArray(m_pos, numVertices);
}

coord_t *MDHistoWorkspaceIterator::getVertexesArray(size_t &numVertices,
                                                    const size_t outDimensions,
                                                    const bool *maskDim) const {
  // Do the same thing as is done in the MDBoxBase
  UNUSED_ARG(numVertices);
  UNUSED_ARG(outDimensions);
  UNUSED_ARG(maskDim);
  throw std::runtime_error("Not Implemented At present time");
}

//----------------------------------------------------------------------------------------------
/// Returns the position of the center of the box pointed to.
Mantid::Kernel::VMD MDHistoWorkspaceIterator::getCenter() const {
  // Get the indices
  Utils::NestedForLoop::GetIndicesFromLinearIndex(m_nd, m_pos, m_indexMaker,
                                                  m_indexMax, m_index);
  // Find the center
  for (size_t d = 0; d < m_nd; d++)
    m_center[d] = m_origin[d] + (coord_t(m_index[d]) + 0.5f) * m_binWidth[d];
  return VMD(m_nd, m_center);
}

//----------------------------------------------------------------------------------------------
/// Returns the number of events/points contained in this box
/// @return 1 always: e.g. there is one (fake) event in the middle of the box.
size_t MDHistoWorkspaceIterator::getNumEvents() const {
  return static_cast<size_t>(m_ws->getNumEventsAt(m_pos));
}

//----------------------------------------------------------------------------------------------
/// For a given event/point in this box, return the run index
uint16_t MDHistoWorkspaceIterator::getInnerRunIndex(size_t /*index*/) const {
  return 0;
  // throw std::runtime_error("MDHistoWorkspaceIterator: No events are
  // contained, so it is not possible to return inner run index.");
}

/// For a given event/point in this box, return the detector ID
int32_t MDHistoWorkspaceIterator::getInnerDetectorID(size_t /*index*/) const {
  return 0;
  // throw std::runtime_error("MDHistoWorkspaceIterator: No events are
  // contained, so it is not possible to return inner detector ID.");
}

/// Returns the position of a given event for a given dimension
coord_t MDHistoWorkspaceIterator::getInnerPosition(size_t /*index*/,
                                                   size_t dimension) const {
  return m_ws->getCenter(m_pos)[dimension];
}

/// Returns the signal of a given event
signal_t MDHistoWorkspaceIterator::getInnerSignal(size_t /*index*/) const {
  return m_ws->getSignalAt(m_pos);
}

/// Returns the error of a given event
signal_t MDHistoWorkspaceIterator::getInnerError(size_t /*index*/) const {
  return m_ws->getErrorAt(m_pos);
}

bool MDHistoWorkspaceIterator::getIsMasked() const {
  return m_ws->getIsMaskedAt(m_pos);
}

/**
 Getter for the linear index
 @return the linear index.
 */
size_t MDHistoWorkspaceIterator::getLinearIndex() const { return m_pos; }

/**
 * Gets indexes of bins/pixels/boxes neighbouring the present iterator location.
 Return all vertex touching neighbours.
 * The number of neighbour indexes returned will depend upon the dimensionality
 of the workspace as well as the presence
 * of boundaries and edges.

 FindNeighbours will return the indexes of neighbours even if they are
 unreachable from the current iterator.
 To verify that the indexes are reachable from the current iterator, run
 isWithinBounds. Note that this is only a concern
 where the workspace iteration is portioned up amongst >1 iterators.

 * @return vector of linear indexes to neighbour locations.
 */
std::vector<size_t> MDHistoWorkspaceIterator::findNeighbourIndexes() const {

  Utils::NestedForLoop::GetIndicesFromLinearIndex(m_nd, m_pos, m_indexMaker,
                                                  m_indexMax, m_index);

  // Filter out indexes that are are not actually neighbours.
  std::vector<size_t> neighbourIndexes; // Accumulate neighbour indexes.
  for (size_t i = 0; i < m_permutationsVertexTouching.size(); ++i) {
    if (m_permutationsVertexTouching[i] == 0) {
      continue;
    }

    size_t neighbour_index = m_pos + m_permutationsVertexTouching[i];
    if (neighbour_index < m_ws->getNPoints() &&
        Utils::isNeighbourOfSubject(m_nd, neighbour_index, m_index,
                                    m_indexMaker, m_indexMax)) {
      neighbourIndexes.push_back(neighbour_index);
    }
  }
  return neighbourIndexes;
}

/**
 * Find neighbor indexes, but only return those that are face-touching.
 *
 * Will return the indexes of neighbours even if they are unreachable from the
 *current iterator.
 * To verify that the indexes are reachable from the current iterator, run
 *isWithinBounds. Note that this is only a concern where the workspace iteration
 *is portioned up amongst >1 iterators.
 * @return
 */
std::vector<size_t>
MDHistoWorkspaceIterator::findNeighbourIndexesFaceTouching() const {
  Utils::NestedForLoop::GetIndicesFromLinearIndex(m_nd, m_pos, m_indexMaker,
                                                  m_indexMax, m_index);

  std::vector<size_t> neighbourIndexes; // Accumulate neighbour indexes.
  for (size_t i = 0; i < m_permutationsFaceTouching.size(); ++i) {
    if (m_permutationsFaceTouching[i] == 0) {
      continue;
    }

    size_t neighbour_index = m_pos + m_permutationsFaceTouching[i];
    if (neighbour_index < m_ws->getNPoints() &&
        Utils::isNeighbourOfSubject(m_nd, neighbour_index, m_index,
                                    m_indexMaker, m_indexMax)) {
      neighbourIndexes.push_back(neighbour_index);
    }
  }
  return neighbourIndexes;
}

/**
 @param index : linear index to inspect against iterator
 @return True only if the index is between the min and max bounds of the
 iterator.
 */
bool MDHistoWorkspaceIterator::isWithinBounds(size_t index) const {
  return index >= m_begin && index < m_max;
}

} // namespace Mantid
} // namespace DataObjects
