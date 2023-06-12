#include "MantidAlgorithms/WorkspaceBoundingBox.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid::Algorithms {

namespace {
constexpr int HISTOGRAM_INDEX{0};
}

WorkspaceBoundingBox::WorkspaceBoundingBox(const API::MatrixWorkspace_const_sptr &workspace) : m_workspace(workspace) {
  if (m_workspace->y(0).size() != 1)
    throw std::runtime_error("This object only works with integrated workspaces");

  m_spectrumInfo = &workspace->spectrumInfo();
}

WorkspaceBoundingBox::WorkspaceBoundingBox() {
  m_spectrumInfo = nullptr; // certain functionality is not available
}

WorkspaceBoundingBox::~WorkspaceBoundingBox() = default;

Kernel::V3D &WorkspaceBoundingBox::position(const std::size_t index) const {
  if (m_cachedPositionIndex != index) {
    if (!m_spectrumInfo)
      throw std::runtime_error("SpectrumInfo object is not initialized");

    m_cachedPosition = m_spectrumInfo->position(index);
    m_cachedPositionIndex = index;
  }
  return m_cachedPosition;
}

double WorkspaceBoundingBox::countsValue(const std::size_t index) const {
  if (m_cachedHistogramYIndex != index) {
    m_cachedYValue = m_workspace->y(index)[HISTOGRAM_INDEX];
    m_cachedHistogramYIndex = index;
  }
  return m_cachedYValue;
}

void WorkspaceBoundingBox::setPosition(const double x, const double y) {
  this->m_xPos = x;
  this->m_yPos = y;
}

void WorkspaceBoundingBox::setCenter(const double x, const double y) {
  this->m_centerXPos = x;
  this->m_centerYPos = y;
}

void WorkspaceBoundingBox::setBounds(const double xMin, const double xMax, const double yMin, const double yMax) {
  this->m_xPosMin = xMin;
  this->m_xPosMax = xMax;
  this->m_yPosMin = yMin;
  this->m_yPosMax = yMax;
}

/** Performs checks on the spectrum located at index to determine if
 *  it is acceptable to be operated on
 *
 *  @param index :: index of spectrum data
 *  @return true/false if its valid
 */
bool WorkspaceBoundingBox::isValidIndex(const std::size_t index) const {
  if (!m_spectrumInfo)
    throw std::runtime_error("SpectrumInfo object is not initialized");
  if (!m_spectrumInfo->hasDetectors(index)) {
    g_log.warning() << "Workspace index " << index << " has no detector assigned to it - discarding\n";
    return false;
  }
  // Skip if we have a monitor or if the detector is masked.
  if (this->m_spectrumInfo->isMonitor(index) || this->m_spectrumInfo->isMasked(index))
    return false;

  // Get the current spectrum
  const auto YIn = this->countsValue(index);
  // Skip if NaN of inf
  if (std::isnan(YIn) || std::isinf(YIn))
    return false;
  return true;
}

/** Searches for the first valid spectrum info in member variable `workspace`
 *
 *  @param numSpec :: the number of spectrum in the workspace to search through
 *  @return index of first valid spectrum
 */
std::size_t WorkspaceBoundingBox::findFirstValidWs(const std::size_t numSpec) const {
  std::size_t i;
  for (i = 0; i < numSpec; ++i) {
    if (isValidIndex(i))
      break;
  }
  return i;
}

/** Sets member variables x/y to new x/y based on
 *  spectrum info and historgram data at the given index
 *
 *  @param index :: index of spectrum data
 *  @return number of points of histogram data at index
 */
double WorkspaceBoundingBox::updatePositionAndReturnCount(const std::size_t index) {
  const auto counts = this->countsValue(index);
  const auto &position = this->position(index);

  this->m_xPos += counts * position.X();
  this->m_yPos += counts * position.Y();

  return counts;
}

/** Compare current mins and maxs to the coordinates of the spectrum at index
 *  expnd mins and maxs to include this spectrum
 *
 *  @param index :: index of spectrum data
 */
void WorkspaceBoundingBox::updateMinMax(const std::size_t index) {
  const auto &position = this->position(index);
  const double x = position.X();
  const double y = position.Y();

  this->m_xPosMin = std::min(x, this->m_xPosMin);
  this->m_xPosMax = std::max(x, this->m_xPosMax);
  this->m_yPosMin = std::min(y, this->m_yPosMin);
  this->m_yPosMax = std::max(y, this->m_yPosMax);
}

/** Checks to see if spectrum at index is within the diameter of the given beamRadius
 *
 *  @param beamRadius :: radius of beam in meters
 *  @param index :: index of spectrum data
 *  @param directBeam :: whether or not the spectrum is subject to the beam
 *  @return number of points of histogram data at index
 */
bool WorkspaceBoundingBox::isOutOfBoundsOfNonDirectBeam(const double beamRadius, const std::size_t index,
                                                        const bool directBeam) {
  if (!directBeam) {
    const auto &position = this->position(index);
    const double dx = position.X() - this->m_centerXPos;
    const double dy = position.Y() - this->m_centerYPos;
    if (dx * dx + dy * dy < beamRadius * beamRadius)
      return false;
  }
  return true;
}

double WorkspaceBoundingBox::calculateDistance() const {
  const auto xExtent = (m_centerXPos - m_xPos);
  const auto yExtent = (m_centerYPos - m_yPos);
  return sqrt(xExtent * xExtent + yExtent * yExtent);
}

double WorkspaceBoundingBox::calculateRadiusX() const { return std::min((m_xPos - m_xPosMin), (m_xPosMax - m_xPos)); }

double WorkspaceBoundingBox::calculateRadiusY() const { return std::min((m_yPos - m_yPosMin), (m_yPosMax - m_yPos)); }

/** Perform normalization on x/y coords over given values
 *
 *  @param x :: value to normalize member x over
 *  @param y :: value to normalize member y over
 */
void WorkspaceBoundingBox::normalizePosition(double x, double y) {
  this->m_xPos /= std::fabs(x);
  this->m_yPos /= std::fabs(y);
}

/** Checks if a given x/y coord is within the bounding box
 *
 *  @param x :: x coordinate
 *  @param y :: y coordinate
 *  @return true/false if it is within the mins/maxs of the box
 */
bool WorkspaceBoundingBox::containsPoint(double x, double y) {
  return (x <= this->m_xPosMax && x >= this->m_xPosMin && y <= m_yPosMax && y >= m_yPosMin);
}

} // namespace Mantid::Algorithms
