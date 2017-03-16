#include "MantidQtSliceViewer/QwtScaleDrawNonOrthogonal.h"

#include "MantidQtAPI/NonOrthogonal.h"
#include <QPainter>
#include <QPalette>
#include "qwt_layout_metrics.h"
#include "qwt_painter.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_engine.h"
#include "qwt_plot_canvas.h"

#include <qmatrix.h>
#define QwtMatrix QMatrix

QwtScaleDrawNonOrthogonal::QwtScaleDrawNonOrthogonal(
    QwtPlot *plot, ScreenDimension screenDimension,
    Mantid::API::IMDWorkspace_sptr workspace, size_t dimX, size_t dimY,
    Mantid::Kernel::VMD slicePoint,
    MantidQt::SliceViewer::NonOrthogonalOverlay *gridPlot)
    : m_plot(plot), m_screenDimension(screenDimension), m_dimX(dimX),
      m_dimY(dimY), m_slicePoint(slicePoint), m_gridPlot(gridPlot) {
  m_fromHklToXyz[0] = 1.0;
  m_fromHklToXyz[1] = 0.0;
  m_fromHklToXyz[2] = 0.0;
  m_fromHklToXyz[3] = 0.0;
  m_fromHklToXyz[4] = 1.0;
  m_fromHklToXyz[5] = 0.0;
  m_fromHklToXyz[6] = 0.0;
  m_fromHklToXyz[7] = 0.0;
  m_fromHklToXyz[8] = 1.0;

  m_fromXyzToHkl[0] = 1.0;
  m_fromXyzToHkl[1] = 0.0;
  m_fromXyzToHkl[2] = 0.0;
  m_fromXyzToHkl[3] = 0.0;
  m_fromXyzToHkl[4] = 1.0;
  m_fromXyzToHkl[5] = 0.0;
  m_fromXyzToHkl[6] = 0.0;
  m_fromXyzToHkl[7] = 0.0;
  m_fromXyzToHkl[8] = 1.0;

  // Set up the transformation matrix
  setTransformationMatrices(workspace);

  // Set up the angles
  // set the angles for the two dimensions
  auto angles =
      MantidQt::API::getGridLineAnglesInRadian(m_fromHklToXyz, m_dimX, m_dimY);
  m_angleX = static_cast<double>(angles.first);
  m_angleY = static_cast<double>(angles.second);
}

void QwtScaleDrawNonOrthogonal::draw(QPainter *painter,
                                     const QPalette &palette) const {
  // Get the ScaleDiv element information, such as min_xyz and max_xyz
  const auto &scaleDivEntries = scaleDiv();
  const auto minXyz = scaleDivEntries.lowerBound();
  const auto maxXyz = scaleDivEntries.upperBound();

  // Get the bottom and left side of the screen in Xyz
  const auto bottomInXyz = getScreenBottomInXyz();
  const auto leftInXyz = getScreenLeftInXyz();

  // Calculate the min_hkl and max_hkl
  double minHkl = 0;
  double maxHkl = 0;
  if (m_screenDimension == ScreenDimension::X) {
    minHkl = fromMixedCoordinatesToHkl(minXyz, bottomInXyz).x();
    maxHkl = fromMixedCoordinatesToHkl(maxXyz, bottomInXyz).x();
  } else if (m_screenDimension == ScreenDimension::Y) {
    minHkl = fromMixedCoordinatesToHkl(leftInXyz, minXyz).y();
    maxHkl = fromMixedCoordinatesToHkl(leftInXyz, maxXyz).y();
  } else {
    throw std::runtime_error(
        "QwtScaleDrawNonOrthogonal: The provided screen "
        "dimension is not valid. It has to be either X or Y.");
  }

  // Calculate appropriate tick mark locations and values in hkl coordinates.
  const auto selectedAxis = m_screenDimension == ScreenDimension::X
                                ? QwtPlot::xBottom
                                : QwtPlot::yLeft;
  const auto maxMajorSteps = m_plot->axisMaxMajor(selectedAxis);
  const auto maxMinorSteps = m_plot->axisMaxMinor(selectedAxis);
  const auto stepSize = m_plot->axisStepSize(selectedAxis);
  const auto *axisScaleEngine = m_plot->axisScaleEngine(selectedAxis);
  const auto scaleDivHkl = axisScaleEngine->divideScale(
      minHkl, maxHkl, maxMajorSteps, maxMinorSteps, stepSize);

  // Transform the scale for tick marks back to xyz. We need to pass the point
  // where to draw in xyz.
  const auto &majorTicksHkl = scaleDivHkl.ticks(QwtScaleDiv::MajorTick);
  const auto &minorTicksHkl = scaleDivHkl.ticks(QwtScaleDiv::MinorTick);
  QwtValueList majorTicksXyz;
  QwtValueList minorTicksXyz;

  if (m_screenDimension == ScreenDimension::X) {
    for (auto &tick : majorTicksHkl) {
      double majorTickXyz = static_cast<double>(fromXtickInHklToXyz(tick));
      majorTicksXyz.push_back(majorTickXyz);
    }
    for (auto &tick : minorTicksHkl) {
      double minorTickXyz = static_cast<double>(fromXtickInHklToXyz(tick));
      minorTicksXyz.push_back(minorTickXyz);
    }
  } else {
    for (auto &tick : majorTicksHkl) {
      double majorTickXyz = static_cast<double>(fromYtickInHklToXyz(tick));
      majorTicksXyz.push_back(majorTickXyz);
    }
    for (auto &tick : minorTicksHkl) {
      double minorTickXyz = static_cast<double>(fromYtickInHklToXyz(tick));
      minorTicksXyz.push_back(minorTickXyz);
    }
  }

  // ***********
  // Draw labels
  // ***********

  if (hasComponent(QwtAbstractScaleDraw::Labels)) {
    painter->save();
    painter->setPen(palette.color(QPalette::Text)); // ignore pen style
    for (int i = 0; i < (int)majorTicksHkl.count(); ++i) {
      drawLabelNonOrthogonal(painter, majorTicksHkl[i], majorTicksXyz[i]);
    }

    painter->restore();
  }

  // **************
  // Draw tickmarks
  // **************
  if (hasComponent(QwtAbstractScaleDraw::Ticks)) {
    painter->save();

    QPen pen = painter->pen();
    pen.setColor(palette.color(QPalette::Foreground));
    painter->setPen(pen);

    // Draw major ticks
    for (int i = 0; i < (int)majorTicksXyz.count(); i++) {
      drawTick(painter, majorTicksXyz[i], tickLength(QwtScaleDiv::MajorTick));
    }

    // Draw minor ticks
    for (int i = 0; i < (int)minorTicksXyz.count(); i++) {
      drawTick(painter, minorTicksXyz[i], tickLength(QwtScaleDiv::MinorTick));
    }

    painter->restore();
  }

  // **************
  // Draw backbone
  // **************
  if (hasComponent(QwtAbstractScaleDraw::Backbone)) {
    painter->save();
    QPen pen = painter->pen();
    pen.setColor(palette.color(QPalette::Foreground));
    painter->setPen(pen);
    drawBackbone(painter);
    painter->restore();
  }

  // ****************
  // Apply grid lines
  // ****************
  if (m_screenDimension == ScreenDimension::X) {
    applyGridLinesX(majorTicksXyz);
  } else {
    applyGridLinesY(majorTicksXyz);
  };
}

