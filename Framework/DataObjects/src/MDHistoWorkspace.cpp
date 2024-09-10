// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDFramesToSpecialCoordinateSystem.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/VMD.h"
#include "MantidKernel/WarningSuppressions.h"

#include <boost/scoped_array.hpp>
#include <cmath>
#include <map>
#include <memory>
#include <optional>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid::DataObjects {
//----------------------------------------------------------------------------------------------
/** Constructor given the 4 dimensions
 * @param dimX :: X dimension binning parameters
 * @param dimY :: Y dimension binning parameters
 * @param dimZ :: Z dimension binning parameters
 * @param dimT :: T (time) dimension binning parameters
 * @param displayNormalization :: optional display normalization to use as the
 * default.
 */
MDHistoWorkspace::MDHistoWorkspace(Mantid::Geometry::MDHistoDimension_sptr dimX,
                                   Mantid::Geometry::MDHistoDimension_sptr dimY,
                                   Mantid::Geometry::MDHistoDimension_sptr dimZ,
                                   Mantid::Geometry::MDHistoDimension_sptr dimT,
                                   Mantid::API::MDNormalization displayNormalization)
    : IMDHistoWorkspace(), numDimensions(0), m_nEventsContributed(std::numeric_limits<uint64_t>::quiet_NaN()),
      m_coordSystem(None), m_displayNormalization(displayNormalization) {
  std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions;
  if (dimX)
    dimensions.emplace_back(std::move(dimX));
  if (dimY)
    dimensions.emplace_back(std::move(dimY));
  if (dimZ)
    dimensions.emplace_back(std::move(dimZ));
  if (dimT)
    dimensions.emplace_back(std::move(dimT));
  this->init(dimensions);
}

//----------------------------------------------------------------------------------------------
/** Constructor given a vector of dimensions
 * @param dimensions :: vector of MDHistoDimension; no limit to how many.
 * @param displayNormalization :: optional display normalization to use as the
 * default.
 */
MDHistoWorkspace::MDHistoWorkspace(std::vector<Mantid::Geometry::MDHistoDimension_sptr> &dimensions,
                                   Mantid::API::MDNormalization displayNormalization)
    : IMDHistoWorkspace(), numDimensions(0), m_nEventsContributed(std::numeric_limits<uint64_t>::quiet_NaN()),
      m_coordSystem(None), m_displayNormalization(displayNormalization) {
  this->init(dimensions);
}

//----------------------------------------------------------------------------------------------
/** Constructor given a vector of dimensions
 * @param dimensions :: vector of MDHistoDimension; no limit to how many.
 * @param displayNormalization :: optional display normalization to use as the
 * default.
 */
MDHistoWorkspace::MDHistoWorkspace(std::vector<Mantid::Geometry::IMDDimension_sptr> &dimensions,
                                   Mantid::API::MDNormalization displayNormalization)
    : IMDHistoWorkspace(), numDimensions(0), m_nEventsContributed(std::numeric_limits<uint64_t>::quiet_NaN()),
      m_coordSystem(None), m_displayNormalization(displayNormalization) {
  this->init(dimensions);
}

//----------------------------------------------------------------------------------------------
/** Copy constructor
 *
 * @param other :: MDHistoWorkspace to copy from.
 */
MDHistoWorkspace::MDHistoWorkspace(const MDHistoWorkspace &other)
    : IMDHistoWorkspace(other), m_nEventsContributed(other.m_nEventsContributed), m_coordSystem(other.m_coordSystem),
      m_displayNormalization(other.m_displayNormalization) {
  // Dimensions are copied by the copy constructor of MDGeometry
  this->cacheValues();
  // Allocate the linear arrays
  m_signals = std::vector<signal_t>(m_length);
  m_errorsSquared = std::vector<signal_t>(m_length);
  m_numEvents = std::vector<signal_t>(m_length);
  m_masks = std::make_unique<bool[]>(m_length);
  // Now copy all the data
  std::copy_n(other.m_signals.begin(), m_length, m_signals.begin());
  std::copy_n(other.m_errorsSquared.begin(), m_length, m_errorsSquared.begin());
  std::copy_n(other.m_numEvents.begin(), m_length, m_numEvents.begin());
  std::copy_n(other.m_masks.get(), m_length, m_masks.get());
}

//----------------------------------------------------------------------------------------------
/** Constructor helper method
 * @param dimensions :: vector of MDHistoDimension; no limit to how many.
 */
void MDHistoWorkspace::init(std::vector<Mantid::Geometry::MDHistoDimension_sptr> &dimensions) {
  std::vector<IMDDimension_sptr> dim2;
  dim2.reserve(dimensions.size());
  std::transform(dimensions.cbegin(), dimensions.cend(), std::back_inserter(dim2),
                 [](const auto dimension) { return std::dynamic_pointer_cast<IMDDimension>(dimension); });
  this->init(dim2);
  m_nEventsContributed = 0;
}

//----------------------------------------------------------------------------------------------
/** Constructor helper method
 * @param dimensions :: vector of IMDDimension; no limit to how many.
 */
void MDHistoWorkspace::init(std::vector<Mantid::Geometry::IMDDimension_sptr> &dimensions) {
  MDGeometry::initGeometry(dimensions);
  this->cacheValues();

  // Allocate the linear arrays
  m_signals = std::vector<signal_t>(m_length);
  m_errorsSquared = std::vector<signal_t>(m_length);
  m_numEvents = std::vector<signal_t>(m_length);
  m_masks = std::make_unique<bool[]>(m_length);
  // Initialize them to NAN (quickly)
  signal_t nan = std::numeric_limits<signal_t>::quiet_NaN();
  this->setTo(nan, nan, nan);
  m_nEventsContributed = 0;
}

//----------------------------------------------------------------------------------------------
/** When all dimensions have been initialized, this caches all the necessary
 * values for later use.
 */
