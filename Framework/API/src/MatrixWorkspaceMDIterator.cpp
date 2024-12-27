// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MatrixWorkspaceMDIterator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"

#include "MantidGeometry/IDetector.h"
#include "MantidKernel/VMD.h"
#include "MantidKernel/cow_ptr.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::API {

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param workspace :: iterate through this workspace
 * @param function :: limiting implicit function
 * @param beginWI :: first workspace index to iterate
 * @param endWI :: end when you reach this workspace index
 */
MatrixWorkspaceMDIterator::MatrixWorkspaceMDIterator(const MatrixWorkspace *workspace,
                                                     Mantid::Geometry::MDImplicitFunction *function, size_t beginWI,
                                                     size_t endWI)
    : m_ws(workspace), m_pos(0), m_max(0), m_function(function), m_errorIsCached(false),
      m_spectrumInfo(m_ws->spectrumInfo()) {
  if (!m_ws)
    throw std::runtime_error("MatrixWorkspaceMDIterator::ctor() NULL MatrixWorkspace");
  m_center = VMD(2);
  m_isBinnedData = m_ws->isHistogramData();
  m_dimY = m_ws->getDimension(1);

  m_beginWI = beginWI;
  if (m_beginWI >= m_ws->getNumberHistograms())
    throw std::runtime_error("MatrixWorkspaceMDIterator: Beginning workspace "
                             "index passed is too high.");

  // End point (handle default)
  m_endWI = endWI;
  if (m_endWI > m_ws->getNumberHistograms())
    m_endWI = m_ws->getNumberHistograms();
  if (m_endWI < m_beginWI)
    throw std::runtime_error("MatrixWorkspaceMDIterator: End point is before the start point.");

  // calculate the indices and the largest index we accept
  m_max = 0;
  m_startIndices.reserve(m_endWI - m_beginWI);
  for (size_t i = m_beginWI; i < m_endWI; ++i) {
    m_startIndices.emplace_back(m_max);
    m_max += m_ws->readY(i).size();
  }

  m_xIndex = 0;
  // Trigger the calculation for the first index
  m_workspaceIndex = size_t(-1); // This makes sure calcWorkspacePos() updates
  calcWorkspacePos(m_beginWI);
}

//----------------------------------------------------------------------------------------------
/** @return the number of points to be iterated on */
size_t MatrixWorkspaceMDIterator::getDataSize() const { return size_t(m_max); }

//----------------------------------------------------------------------------------------------
/** Jump to the index^th cell.
 *  No range checking is performed, for performance reasons!
 *
 * @param index :: point to jump to. Must be 0 <= index < getDataSize().
 */
void MatrixWorkspaceMDIterator::jumpTo(size_t index) {
  m_pos = static_cast<uint64_t>(index); // index into the unraveled workspace
  const auto lower = std::lower_bound(m_startIndices.begin(), m_startIndices.end(), index);
  m_xIndex = m_pos - (*lower); // index into the Y[] array of the spectrum
  size_t newWI = m_beginWI + std::distance(m_startIndices.begin(), lower);
  calcWorkspacePos(newWI);
}

//----------------------------------------------------------------------------------------------
/// Calculate the workspace index/x index for this iterator position
inline void MatrixWorkspaceMDIterator::calcWorkspacePos(size_t newWI) {
  if (newWI >= m_endWI)
    return;

  if (newWI != m_workspaceIndex) {
    m_workspaceIndex = newWI;
    // Copy the vectors. This is more thread-safe
    m_X = m_ws->readX(m_workspaceIndex);
    m_Y = m_ws->readY(m_workspaceIndex);
    m_errorIsCached = false;
    m_center[1] = m_dimY->getX(m_workspaceIndex);

    // Find the vertical bin size
    m_verticalBinSize = 1.0;
    NumericAxis *ax1 = dynamic_cast<NumericAxis *>(m_ws->getAxis(1));
    if (ax1) {
      const MantidVec &yVals = ax1->getValues();
      if (yVals.size() > 1) {
        if (m_workspaceIndex < yVals.size() - 1)
          m_verticalBinSize = yVals[m_workspaceIndex + 1] - yVals[m_workspaceIndex];
        else
          m_verticalBinSize = yVals[m_workspaceIndex] - yVals[m_workspaceIndex - 1];
      }
    }
  }
}

//----------------------------------------------------------------------------------------------
/** @return true if the iterator is valid. Check this at the start of an
 * iteration,
 * in case the very first point is not valid.
 */
bool MatrixWorkspaceMDIterator::valid() const { return (m_pos < m_max); }

//----------------------------------------------------------------------------------------------
/// Advance to the next cell. If the current cell is the last one in the
/// workspace
/// do nothing and return false.
/// @return true if you can continue iterating
bool MatrixWorkspaceMDIterator::next() {
  if (m_function) {
    do {
      m_pos++;
      m_xIndex++;
      if (m_xIndex >= m_Y.size()) {
        m_xIndex = 0;
        this->calcWorkspacePos(m_workspaceIndex + 1);
      }
      this->getCenter();
      // Keep incrementing until you are in the implicit function
    } while (!m_function->isPointContained(m_center) && m_pos < m_max);
    // Is the iteration finished?
    return (m_pos < m_max);
  } else {
    // Go through every point;
    m_pos++;
    m_xIndex++;
    if (m_xIndex >= m_Y.size()) {
      m_xIndex = 0;
      this->calcWorkspacePos(m_workspaceIndex + 1);
    }
    return (m_pos < m_max);
  }
}

//----------------------------------------------------------------------------------------------
/// Advance, skipping a certain number of cells.
/// @param skip :: how many to increase. If 1, then every point will be sampled.
bool MatrixWorkspaceMDIterator::next(size_t skip) {
  this->jumpTo(m_pos + skip);
  return (m_pos < m_max);
}