void QwtScaleDrawNonOrthogonal::drawLabelNonOrthogonal(QPainter *painter,
                                                       double labelValue,
                                                       double labelPos) const {
  QwtText lbl = tickLabel(painter->font(), labelValue);
  if (lbl.isEmpty())
    return;

  QPoint pos = labelPosition(labelPos);

  QSize labelSize = lbl.textSize(painter->font());
  if (labelSize.height() % 2)
    labelSize.setHeight(labelSize.height() + 1);

  const QwtMetricsMap metricsMap = QwtPainter::metricsMap();
  QwtPainter::resetMetricsMap();

  labelSize = metricsMap.layoutToDevice(labelSize);
  pos = metricsMap.layoutToDevice(pos);

  const QwtMatrix m = labelMatrix(pos, labelSize);

  painter->save();
  painter->setMatrix(m, true);
  lbl.draw(painter, QRect(QPoint(0, 0), labelSize));
  QwtPainter::setMetricsMap(metricsMap); // restore metrics map
  painter->restore();
}

void QwtScaleDrawNonOrthogonal::applyGridLinesX(
    const QwtValueList &majorTicksXyz) const {
  auto angle = m_angleY;
  m_gridPlot->updateXGridlines(majorTicksXyz, angle);
  // We need the -1 since we the angle is defined in the mathematical positive
  // sense but we are taking the angle against the y axis in the mathematical
  // negative sense.
}

void QwtScaleDrawNonOrthogonal::applyGridLinesY(
    const QwtValueList &majorTicksXyz) const {
  auto angle = m_angleX;
  m_gridPlot->updateYGridlines(majorTicksXyz, angle);
}

/** Tranform from plot coordinates to pixel coordinates
 * @param xyz :: coordinate point in plot coordinates
 * @return pixel coordinates
*/
QPoint QwtScaleDrawNonOrthogonal::fromXyzToScreen(QPointF xyz) const {
  auto xScreen = m_plot->transform(QwtPlot::xBottom, xyz.x());
  auto yScreen = m_plot->transform(QwtPlot::yLeft, xyz.y());
  return QPoint(xScreen, yScreen);
}

/** Inverse transform: from pixels to plot coords
 * @param screen :: location in pixels
 * @return plot coordinates (float)
*/
QPointF QwtScaleDrawNonOrthogonal::fromScreenToXyz(QPoint screen) const {
  auto x = m_plot->invTransform(QwtPlot::xBottom, screen.x());
  auto y = m_plot->invTransform(QwtPlot::yLeft, screen.y());
  return QPointF(x, y);
}

