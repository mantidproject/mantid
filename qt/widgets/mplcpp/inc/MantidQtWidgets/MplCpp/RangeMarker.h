// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_RANGEMARKER_H
#define MPLCPP_RANGEMARKER_H

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include <QHash>
#include <QVariant>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Wraps a python defined range marker object
 */
class MANTID_MPLCPP_DLL RangeMarker : public Common::Python::InstanceHolder {
public:
  explicit RangeMarker(
      FigureCanvasQt *canvas, QString const &color, double x_minimum,
      double x_maximum,
      QHash<QString, QVariant> const &otherKwargs = QHash<QString, QVariant>());

  void redraw();
  void remove();

  void setColor(QString const &color);
  void setXRange(double minimum, double maximum);
  std::tuple<double, double> getXRange() const;

  void setMinimum(double minimum);
  void setMaximum(double maximum);
  double getMinimum() const;
  double getMaximum() const;

  void mouseMoveStart(double x, double y);
  void mouseMoveStart(int x, int y);
  void mouseMoveStop();
  bool mouseMove(double x, double y);
  bool mouseMove(int x, int y);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_RANGEMARKER_H
