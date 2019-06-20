// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_SINGLEMARKER_H
#define MPLCPP_SINGLEMARKER_H

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include <QHash>
#include <QVariant>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Wraps a python defined single marker object
 */
class MANTID_MPLCPP_DLL SingleMarker : public Common::Python::InstanceHolder {
public:
  explicit SingleMarker(
      FigureCanvasQt *canvas, QString const &color, double position,
      double minimum, double maximum, QString const &markerType,
      QHash<QString, QVariant> const &otherKwargs = QHash<QString, QVariant>());

  void redraw();
  void remove();

  void setColor(QString const &color);
  void setPosition(double position);
  double getPosition() const;

  void setBounds(double minimum, double maximum);
  void setLowerBound(double minimum);
  void setUpperBound(double maximum);

  void mouseMoveStart(double x, double y);
  void mouseMoveStop();
  bool mouseMove(double x, double y);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_SINGLEMARKER_H