//----------------------------------------------------------------------------------------------
/// Returns the normalized signal for this box
signal_t MatrixWorkspaceMDIterator::getNormalizedSignal() const {
  // What is our normalization factor?
  switch (m_normalization) {
  case NoNormalization:
    return m_Y[m_xIndex];
  case VolumeNormalization:
    return m_Y[m_xIndex] / (m_verticalBinSize * (m_X[m_xIndex + 1] - m_X[m_xIndex]));
  case NumEventsNormalization:
    return m_Y[m_xIndex];
  }
  return std::numeric_limits<signal_t>::quiet_NaN();
}

//----------------------------------------------------------------------------------------------
/// Returns the normalized error for this box
signal_t MatrixWorkspaceMDIterator::getNormalizedError() const {
  // What is our normalization factor?
  switch (m_normalization) {
  case NoNormalization:
    return getError();
  case VolumeNormalization:
    return getError() / (m_verticalBinSize * (m_X[m_xIndex + 1] - m_X[m_xIndex]));
  case NumEventsNormalization:
    return getError();
  }
  return std::numeric_limits<signal_t>::quiet_NaN();
}

/// Returns the signal for this box, same as innerSignal
signal_t MatrixWorkspaceMDIterator::getSignal() const { return m_Y[m_xIndex]; }

/// Returns the error for this box, same as innerError
signal_t MatrixWorkspaceMDIterator::getError() const {
  if (!m_errorIsCached) {
    m_E = m_ws->readE(m_workspaceIndex);
    m_errorIsCached = true;
  }

  return m_E[m_xIndex];
}

//----------------------------------------------------------------------------------------------
/// Return a list of vertexes defining the volume pointed to
std::unique_ptr<coord_t[]> MatrixWorkspaceMDIterator::getVertexesArray(size_t & /*numVertices*/) const {
  throw std::runtime_error("MatrixWorkspaceMDIterator::getVertexesArray() not implemented yet");
}

std::unique_ptr<coord_t[]> MatrixWorkspaceMDIterator::getVertexesArray(size_t & /*numVertices*/,
                                                                       const size_t /*outDimensions*/,
                                                                       const bool * /*maskDim*/) const {
  throw std::runtime_error("MatrixWorkspaceMDIterator::getVertexesArray() not implemented yet");
}

//----------------------------------------------------------------------------------------------
/// Returns the position of the center of the box pointed to.
Mantid::Kernel::VMD MatrixWorkspaceMDIterator::getCenter() const {
  // Place the point in X dimension
  if (m_isBinnedData)
    m_center[0] = VMD_t((m_X[m_xIndex] + m_X[m_xIndex + 1]) / 2.0);
  else
    m_center[0] = VMD_t(m_X[m_xIndex]);
  return m_center;
}

//----------------------------------------------------------------------------------------------
/// Returns the number of events/points contained in this box
/// @return 1 always: e.g. there is one (fake) event in the middle of the box.
size_t MatrixWorkspaceMDIterator::getNumEvents() const { return 1; }

//----------------------------------------------------------------------------------------------
/// For a given event/point in this box, return the associated experiment-info index
uint16_t MatrixWorkspaceMDIterator::getInnerExpInfoIndex(size_t /*index*/) const { return 0; }

/// For a given event/point in this box, return the associated experiment-info index
uint16_t MatrixWorkspaceMDIterator::getInnerGoniometerIndex(size_t /*index*/) const { return 0; }

/// For a given event/point in this box, return the detector ID
int32_t MatrixWorkspaceMDIterator::getInnerDetectorID(size_t /*index*/) const { return 0; }

/// Returns the position of a given event for a given dimension
coord_t MatrixWorkspaceMDIterator::getInnerPosition(size_t /*index*/, size_t dimension) const {
  return this->getCenter()[dimension];
}

/// Returns the signal of a given event
signal_t MatrixWorkspaceMDIterator::getInnerSignal(size_t /*index*/) const { return this->getSignal(); }

/// Returns the error of a given event
signal_t MatrixWorkspaceMDIterator::getInnerError(size_t /*index*/) const { return this->getError(); }

/**
 * Getter for the masked state of the workspace.
 * @returns True if the detector/detector-group at the workspace index is
 * masked, or if there is no detector at that index.
 */
bool MatrixWorkspaceMDIterator::getIsMasked() const {
  if (!m_spectrumInfo.hasDetectors(m_workspaceIndex)) {
    return true;
  }
  return m_spectrumInfo.isMasked(m_workspaceIndex);
}

/**
 * Find neighbour indexes
 * @return Neighbour indexes to the current index.
 */
std::vector<size_t> MatrixWorkspaceMDIterator::findNeighbourIndexes() const {
  throw std::runtime_error("MatrixWorkspaceMDIterator does not implement findNeighbourIndexes");
}

/**
 * Find neighbour indexes face touching.
 * @return Neighbour indexes to the current index.
 */
std::vector<size_t> MatrixWorkspaceMDIterator::findNeighbourIndexesFaceTouching() const {
  throw std::runtime_error("MatrixWorkspaceMDIterator does not implement "
                           "findNeighbourIndexesFaceTouching");
}

size_t MatrixWorkspaceMDIterator::getLinearIndex() const {
  throw std::runtime_error("MatrixWorkspaceMDIterator does not implement getLinearIndex");
}

bool MatrixWorkspaceMDIterator::isWithinBounds(const size_t /*index*/) const {
  throw std::runtime_error("MatrixWorkspaceMDIterator does not implement isWithinBounds");
}

} // namespace Mantid::API
