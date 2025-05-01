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
class MANTID_MPLCPP_DLL PeakMarker : public Common::Python::InstanceHolder {
public:
  explicit PeakMarker(const FigureCanvasQt *canvas, int peakID, double x, double height, double fwhm, double background,
                      QHash<QString, QVariant> const &otherKwargs = QHash<QString, QVariant>());

  void redraw();
  void remove();

  void updatePeak(double centre, double height, double fwhm, double background);
  std::tuple<double, double, double> peakProperties() const;

  bool isMoving() const;

  void select();
  void deselect();

  void setVisible(bool visible);

  void mouseMoveStart(double x, double y);
  void mouseMoveStop();
  bool mouseMove(double x, double y);
  void mouseMoveHover(double x, double y);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
