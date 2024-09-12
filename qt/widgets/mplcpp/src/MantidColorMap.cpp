// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/MantidColorMap.h"
#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QStringList>
#include <QWidget>

using namespace MantidQt::Widgets::Common;

namespace MantidQt::Widgets::MplCpp {

// ------------------------ Static methods ---------------------
/**
 * @brief Ask the user to select a color map. Shows
 * @param previous
 * @param parent A widget to act as parent for the chooser dialog
 * @return
 */
std::pair<QString, bool> MantidColorMap::chooseColorMap(const std::pair<QString, bool> &previous, QWidget *parent) {
  static QStringList allowedCMaps{"coolwarm", "gray", "jet", "plasma", "summer", "winter", "viridis"};
  const int currentIdx = allowedCMaps.indexOf(previous.first);

  // create simple dialog to capture color map and highlight flag
  QDialog colormapDialog(parent);
  QFormLayout form(&colormapDialog);

  QComboBox *colorMapCombo = new QComboBox(&colormapDialog);
  colorMapCombo->addItems(allowedCMaps);
  colorMapCombo->setCurrentIndex(currentIdx >= 0 ? currentIdx : 0);
  form.addRow(colorMapCombo);

  QCheckBox *highlightZeroCounts = new QCheckBox(&colormapDialog);
  highlightZeroCounts->setChecked(previous.second);
  form.addRow(new QLabel("Highlight Detectors With Zero Counts"), highlightZeroCounts);

  QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &colormapDialog);
  form.addRow(&buttonBox);
  QObject::connect(&buttonBox, SIGNAL(accepted()), &colormapDialog, SLOT(accept()));
  QObject::connect(&buttonBox, SIGNAL(rejected()), &colormapDialog, SLOT(reject()));

  // display modal dialog
  if (colormapDialog.exec() == QDialog::Accepted) {
    QString item = colorMapCombo->currentText();
    bool highlightZeros = highlightZeroCounts->isChecked();
    return std::make_pair(item, highlightZeros);
  } else {
    return previous;
  }
}

/**
 * @return The name of the default colormap
 */
QString MantidColorMap::defaultColorMap() { return defaultCMapName(); }

/**
 * @brief Check if a given color map exists. This interface has to
 * match the existing interface in Plotting.
 * @param name The name of a colormap
 * @return The same name passed to the function if it exists
 * @throws std::invalid_argument if the colomap does not exist
 */
QString MantidColorMap::exists(const QString &name) {
  getCMap(name); // throws if it does not exist
  return name;
}

// ------------------------ Public methods ---------------------

/**
 * Construct a default colormap
 */
MantidColorMap::MantidColorMap() : m_mappable(Normalize(0, 1), getCMap(defaultCMapName())) {}

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
    m_mappable.setNorm(SymLogNorm(SymLogNorm::DefaultLinearThreshold, SymLogNorm::DefaultLinearScale, 0, 1));
    break;
  case ScaleType::Power:
    m_mappable.setNorm(PowerNorm(m_gamma, 0, 1));
  }
}

/**
 * @return The current scale type of the map
 */
MantidColorMap::ScaleType MantidColorMap::getScaleType() const { return m_scaleType; }

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
QRgb MantidColorMap::rgb(double vmin, double vmax, double value) {
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
std::vector<QRgb> MantidColorMap::rgb(double vmin, double vmax, const std::vector<double> &values) {
  m_mappable.setClim(vmin, vmax);
  return m_mappable.toRGBA(values);
}

} // namespace MantidQt::Widgets::MplCpp
