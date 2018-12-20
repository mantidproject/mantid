#include "MantidQtWidgets/InstrumentView/RectF.h"

namespace MantidQt {
namespace MantidWidgets {
void RectF::moveCenter(const QPointF &p) {
  double xShift = xSpan() / 2;
  double yShift = ySpan() / 2;
  m_x0 = p.x() - xShift;
  m_x1 = p.x() + xShift;
  m_y0 = p.y() - yShift;
  m_y1 = p.y() + yShift;
}

QPointF RectF::vertex(size_t i) const {
  switch (i) {
  case 0:
    return QPointF(m_x0, m_y0);
  case 1:
    return QPointF(m_x0, m_y1);
  case 2:
    return QPointF(m_x1, m_y1);
  case 3:
    return QPointF(m_x1, m_y0);
  }
  throw std::range_error("Rectangle vertex index is out of range");
}

void RectF::setVertex(size_t i, const QPointF &p) {
  switch (i) {
  case 0:
    m_x0 = p.x();
    m_y0 = p.y();
    return;
  case 1:
    m_x0 = p.x();
    m_y1 = p.y();
    return;
  case 2:
    m_x1 = p.x();
    m_y1 = p.y();
    return;
  case 3:
    m_x1 = p.x();
    m_y0 = p.y();
    return;
  }
  throw std::range_error("Rectangle vertex index is out of range");
}

bool RectF::contains(double x, double y) const {
  double dx = m_x0 < m_x1 ? x - m_x0 : x - m_x1;
  if (dx < 0 || dx > width())
    return false;
  double dy = m_y0 < m_y1 ? y - m_y0 : y - m_y1;
  return !(dy < 0 || dy > height());
}

bool RectF::contains(const RectF &rect) {
  return contains(rect.p0()) && contains(rect.p1());
}

/**
 * Create a transformation from this rectangle to a QRectF.
 * @param trans :: The result transform.
 * @param rect  :: The transform's destination QRectF.
 */
void RectF::findTransform(QTransform &trans, const QRectF &rect) const {
  double m11 = rect.width() / xSpan();
  double m22 = -rect.height() / ySpan();
  trans.reset();
  trans.translate(rect.left() - m11 * x0(), rect.bottom() - m22 * y0());
  trans.scale(m11, m22);
}

void RectF::include(const QPointF &p) {
  if ((p.x() - m_x0) / xSpan() < 0) {
    m_x0 = p.x();
  } else if ((p.x() - m_x1) / xSpan() > 0) {
    m_x1 = p.x();
  }
  if ((p.y() - m_y0) / ySpan() < 0) {
    m_y0 = p.y();
  } else if ((p.y() - m_y1) / ySpan() > 0) {
    m_y1 = p.y();
  }
}

void RectF::unite(const RectF &rect) {
  include(rect.p0());
  include(rect.p1());
}

} // namespace MantidWidgets
} // namespace MantidQt
