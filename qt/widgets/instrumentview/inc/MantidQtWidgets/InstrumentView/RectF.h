// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef RECTF_H
#define RECTF_H

#include <QPoint>
#include <QRectF>
#include <QSize>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <ostream>
#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

/**

A class for a axis aligned rectangle defined by its two opposite points.
If the rectangle is used to define a coord system the first of the two points
is its origin.

width() and height() always return a non-negative number.

*/
class RectF {
public:
  /// The default constructor creates an empty rectangle at (0,0)
  RectF() : m_x0(), m_y0(), m_x1(), m_y1() {}
  /// Set the first point at origin and the second is shifted in positive
  /// directions of the x and y axes by vector size.
  RectF(const QPointF &origin, const QSizeF &size);
  /// Explicitly set the two points
  RectF(const QPointF &point0, const QPointF &point1);
  explicit RectF(const QRectF &rect);
  /// Copy constructor
  RectF(const RectF &rect) { *this = rect; }
  /// Copy assignment operator
  RectF &operator=(const RectF &rect);

  bool isEmpty() const;
  QPointF center() const;
  void moveCenter(const QPointF &p);

  inline double x0() const { return m_x0; }
  inline double y0() const { return m_y0; }
  inline double x1() const { return m_x1; }
  inline double y1() const { return m_y1; }

  QPointF p0() const { return QPointF(m_x0, m_y0); }
  QPointF p1() const { return QPointF(m_x1, m_y1); }

  QPointF vertex(size_t i) const;
  void setVertex(size_t i, const QPointF &p);

  void translate(double dx, double dy);
  void translate(const QPointF &p);

  RectF translated(double dx, double dy) const;
  RectF translated(const QPointF &p) const;

  /**
   * Adjust the rect by moving the defining points.
   * @param dp0 :: Vector to be added to the first point.
   * @param dp1 :: Vector to be added to the second point.
   */
  void adjust(const QPointF &dp0, const QPointF &dp1);

  /// Expand the rectangle if needed to include a point.
  void include(const QPointF &p);
  void unite(const RectF &rect);

  inline void xFlip();
  inline void yFlip();

  QSizeF size() const;
  double width() const;
  double height() const;

  /// x1 - x0
  double xSpan() const;
  /// y1 - y0
  double ySpan() const;

  bool contains(const QPointF &p) const { return contains(p.x(), p.y()); }
  bool contains(double x, double y) const;
  bool contains(const RectF &rect);

  void findTransform(QTransform &trans, const QRectF &rect) const;

  QRectF toQRectF() const;

private:
  double m_x0;
  double m_y0;
  double m_x1;
  double m_y1;
};

/*****************************************************************************
RectF inline member functions
*****************************************************************************/

inline RectF::RectF(const QPointF &origin, const QSizeF &size) {
  m_x0 = origin.x();
  m_y0 = origin.y();
  m_x1 = m_x0 + size.width();
  m_y1 = m_y0 + size.height();
}

inline RectF::RectF(const QPointF &point0, const QPointF &point1) {
  m_x0 = point0.x();
  m_y0 = point0.y();
  m_x1 = point1.x();
  m_y1 = point1.y();
}

inline RectF::RectF(const QRectF &rect) {
  m_x0 = rect.left();
  m_y0 = rect.top();
  m_x1 = rect.right();
  m_y1 = rect.bottom();
}

inline RectF &RectF::operator=(const RectF &rect) {
  m_x0 = rect.m_x0;
  m_y0 = rect.m_y0;
  m_x1 = rect.m_x1;
  m_y1 = rect.m_y1;
  return *this;
}

inline bool RectF::isEmpty() const { return m_x0 == m_x1 || m_y0 == m_y1; }

inline QPointF RectF::center() const {
  return QPointF((m_x0 + m_x1) / 2, (m_y0 + m_y1) / 2);
}

inline double RectF::width() const { return fabs(m_x1 - m_x0); }

inline double RectF::height() const { return fabs(m_y1 - m_y0); }

inline double RectF::xSpan() const { return m_x1 - m_x0; }

inline double RectF::ySpan() const { return m_y1 - m_y0; }

inline QSizeF RectF::size() const { return QSizeF(width(), height()); }

inline void RectF::translate(double dx, double dy) {
  m_x0 += dx;
  m_y0 += dy;
  m_x1 += dx;
  m_y1 += dy;
}

inline void RectF::translate(const QPointF &p) {
  m_x0 += p.x();
  m_y0 += p.y();
  m_x1 += p.x();
  m_y1 += p.y();
}

inline RectF RectF::translated(double dx, double dy) const {
  return RectF(QPointF(m_x0 + dx, m_y0 + dy), QPointF(m_x1 + dx, m_y1 + dy));
}

inline RectF RectF::translated(const QPointF &p) const {
  return RectF(QPointF(m_x0 + p.x(), m_y0 + p.y()),
               QPointF(m_x1 + p.x(), m_y1 + p.y()));
}

inline void RectF::adjust(const QPointF &dp0, const QPointF &dp1) {
  m_x0 += dp0.x();
  m_y0 += dp0.y();
  m_x1 += dp1.x();
  m_y1 += dp1.y();
}

inline void RectF::xFlip() { std::swap(m_x0, m_x1); }

inline void RectF::yFlip() { std::swap(m_y0, m_y1); }

inline QRectF RectF::toQRectF() const {
  return QRectF(m_x0, m_y0, xSpan(), ySpan());
}

inline std::ostream &operator<<(std::ostream &ostr, const RectF &rect) {
  ostr << '[' << rect.x0() << ',' << rect.x1() << ';' << rect.y0() << ','
       << rect.y1() << ']';
  return ostr;
}

inline std::ostream &operator<<(std::ostream &ostr, const QRectF &rect) {
  ostr << '[' << rect.left() << ',' << rect.right() << ';' << rect.top() << ','
       << rect.bottom() << ']';
  return ostr;
}

inline std::ostream &operator<<(std::ostream &ostr, const QPointF &p) {
  ostr << '(' << p.x() << ',' << p.y() << ')';
  return ostr;
}
} // namespace MantidWidgets
} // namespace MantidQt

#endif // RECTF_H
