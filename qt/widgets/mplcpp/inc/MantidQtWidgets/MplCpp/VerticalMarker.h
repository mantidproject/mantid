// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_VERTICALMARKER_H
#define MPLCPP_VERTICALMARKER_H

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include <QHash>
#include <QVariant>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Wraps a python defined vertical marker object
 */
class MANTID_MPLCPP_DLL VerticalMarker : public Common::Python::InstanceHolder {
public:
  explicit VerticalMarker(
      FigureCanvasQt *canvas, QString const &colour, double x,
      QHash<QString, QVariant> const &otherKwargs = QHash<QString, QVariant>());
  void redraw();
  void remove();

  void setColor(QString const &colour);
  void setXPosition(double x);

  void mouseMoveStart(double x, double y);
  void mouseMoveStop();
  bool mouseMove(double x);

  bool isMoving();

  std::tuple<double, double> transformPixelsToCoords(int xPixels, int yPixels);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_VERTICALMARKER_H
