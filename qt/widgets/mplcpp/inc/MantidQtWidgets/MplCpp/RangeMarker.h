// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  explicit RangeMarker(const FigureCanvasQt *canvas, QString const &color, double x_minimum, double x_maximum,
                       QString const &rangeType,
                       QHash<QString, QVariant> const &otherKwargs = QHash<QString, QVariant>());

  void redraw();
  void remove();

  void setColor(QString const &color);

  void setBounds(double lowerBound, double upperBound);
  void setLowerBound(double lowerBound);
  void setUpperBound(double upperBound);

  void setRange(double minimum, double maximum);
  std::tuple<double, double> getRange() const;

  void setMinimum(double minimum);
  void setMaximum(double maximum);
  double getMinimum() const;
  double getMaximum() const;

  void mouseMoveStart(double x, double y);
  void mouseMoveStop();
  bool mouseMove(double x, double y);

  bool isMoving();
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
