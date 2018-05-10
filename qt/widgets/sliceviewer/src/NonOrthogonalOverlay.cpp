#include "MantidQtWidgets/SliceViewer/NonOrthogonalOverlay.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Utils.h"
#include "MantidQtWidgets/Common/NonOrthogonal.h"
#include <QPainter>
#include <QRect>
#include <QShowEvent>
#include <numeric>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_div.h>

using namespace Mantid::Kernel;

namespace MantidQt {
namespace SliceViewer {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NonOrthogonalOverlay::NonOrthogonalOverlay(QwtPlot *plot, QWidget *parent)
    : QWidget(parent), m_plot(plot), m_xAngle(0.), m_yAngle(0.) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
NonOrthogonalOverlay::~NonOrthogonalOverlay() {}

/// Return the recommended size of the widget
QSize NonOrthogonalOverlay::sizeHint() const {
  // TODO: Is there a smarter way to find the right size?
  return QSize(20000, 20000);
}

QSize NonOrthogonalOverlay::size() const { return m_plot->canvas()->size(); }
int NonOrthogonalOverlay::height() const { return m_plot->canvas()->height(); }
int NonOrthogonalOverlay::width() const { return m_plot->canvas()->width(); }

//----------------------------------------------------------------------------------------------
/** Tranform from plot coordinates to pixel coordinates
 * @param coords :: coordinate point in plot coordinates
 * @return pixel coordinates */
QPoint NonOrthogonalOverlay::transform(QPointF coords) const {
  auto xA = m_plot->transform(QwtPlot::xBottom, coords.x());
  auto yA = m_plot->transform(QwtPlot::yLeft, coords.y());
  return QPoint(xA, yA);
}

//----------------------------------------------------------------------------------------------
/** Inverse transform: from pixels to plot coords
 * @param pixels :: location in pixels
 * @return plot coordinates (float)   */
QPointF NonOrthogonalOverlay::invTransform(QPoint pixels) const {
  auto xA = m_plot->invTransform(QwtPlot::xBottom, pixels.x());
  auto yA = m_plot->invTransform(QwtPlot::yLeft, pixels.y());
  return QPointF(xA, yA);
}

void NonOrthogonalOverlay::updateXGridlines(QwtValueList xAxisTicks,
                                            double xAngle) {
  m_xAxisTicks = xAxisTicks;
  m_xAngle = xAngle;
  auto size = xAxisTicks.size();
  if (m_xAngle != 0 && size >= 1) {
    double firstTick;
    double lastTick;
    double diff;
    firstTick = xAxisTicks.at(0);
    lastTick = xAxisTicks.last();
    diff = xAxisTicks.at(1) - firstTick;
    for (auto j = 0; j < size * 2; j++) {
      auto tick = firstTick - diff * j;
      m_xAxisTicks.append(tick);
      tick = lastTick + diff * j;
      m_xAxisTicks.append(tick);
    }
  }
}

void NonOrthogonalOverlay::updateYGridlines(QwtValueList yAxisTicks,
                                            double yAngle) {
  m_yAxisTicks = yAxisTicks;
  m_yAngle = yAngle;
  auto size = yAxisTicks.size();
  if (m_yAngle != 0 && size >= 1) {
    double firstTick;
    double lastTick;
    double diff;
    firstTick = yAxisTicks.at(0);
    lastTick = yAxisTicks.last();
    diff = yAxisTicks.at(1) - firstTick;
    for (auto j = 0; j < size * 2; j++) {
      auto tick = firstTick - diff * j;
      m_yAxisTicks.append(tick);
      tick = lastTick + diff * j;
      m_yAxisTicks.append(tick);
    }
  }
}

//----------------------------------------------------------------------------------------------
/// Paint the overlay
void NonOrthogonalOverlay::paintEvent(QPaintEvent * /*event*/) {

  if (m_enabled) {
    QPainter painter(this);

    QPen gridPen(QColor(100, 100, 100, 100)); // grey
    gridPen.setWidth(1);
    gridPen.setCapStyle(Qt::FlatCap);
    gridPen.setStyle(Qt::DashLine);

    const auto widthScreen = width();
    const auto heightScreen = height();

    drawYLines(painter, gridPen, widthScreen, m_yAxisTicks, m_yAngle);
    drawXLines(painter, gridPen, heightScreen, m_xAxisTicks, m_xAngle);
  }
}

void NonOrthogonalOverlay::drawYLines(QPainter &painter, QPen &gridPen,
                                      int widthScreen, QwtValueList yAxisTicks,
                                      double yAngle) {

  auto offset = yAngle == 0. ? 0. : widthScreen * tan(yAngle);
  painter.setPen(gridPen);
  for (auto &tick : yAxisTicks) {
    auto tickScreen = m_plot->transform(QwtPlot::yLeft, tick);
    auto start = QPointF(0, tickScreen);
    auto end = QPointF(widthScreen, tickScreen - offset);
    painter.drawLine(start, end);
  }
}

void NonOrthogonalOverlay::drawXLines(QPainter &painter, QPen &gridPen,
                                      int heightScreen, QwtValueList xAxisTicks,
                                      double xAngle) {
  xAngle *= -1.f;
  auto offset = xAngle == 0. ? 0. : heightScreen * tan(xAngle);
  painter.setPen(gridPen);
  for (auto &tick : xAxisTicks) {
    auto tickScreen = m_plot->transform(QwtPlot::xBottom, tick);
    auto start = QPointF(tickScreen, heightScreen);
    auto end = QPointF(tickScreen + offset, 0);
    painter.drawLine(start, end);
  }
}

void NonOrthogonalOverlay::enable() { m_enabled = true; }

void NonOrthogonalOverlay::disable() { m_enabled = false; }

} // namespace SliceViewer
} // namespace MantidQt
