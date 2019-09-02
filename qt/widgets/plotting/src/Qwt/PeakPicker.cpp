// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/PeakPicker.h"
#include "MantidQtWidgets/Plotting/Qwt/PreviewPlot.h"

#include <qwt_painter.h>
#include <qwt_plot_canvas.h>

#include <QMouseEvent>
#include <QPainter>

#include "MantidAPI/FunctionFactory.h"

namespace MantidQt {
namespace MantidWidgets {
const double PeakPicker::DRAG_SENSITIVITY = 5.0;
const Qt::CursorShape PeakPicker::DEFAULT_CURSOR = Qt::PointingHandCursor;

/**
 * @param plot :: A plot this peak picker should operate on
 * @param color :: Peak picker color
 */
PeakPicker::PeakPicker(QwtPlot *plot, QColor color)
    : QwtPlotPicker(plot->canvas()), QwtPlotItem(), m_plot(plot),
      m_basePen(color, 0, Qt::SolidLine), m_widthPen(color, 0, Qt::DashLine),
      m_isMoving(false), m_isResizing(false), m_peak() {
  attach(plot);
  plot->canvas()->setCursor(DEFAULT_CURSOR);
}

PeakPicker::PeakPicker(PreviewPlot *plot, QColor color)
    : QwtPlotPicker(plot->canvas()), QwtPlotItem(), m_plot(plot->getPlot()),
      m_basePen(color, 0, Qt::SolidLine), m_widthPen(color, 0, Qt::DashLine),
      m_isMoving(false), m_isResizing(false), m_peak() {
  attach(m_plot);
  m_plot->canvas()->setCursor(DEFAULT_CURSOR);
}

bool PeakPicker::eventFilter(QObject *object, QEvent *event) {
  UNUSED_ARG(object);

  if (!m_peak) {
    return false; // Peak not set - nothing to do
  }

  switch (event->type()) {
  case QEvent::MouseButtonPress: {
    auto mouseEvent = static_cast<QMouseEvent *>(event);
    Qt::KeyboardModifiers mod = mouseEvent->modifiers();
    QPoint p = mouseEvent->pos();

    // Widget coordinates of left and right width bars
    int xLeft = m_plot->transform(QwtPlot::xBottom,
                                  m_peak->centre() - (m_peak->fwhm() / 2.0));
    int xRight = m_plot->transform(QwtPlot::xBottom,
                                   m_peak->centre() + (m_peak->fwhm() / 2.0));

    // If clicked with Ctrl pressed, or close enough to one of the width bars -
    // start resizing
    if (mod.testFlag(Qt::ControlModifier) ||
        std::abs(p.x() - xLeft) < DRAG_SENSITIVITY ||
        std::abs(p.x() - xRight) < DRAG_SENSITIVITY) {
      m_isResizing = true;
      m_plot->canvas()->setCursor(Qt::SizeHorCursor);
    }

    // Widget point of the peak tip
    QPoint peakTip;
    peakTip.setX(m_plot->transform(QwtPlot::xBottom, m_peak->centre()));
    peakTip.setY(m_plot->transform(QwtPlot::yLeft, m_peak->height()));

    // If clicked with Shift pressed or close enough to peak tip - start moving
    if (mod.testFlag(Qt::ShiftModifier) ||
        QLineF(p, peakTip).length() < DRAG_SENSITIVITY) {
      m_isMoving = true;
      m_plot->canvas()->setCursor(Qt::SizeAllCursor);
    }

    // XXX: fall through intentionally, so that the user instantly sees a new
    // PeakPicker
    //      position when starts dragging
  }
  case QEvent::MouseMove: {
    QPoint p = static_cast<QMouseEvent *>(event)->pos();

    // Move, if moving in process
    if (m_isMoving) {
      m_peak->setCentre(m_plot->invTransform(QwtPlot::xBottom, p.x()));
      m_peak->setHeight(m_plot->invTransform(QwtPlot::yLeft, p.y()));
    }

    // Resize, if resizing in process
    if (m_isResizing) {
      m_peak->setFwhm(fabs(m_peak->centre() -
                           m_plot->invTransform(QwtPlot::xBottom, p.x())) *
                      2);
    }

    // If moving or resizing in process - update the plot and accept the event
    if (m_isResizing || m_isMoving) {
      m_plot->replot();
      emit changed();
      return true;
    }

    break;
  }
  case QEvent::MouseButtonRelease: {
    // If are moving or resizing - stop
    if (m_isMoving || m_isResizing) {
      m_isMoving = m_isResizing = false;
      m_plot->canvas()->setCursor(DEFAULT_CURSOR);
      return true;
    }

    break;
  }
  default:
    break;
  }

  return false;
}

Mantid::API::IPeakFunction_const_sptr PeakPicker::peak() const {
  return m_peak;
}

void PeakPicker::setPeak(const Mantid::API::IPeakFunction_const_sptr &peak) {
  // Copy the function
  m_peak = boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(
      Mantid::API::FunctionFactory::Instance().createInitialized(
          peak->asString()));
}

void PeakPicker::draw(QPainter *painter, const QwtScaleMap &xMap,
                      const QwtScaleMap &yMap, const QRect &canvasRect) const {
  if (!m_peak) {
    return; // Peak not set - nothing to do
  }

  painter->setPen(m_basePen);

  int x = xMap.transform(m_peak->centre());
  int y = yMap.transform(m_peak->height());
  int y0 = yMap.transform(0);

  // Draw vertical line from peak base to peak tip
  QwtPainter::drawLine(painter, x, y0, x, y);

  int xMin = xMap.transform(m_peak->centre() - (m_peak->fwhm() / 2.0));
  int xMax = xMap.transform(m_peak->centre() + (m_peak->fwhm() / 2.0));

  // Draw horizontal line at peak base
  QwtPainter::drawLine(painter, xMin, y0, xMax, y0);

  int yTop = canvasRect.top();
  int yBottom = canvasRect.bottom();

  painter->setPen(m_widthPen);

  // Draw width lines
  QwtPainter::drawLine(painter, xMin, yBottom, xMin, yTop);
  QwtPainter::drawLine(painter, xMax, yBottom, xMax, yTop);
}

} // namespace MantidWidgets
} // namespace MantidQt
