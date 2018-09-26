#include "MantidQtWidgets/MplCpp/MantidColorMap.h"
#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <QInputDialog>
#include <QStringList>
#include <QWidget>

namespace {
QString DEFAULT_CMAP = "viridis";
}

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
QString MantidColorMap::defaultColorMap() { return DEFAULT_CMAP; }

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
MantidColorMap::MantidColorMap() : m_cmap(getCMap(DEFAULT_CMAP)) {}

/**
 * Reset the colormap to the default
 */
void MantidColorMap::setupDefaultMap() { loadMap(DEFAULT_CMAP); }

/**
 * Load the given colormap into the object
 * @param name The name of an existing colormap
 * @return True if it suceeded, false otherwise
 */
bool MantidColorMap::loadMap(const QString &name) {
  try {
    m_cmap = getCMap(name);
    return true;
  } catch (std::exception &) {
    return false;
  }
}

/**
 * Switch the scale type of the map
 * @param type An enumeration giving the type of scale to use
 */
void MantidColorMap::changeScaleType(MantidColorMap::ScaleType type) {
  m_scaleType = type;
}

/**
 * @return The current scale type of the map
 */
MantidColorMap::ScaleType MantidColorMap::getScaleType() const {
  return m_scaleType;
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
  ScalarMappable mappable(Normalize(vmin, vmax), m_cmap);
  return mappable.toRGBA(value);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
