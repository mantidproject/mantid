// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_MANTIDCOLORMAP_H
#define MPLCPP_MANTIDCOLORMAP_H

#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <QRgb>
#include <QString>

// Forward delcarations
class QWidget;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief The MantidColormap exists to provide an matplotlib-based
 * implementation that satisfies the same colormap interface used by the Qt4
 * Qwt-based version in the Plotting library.
 */
class MANTID_MPLCPP_DLL MantidColorMap {
public:
  /// Define the possible scale types
  enum class ScaleType { Linear = 0, Log10 = 1, Power = 2 };

  static QString chooseColorMap(const QString &previous, QWidget *parent);
  static QString defaultColorMap();
  static QString exists(const QString &name);

public:
  MantidColorMap();
  void setupDefaultMap();
  bool loadMap(const QString &name);
  void changeScaleType(ScaleType type);
  ScaleType getScaleType() const;
  void setNthPower(double gamma);
  double getNthPower() const { return m_gamma; }
  Colormap cmap() const { return m_mappable.cmap(); }

  QRgb rgb(double vmin, double vmax, double value) const;
  std::vector<QRgb> rgb(double vmin, double vmax,
                        const std::vector<double> &values) const;

private:
  mutable ScalarMappable m_mappable;
  ScaleType m_scaleType;
  double m_gamma = {2.0};
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_MANTIDCOLORMAP_H