void MDHistoWorkspace::cacheValues() {
  // Copy the dimensions array
  numDimensions = m_dimensions.size();

  // For indexing.
  if (numDimensions < 4)
    indexMultiplier = std::vector<size_t>(4);
  else
    indexMultiplier = std::vector<size_t>(numDimensions);

  // For quick indexing, accumulate these values
  // First multiplier
  indexMultiplier[0] = m_dimensions[0]->getNBins();
  for (size_t d = 1; d < numDimensions; d++)
    indexMultiplier[d] = indexMultiplier[d - 1] * m_dimensions[d]->getNBins();

  // This is how many dense data points
  m_length = indexMultiplier[numDimensions - 1];

  // Now fix things for < 4 dimensions. Indices > the number of dimensions will
  // be ignored (*0)
  for (size_t d = numDimensions - 1; d < 4; d++)
    indexMultiplier[d] = 0;

  // Compute the volume of each cell.
  coord_t volume = 1.0;
  for (size_t i = 0; i < numDimensions; ++i)
    volume *= m_dimensions[i]->getBinWidth();
  m_inverseVolume = 1.0f / volume;

  // Continue with the vertexes array
  this->initVertexesArray();
  m_nEventsContributed = 0;
}
//----------------------------------------------------------------------------------------------
/** After initialization, call this to initialize the vertexes array
 * to the vertexes of the 0th box.
 * Will be used by getVertexesArray()
 * */
void MDHistoWorkspace::initVertexesArray() {
  size_t nd = numDimensions;
  // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd
  // bits
  size_t numVertices = size_t{1} << numDimensions;

  // Allocate the array of the right size
  m_vertexesArray = std::vector<coord_t>(nd * numVertices);

  // For each vertex, increase an integeer
  for (size_t i = 0; i < numVertices; ++i) {
    // Start point index in the output array;
    size_t outIndex = i * nd;

    // Coordinates of the vertex
    for (size_t d = 0; d < nd; d++) {
      // Use a bit mask to look at each bit of the integer we are iterating
      // through.
      size_t mask = size_t{1} << d;
      if ((i & mask) > 0) {
        // Bit is 1, use the max of the dimension
        m_vertexesArray[outIndex + d] = m_dimensions[d]->getX(1);
      } else {
        // Bit is 0, use the min of the dimension
        m_vertexesArray[outIndex + d] = m_dimensions[d]->getX(0);
      }
    } // (for each dimension)
  }

  // Now set the m_boxLength and origin
  m_boxLength = std::vector<coord_t>(nd);
  m_origin = std::vector<coord_t>(nd);
  for (size_t d = 0; d < nd; d++) {
    m_boxLength[d] = m_dimensions[d]->getX(1) - m_dimensions[d]->getX(0);
    m_origin[d] = m_dimensions[d]->getX(0);
  }

  // The index calculator
  m_indexMax = std::vector<size_t>(numDimensions);
  for (size_t d = 0; d < nd; d++)
    m_indexMax[d] = m_dimensions[d]->getNBins();
  m_indexMaker = std::vector<size_t>(numDimensions);
  Utils::NestedForLoop::SetUpIndexMaker(numDimensions, m_indexMaker.data(), m_indexMax.data());
}

//----------------------------------------------------------------------------------------------
/** Sets all signals/errors in the workspace to the given values
 *
 * @param signal :: signal value to set
 * @param errorSquared :: error (squared) value to set
 * @param numEvents :: the number of events in each bin.
 */
void MDHistoWorkspace::setTo(signal_t signal, signal_t errorSquared, signal_t numEvents) {
  std::fill_n(m_signals.begin(), m_length, signal);
  std::fill_n(m_errorsSquared.begin(), m_length, errorSquared);
  std::fill_n(m_numEvents.begin(), m_length, numEvents);
  std::fill_n(m_masks.get(), m_length, false);
  m_nEventsContributed = static_cast<uint64_t>(numEvents) * m_length;
}

//----------------------------------------------------------------------------------------------
/** Apply an implicit function to each point; if false, set to the given value.
 *
 * @param function :: the implicit function to apply
 * @param signal :: signal value to set when function evaluates to false
 * @param errorSquared :: error value to set when function evaluates to false
 */
