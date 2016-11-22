#include "MantidQtSliceViewer/NonOrthogonalOverlay.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qpainter.h>
#include <QRect>
#include <QShowEvent>
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
    : QWidget(parent), m_plot(plot), m_tickNumber(20), m_numberAxisEdge(0.95) {
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

  m_pointA = QPointF(0, 0);
  m_pointB = QPointF(0, 0);
  m_pointC = QPointF(0, 0);
  m_dim0Max = 0;
  m_showLine = false;
  m_width = 0.1;
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
NonOrthogonalOverlay::~NonOrthogonalOverlay() {}

/// Return the recommended size of the widget
QSize NonOrthogonalOverlay::sizeHint() const {
  // TODO: Is there a smarter way to find the right size?
  return QSize(20000, 20000);
  // Always as big as the canvas
  // return m_plot->canvas()->size();
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
  auto angles =
      MantidQt::API::getAnglesInRadian(m_fromHklToOrthogonal, m_dimX, m_dimY);
  m_angleX = static_cast<double>(angles.first);
  m_angleY = static_cast<double>(angles.second);
}

QPointF NonOrthogonalOverlay::skewMatrixApply(double x, double y) {
  VMD coords(m_ws->get()->getNumDims());
  coords[m_dimX] = static_cast<float>(x);
  coords[m_dimY] = static_cast<float>(y);
  API::transformLookpointToWorkspaceCoordGeneric(coords, m_fromOrthogonalToHkl,
                                                 m_dimX, m_dimY);
  auto xNew = coords[m_dimX];
  auto yNew = coords[m_dimY];

  return QPointF(xNew, yNew);
}


void NonOrthogonalOverlay::setAxesPoints() {
  auto ws = m_ws->get();
  m_dim0Max = ws->getDimension(0)->getMaximum();
  m_dim0Max = m_dim0Max * 1.1; // to set axis slightly back from slice
  m_originPoint = -(m_dim0Max);
  m_endPoint = m_dim0Max; // works for both max Y and X
  m_pointA = skewMatrixApply(m_originPoint, m_originPoint);
  m_pointB = skewMatrixApply(m_endPoint, m_originPoint);
  m_pointC = skewMatrixApply(m_originPoint, m_endPoint);
}

void NonOrthogonalOverlay::calculateAxesSkew(Mantid::API::IMDWorkspace_sptr *ws,
                                             size_t dimX, size_t dimY) {
  m_ws = ws;
  m_dimX = dimX;
  m_dimY = dimY;

  if (API::isHKLDimensions(*m_ws, m_dimX, m_dimY)) {
    setSkewMatrix();
    setAxesPoints();
  }
}


//----------------------------------------------------------------------------------------------
/// Paint the overlay
void NonOrthogonalOverlay::paintEvent(QPaintEvent * /*event*/) {

  QPainter painter(this);

  QPen gridPen(QColor(160, 160, 160, 100)); // grey
  QPen numberPen(QColor(160, 160, 160, 255));
  // --- Draw the central line ---
  if (m_showLine) {

    gridPen.setWidth(1);
    gridPen.setCapStyle(Qt::FlatCap);
    gridPen.setStyle(Qt::DashLine);
    const auto widthScreen = width();
    const auto heightScreen = height();
    painter.setPen(gridPen);

    const int numberOfGridLines = 10;
    drawYLines(painter, numberPen, gridPen, widthScreen, heightScreen,
               numberOfGridLines, m_angleY);
    drawXLines(painter, numberPen, gridPen, widthScreen, heightScreen,
               numberOfGridLines, m_angleX);
  }
}

void NonOrthogonalOverlay::drawYLines(QPainter &painter, QPen &numberPen,
                                      QPen &gridPen, int widthScreen,
                                      int heightScreen, int numberOfGridLines,
                                      double angle) {

  // Draw Y grid lines - in a orthogonal world these lines will be parallel to
  // the Y axes.
  const auto increment = widthScreen / numberOfGridLines;

  // We need the -1 since we the angle is defined in the mathematical positive
  // sense but we are taking the angle against the y axis in the mathematical
  // negative sense.
  auto xOffsetForYLine = angle == 0. ? 0. : heightScreen * std::tan(-1 * angle);

  // We need to make sure that the we don't have a blank area. This "blankness"
  // is determined by the angle. The extra lines which need to be drawn are
  // given by
  // lineSpacing/(x offset of line on the top), ie. increment/xOffsetForYLine
  const int additionalLinesToDraw =
      static_cast<int>(std::ceil(xOffsetForYLine / increment));
  int index = angle < 0 ? 0 - additionalLinesToDraw : additionalLinesToDraw;

  QString label;
  for (; index < numberOfGridLines; ++index) {
    const auto xValue = increment * index;
    auto start = QPointF(xValue, heightScreen);
    auto end = QPointF(xValue + xOffsetForYLine, 0);
    painter.setPen(gridPen);
    painter.drawLine(start, end);

    // Set the label on the x axis
    auto pointInOrthogonalCoordinates = invTransform(start.toPoint());
    auto pointInNonOrthogonalCoordinates = skewMatrixApply(
        pointInOrthogonalCoordinates.x(), pointInOrthogonalCoordinates.y());
    label = QString::number(pointInNonOrthogonalCoordinates.x(), 'e', 2);
    painter.setPen(numberPen);
    painter.drawText(start, label);
  }
}

void NonOrthogonalOverlay::drawXLines(QPainter &painter, QPen &numberPen,
                                      QPen &gridPen, int widthScreen,
                                      int heightScreen, int numberOfGridLines,
                                      double angle) {

  // Draw X grid lines - in a orthogonal world these lines will be parallel to
  // the X axes.
  const auto increment = heightScreen / numberOfGridLines;

  auto yOffsetForXLine = angle == 0. ? 0. : widthScreen * std::tan(angle);

  const int additionalLinesToDraw =
      static_cast<int>(std::ceil(yOffsetForXLine / increment));
  int index = angle > 0 ? additionalLinesToDraw : 0 - additionalLinesToDraw;
  QString label;
  for (; index < numberOfGridLines; ++index) {
    const auto yValue = increment * index;
    auto start = QPointF(0, yValue);
    auto end = QPointF(widthScreen, yValue - yOffsetForXLine);
    painter.setPen(gridPen);
    painter.drawLine(start, end);

    // Set the label on the y axis
    auto pointInOrthogonalCoordinates = invTransform(start.toPoint());
    auto pointInNonOrthogonalCoordinates = skewMatrixApply(
        pointInOrthogonalCoordinates.x(), pointInOrthogonalCoordinates.y());
    label = QString::number(pointInNonOrthogonalCoordinates.y(), 'e', 2);
    painter.setPen(numberPen);
    painter.drawText(QPointF(label.size(), yValue), label);
  }
}

} // namespace Mantid
} // namespace SliceViewer