QPointF QwtScaleDrawNonOrthogonal::fromMixedCoordinatesToHkl(double x,
                                                             double y) const {
  Mantid::Kernel::VMD coords = m_slicePoint;
  coords[m_dimX] = static_cast<Mantid::Kernel::VMD_t>(x);
  coords[m_dimY] = static_cast<Mantid::Kernel::VMD_t>(y);
  MantidQt::API::transformLookpointToWorkspaceCoordGeneric(
      coords, m_fromXyzToHkl, m_dimX, m_dimY, m_missingDimension);
  auto coord1 = coords[m_dimX];
  auto coord2 = coords[m_dimY];
  return QPointF(coord1, coord2);
}

double QwtScaleDrawNonOrthogonal::fromXtickInHklToXyz(double tick) const {
  // First we convert everything to hkl. Here the m_dimY coordiante
  // is in xyz, so we need to bring it to hkl
  auto tickPointHkl = m_slicePoint;
  tickPointHkl[m_dimX] = static_cast<float>(tick);
  auto heightScreenInXyz =
      m_plot->invTransform(QwtPlot::yLeft, m_plot->canvas()->height());
  tickPointHkl[m_dimY] = static_cast<float>(heightScreenInXyz);

  tickPointHkl[m_dimY] =
      (tickPointHkl[m_dimY] -
       m_fromHklToXyz[3 * m_dimY + m_dimX] * tickPointHkl[m_dimX] -
       m_fromHklToXyz[3 * m_dimY + m_missingDimension] *
           tickPointHkl[m_missingDimension]) /
      m_fromHklToXyz[3 * m_dimY + m_dimY];

  // convert from hkl to xyz
  auto tickPointXyz = fromHklToXyz(tickPointHkl);
  return tickPointXyz[m_dimX];
}

double QwtScaleDrawNonOrthogonal::fromYtickInHklToXyz(double tick) const {
  // First we convert everything to hkl. Here the m_dimX coordiante
  // is in xyz, so we need to bring it to hkl
  auto tickPointHkl = m_slicePoint;
  tickPointHkl[m_dimY] = static_cast<float>(tick);

  auto widthScreenInXyz = m_plot->invTransform(QwtPlot::xBottom, 0);
  tickPointHkl[m_dimX] = static_cast<float>(widthScreenInXyz);

  tickPointHkl[m_dimX] =
      (tickPointHkl[m_dimX] -
       m_fromHklToXyz[3 * m_dimX + m_dimY] * tickPointHkl[m_dimY] -
       m_fromHklToXyz[3 * m_dimX + m_missingDimension] *
           tickPointHkl[m_missingDimension]) /
      m_fromHklToXyz[3 * m_dimX + m_dimX];

  // convert from hkl to xyz
  auto tickPointXyz = fromHklToXyz(tickPointHkl);
  return tickPointXyz[m_dimY];
}

Mantid::Kernel::VMD
QwtScaleDrawNonOrthogonal::fromHklToXyz(const Mantid::Kernel::VMD &hkl) const {
  Mantid::Kernel::VMD xyz(hkl);
  for (int i = 0; i < 3; ++i) {
    xyz[i] = 0;
    for (int j = 0; j < 3; ++j) {
      xyz[i] += m_fromHklToXyz[j + i * 3] * hkl[j];
    }
  }
  return xyz;
}

void QwtScaleDrawNonOrthogonal::setTransformationMatrices(
    Mantid::API::IMDWorkspace_sptr workspace) {
  m_missingDimension =
      MantidQt::API::getMissingHKLDimensionIndex(workspace, m_dimX, m_dimY);

  if (MantidQt::API::isHKLDimensions(workspace, m_dimX, m_dimY)) {
    Mantid::Kernel::DblMatrix skewMatrix(3, 3, true);
    MantidQt::API::provideSkewMatrix(skewMatrix, workspace);
    MantidQt::API::transformFromDoubleToCoordT(skewMatrix, m_fromXyzToHkl);
    skewMatrix.Invert();
    MantidQt::API::transformFromDoubleToCoordT(skewMatrix, m_fromHklToXyz);
  }
}

qreal QwtScaleDrawNonOrthogonal::getScreenBottomInXyz() const {
  auto screenBottomY = m_plot->canvas()->height();
  QPoint screenBottom(0, screenBottomY);
  return fromScreenToXyz(screenBottom).y();
}

qreal QwtScaleDrawNonOrthogonal::getScreenLeftInXyz() const {
  QPoint screenLeft(0, 0);
  return fromScreenToXyz(screenLeft).x();
}

void QwtScaleDrawNonOrthogonal::updateSlicePoint(
    Mantid::Kernel::VMD newSlicepoint) {
  m_slicePoint = newSlicepoint;
}