void MDHistoWorkspace::applyImplicitFunction(Mantid::Geometry::MDImplicitFunction *function, signal_t signal,
                                             signal_t errorSquared) {
  if (numDimensions < 3)
    throw std::invalid_argument("Need 3 dimensions for ImplicitFunction.");
  Mantid::coord_t coord[3];
  for (size_t x = 0; x < m_dimensions[0]->getNBins(); x++) {
    coord[0] = m_dimensions[0]->getX(x);
    for (size_t y = 0; y < m_dimensions[1]->getNBins(); y++) {
      coord[1] = m_dimensions[1]->getX(y);
      for (size_t z = 0; z < m_dimensions[2]->getNBins(); z++) {
        coord[2] = m_dimensions[2]->getX(z);

        if (!function->isPointContained(coord)) {
          m_signals[x + indexMultiplier[0] * y + indexMultiplier[1] * z] = signal;
          m_errorsSquared[x + indexMultiplier[0] * y + indexMultiplier[1] * z] = errorSquared;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------
/** For the volume at the given linear index,
 * Return the vertices of every corner of the box, as
 * a bare array of length numVertices * nd
 *
 * @param linearIndex :: index into the workspace. Same as for
 *getSignalAt(index)
 * @param[out] numVertices :: returns the number of vertices in the array.
 * @return the bare array. This should be deleted by the caller!
 * */
std::unique_ptr<coord_t[]> MDHistoWorkspace::getVertexesArray(size_t linearIndex, size_t &numVertices) const {
  // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd
  // bits
  numVertices = static_cast<size_t>(1) << numDimensions; // Cast avoids warning about
                                                         // result of 32-bit shift
                                                         // implicitly converted to 64 bits
                                                         // on MSVC

  // Index into each dimension. Built from the linearIndex.
  size_t dimIndexes[10];
  Utils::NestedForLoop::GetIndicesFromLinearIndex(numDimensions, linearIndex, m_indexMaker.data(), m_indexMax.data(),
                                                  dimIndexes);

  // The output vertexes coordinates
  auto out = std::make_unique<coord_t[]>(numDimensions * numVertices);
  for (size_t i = 0; i < numVertices; ++i) {
    size_t outIndex = i * numDimensions;
    // Offset the 0th box by the position of this linear index, in each
    // dimension
    for (size_t d = 0; d < numDimensions; d++)
      out[outIndex + d] = m_vertexesArray[outIndex + d] + m_boxLength[d] * static_cast<coord_t>(dimIndexes[d]);
  }

  return out;
}

//----------------------------------------------------------------------------------------------
/** Return the position of the center of a bin at a given position
 *
 * @param linearIndex :: linear index into the workspace
 * @return VMD vector of the center position
 */
Mantid::Kernel::VMD MDHistoWorkspace::getCenter(size_t linearIndex) const {
  // Index into each dimension. Built from the linearIndex.
  size_t dimIndexes[10];
  Utils::NestedForLoop::GetIndicesFromLinearIndex(numDimensions, linearIndex, m_indexMaker.data(), m_indexMax.data(),
                                                  dimIndexes);

  // The output vertexes coordinates
  VMD out(numDimensions);
  // Offset the 0th box by the position of this linear index, in each dimension,
  // plus a half
  for (size_t d = 0; d < numDimensions; d++)
    out[d] = m_vertexesArray[d] + m_boxLength[d] * (static_cast<coord_t>(dimIndexes[d]) + 0.5f);
  return out;
}

//----------------------------------------------------------------------------------------------
/** Get the signal at a particular coordinate in the workspace.
 *
 * @param coords :: numDimensions-sized array of the coordinates to look at
 * @param normalization : Normalisation to use.
 * @return the (normalized) signal at a given coordinates.
 *         NaN if outside the range of this workspace
 */
signal_t MDHistoWorkspace::getSignalAtCoord(const coord_t *coords,
                                            const Mantid::API::MDNormalization &normalization) const {
  size_t linearIndex = this->getLinearIndexAtCoord(coords);
  if (linearIndex < m_length) {
    signal_t normalizer = getNormalizationFactor(normalization, linearIndex);
    return m_signals[linearIndex] * normalizer;
  } else
    return std::numeric_limits<signal_t>::quiet_NaN();
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
signal_t MDHistoWorkspace::getSignalWithMaskAtCoord(const coord_t *coords,
                                                    const Mantid::API::MDNormalization &normalization) const {
  size_t linearIndex = this->getLinearIndexAtCoord(coords);
  if (linearIndex == std::numeric_limits<size_t>::max() || this->getIsMaskedAt(linearIndex)) {
    return MDMaskValue;
  }
  return getSignalAtCoord(coords, normalization);
}

//----------------------------------------------------------------------------------------------
/** Get the linear index into the histo array at these coordinates
 *
 * @param coords :: ND-sized array of coordinates
 * @return the linear index, or size_t(-1) if out of range.
 */
size_t MDHistoWorkspace::getLinearIndexAtCoord(const coord_t *coords) const {
  // Build up the linear index, dimension by dimension
  size_t linearIndex = 0;
  for (size_t d = 0; d < numDimensions; d++) {
    coord_t x = coords[d] - m_origin[d];
    auto ix = size_t(x / m_boxLength[d]);
    if (ix >= m_indexMax[d] || (x < 0))
      return size_t(-1);
    linearIndex += ix * m_indexMaker[d];
  }
  return linearIndex;
}

//----------------------------------------------------------------------------------------------
/** Create IMDIterators from this MDHistoWorkspace
 *
 * @param suggestedNumCores :: split the iterators into this many cores (if
 *threadsafe)
 * @param function :: implicit function to limit range. NOT owned by this method
 *call.
 * @return MDHistoWorkspaceIterator vector
 */
std::vector<std::unique_ptr<Mantid::API::IMDIterator>>
MDHistoWorkspace::createIterators(size_t suggestedNumCores, Mantid::Geometry::MDImplicitFunction *function) const {
  size_t numCores = suggestedNumCores;
  if (!this->threadSafe())
    numCores = 1;
  size_t numElements = this->getNPoints();
  if (numCores > numElements)
    numCores = numElements;
  if (numCores < 1)
    numCores = 1;

  // Create one iterator per core, splitting evenly amongst spectra
  std::vector<std::unique_ptr<IMDIterator>> out;
  for (size_t i = 0; i < numCores; i++) {
    size_t begin = (i * numElements) / numCores;
    size_t end = ((i + 1) * numElements) / numCores;
    if (end > numElements)
      end = numElements;

    // Clone the MDImplicitFunction if necessary.
    Mantid::Geometry::MDImplicitFunction *clonedFunction = function;
    if (function)
      clonedFunction = new Mantid::Geometry::MDImplicitFunction(*function);

    out.emplace_back(std::make_unique<MDHistoWorkspaceIterator>(this, clonedFunction, begin, end));
  }
  return out;
}

//----------------------------------------------------------------------------------------------
/** Return the memory used, in bytes */
size_t MDHistoWorkspace::getMemorySize() const { return m_length * (sizeOfElement()); }

//----------------------------------------------------------------------------------------------
/// @return a vector containing a copy of the signal data in the workspace.
std::vector<signal_t> MDHistoWorkspace::getSignalDataVector() const {
  std::vector<signal_t> out;
  out.resize(m_length, 0.0);
  for (size_t i = 0; i < m_length; ++i)
    out[i] = m_signals[i];
  // This copies again! :(
  return out;
}

/// @return a vector containing a copy of the error data in the workspace.
std::vector<signal_t> MDHistoWorkspace::getErrorDataVector() const {
  std::vector<signal_t> out;
  out.resize(m_length, 0.0);
  for (size_t i = 0; i < m_length; ++i)
    out[i] = m_errorsSquared[i];
  // This copies again! :(
  return out;
}

/** @return true if the point is within the workspace (including the max edges)
 * */
bool pointInWorkspace(const MDHistoWorkspace *ws, const VMD &point) {
  for (size_t d = 0; d < ws->getNumDims(); d++) {
    IMDDimension_const_sptr dim = ws->getDimension(d);
    if ((point[d] < dim->getMinimum()) || (point[d] > dim->getMaximum()))
      return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------
/** Obtain coordinates for a line plot through a MDWorkspace.
 * Cross the workspace from start to end points, recording the signal along the
 *line at bin centres of unmasked bins.
 *
 * @param start :: coordinates of the start point of the line
 * @param end :: coordinates of the end point of the line
 * @param normalize :: how to normalize the signal
 * @returns :: LinePlot with x as linearly spaced points along the line
 * between start and end, y set to the normalized signal for each bin with
 * Length = length(x) - 1 and e as the error vector for each bin.
 */
IMDWorkspace::LinePlot MDHistoWorkspace::getLinePlot(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &end,
                                                     Mantid::API::MDNormalization normalize) const {

  return this->getLinePoints(start, end, normalize, true);
}

//----------------------------------------------------------------------------------------------
/** Obtain coordinates for a line plot through a MDWorkspace.
 * Cross the workspace from start to end points, recording the signal along the
 *lin at either bin boundaries, or halfway between bin boundaries (which is bin
 *centres if the line is dimension aligned). If recording halfway values then
 *omit points in masked bins.
 *
 * @param start :: coordinates of the start point of the line
 * @param end :: coordinates of the end point of the line
 * @param normalize :: how to normalize the signal
 * @returns :: LinePlot with x as the boundaries of the bins, relative
 * to start of the line, y set to the normalized signal for each bin with
 * Length = length(x) - 1 and e as the error vector for each bin.
 * @param bin_centres :: if true then record points halfway between bin
 *boundaries, otherwise record on bin boundaries
 */
IMDWorkspace::LinePlot MDHistoWorkspace::getLinePoints(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &end,
                                                       Mantid::API::MDNormalization normalize,
                                                       const bool bin_centres) const {
  LinePlot line;

  size_t nd = this->getNumDims();
  if (start.getNumDims() != nd)
    throw std::runtime_error("Start point must have the same number of "
                             "dimensions as the workspace.");
  if (end.getNumDims() != nd)
    throw std::runtime_error("End point must have the same number of dimensions as the workspace.");

  // Unit-vector of the direction
  VMD dir = end - start;
  const auto length = dir.normalize();

  const std::set<coord_t> boundaries = getBinBoundariesOnLine(start, end, nd, dir, length);

  if (boundaries.empty()) {
    this->makeSinglePointWithNaN(line.x, line.y, line.e);

    // Require x.size() = y.size()+1 if recording bin boundaries
    if (!bin_centres)
      line.x.emplace_back(length);

    return line;
  } else {
    // Get the first point
    std::set<coord_t>::iterator it;
    it = boundaries.cbegin();

    coord_t lastLinePos = *it;
    VMD lastPos = start + (dir * lastLinePos);
    if (!bin_centres) {
      line.x.emplace_back(lastLinePos);
    }

    ++it;
    for (; it != boundaries.cend(); ++it) {
      // This is our current position along the line
      const coord_t linePos = *it;

      // This is the full position at this boundary
      VMD pos = start + (dir * linePos);

      // Position in the middle of the bin
      VMD middle = (pos + lastPos) * 0.5;

      // Find the signal in this bin
      const auto linearIndex = this->getLinearIndexAtCoord(middle.getBareArray());

      if (bin_centres && !(linearIndex == std::numeric_limits<size_t>::max() || this->getIsMaskedAt(linearIndex))) {
        auto bin_centrePos = static_cast<coord_t>((linePos + lastLinePos) * 0.5);
        line.x.emplace_back(bin_centrePos);
      } else if (!bin_centres)
        line.x.emplace_back(linePos);

      if (linearIndex < m_length) {

        auto normalizer = getNormalizationFactor(normalize, linearIndex);
        // And add the normalized signal/error to the list too
        auto signal = this->getSignalAt(linearIndex) * normalizer;
        if (std::isinf(signal)) {
          // The plotting library (qwt) doesn't like infs.
          signal = std::numeric_limits<signal_t>::quiet_NaN();
        }
        if (!bin_centres || !this->getIsMaskedAt(linearIndex)) {
          line.y.emplace_back(signal);
          line.e.emplace_back(this->getErrorAt(linearIndex) * normalizer);
        }
        // Save the position for next bin
        lastPos = pos;
      } else {
        // Invalid index. This shouldn't happen
        line.y.emplace_back(std::numeric_limits<signal_t>::quiet_NaN());
        line.e.emplace_back(std::numeric_limits<signal_t>::quiet_NaN());
      }

      lastLinePos = linePos;

    } // for each unique boundary

    // If all bins were masked
    if (line.x.empty()) {
      this->makeSinglePointWithNaN(line.x, line.y, line.e);
    }
  }
  return line;
}

//----------------------------------------------------------------------------------------------
/** Obtain coordinates for a line plot through a MDWorkspace.
 * Cross the workspace from start to end points, recording the signal along the
 *line.
 * Sets the x,y vectors to the histogram bin boundaries and counts
 *
 * @param start :: coordinates of the start point of the line
 * @param end :: coordinates of the end point of the line
 * @param normalize :: how to normalize the signal
 * @returns :: LinePlot with points at bin boundaries
 */
IMDWorkspace::LinePlot MDHistoWorkspace::getLineData(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &end,
                                                     Mantid::API::MDNormalization normalize) const {
  return this->getLinePoints(start, end, normalize, false);
}

//----------------------------------------------------------------------------------------------
/** Find the normalization factor
 *
 * @param normalize :: how to normalize the signal
 * @param linearIndex :: the position in the workspace of the signal value to be
 *normalized
 * @returns :: the normalization factor
 */
signal_t MDHistoWorkspace::getNormalizationFactor(const MDNormalization &normalize, size_t linearIndex) const {
  signal_t normalizer = 1.0;
  switch (normalize) {
  case NoNormalization:
    return normalizer;
  case VolumeNormalization:
    return m_inverseVolume;
  case NumEventsNormalization:
    return 1.0 / m_numEvents[linearIndex];
  }
  return normalizer;
}

//----------------------------------------------------------------------------------------------
/** Get ordered list of boundaries in position-along-the-line coordinates
 *
 * @param start :: start of the line
 * @param end :: end of the line
 * @param nd :: number of dimensions
 * @param dir :: vector of the direction
 * @param length :: unit-vector of the direction
 * @returns :: ordered list of boundaries
 */
std::set<coord_t> MDHistoWorkspace::getBinBoundariesOnLine(const VMD &start, const VMD &end, size_t nd, const VMD &dir,
                                                           coord_t length) const {
  std::set<coord_t> boundaries;

  // Start with the start/end points, if they are within range.
  if (pointInWorkspace(this, start))
    boundaries.insert(0.0f);
  if (pointInWorkspace(this, end))
    boundaries.insert(length);

  // Next, we go through each dimension and see where the bin boundaries
  // intersect the line.
  for (size_t d = 0; d < nd; d++) {
    IMDDimension_const_sptr dim = getDimension(d);
    coord_t lineStartX = start[d];

    if (dir[d] != 0.0) {
      auto nbounds = dim->getNBoundaries();
      for (size_t i = 0; i < nbounds; i++) {
        // Position in this coordinate
        coord_t thisX = dim->getX(i);
        // Position along the line. Is this between the start and end of it?
        coord_t linePos = (thisX - lineStartX) / dir[d];
        if (linePos >= 0 && linePos <= length) {
          // Full position
          VMD pos = start + (dir * linePos);
          // This is a boundary if the line point is inside the workspace
          if (pointInWorkspace(this, pos))
            boundaries.insert(linePos);
        }
      }
    }
  }
  return boundaries;
}

//==============================================================================================
//============================== ARITHMETIC OPERATIONS
//=========================================
//==============================================================================================

//----------------------------------------------------------------------------------------------
/** Check if the two workspace's sizes match (for comparison or
 *element-by-element operation
 *
 * @param other :: the workspace to compare to
 * @param operation :: descriptive string (for the error message)
 * @throw an error if they don't match
 */
void MDHistoWorkspace::checkWorkspaceSize(const MDHistoWorkspace &other, const std::string &operation) {
  if (other.getNumDims() != this->getNumDims())
    throw std::invalid_argument("Cannot perform the " + operation +
                                " operation on this MDHistoWorkspace. The "
                                "number of dimensions does not match.");
  if (other.m_length != this->m_length)
    throw std::invalid_argument("Cannot perform the " + operation +
                                " operation on this MDHistoWorkspace. The "
                                "length of the signals vector does not match.");
}

//----------------------------------------------------------------------------------------------
/** Perform the += operation, element-by-element, for two MDHistoWorkspace's
 *
 * @param b :: workspace on the RHS of the operation
 * @return *this after operation */
MDHistoWorkspace &MDHistoWorkspace::operator+=(const MDHistoWorkspace &b) {
  add(b);
  return *this;
}

//----------------------------------------------------------------------------------------------
/** Perform the += operation, element-by-element, for two MDHistoWorkspace's
 *
 * @param b :: workspace on the RHS of the operation
 * */
void MDHistoWorkspace::add(const MDHistoWorkspace &b) {
  checkWorkspaceSize(b, "add");
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] += b.m_signals[i];
    m_errorsSquared[i] += b.m_errorsSquared[i];
    m_numEvents[i] += b.m_numEvents[i];
  }
  m_nEventsContributed += b.m_nEventsContributed;
}

//----------------------------------------------------------------------------------------------
/** Perform the += operation with a scalar as the RHS argument
 *
 * @param signal :: signal to apply
 * @param error :: error (not squared) to apply
 * */
void MDHistoWorkspace::add(const signal_t signal, const signal_t error) {
  signal_t errorSquared = error * error;
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] += signal;
    m_errorsSquared[i] += errorSquared;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the -= operation, element-by-element, for two MDHistoWorkspace's
 *
 * @param b :: workspace on the RHS of the operation
 * @return *this after operation */
MDHistoWorkspace &MDHistoWorkspace::operator-=(const MDHistoWorkspace &b) {
  subtract(b);
  return *this;
}

//----------------------------------------------------------------------------------------------
/** Perform the -= operation, element-by-element, for two MDHistoWorkspace's
 *
 * @param b :: workspace on the RHS of the operation
 * */
void MDHistoWorkspace::subtract(const MDHistoWorkspace &b) {
  checkWorkspaceSize(b, "subtract");
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] -= b.m_signals[i];
    m_errorsSquared[i] += b.m_errorsSquared[i];
    m_numEvents[i] += b.m_numEvents[i];
  }
  m_nEventsContributed += b.m_nEventsContributed;
}

//----------------------------------------------------------------------------------------------
/** Perform the -= operation with a scalar as the RHS argument
 *
 * @param signal :: signal to apply
 * @param error :: error (not squared) to apply
 * */
void MDHistoWorkspace::subtract(const signal_t signal, const signal_t error) {
  signal_t errorSquared = error * error;
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] -= signal;
    m_errorsSquared[i] += errorSquared;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the *= operation, element-by-element, for two MDHistoWorkspace's
 *
 * Error propagation of \f$ f = a * b \f$  is given by:
 * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
 *
 * @param b_ws :: workspace on the RHS of the operation
 * @return *this after operation */
MDHistoWorkspace &MDHistoWorkspace::operator*=(const MDHistoWorkspace &b_ws) {
  multiply(b_ws);
  return *this;
}

//----------------------------------------------------------------------------------------------
/** Perform the *= operation, element-by-element, for two MDHistoWorkspace's
 *
 * Error propagation of \f$ f = a * b \f$  is given by:
 * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
 * Rewritten as:
 * \f$ df^2 = b^2 da^2 + a^2 * db^2 \f$
 * to avoid problems when a or b are 0
 *
 * @param b_ws :: workspace on the RHS of the operation
 * */
void MDHistoWorkspace::multiply(const MDHistoWorkspace &b_ws) {
  checkWorkspaceSize(b_ws, "multiply");
  for (size_t i = 0; i < m_length; ++i) {
    signal_t a = m_signals[i];
    signal_t da2 = m_errorsSquared[i];

    signal_t b = b_ws.m_signals[i];
    signal_t db2 = b_ws.m_errorsSquared[i];

    signal_t f = a * b;
    signal_t df2 = da2 * b * b + db2 * a * a;

    m_signals[i] = f;
    m_errorsSquared[i] = df2;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the *= operation with a scalar as the RHS argument
 *
 * Error propagation of \f$ f = a * b \f$  is given by:
 * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
 *  Rewritten as:
 * \f$ df^2 = b^2 da^2 + a^2 * db^2 \f$
 * to avoid problems when a or b are 0
 *
 * @param signal :: signal to apply
 * @param error :: error (not squared) to apply
 */
void MDHistoWorkspace::multiply(const signal_t signal, const signal_t error) {
  signal_t b = signal;
  signal_t db2 = error * error;

  for (size_t i = 0; i < m_length; ++i) {
    signal_t a = m_signals[i];
    signal_t da2 = m_errorsSquared[i];

    signal_t f = a * b;
    signal_t df2 = da2 * b * b + db2 * a * a;

    m_signals[i] = f;
    m_errorsSquared[i] = df2;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the /= operation, element-by-element, for two MDHistoWorkspace's
 *
 * Error propagation of \f$ f = a / b \f$  is given by:
 * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
 *
 * @param b_ws :: workspace on the RHS of the operation
 * @return *this after operation */
MDHistoWorkspace &MDHistoWorkspace::operator/=(const MDHistoWorkspace &b_ws) {
  divide(b_ws);
  return *this;
}

//----------------------------------------------------------------------------------------------
/** Perform the /= operation, element-by-element, for two MDHistoWorkspace's
 *
 * Error propagation of \f$ f = a / b \f$  is given by:
 * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
 * Rewritten as:
 * \f$ df^2 = da^2 / b^2 + db^2 *f^2 / b^2 \f$
 * to avoid problems when a or b are 0
 *
 * @param b_ws :: workspace on the RHS of the operation
 **/
void MDHistoWorkspace::divide(const MDHistoWorkspace &b_ws) {
  checkWorkspaceSize(b_ws, "divide");
  for (size_t i = 0; i < m_length; ++i) {
    signal_t a = m_signals[i];
    signal_t da2 = m_errorsSquared[i];

    signal_t b = b_ws.m_signals[i];
    signal_t db2 = b_ws.m_errorsSquared[i];

    signal_t f = a / b;
    signal_t df2 = da2 / (b * b) + db2 * f * f / (b * b);

    m_signals[i] = f;
    m_errorsSquared[i] = df2;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the /= operation with a scalar as the RHS argument
 *
 * Error propagation of \f$ f = a / b \f$  is given by:
 * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
 * Rewritten as:
 * \f$ df^2 = da^2 / b^2 + db^2 *f^2 / b^2 \f$
 * to avoid problems when a or b are 0
 *
 * @param signal :: signal to apply
 * @param error :: error (not squared) to apply
 **/
void MDHistoWorkspace::divide(const signal_t signal, const signal_t error) {
  signal_t b = signal;
  signal_t db2 = error * error;
  signal_t db2_relative = db2 / (b * b);
  for (size_t i = 0; i < m_length; ++i) {
    signal_t a = m_signals[i];
    signal_t da2 = m_errorsSquared[i];

    signal_t f = a / b;
    signal_t df2 = da2 / (b * b) + db2_relative * f * f;

    m_signals[i] = f;
    m_errorsSquared[i] = df2;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the natural logarithm on each signal in the workspace.
 *
 * Error propagation of \f$ f = ln(a) \f$  is given by:
 * \f$ df^2 = a^2 / da^2 \f$
 */
void MDHistoWorkspace::log(double filler) {
  for (size_t i = 0; i < m_length; ++i) {
    signal_t a = m_signals[i];
    signal_t da2 = m_errorsSquared[i];
    if (a <= 0) {
      m_signals[i] = filler;
      m_errorsSquared[i] = 0;
    } else {
      m_signals[i] = std::log(a);
      m_errorsSquared[i] = da2 / (a * a);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the base-10 logarithm on each signal in the workspace.
 *
 * Error propagation of \f$ f = ln(a) \f$  is given by:
 * \f$ df^2 = (ln(10)^-2) * a^2 / da^2 \f$
 */
void MDHistoWorkspace::log10(double filler) {
  for (size_t i = 0; i < m_length; ++i) {
    signal_t a = m_signals[i];
    signal_t da2 = m_errorsSquared[i];
    if (a <= 0) {
      m_signals[i] = filler;
      m_errorsSquared[i] = 0;
    } else {
      m_signals[i] = std::log10(a);
      m_errorsSquared[i] = 0.1886117 * da2 / (a * a); // 0.1886117  = ln(10)^-2
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the exp() function on each signal in the workspace.
 *
 * Error propagation of \f$ f = exp(a) \f$  is given by:
 * \f$ df^2 = f^2 * da^2 \f$
 */
void MDHistoWorkspace::exp() {
  for (size_t i = 0; i < m_length; ++i) {
    signal_t f = std::exp(m_signals[i]);
    signal_t da2 = m_errorsSquared[i];
    m_signals[i] = f;
    m_errorsSquared[i] = f * f * da2;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the power function (signal^exponent) on each signal S in the
 *workspace.
 *
 * Error propagation of \f$ f = a^b \f$  is given by:
 * \f$ df^2 = f^2 * b^2 * (da^2 / a^2) \f$
 */
void MDHistoWorkspace::power(double exponent) {
  double exponent_squared = exponent * exponent;
  for (size_t i = 0; i < m_length; ++i) {
    signal_t a = m_signals[i];
    signal_t f = std::pow(a, exponent);
    signal_t da2 = m_errorsSquared[i];
    m_signals[i] = f;
    m_errorsSquared[i] = f * f * exponent_squared * da2 / (a * a);
  }
}

//==============================================================================================
//============================== BOOLEAN OPERATIONS
//============================================
//==============================================================================================

//----------------------------------------------------------------------------------------------
/// @cond DOXYGEN_BUG
/** A boolean &= (and) operation, element-by-element, for two
 *MDHistoWorkspace's.
 *
 * 0.0 is "false", all other values are "true". All errors are set to 0.
 *
 * @param b :: workspace on the RHS of the operation
 * @return *this after operation */
MDHistoWorkspace &MDHistoWorkspace::operator&=(const MDHistoWorkspace &b) {
  checkWorkspaceSize(b, "&= (and)");
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] = ((m_signals[i] != 0 && !m_masks[i]) && (b.m_signals[i] != 0 && !b.m_masks[i])) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0;
  }
  return *this;
}
/// @endcond DOXYGEN_BUG

//----------------------------------------------------------------------------------------------
/** A boolean |= (or) operation, element-by-element, for two MDHistoWorkspace's.
 *
 * 0.0 is "false", all other values are "true". All errors are set to 0.
 *
 * @param b :: workspace on the RHS of the operation
 * @return *this after operation */
MDHistoWorkspace &MDHistoWorkspace::operator|=(const MDHistoWorkspace &b) {
  checkWorkspaceSize(b, "|= (or)");
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] = ((m_signals[i] != 0 && !m_masks[i]) || (b.m_signals[i] != 0 && !b.m_masks[i])) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0;
  }
  return *this;
}

//----------------------------------------------------------------------------------------------
/** A boolean ^= (xor) operation, element-by-element, for two
 *MDHistoWorkspace's.
 *
 * 0.0 is "false", all other values are "true". All errors are set to 0.
 *
 * @param b :: workspace on the RHS of the operation
 * @return *this after operation */
MDHistoWorkspace &MDHistoWorkspace::operator^=(const MDHistoWorkspace &b) {
  checkWorkspaceSize(b, "^= (xor)");
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] = ((m_signals[i] != 0 && !m_masks[i]) ^ (b.m_signals[i] != 0 && !b.m_masks[i])) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0;
  }
  return *this;
}

//----------------------------------------------------------------------------------------------
/** A boolean not operation, performed in-place.
 * All errors are set to 0.
 *
 * 0.0 is "false", all other values are "true". All errors are set to 0.
 */
void MDHistoWorkspace::operatorNot() {
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] = (m_signals[i] == 0.0 || m_masks[i]) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0.0;
  }
}

//----------------------------------------------------------------------------------------------
/** Turn this workspace into a boolean workspace, where
 * signal[i] -> becomes true (1.0) if it is < b[i].
 * signal[i] -> becomes false (0.0) otherwise
 * Errors are set to 0.
 *
 * @param b :: workspace on the RHS of the comparison.
 */
void MDHistoWorkspace::lessThan(const MDHistoWorkspace &b) {
  checkWorkspaceSize(b, "lessThan");
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] = (m_signals[i] < b.m_signals[i]) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0.0;
  }
}

//----------------------------------------------------------------------------------------------
/** Turn this workspace into a boolean workspace, where
 * signal[i] -> becomes true (1.0) if it is < signal.
 * signal[i] -> becomes false (0.0) otherwise
 * Errors are set to 0.
 *
 * @param signal :: signal value on the RHS of the comparison.
 */
void MDHistoWorkspace::lessThan(const signal_t signal) {
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] = (m_signals[i] < signal) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0.0;
  }
}

//----------------------------------------------------------------------------------------------
/** Turn this workspace into a boolean workspace, where
 * signal[i] -> becomes true (1.0) if it is > b[i].
 * signal[i] -> becomes false (0.0) otherwise
 * Errors are set to 0.
 *
 * @param b :: workspace on the RHS of the comparison.
 */
void MDHistoWorkspace::greaterThan(const MDHistoWorkspace &b) {
  checkWorkspaceSize(b, "greaterThan");
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] = (m_signals[i] > b.m_signals[i]) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Turn this workspace into a boolean workspace, where
 * signal[i] -> becomes true (1.0) if it is > signal.
 * signal[i] -> becomes false (0.0) otherwise
 * Errors are set to 0.
 *
 * @param signal :: signal value on the RHS of the comparison.
 */
void MDHistoWorkspace::greaterThan(const signal_t signal) {
  for (size_t i = 0; i < m_length; ++i) {
    m_signals[i] = (m_signals[i] > signal) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Turn this workspace into a boolean workspace, where
 * signal[i] -> becomes true (1.0) if it is == b[i].
 * signal[i] -> becomes false (0.0) otherwise
 * Errors are set to 0.
 *
 * @param b :: workspace on the RHS of the comparison.
 * @param tolerance :: accept this deviation from a perfect equality
 */
void MDHistoWorkspace::equalTo(const MDHistoWorkspace &b, const signal_t tolerance) {
  checkWorkspaceSize(b, "equalTo");
  for (size_t i = 0; i < m_length; ++i) {
    signal_t diff = fabs(m_signals[i] - b.m_signals[i]);
    m_signals[i] = (diff < tolerance) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Turn this workspace into a boolean workspace, where
 * signal[i] -> becomes true (1.0) if it is == signal.
 * signal[i] -> becomes false (0.0) otherwise
 * Errors are set to 0.
 *
 * @param signal :: signal value on the RHS of the comparison.
 * @param tolerance :: accept this deviation from a perfect equality
 */
void MDHistoWorkspace::equalTo(const signal_t signal, const signal_t tolerance) {
  for (size_t i = 0; i < m_length; ++i) {
    signal_t diff = fabs(m_signals[i] - signal);
    m_signals[i] = (diff < tolerance) ? 1.0 : 0.0;
    m_errorsSquared[i] = 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Copy the values from another workspace onto this workspace, but only
 * where a mask is true (non-zero)
 *
 * For example, in matlab or numpy python, you might write something like:
 *  "mask = (array < 5.0); array[mask] = other[mask];"
 *
 * The equivalent here is:
 *  mask = array;
 *  mask.lessThan(5.0);
 *  array.setUsingMask(mask, other);
 *
 * @param mask :: MDHistoWorkspace where (signal == 0.0) means false, and
 *(signal != 0.0) means true.
 * @param values :: MDHistoWorkspace of values to copy.
 */
void MDHistoWorkspace::setUsingMask(const MDHistoWorkspace &mask, const MDHistoWorkspace &values) {
  checkWorkspaceSize(mask, "setUsingMask");
  checkWorkspaceSize(values, "setUsingMask");
  for (size_t i = 0; i < m_length; ++i) {
    if (mask.m_signals[i] != 0.0) {
      m_signals[i] = values.m_signals[i];
      m_errorsSquared[i] = values.m_errorsSquared[i];
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Copy the values from another workspace onto this workspace, but only
 * where a mask is true (non-zero)
 *
 * For example, in matlab or numpy python, you might write something like:
 *  "mask = (array < 5.0); array[mask] = other[mask];"
 *
 * The equivalent here is:
 *  mask = array;
 *  mask.lessThan(5.0);
 *  array.setUsingMask(mask, other);
 *
 * @param mask :: MDHistoWorkspace where (signal == 0.0) means false, and
 *(signal != 0.0) means true.
 * @param signal :: signal to set everywhere mask is true
 * @param error :: error (not squared) to set everywhere mask is true
 */
void MDHistoWorkspace::setUsingMask(const MDHistoWorkspace &mask, const signal_t signal, const signal_t error) {
  signal_t errorSquared = error * error;
  checkWorkspaceSize(mask, "setUsingMask");
  for (size_t i = 0; i < m_length; ++i) {
    if (mask.m_signals[i] != 0.0) {
      m_signals[i] = signal;
      m_errorsSquared[i] = errorSquared;
    }
  }
}

/**
Setter for the masking region.
Does not perform any clearing. Multiple calls are compounded.
@param maskingRegion : Implicit function defining mask region.
*/
void MDHistoWorkspace::setMDMasking(std::unique_ptr<Mantid::Geometry::MDImplicitFunction> maskingRegion) {
  if (maskingRegion != nullptr) {
    for (size_t i = 0; i < this->getNPoints(); ++i) {
      // If the function masks the point, then mask it, otherwise leave it as it
      // is.
      if (maskingRegion->isPointContained(this->getCenter(i))) {
        this->setMDMaskAt(i, true);
      }
    }
  }
}

/**
 * Set the masking
 * @param index : linear index to mask
 * @param mask : True to mask. False to clear.
 */
void MDHistoWorkspace::setMDMaskAt(const size_t &index, bool mask) {
  m_masks[index] = mask;
  if (mask) {
    // Set signal and error of masked points to the value of MDMaskValue
    this->setSignalAt(index, MDMaskValue);
    this->setErrorSquaredAt(index, MDMaskValue);
  }
}

/**
 * Clear any existing masking.
 * Note that this clears the mask flag but does not restore the data
 * which was set to NaN when it was masked.
 */
void MDHistoWorkspace::clearMDMasking() {
  for (size_t i = 0; i < this->getNPoints(); ++i) {
    m_masks[i] = false;
  }
}

uint64_t MDHistoWorkspace::getNEvents() const {
  volatile uint64_t cach = this->m_nEventsContributed;
  if (cach != this->m_nEventsContributed) {
    if (m_numEvents.empty())
      m_nEventsContributed = std::numeric_limits<uint64_t>::quiet_NaN();
    else
      m_nEventsContributed = sumNContribEvents();
  }
  return m_nEventsContributed;
}

uint64_t MDHistoWorkspace::sumNContribEvents() const {
  uint64_t sum(0);
  for (size_t i = 0; i < m_length; ++i)
    sum += uint64_t(m_numEvents[i]);

  return sum;
}

/**
 * Get the Q frame system (if any) to use.
 */
GNU_DIAG_OFF("strict-aliasing")
Kernel::SpecialCoordinateSystem MDHistoWorkspace::getSpecialCoordinateSystem() const {
  MDFramesToSpecialCoordinateSystem converter;
  auto coordinatesFromMDFrames = converter(this);
  auto coordinates = m_coordSystem;
  if (coordinatesFromMDFrames) {
    coordinates = coordinatesFromMDFrames.value();
  }
  return coordinates;
}

/**
Set the special coordinate system (if any) to use.
@param coordinateSystem : Special coordinate system to use.
*/
void MDHistoWorkspace::setCoordinateSystem(const Kernel::SpecialCoordinateSystem coordinateSystem) {
  m_coordSystem = coordinateSystem;
}

/**
 * Static helper method.
 * @return The size of an element in the MDEventWorkspace.
 */
size_t MDHistoWorkspace::sizeOfElement() { return (3 * sizeof(signal_t)) + sizeof(bool); }

/**
Preferred normalization to use for visual purposes.
*/
MDNormalization MDHistoWorkspace::displayNormalization() const {
  return m_displayNormalization; // Normalize by the number of events.
}

/**
Preferred normalization to use for visual purposes.
*/
MDNormalization MDHistoWorkspace::displayNormalizationHisto() const {
  return displayNormalization(); // Normalize by the number of events.
}

void MDHistoWorkspace::setDisplayNormalization(const Mantid::API::MDNormalization &preferredNormalization) {
  m_displayNormalization = preferredNormalization;
}

} // namespace Mantid::DataObjects
