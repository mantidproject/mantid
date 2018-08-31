#include "MantidQtWidgets/InstrumentView/PeakMarker2D.h"
#include "MantidQtWidgets/InstrumentView/PeakOverlay.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

/// Default size in screen pixels of the marker's symbol
const int PeakMarker2D::g_defaultMarkerSize = 5;

/**
 * Constructor.
 * @param peakOverlay :: shape for the overlay
 * @param u :: x screen coordinate of peak center
 * @param v :: y screen coordinate of peak center
 * @param style :: marker symbol style
 */
PeakMarker2D::PeakMarker2D(PeakOverlay &peakOverlay, double u, double v,
                           const Style &style)
    : m_peakOverlay(peakOverlay), m_symbol(style.symbol), m_h(-99.0),
      m_k(-99.0), m_l(-99.0), m_detID(-99), m_row(-1) {
  setColor(style.color);
  if (style.size > 0) {
    m_markerSize = style.size;
  } else {
    m_markerSize = g_defaultMarkerSize;
  }
  const QPointF &centre = QPointF(u, v);
  m_boundingRect =
      RectF(centre - QPointF((qreal)m_markerSize / 2, (qreal)m_markerSize / 2),
            QSizeF((qreal)m_markerSize, (qreal)m_markerSize));
  setScalable(false);
}

/**
 * Return the style of the marker.
 */
PeakMarker2D::Style PeakMarker2D::getStyle() const {
  return Style(m_symbol, getColor(), m_markerSize);
}

bool PeakMarker2D::selectAt(const QPointF &p) const { return contains(p); }

void PeakMarker2D::drawShape(QPainter &painter) const {
  // draw the symbol
  switch (m_symbol) {
  case Circle:
    drawCircle(painter);
    break;
  case Diamond:
    drawDiamond(painter);
    break;
  case Square:
    drawSquare(painter);
    break;
  default:
    drawCircle(painter);
  }
  // calculate label's area on the screen
  QFontMetrics fm(painter.font());
  QRect r = fm.boundingRect(m_label);
  m_labelRect = QRectF(r);
  m_labelRect.moveTo(m_boundingRect.x1() + m_markerSize,
                     m_boundingRect.y1() - m_markerSize);
}

void PeakMarker2D::addToPath(QPainterPath &path) const {
  path.addRect(m_boundingRect.toQRectF());
}

/// Set new marker size to s
void PeakMarker2D::setMarkerSize(const int &s) {
  if (s > 0) {
    m_markerSize = s;
  }
}

/// Draw marker as a circle
void PeakMarker2D::drawCircle(QPainter &painter) const {
  QPainterPath path;
  path.addEllipse(m_boundingRect.toQRectF());
  painter.fillPath(path, m_color);
}

/// Draw marker as a diamond
void PeakMarker2D::drawDiamond(QPainter &painter) const {
  QPointF dp = origin();
  QPointF mdp(-dp.x(), -dp.y());
  // draw a diamond as a square rotated by 45 degrees
  painter.save();
  painter.translate(dp);
  painter.rotate(45);
  painter.translate(mdp);
  QPainterPath path;
  path.addRect(m_boundingRect.toQRectF());
  painter.fillPath(path, m_color);
  painter.restore();
}

/// Draw marker as a square
void PeakMarker2D::drawSquare(QPainter &painter) const {
  QPainterPath path;
  path.addRect(m_boundingRect.toQRectF());
  painter.fillPath(path, m_color);
}

/**
 * Save some peak information.
 */
void PeakMarker2D::setPeak(const Mantid::Geometry::IPeak &peak, int row) {
  m_h = peak.getH();
  m_k = peak.getK();
  m_l = peak.getL();
  m_label = QString("%1 %2 %3")
                .arg(QString::number(m_h, 'g', 2), QString::number(m_k, 'g', 2),
                     QString::number(m_l, 'g', 2));
  m_detID = peak.getDetectorID();
  m_row = row;
}

/**
 * Return reference to the peak.
 */
const Mantid::Geometry::IPeak &PeakMarker2D::getPeak() const {
  return m_peakOverlay.getPeak(m_row);
}

} // namespace MantidWidgets
} // namespace MantidQt
