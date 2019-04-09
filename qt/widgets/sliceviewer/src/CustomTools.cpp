// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/CustomTools.h"
#include <iosfwd>

namespace MantidQt {
namespace SliceViewer {

CustomPicker::CustomPicker(int xAxis, int yAxis, QwtPlotCanvas *canvas)
    : QwtPlotPicker(xAxis, yAxis, 0, CrossRubberBand, AlwaysOn, canvas) {
  setSelectionFlags(QwtPicker::PointSelection);
  setRubberBand(QwtPicker::CrossRubberBand);
  canvas->setMouseTracking(true);
}

/** Called each time the mouse moves over the canvas */
QwtText CustomPicker::trackerText(const QwtDoublePoint &pos) const {
  emit mouseMoved(pos.x(), pos.y());
  return QwtText();
}

void CustomPicker::widgetMouseMoveEvent(QMouseEvent *e) {
  if (!isActive()) {
    setSelectionFlags(QwtPicker::PointSelection);

    begin();
    append(e->pos());
  }

  QwtPlotPicker::widgetMouseMoveEvent(e);
}

void CustomPicker::widgetLeaveEvent(QEvent * /*unused*/) { end(); }

void CustomMagnifier::rescale(double factor) {
  if (factor != 0.0) {
    QwtPlotMagnifier::rescale(1 / factor);
    emit rescaled(factor);
  }
}
} // namespace SliceViewer
} // namespace MantidQt
