// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/MantidQwtWorkspaceData.h"

#include <cmath>

namespace {
/// Minimum value considered positive
constexpr double MIN_POSITIVE = 1e-3;
/// Maximum value considered positive
constexpr double MAX_POSITIVE = 1e30;
/// Arbitrary multiplier between min/max if they are equal
constexpr double MIN_MAX_DELTA = 1.001;
} // namespace

MantidQwtWorkspaceData::MantidQwtWorkspaceData(bool logScaleY)
    : m_logScaleY(logScaleY), m_minY(0), m_minPositive(0), m_maxY(0),
      m_plottable(DataStatus::Undefined), m_isWaterfall(false), m_offsetX(0),
      m_offsetY(0) {}

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
  m_plottable = data.m_plottable;
  m_isWaterfall = data.m_isWaterfall;
  m_offsetX = data.m_offsetX;
  m_offsetY = data.m_offsetY;
  return *this;
}

/// Calculate absolute minimum and maximum values in a vector. Also find the
/// smallest positive value.
void MantidQwtWorkspaceData::calculateYMinAndMax() const {
  // Set this to true to get the "real" data size
  // It's correct value is then recalculated below. This is not
  // too nice but a big refactor is not worth it given the new
  // workbench/plotting developments.
  m_plottable = DataStatus::Plottable;
  m_minY = m_maxY = m_minPositive = 0.0;

  double ymin(std::numeric_limits<double>::max()),
      ymax(-std::numeric_limits<double>::max()),
      yminPos(std::numeric_limits<double>::max());
  for (size_t i = 0; i < size(); ++i) {
    auto val = y(i);
    // skip NaNs
    if (!std::isfinite(val))
      continue;

    // Update our values as appropriate
    if (val < ymin)
      ymin = val;
    if (val > 0.0 && val < yminPos)
      yminPos = val;
    if (val > ymax)
      ymax = val;
  }

  if (ymin < std::numeric_limits<double>::max()) {
    // Values are sensible range
    m_minY = ymin;
    // Ensure there is a difference beteween max and min
    m_maxY = (ymax != ymin) ? ymax : ymin * MIN_MAX_DELTA;

    // Minimum positive value is kept for log scales
    if (yminPos < std::numeric_limits<double>::max()) {
      m_minPositive = yminPos;
      m_plottable = DataStatus::Plottable;
    } else {
      // All values are <= 0
      m_minPositive = MIN_POSITIVE;
      m_plottable =
          logScaleY() ? DataStatus::NotPlottable : DataStatus::Plottable;
    }
  } else {
    // Set to arbitrary values (this is unlikely to happen)
    m_minY = 0.0;
    m_maxY = MAX_POSITIVE;
    m_minPositive = MIN_POSITIVE;
    m_plottable = DataStatus::NotPlottable;
  }
}

void MantidQwtWorkspaceData::setLogScaleY(bool on) {
  m_logScaleY = on;
  calculateYMinAndMax();
}

bool MantidQwtWorkspaceData::logScaleY() const { return m_logScaleY; }

void MantidQwtWorkspaceData::setMinimumPositiveValue(const double v) {
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
  if (m_plottable == DataStatus::Undefined) {
    calculateYMinAndMax();
  }
  return m_logScaleY ? m_minPositive : m_minY;
}

/**
 * Depending upon whether the log options have been set.
 * @return the highest y value.
 */
double MantidQwtWorkspaceData::getYMax() const {
  if (m_plottable == DataStatus::Undefined) {
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

size_t MantidQwtWorkspaceData::esize() const {
  if (!isPlottable()) {
    return 0;
  }
  return this->size();
}

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

/**
 * @brief MantidQwtWorkspaceData::isPlottable
 * Data is considered plottable if either:
 *   - scale != log or
 *   - scale == log & all(y) > 0.0
 * @return True if the data is considered plottable, false otherwise
 */
bool MantidQwtWorkspaceData::isPlottable() const {
  return (m_plottable == DataStatus::Plottable);
}

//------------------------------------------------------------------------------
// MantidQwtMatrixWorkspaceData class
//------------------------------------------------------------------------------
MantidQwtMatrixWorkspaceData::MantidQwtMatrixWorkspaceData(bool logScaleY)
    : MantidQwtWorkspaceData(logScaleY) {}
