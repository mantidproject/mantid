#include "MantidAlgorithms/WorkspaceBoundingBox.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

WorkspaceBoundingBox::WorkspaceBoundingBox(API::MatrixWorkspace_sptr workspace) {
  this->workspace = workspace;
  this->spectrumInfo = &workspace->spectrumInfo();
}

WorkspaceBoundingBox::WorkspaceBoundingBox() {}

WorkspaceBoundingBox::~WorkspaceBoundingBox() {}

Kernel::V3D &WorkspaceBoundingBox::position(int index) {
  if (m_cachedPositionIndex == -1 || m_cachedPositionIndex != index) {
    m_cachedPosition = this->spectrumInfo->position(index);
    m_cachedPositionIndex = index;
  }
  return m_cachedPosition;
}

const HistogramData::HistogramY &WorkspaceBoundingBox::histogramY(int index) {
  if (m_cachedHistogramYIndex == -1 || m_cachedHistogramYIndex != index) {
    auto &YIn = this->workspace->y(index);
    m_cachedHistogramY = &YIn;
    m_cachedHistogramYIndex = index;
  }
  return *m_cachedHistogramY;
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
bool WorkspaceBoundingBox::isValidWs(int index) {
  if (!this->spectrumInfo->hasDetectors(index)) {
    g_log.warning() << "Workspace index " << index << " has no detector assigned to it - discarding\n";
    return false;
  }
  // Skip if we have a monitor or if the detector is masked.
  if (this->spectrumInfo->isMonitor(index) || this->spectrumInfo->isMasked(index))
    return false;

  // Get the current spectrum
  auto &YIn = this->histogramY(index);
  // Skip if NaN of inf
  if (std::isnan(YIn[m_specID]) || std::isinf(YIn[m_specID]))
    return false;
  return true;
}

/** Searches for the first valid spectrum info in member variable `workspace`
 *
 *  @param numSpec :: the number of spectrum in the workspace to search through
 *  @return index of first valid spectrum
 */
int WorkspaceBoundingBox::findFirstValidWs(const int numSpec) {
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
  auto &YIn = this->histogramY(index);
  double x = this->position(index).X();
  double y = this->position(index).Y();
  this->x += YIn[m_specID] * x;
  this->y += YIn[m_specID] * y;
  return YIn[m_specID];
}

/** Compare current mins and maxs to the coordinates of the spectrum at index
 *  expnd mins and maxs to include this spectrum
 *
 *  @param index :: index of spectrum data
 */
void WorkspaceBoundingBox::updateMinMax(int index) {
  double x = this->position(index).X();
  double y = this->position(index).Y();
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
  double x = this->position(index).X();
  double y = this->position(index).Y();
  if (!directBeam) {
    double dx = x - this->centerX;
    double dy = y - this->centerY;
    if (dx * dx + dy * dy < beamRadius * beamRadius)
      return false;
  }
  return true;
}

double WorkspaceBoundingBox::calculateDistance() {
  return sqrt((centerX - x) * (centerX - x) + (centerY - y) * (centerY - y));
}
double WorkspaceBoundingBox::calculateRadiusX() { return std::min((x - xMin), (xMax - x)); }
double WorkspaceBoundingBox::calculateRadiusY() { return std::min((y - yMin), (yMax - y)); }

/** Perform normalization on x/y coords over given values
 *
 *  @param x :: value to normalize member x over
 *  @param y :: value to normalize member y over
 */
void WorkspaceBoundingBox::normalizePosition(double x, double y) {
  this->x /= x;
  this->y /= y;
}

/** Checks if a given x/y coord is within the bounding box
 *
 *  @param x :: x coordinate
 *  @param y :: y coordinate
 *  @return true/false if it is within the mins/maxs of the box
 */
bool WorkspaceBoundingBox::containsPoint(double x, double y) {
  if (x > this->xMax || x < this->xMin || y > yMax || y < yMin)
    return false;
  return true;
}

} // namespace Algorithms
} // namespace Mantid