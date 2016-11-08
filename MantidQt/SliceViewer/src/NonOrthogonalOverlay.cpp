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
  m_skewMatrix[0] = 1.0;
  m_skewMatrix[1] = 0.0;
  m_skewMatrix[2] = 0.0;
  m_skewMatrix[3] = 0.0;
  m_skewMatrix[4] = 1.0;
  m_skewMatrix[5] = 0.0;
  m_skewMatrix[6] = 0.0;
  m_skewMatrix[7] = 0.0;
  m_skewMatrix[8] = 1.0;

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
  skewMatrix.Invert();
  // transform from double to coord_t
  std::size_t index = 0;
  for (std::size_t i = 0; i < skewMatrix.numRows(); ++i) {
    for (std::size_t j = 0; j < skewMatrix.numCols(); ++j) {
      m_skewMatrix[index] = static_cast<Mantid::coord_t>(skewMatrix[i][j]);
      ++index;
    }
  }
}

QPointF NonOrthogonalOverlay::skewMatrixApply(double x, double y) {
  // keyed array,
  std::vector<double> dimensions(3, 0);
  dimensions.at(m_dimX) = x;
  dimensions.at(m_dimY) = y;
  const auto angle_H = dimensions[0];
  const auto angle_K = dimensions[1];
  const auto angle_L = dimensions[2];

  auto dimX = angle_H * m_skewMatrix[0 + 3 * m_dimX] +
              angle_K * m_skewMatrix[1 + 3 * m_dimX] +
              angle_L * m_skewMatrix[2 + 3 * m_dimX];
  auto dimY = angle_H * m_skewMatrix[0 + 3 * m_dimY] +
              angle_K * m_skewMatrix[1 + 3 * m_dimY] +
              angle_L * m_skewMatrix[2 + 3 * m_dimY];

  return QPointF(dimX, dimY);
}

void NonOrthogonalOverlay::zoomChanged(QwtDoubleInterval xint,
                                       QwtDoubleInterval yint) {
  m_xMinVis = xint.minValue();
  m_xMaxVis = xint.maxValue();
  m_yMinVis = yint.minValue();
  m_yMaxVis = yint.maxValue();

  double xBuffer = (m_xMaxVis - m_xMinVis);
  double yBuffer = (m_yMaxVis - m_yMinVis);
  m_xMaxVisBuffered = m_xMaxVis + xBuffer;
  m_xMinVisBuffered = m_xMinVis - xBuffer;
  m_yMaxVisBuffered = m_yMaxVis + yBuffer;
  m_yMinVisBuffered = m_yMinVis - yBuffer;

  calculateTickMarks();
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

void NonOrthogonalOverlay::clearAllAxisPointVectors() {
  m_axisXPointVec.clear();
  m_axisYPointVec.clear();
  m_xAxisTickStartVec.clear();
  m_yAxisTickStartVec.clear();
  m_xAxisTickEndVec.clear();
  m_yAxisTickEndVec.clear();
  m_xNumbers.clear();
  m_yNumbers.clear();
}

void NonOrthogonalOverlay::calculateTickMarks() { // assumes X axis
  clearAllAxisPointVectors();

  auto percentageOfLineX =
      (((m_xMaxVisBuffered) - (m_xMinVisBuffered)) / m_tickNumber);
  auto percentageOfLineY =
      (((m_yMaxVisBuffered) - (m_yMinVisBuffered)) / m_tickNumber);
  for (size_t i = 0; i <= static_cast<size_t>(m_tickNumber); i++) {
    double axisPointX = (percentageOfLineX * i) + m_xMinVisBuffered;
    double axisPointY = (percentageOfLineY * i) + m_yMinVisBuffered;
    m_axisXPointVec.push_back(axisPointX);
    m_xNumbers.push_back(skewMatrixApply(axisPointX, m_yMinVis));
    m_xNumbers[i].setY(m_yMinVis);
    m_yNumbers.push_back(skewMatrixApply(m_xMinVis, axisPointY));
    m_yNumbers[i].setX(m_xMinVis);
    m_xAxisTickStartVec.push_back(
        skewMatrixApply(axisPointX, (m_yMinVisBuffered)));
    m_xAxisTickEndVec.push_back(
        skewMatrixApply(axisPointX, (m_yMaxVisBuffered)));
    m_axisYPointVec.push_back(axisPointY);
    m_yAxisTickStartVec.push_back(
        skewMatrixApply((m_xMinVisBuffered), axisPointY));
    m_yAxisTickEndVec.push_back(
        skewMatrixApply((m_xMaxVisBuffered), axisPointY));
  }

  update();
}

//----------------------------------------------------------------------------------------------
/// Paint the overlay
void NonOrthogonalOverlay::paintEvent(QPaintEvent * /*event*/) {

  QPainter painter(this);

  QPen centerPen(QColor(0, 0, 0, 200));     // black
  QPen gridPen(QColor(160, 160, 160, 100)); // grey
  QPen numberPen(QColor(160, 160, 160, 255));
  // --- Draw the central line ---
  if (m_showLine) {
    centerPen.setWidth(2);
    centerPen.setCapStyle(Qt::SquareCap);
    painter.setPen(centerPen);
    painter.drawLine(transform(m_pointA), transform(m_pointB));
    painter.drawLine(transform(m_pointA), transform(m_pointC));
    gridPen.setWidth(1);
    gridPen.setCapStyle(Qt::FlatCap);
    gridPen.setStyle(Qt::DashLine);

    for (size_t i = 0; i < m_axisYPointVec.size(); i++) {
      painter.setPen(gridPen);
      painter.drawLine(transform(m_xAxisTickStartVec[i]),
                       transform(m_xAxisTickEndVec[i]));
      painter.drawLine(transform(m_yAxisTickStartVec[i]),
                       transform(m_yAxisTickEndVec[i]));
      painter.setPen(numberPen);
      if ((m_xNumbers[i].x() > m_xMinVis) &&
          (m_xNumbers[i].x() < (m_xMaxVis * m_numberAxisEdge))) {
        painter.drawText(transform(m_xNumbers[i]),
                         QString::number(m_axisXPointVec[i], 'g', 3));
      }
      if ((m_yNumbers[i].y() > m_yMinVis) &&
          (m_yNumbers[i].y() < (m_yMaxVis * m_numberAxisEdge))) {
        painter.drawText(transform(m_yNumbers[i]),
                         QString::number(m_axisYPointVec[i], 'g', 3));
      }
    }
  }
}

} // namespace Mantid
} // namespace SliceViewer
