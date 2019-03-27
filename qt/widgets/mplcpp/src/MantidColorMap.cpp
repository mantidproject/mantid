// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/MantidColorMap.h"
#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <QInputDialog>
#include <QStringList>
#include <QWidget>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

// ------------------------ Static methods ---------------------
/**
 * @brief Ask the user to select a color map. Shows
 * @param previous
 * @param parent A widget to act as parent for the chooser dialog
 * @return
 */
QString MantidColorMap::chooseColorMap(const QString &previous,
                                       QWidget *parent) {
  static QStringList allowedCMaps{"coolwarm", "jet", "summer", "winter",
                                  "viridis"};
  const int currentIdx = allowedCMaps.indexOf(previous);
  bool ok;
  QString item =
      QInputDialog::getItem(parent, "Select colormap...", "Name:", allowedCMaps,
                            currentIdx >= 0 ? currentIdx : 0, false, &ok);
  if (ok && !item.isEmpty())
    return item;
  else
    return previous;
}

/**
 * @return The name of the default colormap
 */
QString MantidColorMap::defaultColorMap() { return defaultCMapName(); }

/**
 * @brief Check if a given color map exists. This interface has to
 * match the existing interface in LegacyQwt.
 * @param name The name of a colormap
 * @return The same name passed to the function if it exists
 * @throws std::runtime_error if the colomap does not exist
 */
QString MantidColorMap::exists(const QString &name) {
  getCMap(name); // throws if it does not exist
  return name;
}

// ------------------------ Public methods ---------------------

/**
 * Construct a default colormap
 */
MantidColorMap::MantidColorMap()
    : m_mappable(Normalize(0, 1), getCMap(defaultCMapName())) {}

/**
 * Reset the colormap to the default
 */
void MantidColorMap::setupDefaultMap() { loadMap(defaultCMapName()); }

/**
 * Load the given colormap into the object
 * @param name The name of an existing colormap
 * @return True if it suceeded, false otherwise
 */
bool MantidColorMap::loadMap(const QString &name) {
  if (cmapExists(name)) {
    m_mappable.setCmap(name);
    return true;
  } else {
    return false;
  }
}

/**
 * Switch the scale type of the map
 * @param type An enumeration giving the type of scale to use
 */
void MantidColorMap::changeScaleType(MantidColorMap::ScaleType type) {
  if (type == m_scaleType)
    return;
  m_scaleType = type;
  switch (type) {
  case ScaleType::Linear:
    m_mappable.setNorm(Normalize());
    break;
  case ScaleType::Log10:
    m_mappable.setNorm(SymLogNorm(SymLogNorm::DefaultLinearThreshold,
                                  SymLogNorm::DefaultLinearScale, 0, 1));
    break;
  case ScaleType::Power:
    m_mappable.setNorm(PowerNorm(m_gamma, 0, 1));
  }
}

/**
 * @return The current scale type of the map
 */
MantidColorMap::ScaleType MantidColorMap::getScaleType() const {
  return m_scaleType;
}

/**
 * @brief Set the value of the exponent for the power scale
 * @param gamma The value of the exponent
 */
void MantidColorMap::setNthPower(double gamma) {
  m_gamma = gamma;
  m_mappable.setNorm(PowerNorm(m_gamma, 0, 1));
}

/**
 * @brief Compute an RGB color value on the current scale type for the given
 * data value
 * @param vmin The minimum value of the data range
 * @param vmax The maximum value of the data range
 * @param value The value to be transformed
 * @return An instance QRgb describing the color
 */
QRgb MantidColorMap::rgb(double vmin, double vmax, double value) const {
  m_mappable.setClim(vmin, vmax);
  return m_mappable.toRGBA(value);
}

/**
 * @brief Compute RGB color values on the current scale type for the given
 * data values
 * @param vmin The minimum value of the data range
 * @param vmax The maximum value of the data range
 * @param values The values to be transformed
 * @return An array of QRgb describing the colors
 */
std::vector<QRgb> MantidColorMap::rgb(double vmin, double vmax,
                                      const std::vector<double> &values) const {
  m_mappable.setClim(vmin, vmax);
  return m_mappable.toRGBA(values);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
