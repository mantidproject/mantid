#ifndef MPLCPP_MANTIDCOLORMAP_H
#define MPLCPP_MANTIDCOLORMAP_H
/*
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
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
 * Qwt-based version in the LegacyQwt library.
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

  QRgb rgb(double vmin, double vmax, double value) const;

private:
  mutable ScalarMappable m_mappable;
  ScaleType m_scaleType;
  double m_gamma = {2.0};
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_MANTIDCOLORMAP_H
