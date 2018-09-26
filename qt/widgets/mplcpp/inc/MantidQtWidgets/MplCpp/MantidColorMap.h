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
#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

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
  enum class ScaleType { Linear = 0, Log10, Power };

  static QString chooseColorMap(const QString &previous, QWidget *parent);
  static QString defaultColorMap();
  static QString exists(const QString &name);

public:
  MantidColorMap();
  void setupDefaultMap();
  bool loadMap(const QString &name);
  void changeScaleType(ScaleType type);
  ScaleType getScaleType() const;
  void setNthPower(double gamma) {}
  double getNthPower() const { return 2.0; }

  QRgb rgb(double vmin, double vmax, double value) const;

private:
  Colormap m_cmap;
  ScaleType m_scaleType = {ScaleType::Linear};
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_MANTIDCOLORMAP_H
