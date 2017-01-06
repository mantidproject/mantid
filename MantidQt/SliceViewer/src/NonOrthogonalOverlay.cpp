#include "MantidQtSliceViewer/NonOrthogonalOverlay.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qpainter.h>
#include <QRect>
#include <QShowEvent>
#include <qwt_scale_div.h>
#include "MantidKernel/Utils.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtAPI/NonOrthogonal.h"
#include <numeric>

using namespace Mantid::Kernel;

namespace MantidQt {
namespace SliceViewer {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NonOrthogonalOverlay::NonOrthogonalOverlay(QwtPlot *plot, QWidget *parent)
    : QWidget(parent), m_plot(plot), m_angleX(0.), m_angleY(0.) {
  m_fromHklToOrthogonal[0] = 1.0;
  m_fromHklToOrthogonal[1] = 0.0;
  m_fromHklToOrthogonal[2] = 0.0;
  m_fromHklToOrthogonal[3] = 0.0;
  m_fromHklToOrthogonal[4] = 1.0;
  m_fromHklToOrthogonal[5] = 0.0;
  m_fromHklToOrthogonal[6] = 0.0;
  m_fromHklToOrthogonal[7] = 0.0;
  m_fromHklToOrthogonal[8] = 1.0;

  m_fromOrthogonalToHkl[0] = 1.0;
  m_fromOrthogonalToHkl[1] = 0.0;
  m_fromOrthogonalToHkl[2] = 0.0;
  m_fromOrthogonalToHkl[3] = 0.0;
  m_fromOrthogonalToHkl[4] = 1.0;
  m_fromOrthogonalToHkl[5] = 0.0;
  m_fromOrthogonalToHkl[6] = 0.0;
  m_fromOrthogonalToHkl[7] = 0.0;
  m_fromOrthogonalToHkl[8] = 1.0;
}

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

void NonOrthogonalOverlay::setSkewMatrix() {
  Mantid::Kernel::DblMatrix skewMatrix(3, 3, true);
  API::provideSkewMatrix(skewMatrix, *m_ws);
  API::transformFromDoubleToCoordT(skewMatrix, m_fromOrthogonalToHkl);
  skewMatrix.Invert();
  API::transformFromDoubleToCoordT(skewMatrix, m_fromHklToOrthogonal);

  // set the angles for the two dimensions
  auto angles = MantidQt::API::getGridLineAnglesInRadian(m_fromHklToOrthogonal,
                                                         m_dimX, m_dimY);
  m_angleX = static_cast<double>(angles.first);
  m_angleY = static_cast<double>(angles.second);
}

QPointF NonOrthogonalOverlay::skewMatrixApply(double x, double y) {
  VMD coords = m_slicePoint;
  coords[m_dimX] = static_cast<Mantid::Kernel::VMD_t>(x);
  coords[m_dimY] = static_cast<Mantid::Kernel::VMD_t>(y);
  API::transformLookpointToWorkspaceCoordGeneric(
      coords, m_fromOrthogonalToHkl, m_dimX, m_dimY, m_missingHKLDim);
  auto xNew = coords[m_dimX];
  auto yNew = coords[m_dimY];
  return QPointF(xNew, yNew);
}

void NonOrthogonalOverlay::calculateAxesSkew(Mantid::API::IMDWorkspace_sptr *ws,
                                             size_t dimX, size_t dimY,
                                             Mantid::Kernel::VMD slicePoint) {
  m_ws = ws;
  m_dimX = dimX;
  m_dimY = dimY;
  m_missingHKLDim = API::getMissingHKLDimensionIndex(*m_ws, m_dimX, m_dimY);
  m_slicePoint = slicePoint;

  if (API::isHKLDimensions(*m_ws, m_dimX, m_dimY)) {
    setSkewMatrix();
  }
}

void NonOrthogonalOverlay::updateXGridlines(QwtValueList xAxisTicks,
                                            double xAngle) {
  m_xAxisTicks = xAxisTicks;
  m_xAngle = xAngle;
  auto size = xAxisTicks.size();
  if (m_xAngle != 0 && size >= 1) {
    double firstTick;
    double diff;
    firstTick = xAxisTicks.at(0);
    diff = xAxisTicks.at(1) - firstTick;
    for (auto j = 0; j < size * 2; j++) {
      auto tick = firstTick - diff * j;
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
    double diff;
    firstTick = yAxisTicks.at(0);
    diff = yAxisTicks.at(1) - firstTick;
    for (auto j = 0; j < size * 2; j++) {
      auto tick = firstTick - diff * j;
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

    const int numberOfGridLines = 10;
    drawYLines(painter, gridPen, widthScreen, heightScreen, m_yAxisTicks,
               m_yAngle);
    drawXLines(painter, gridPen, widthScreen, heightScreen, m_xAxisTicks,
               m_xAngle);
  }
}

void NonOrthogonalOverlay::drawYLines(QPainter &painter, QPen &gridPen,
                                      int widthScreen, int heightScreen,
                                      QwtValueList yAxisTicks, double yAngle) {

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
                                      int widthScreen, int heightScreen,
                                      QwtValueList xAxisTicks, double xAngle) {
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

void NonOrthogonalOverlay::setSlicePoint(Mantid::Kernel::VMD slicePoint) {
  m_slicePoint = slicePoint;
}

void NonOrthogonalOverlay::enable() { m_enabled = true; }

void NonOrthogonalOverlay::disable() { m_enabled = false; }

} // namespace Mantid
} // namespace SliceViewer
