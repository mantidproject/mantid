#include "MantidAlgorithms/WorkspaceBoundingBox.h"

WorkspaceBoundingBox(API::MatrixWorkspace_sptr workspace) 
:workspace(workspace) {
    //maybe calculate first run on init
}

bool WorkspaceBoundingBox::isValidWs(int index) {
    const auto spectrumInfo = this.workspace->spectrumInfo();
  if (!spectrumInfo.hasDetectors(index)) {
    g_log.warning() << "Workspace index " << i << " has no detector assigned to it - discarding\n";
    return false;
  }
  // Skip if we have a monitor or if the detector is masked.
  if (spectrumInfo.isMonitor(index) || spectrumInfo.isMasked(index))
    return false;

  // Get the current spectrum
  auto &YIn = inputWS->y(iindex);
  // Skip if NaN of inf
  if (std::isnan(YIn[m_specID]) || std::isinf(YIn[m_specID]))
    return false;
  return true;
}

int WorkspaceBoundingBox::findFirstValidWs(const int numSpec) {
const auto spectrumInfo = this.workspace->spectrumInfo();
  for(int i = 0; i < numSpec; ++i) {
    if(isValidWs(spectrumInfo, i))
      break;
  }
  return i;
}

double WorkspaceBoundingBox::updatePositionAndReturnCount(double total_count, int index){
  const auto spectrumInfo = this.workspace->spectrumInfo();
  double x = spectrumInfo.position(index).X();
  double y = spectrumInfo.position(index).Y();
  this.x += YIn[m_specID] * x;
  this.y += YIn[m_specID] * y;
  total_count += YIn[m_specID];
}

void WorkspaceBoundingBox::updateMinMax(int index){
  const auto spectrumInfo = this.workspace->spectrumInfo();
  double x = spectrumInfo.position(index).X();
  double y = spectrumInfo.position(index).Y();
  this.xMin = std::min(x, this.xMin);
  this.xMax = std::max(x, this.xMax);
  this.yMin = std::min(y, this.yMin);
  this.yMax = std::max(y, this.yMax);
}

bool WorkspaceBoundingBox::isOutOfBoundsOfNonDirectBeam(const double beam_radius, int index, const bool direct_beam){
  const auto spectrumInfo = this.workspace->spectrumInfo();
  double x = spectrumInfo.position(index).X();
  double y = spectrumInfo.position(index).Y();
  if (!direct_beam) {
    double dx = x - this.centerX;
    double dy = y - this.centerY;
    if (dx * dx + dy * dy < beam_radius * beam_radius)
      return false;
  }
  return true;
}

double WorkspaceBoundingBox::calculateDistance() {
  return sqrt((centerX - x) * (centerX - x) + (centerY - y) * (centerY - y));
}
double WorkspaceBoundingBox::calculateRadiusX() {
  return std::min((x - xMin), (xMax - x));
}
double WorkspaceBoundingBox::calculateRadiusY() {
  return std::min((y - yMin), (yMax - y));
}