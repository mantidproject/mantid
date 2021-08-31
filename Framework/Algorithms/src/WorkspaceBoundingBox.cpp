#include "MantidAlgorithms/WorkspaceBoundingBox.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"

namespace Mantid {
namespace Algorithms {

WorkspaceBoundingBox::WorkspaceBoundingBox(API::MatrixWorkspace_sptr workspace, Kernel::Logger &g_log)
    : workspace(workspace), g_log(g_log) {}

WorkspaceBoundingBox::WorkspaceBoundingBox(Kernel::Logger &g_log) : g_log(g_log) {}

WorkspaceBoundingBox::~WorkspaceBoundingBox() {}

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

bool WorkspaceBoundingBox::isValidWs(int index) {
  const auto spectrumInfo = this->workspace->spectrumInfo();
  if (!spectrumInfo.hasDetectors(index)) {
    g_log.warning() << "Workspace index " << index << " has no detector assigned to it - discarding\n";
    return false;
  }
  // Skip if we have a monitor or if the detector is masked.
  if (spectrumInfo.isMonitor(index) || spectrumInfo.isMasked(index))
    return false;

  // Get the current spectrum
  auto &YIn = this->workspace->y(index);
  // Skip if NaN of inf
  if (std::isnan(YIn[m_specID]) || std::isinf(YIn[m_specID]))
    return false;
  return true;
}

int WorkspaceBoundingBox::findFirstValidWs(const int numSpec) {
  const auto spectrumInfo = this->workspace->spectrumInfo();
  int i;
  for (i = 0; i < numSpec; ++i) {
    if (isValidWs(i))
      break;
  }
  return i;
}

double WorkspaceBoundingBox::updatePositionAndReturnCount(int index) {
  const auto spectrumInfo = this->workspace->spectrumInfo();
  auto &YIn = this->workspace->y(index);
  double x = spectrumInfo.position(index).X();
  double y = spectrumInfo.position(index).Y();
  this->x += YIn[m_specID] * x;
  this->y += YIn[m_specID] * y;
  return YIn[m_specID];
}

void WorkspaceBoundingBox::updateMinMax(int index) {
  const auto spectrumInfo = this->workspace->spectrumInfo();
  double x = spectrumInfo.position(index).X();
  double y = spectrumInfo.position(index).Y();
  this->xMin = std::min(x, this->xMin);
  this->xMax = std::max(x, this->xMax);
  this->yMin = std::min(y, this->yMin);
  this->yMax = std::max(y, this->yMax);
}

bool WorkspaceBoundingBox::isOutOfBoundsOfNonDirectBeam(const double beamRadius, int index, const bool directBeam) {
  const auto spectrumInfo = this->workspace->spectrumInfo();
  double x = spectrumInfo.position(index).X();
  double y = spectrumInfo.position(index).Y();
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

void WorkspaceBoundingBox::normalizePosition(double x, double y) {
  this->x /= x;
  this->y /= y;
}

bool WorkspaceBoundingBox::containsPoint(double x, double y) {
  if (x > this->xMax || x < this->xMin || y > yMax || y < yMin)
    return false;
  return true;
}

} // namespace Algorithms
} // namespace Mantid