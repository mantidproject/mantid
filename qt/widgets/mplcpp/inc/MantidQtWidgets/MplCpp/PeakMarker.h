// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_PEAKMARKER_H
#define MPLCPP_PEAKMARKER_H

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
class MANTID_MPLCPP_DLL PeakMarker : public Common::Python::InstanceHolder {
public:
  explicit PeakMarker(
      FigureCanvasQt *canvas, int peakID, double x, double yTop, double yBottom,
      double fwhm,
      QHash<QString, QVariant> const &otherKwargs = QHash<QString, QVariant>());

  void redraw();
  void remove();

  void updatePeak(double centre, double height, double fwhm);
  std::tuple<double, double, double> peakProperties() const;

  bool isMoving() const;

  void select();
  void deselect();

  void mouseMoveStart(double x, double y);
  void mouseMoveStop();
  bool mouseMove(double x, double y);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_PEAKMARKER_H
