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

WorkspaceBoundingBox::~WorkspaceBoundingBox() {}

Kernel::V3D &WorkspaceBoundingBox::position(int index) const {
  if (m_cachedPositionIndex != index) {
    if (!m_spectrumInfo)
      throw std::runtime_error("SpectrumInfo object is not initialized");

    m_cachedPosition = m_spectrumInfo->position(index);
    m_cachedPositionIndex = index;
  }
  return m_cachedPosition;
}

double WorkspaceBoundingBox::yValue(const int index) const {
  if (m_cachedHistogramYIndex != index) {
    m_cachedYValue = m_workspace->y(index)[HISTOGRAM_INDEX];
    m_cachedHistogramYIndex = index;
  }
  return m_cachedYValue;
}

void WorkspaceBoundingBox::setPosition(double x, double y) {
  this->x = x;
  this->y = y;
}

void WorkspaceBoundingBox::setCenter(double x, double y) {
  this->centerX = x;
  this->centerY = y;
}

void WorkspaceBoundingBox::setBounds(double xMin, double xMax, double yMin, double yMax) {
  this->xMin = xMin;
  this->xMax = xMax;
  this->yMin = yMin;
  this->yMax = yMax;
}

/** Performs checks on the spectrum located at index to determine if
 *  it is acceptable to be operated on
 *
 *  @param index :: index of spectrum data
 *  @return true/false if its valid
 */
bool WorkspaceBoundingBox::isValidWs(int index) const {
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
  const auto YIn = this->yValue(index);
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
int WorkspaceBoundingBox::findFirstValidWs(const int numSpec) const {
  int i;
  for (i = 0; i < numSpec; ++i) {
    if (isValidWs(i))
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
double WorkspaceBoundingBox::updatePositionAndReturnCount(int index) {
  const auto YIn = this->yValue(index);
  const auto &position = this->position(index);

  this->x += YIn * position.X();
  this->y += YIn * position.Y();

  return YIn;
}

/** Compare current mins and maxs to the coordinates of the spectrum at index
 *  expnd mins and maxs to include this spectrum
 *
 *  @param index :: index of spectrum data
 */
void WorkspaceBoundingBox::updateMinMax(int index) {
  const auto &position = this->position(index);
  const double x = position.X();
  const double y = position.Y();

  this->xMin = std::min(x, this->xMin);
  this->xMax = std::max(x, this->xMax);
  this->yMin = std::min(y, this->yMin);
  this->yMax = std::max(y, this->yMax);
}

/** Checks to see if spectrum at index is within the diameter of the given beamRadius
 *
 *  @param beamRadius :: radius of beam in meters
 *  @param index :: index of spectrum data
 *  @param directBeam :: whether or not the spectrum is subject to the beam
 *  @return number of points of histogram data at index
 */
bool WorkspaceBoundingBox::isOutOfBoundsOfNonDirectBeam(const double beamRadius, int index, const bool directBeam) {
  if (!directBeam) {
    const auto &position = this->position(index);
    const double dx = position.X() - this->centerX;
    const double dy = position.Y() - this->centerY;
    if (dx * dx + dy * dy < beamRadius * beamRadius)
      return false;
  }
  return true;
}

double WorkspaceBoundingBox::calculateDistance() const {
  const auto xExtent = (centerX - x);
  const auto yExtent = (centerY - y);
  return sqrt(xExtent * xExtent + yExtent * yExtent);
}

double WorkspaceBoundingBox::calculateRadiusX() const { return std::min((x - xMin), (xMax - x)); }

double WorkspaceBoundingBox::calculateRadiusY() const { return std::min((y - yMin), (yMax - y)); }

/** Perform normalization on x/y coords over given values
 *
 *  @param x :: value to normalize member x over
 *  @param y :: value to normalize member y over
 */
void WorkspaceBoundingBox::normalizePosition(double x, double y) {
  this->x /= std::fabs(x);
  this->y /= std::fabs(y);
}

/** Checks if a given x/y coord is within the bounding box
 *
 *  @param x :: x coordinate
 *  @param y :: y coordinate
 *  @return true/false if it is within the mins/maxs of the box
 */
bool WorkspaceBoundingBox::containsPoint(double x, double y) {
  return (x <= this->xMax && x >= this->xMin && y <= yMax && y >= yMin);
}

} // namespace Mantid::Algorithms
