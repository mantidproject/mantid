#include "MantidQtAPI/MantidQwtWorkspaceData.h"

#include <cmath>

MantidQwtWorkspaceData::MantidQwtWorkspaceData(bool logScaleY)
    : m_logScaleY(logScaleY), m_minY(0), m_minPositive(0), m_maxY(0),
      m_isWaterfall(false), m_offsetX(0), m_offsetY(0) {}

MantidQwtWorkspaceData::MantidQwtWorkspaceData(
    const MantidQwtWorkspaceData &data) {
  *this = data;
}

/// This cannot be declared default because QwtData doesn't implement operator=.
MantidQwtWorkspaceData &MantidQwtWorkspaceData::
operator=(const MantidQwtWorkspaceData &data) {
  m_logScaleY = data.m_logScaleY;
  m_minY = data.m_minY;
  m_minPositive = data.m_minPositive;
  m_maxY = data.m_maxY;
  m_isWaterfall = data.m_isWaterfall;
  m_offsetX = data.m_offsetX;
  m_offsetY = data.m_offsetY;
  return *this;
}

/// Calculate absolute minimum and maximum values in a vector. Also find the
/// smallest positive value.
void MantidQwtWorkspaceData::calculateYMinAndMax() const {

  const double maxDouble = std::numeric_limits<double>::max();
  double curMin = maxDouble;
  double curMinPos = maxDouble;
  double curMax = -maxDouble;
  for (size_t i = 0; i < size(); ++i) {
    auto val = y(i);
    // skip NaNs
    if (!std::isfinite(val))
      continue;

    // Update our values as appropriate
    if (val < curMin)
      curMin = val;
    if (val < curMinPos && val > 0)
      curMinPos = val;
    if (val > curMax)
      curMax = val;
  }

  // Save the results
  if (curMin == maxDouble) {
    m_minY = 0.0;
    m_minPositive = 0.1;
    m_maxY = 1.0;
    return;
  } else {
    m_minY = curMin;
  }

  if (curMax == curMin) {
    curMax *= 1.1;
  }
  m_maxY = curMax;

  if (curMinPos == maxDouble) {
    m_minPositive = 0.1;
  } else {
    m_minPositive = curMinPos;
  }
}

void MantidQwtWorkspaceData::setLogScaleY(bool on) { m_logScaleY = on; }

bool MantidQwtWorkspaceData::logScaleY() const { return m_logScaleY; }

void MantidQwtWorkspaceData::saveLowestPositiveValue(const double v) {
  if (v > 0)
    m_minPositive = v;
}

void MantidQwtWorkspaceData::setXOffset(const double x) { m_offsetX = x; }

void MantidQwtWorkspaceData::setYOffset(const double y) { m_offsetY = y; }

void MantidQwtWorkspaceData::setWaterfallPlot(bool on) { m_isWaterfall = on; }

bool MantidQwtWorkspaceData::isWaterfallPlot() const { return m_isWaterfall; }

/**
 * Depending upon whether the log options have been set.
 * @return the lowest y value.
 */
double MantidQwtWorkspaceData::getYMin() const {
  if (m_minPositive == 0.0) {
    calculateYMinAndMax();
  }
  return m_logScaleY ? m_minPositive : m_minY;
}

/**
 * Depending upon whether the log options have been set.
 * @return the highest y value.
 */
double MantidQwtWorkspaceData::getYMax() const {
  if (m_minPositive == 0.0) {
    calculateYMinAndMax();
  }
  if (m_logScaleY && m_maxY <= 0)
    return m_isWaterfall ? m_minPositive + m_offsetY : m_minPositive;
  else
    return m_isWaterfall ? m_maxY + m_offsetY : m_maxY;
}

double MantidQwtWorkspaceData::x(size_t i) const { return getX(i); }

double MantidQwtWorkspaceData::y(size_t i) const {
  double tmp = getY(i);
  if (m_logScaleY && tmp <= 0.) {
    tmp = m_minPositive;
  }
  return tmp;
}

size_t MantidQwtWorkspaceData::esize() const { return this->size(); }

double MantidQwtWorkspaceData::e(size_t i) const {
  double ei = getE(i);
  if (m_logScaleY) {
    double yi = getY(i);
    if (yi <= 0.0)
      return 0;
    else
      return ei;
  } else
    return ei;
}

double MantidQwtWorkspaceData::ex(size_t i) const { return getEX(i); }

MantidQwtMatrixWorkspaceData::MantidQwtMatrixWorkspaceData(bool logScaleY)
    : MantidQwtWorkspaceData(logScaleY) {}
