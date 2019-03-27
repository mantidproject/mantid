// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/Shape2D.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <QLine>
#include <QMap>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

// number of control points common for all shapes
const size_t Shape2D::NCommonCP = 4;
// size (== width/2 == height/2) of each control point
const double Shape2D::sizeCP = 3;

/**
 * Set default border color to red and fill color to default Qt color
 * (==QColor()).
 */
Shape2D::Shape2D()
    : m_color(Qt::red), m_fill_color(QColor()), m_scalable(true),
      m_editing(false), m_selected(false), m_visible(true) {}

/**
 * Calls virtual drawShape() method to draw the actial shape.
 * Draws bounding rect and control points if the shape is selected.
 *
 * @param painter :: QPainter used for drawing.
 */
void Shape2D::draw(QPainter &painter) const {
  if (!m_visible)
    return;
  painter.setPen(QPen(m_color, 0));
  this->drawShape(painter);
  if (m_editing || m_selected) {
    painter.setPen(QPen(QColor(255, 255, 255, 100), 0));
    painter.drawRect(m_boundingRect.toQRectF());
    size_t np = NCommonCP;
    double rsize = 2;
    int alpha = 100;
    if (m_editing) {
      // if editing show all CP, make them bigger and opaque
      np = getNControlPoints();
      rsize = sizeCP;
      alpha = 255;
    }
    for (size_t i = 0; i < np; ++i) {
      QPointF p = painter.transform().map(getControlPoint(i));
      QRectF r(p - QPointF(rsize, rsize), p + QPointF(rsize, rsize));
      painter.save();
      painter.resetTransform();
      painter.fillRect(r, QColor(255, 255, 255, alpha));
      r.adjust(-1, -1, 0, 0);
      painter.setPen(QPen(QColor(0, 0, 0, alpha), 0));
      painter.drawRect(r);
      painter.restore();
    }
  }
}

/**
 * Return total number of control points for this shape.
 */
size_t Shape2D::getNControlPoints() const {
  return NCommonCP + this->getShapeNControlPoints();
}

/**
 * Return coordinates of i-th control point.
 *
 * @param i :: Index of a control point. 0 <= i < getNControlPoints().
 */
QPointF Shape2D::getControlPoint(size_t i) const {
  if (i >= getNControlPoints()) {
    throw std::range_error("Control point index is out of range");
  }

  if (i < 4)
    return m_boundingRect.vertex(i);

  return getShapeControlPoint(i - NCommonCP);
}

void Shape2D::setControlPoint(size_t i, const QPointF &pos) {
  if (i >= getNControlPoints()) {
    throw std::range_error("Control point index is out of range");
  }

  if (i < 4) {
    m_boundingRect.setVertex(i, pos);
    refit();
  }
  // else ?
  else
    setShapeControlPoint(i - NCommonCP, pos);
  resetBoundingRect();
}

/**
 * Move the shape.
 *
 * @param dp :: The shift vector.
 */
void Shape2D::moveBy(const QPointF &dp) {
  m_boundingRect.translate(dp);
  refit();
}

/**
 * Adjust the bound of the bounding rect. Calls virtual method refit()
 * to resize the shape in order to fit into the new bounds.
 */
void Shape2D::adjustBoundingRect(double dx1, double dy1, double dx2,
                                 double dy2) {
  double dwidth = dx2 - dx1;
  if (dwidth <= -m_boundingRect.xSpan()) {
    double mu = m_boundingRect.xSpan() / fabs(dwidth);
    dx1 *= mu;
    dx2 *= mu;
  }
  double dheight = dy2 - dy1;
  if (dheight <= -m_boundingRect.ySpan()) {
    double mu = m_boundingRect.ySpan() / fabs(dheight);
    dy1 *= mu;
    dy2 *= mu;
  }
  m_boundingRect.adjust(QPointF(dx1, dy1), QPointF(dx2, dy2));
  refit();
}

/**
 * Assign new bounding rect. Calls virtual method refit()
 * to resize the shape in order to fit into the new bounds.
 */
void Shape2D::setBoundingRect(const RectF &rect) {
  m_boundingRect = rect;
  refit();
}

/**
 * Check if the shape masks a point.
 *
 * @param p :: Point to check.
 */
bool Shape2D::isMasked(const QPointF &p) const {
  return m_fill_color != QColor() && contains(p);
}

/** Load shape 2D state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 * @return a new shape2D with old state applied
 */
Shape2D *Shape2D::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);

  if (!tsv.selectLine("Type"))
    return nullptr;

  std::string type;
  tsv >> type;

  Shape2D *shape = loadShape2DFromType(type, lines);
  if (!shape)
    return nullptr;

  if (tsv.selectLine("Properties")) {
    bool scalable, editing, selected, visible;
    tsv >> scalable >> editing >> selected >> visible;

    shape->setScalable(scalable);
    shape->edit(editing);
    shape->setSelected(selected);
    shape->setVisible(visible);
  }

  if (tsv.selectLine("Color")) {
    QColor color;
    tsv >> color;
    shape->setColor(color);
  }

  if (tsv.selectLine("FillColor")) {
    QColor color;
    tsv >> color;
    shape->setFillColor(color);
  }

  return shape;
}

/**
 * Instantiate different types of Shape2D from a string
 *
 * @param type :: a string representing the type e.g. ellipse
 * @param lines :: Mantid project lines to parse state from
 * @return a new instance of a Shape2D
 */
Shape2D *Shape2D::loadShape2DFromType(const std::string &type,
                                      const std::string &lines) {
  Shape2D *shape = nullptr;

  if (type == "ellipse") {
    shape = Shape2DEllipse::loadFromProject(lines);
  } else if (type == "rectangle") {
    shape = Shape2DRectangle::loadFromProject(lines);
  } else if (type == "ring") {
    shape = Shape2DRing::loadFromProject(lines);
  } else if (type == "free") {
    shape = Shape2DFree::loadFromProject(lines);
  }

  return shape;
}

/** Save the state of the shape 2D to a Mantid project file
 * @return a string representing the state of the shape 2D
 */
std::string Shape2D::saveToProject() const {
  API::TSVSerialiser tsv;
  bool props[]{m_scalable, m_editing, m_selected, m_visible};

  tsv.writeLine("Properties");
  for (auto prop : props) {
    tsv << prop;
  }

  auto color = getColor();
  tsv.writeLine("Color") << color;

  auto fillColor = getFillColor();
  tsv.writeLine("FillColor") << fillColor;

  return tsv.outputLines();
}

// --- Shape2DEllipse --- //

Shape2DEllipse::Shape2DEllipse(const QPointF &center, double radius1,
                               double radius2)
    : Shape2D() {
  if (radius2 == 0) {
    radius2 = radius1;
  }
  QPointF dr(radius1, radius2);
  m_boundingRect = RectF(center - dr, center + dr);
}

void Shape2DEllipse::drawShape(QPainter &painter) const {
  QRectF drawRect = m_boundingRect.toQRectF();
  painter.drawEllipse(drawRect);
  if (m_fill_color != QColor()) {
    QPainterPath path;
    path.addEllipse(drawRect);
    painter.fillPath(path, m_fill_color);
  }
}

void Shape2DEllipse::addToPath(QPainterPath &path) const {
  path.addEllipse(m_boundingRect.toQRectF());
}

bool Shape2DEllipse::selectAt(const QPointF &p) const {
  if (m_fill_color != QColor()) { // filled ellipse
    return contains(p);
  }

  double a = m_boundingRect.xSpan() / 2;
  if (a == 0.0)
    a = 1.0;
  double b = m_boundingRect.ySpan() / 2;
  if (b == 0.0)
    b = 1.0;
  double xx = m_boundingRect.x0() + a - double(p.x());
  double yy = m_boundingRect.y0() + b - double(p.y());

  double f = fabs(xx * xx / (a * a) + yy * yy / (b * b) - 1);

  return f < 0.1;
}

bool Shape2DEllipse::contains(const QPointF &p) const {
  if (m_boundingRect.isEmpty())
    return false;
  QPointF pp = m_boundingRect.center() - p;
  double a = m_boundingRect.xSpan() / 2;
  if (a == 0.0)
    a = 1.0;
  double b = m_boundingRect.ySpan() / 2;
  if (b == 0.0)
    b = 1.0;
  double xx = pp.x();
  double yy = pp.y();

  double f = xx * xx / (a * a) + yy * yy / (b * b);

  return f <= 1.0;
}

QStringList Shape2DEllipse::getDoubleNames() const {
  QStringList res;
  res << "radius1"
      << "radius2";
  return res;
}

double Shape2DEllipse::getDouble(const QString &prop) const {
  if (prop == "radius1") {
    return m_boundingRect.width() / 2;
  } else if (prop == "radius2") {
    return m_boundingRect.height() / 2;
  }
  return 0.0;
}

void Shape2DEllipse::setDouble(const QString &prop, double value) {
  if (prop == "radius1") {
    if (value <= 0.0)
      value = 1.0;
    double d = value - m_boundingRect.width() / 2;
    adjustBoundingRect(-d, 0, d, 0);
  } else if (prop == "radius2") {
    if (value <= 0.0)
      value = 1.0;
    double d = value - m_boundingRect.height() / 2;
    adjustBoundingRect(0, -d, 0, d);
  }
}

QPointF Shape2DEllipse::getPoint(const QString &prop) const {
  if (prop == "center" || prop == "centre") {
    return m_boundingRect.center();
  }
  return QPointF();
}

void Shape2DEllipse::setPoint(const QString &prop, const QPointF &value) {
  if (prop == "center" || prop == "centre") {
    m_boundingRect.moveCenter(value);
  }
}

/** Load shape 2D state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 * @return a new shape2D in the shape of a ellipse
 */
Shape2D *Shape2DEllipse::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);
  tsv.selectLine("Parameters");
  double radius1, radius2, x, y;
  tsv >> radius1 >> radius2 >> x >> y;
  return new Shape2DEllipse(QPointF(x, y), radius1, radius2);
}

/** Save the state of the shape 2D ellipe to a Mantid project file
 * @return a string representing the state of the shape 2D
 */
std::string Shape2DEllipse::saveToProject() const {
  API::TSVSerialiser tsv;
  double radius1 = getDouble("radius1");
  double radius2 = getDouble("radius2");
  auto centre = getPoint("centre");

  tsv.writeLine("Type") << "ellipse";
  tsv.writeLine("Parameters") << radius1 << radius2 << centre.x(), centre.y();
  tsv.writeRaw(Shape2D::saveToProject());
  return tsv.outputLines();
}
// --- Shape2DRectangle --- //

Shape2DRectangle::Shape2DRectangle() { m_boundingRect = RectF(); }

Shape2DRectangle::Shape2DRectangle(const QPointF &p0, const QPointF &p1) {
  m_boundingRect = RectF(p0, p1);
}

Shape2DRectangle::Shape2DRectangle(const QPointF &p0, const QSizeF &size) {
  m_boundingRect = RectF(p0, size);
}

bool Shape2DRectangle::selectAt(const QPointF &p) const {
  if (m_fill_color != QColor()) { // filled rectangle
    return contains(p);
  }

  RectF outer(m_boundingRect);
  outer.adjust(QPointF(-2, -2), QPointF(2, 2));
  RectF inner(m_boundingRect);
  inner.adjust(QPointF(2, 2), QPointF(-2, -2));
  return outer.contains(p) && !inner.contains(p);
}

void Shape2DRectangle::drawShape(QPainter &painter) const {
  QRectF drawRect = m_boundingRect.toQRectF();
  painter.drawRect(drawRect);
  if (m_fill_color != QColor()) {
    QPainterPath path;
    path.addRect(drawRect);
    painter.fillPath(path, m_fill_color);
  }
}

void Shape2DRectangle::addToPath(QPainterPath &path) const {
  path.addRect(m_boundingRect.toQRectF());
}

/** Load shape 2D state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 * @return a new shape2D in the shape of a rectangle
 */
Shape2D *Shape2DRectangle::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);
  tsv.selectLine("Parameters");
  double x0, y0, x1, y1;
  tsv >> x0 >> y0 >> x1 >> y1;
  QPointF point1(x0, y0);
  QPointF point2(x1, y1);
  return new Shape2DRectangle(point1, point2);
}

/** Save the state of the shape 2D rectangle to a Mantid project file
 * @return a string representing the state of the shape 2D
 */
std::string Shape2DRectangle::saveToProject() const {
  API::TSVSerialiser tsv;
  auto x0 = m_boundingRect.x0();
  auto x1 = m_boundingRect.x1();
  auto y0 = m_boundingRect.y0();
  auto y1 = m_boundingRect.y1();

  tsv.writeLine("Type") << "rectangle";
  tsv.writeLine("Parameters") << x0 << y0 << x1 << y1;
  tsv.writeRaw(Shape2D::saveToProject());
  return tsv.outputLines();
}

// --- Shape2DRing --- //

Shape2DRing::Shape2DRing(Shape2D *shape, double xWidth, double yWidth)
    : m_outer_shape(shape), m_xWidth(xWidth), m_yWidth(yWidth) {
  m_inner_shape = m_outer_shape->clone();
  m_inner_shape->getBoundingRect();
  m_inner_shape->adjustBoundingRect(m_xWidth, m_yWidth, -m_xWidth, -m_yWidth);
  resetBoundingRect();
  m_outer_shape->setFillColor(QColor());
  m_inner_shape->setFillColor(QColor());
}

Shape2DRing::Shape2DRing(const Shape2DRing &ring)
    : Shape2D(), m_outer_shape(ring.m_outer_shape->clone()),
      m_inner_shape(ring.m_inner_shape->clone()), m_xWidth(ring.m_xWidth),
      m_yWidth(ring.m_yWidth) {
  resetBoundingRect();
}

bool Shape2DRing::selectAt(const QPointF &p) const { return contains(p); }

bool Shape2DRing::contains(const QPointF &p) const {
  return m_outer_shape->contains(p) && !m_inner_shape->contains(p);
}

void Shape2DRing::drawShape(QPainter &painter) const {
  m_outer_shape->draw(painter);
  m_inner_shape->draw(painter);
  if (m_fill_color != QColor()) {
    QPainterPath path;
    m_outer_shape->addToPath(path);
    m_inner_shape->addToPath(path);
    painter.fillPath(path, m_fill_color);
  }
}

void Shape2DRing::refit() {
  if (m_xWidth <= 0)
    m_xWidth = 0.000001;
  if (m_yWidth <= 0)
    m_yWidth = 0.000001;
  double xWidth = m_xWidth;
  double yWidth = m_yWidth;
  double max_width = m_boundingRect.width() / 2;
  if (xWidth > max_width)
    xWidth = max_width;
  double max_height = m_boundingRect.height() / 2;
  if (yWidth > max_height)
    yWidth = max_height;
  m_outer_shape->setBoundingRect(m_boundingRect);
  m_inner_shape->setBoundingRect(m_boundingRect);
  m_inner_shape->adjustBoundingRect(xWidth, yWidth, -xWidth, -yWidth);
}

void Shape2DRing::resetBoundingRect() {
  m_boundingRect = m_outer_shape->getBoundingRect();
}

QPointF Shape2DRing::getShapeControlPoint(size_t i) const {
  RectF rect = m_inner_shape->getBoundingRect();
  switch (i) {
  case 0:
    return QPointF(rect.center().x(), rect.y1());
  case 1:
    return QPointF(rect.center().x(), rect.y0());
  case 2:
    return QPointF(rect.x0(), rect.center().y());
  case 3:
    return QPointF(rect.x1(), rect.center().y());
  }
  return QPointF();
}

void Shape2DRing::setShapeControlPoint(size_t i, const QPointF &pos) {
  QPointF dp = pos - getShapeControlPoint(i);

  switch (i) {
  case 0:
    m_yWidth -= dp.y();
    break;
  case 1:
    m_yWidth += dp.y();
    break;
  case 2:
    m_xWidth += dp.x();
    break;
  case 3:
    m_xWidth -= dp.x();
    break;
  }
  refit();
}

QStringList Shape2DRing::getDoubleNames() const {
  QStringList res;
  res << "xwidth"
      << "ywidth";
  return res;
}

double Shape2DRing::getDouble(const QString &prop) const {
  if (prop == "xwidth") {
    return m_xWidth;
  }
  if (prop == "ywidth") {
    return m_yWidth;
  }
  return 0.0;
}

void Shape2DRing::setDouble(const QString &prop, double value) {
  if (prop == "xwidth") {
    m_xWidth = value;
    refit();
  }
  if (prop == "ywidth") {
    m_yWidth = value;
    refit();
  }
}

QPointF Shape2DRing::getPoint(const QString &prop) const {
  if (prop == "center") {
    return m_boundingRect.center();
  }
  return QPointF();
}

void Shape2DRing::setPoint(const QString &prop, const QPointF &value) {
  if (prop == "center") {
    m_boundingRect.moveCenter(value);
  }
}

void Shape2DRing::setColor(const QColor &color) {
  m_inner_shape->setColor(color);
  m_outer_shape->setColor(color);
}

/** Load shape 2D state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 * @return a new shape2D in the shape of a ring
 */
Shape2D *Shape2DRing::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);
  tsv.selectLine("Parameters");
  double xWidth, yWidth;
  tsv >> xWidth >> yWidth;

  tsv.selectSection("shape");
  std::string baseShapeLines;
  tsv >> baseShapeLines;

  auto baseShape = Shape2D::loadFromProject(baseShapeLines);
  return new Shape2DRing(baseShape, xWidth, yWidth);
}

/** Save the state of the shape 2D ring to a Mantid project file
 * @return a string representing the state of the shape 2D
 */
std::string Shape2DRing::saveToProject() const {
  API::TSVSerialiser tsv;
  auto xWidth = getDouble("xwidth");
  auto yWidth = getDouble("ywidth");
  auto baseShape = getOuterShape();

  tsv.writeLine("Type") << "ring";
  tsv.writeLine("Parameters") << xWidth << yWidth;
  tsv.writeSection("shape", baseShape->saveToProject());
  tsv.writeRaw(Shape2D::saveToProject());
  return tsv.outputLines();
}

//------------------------------------------------------------------------------

/// Construct a zero-sized shape.
Shape2DFree::Shape2DFree(const QPointF &p) : m_polygon(QRectF(p, p)) {
  resetBoundingRect();
}

/// The shape can be selected if it contains the point.
bool Shape2DFree::selectAt(const QPointF &p) const { return contains(p); }

/// Check if a point is inside the shape.
bool Shape2DFree::contains(const QPointF &p) const {
  return m_polygon.containsPoint(p, Qt::OddEvenFill);
}

/// Add to a larger shape.
void Shape2DFree::addToPath(QPainterPath &path) const {
  path.addPolygon(m_polygon);
}

/// Draw.
void Shape2DFree::drawShape(QPainter &painter) const {
  QPainterPath path;
  path.addPolygon(m_polygon);
  painter.fillPath(path, m_fill_color);
  painter.drawPath(m_outline);
}

/// Rescale polygon's verices to fit to the new bounding rect.
void Shape2DFree::refit() {
  auto brOld = getPolygonBoundingRect();
  auto &brNew = m_boundingRect;
  if (brNew.xSpan() < 0.0)
    brNew.xFlip();
  if (brNew.ySpan() < 0.0)
    brNew.yFlip();

  auto xs0 = brNew.x0();
  auto x0 = brOld.x0();
  auto xScale = brNew.width() / brOld.width();

  auto ys0 = brNew.y0();
  auto y0 = brOld.y0();
  auto yScale = brNew.height() / brOld.height();
  for (int i = 0; i < m_polygon.size(); ++i) {
    auto &p = m_polygon[i];
    p.rx() = xs0 + xScale * (p.x() - x0);
    p.ry() = ys0 + yScale * (p.y() - y0);
  }
  resetBoundingRect();
}

/// Recalculate the bounding rect.
/// Also make the new border outline.
/// QPolygonF cannot have holes or disjointed parts,
/// it's a single closed line. The outline (implemented as a QPainterPath)
/// makes it look like it have holes.
void Shape2DFree::resetBoundingRect() {
  m_boundingRect = getPolygonBoundingRect();
  // Clear the outline path.
  m_outline = QPainterPath();
  if (m_polygon.isEmpty())
    return;

  // If the polygon has apparent holes/discontinuities
  // it will have extra pairs of edges which we don't want
  // to draw.
  auto n = m_polygon.size() - 1;
  // Find those vertices at which we must break the polygon
  // to get rid of these extra edges.
  QList<int> breaks;
  breaks.push_back(0);
  for (int i = 1; i < m_polygon.size() - 1; ++i) {
    auto p = m_polygon[i];
    auto j = m_polygon.indexOf(p, i + 1);
    if (j != -1) {
      auto i1 = i + 1;
      auto j1 = j - 1;
      if (m_polygon[i1] == m_polygon[j1]) {
        breaks.push_back(i);
        breaks.push_back(i1);
        breaks.push_back(j1);
        breaks.push_back(j);
      }
    }
  }
  if (breaks.back() != n) {
    breaks.push_back(n);
  }
  qSort(breaks);

  m_outline.moveTo(m_polygon[0]);
  int j1 = 0;
  // Add contiguous portions of the polygon to the outline
  // and break at points from breaks list.
  for (int i = 0; i < breaks.size(); ++i) {
    auto j = breaks[i];
    if (j == j1 + 1) {
      m_outline.moveTo(m_polygon[j]);
    } else {
      for (auto k = j1; k <= j; ++k) {
        m_outline.lineTo(m_polygon[k]);
      }
    }
    j1 = j;
  }
}

/// Convert the bounding rect computed by QPolygonF to RectF
RectF Shape2DFree::getPolygonBoundingRect() const {
  auto br = m_polygon.boundingRect();
  auto x0 = br.left();
  auto x1 = br.right();
  if (x0 > x1)
    std::swap(x0, x1);
  auto y0 = br.bottom();
  auto y1 = br.top();
  if (y0 > y1)
    std::swap(y0, y1);
  return RectF(QPointF(x0, y0), QPointF(x1, y1));
}

/// Add a polygon to this shape.
void Shape2DFree::addPolygon(const QPolygonF &polygon) {
  m_polygon = m_polygon.united(polygon);
  resetBoundingRect();
}

/// Subtract a polygon from this shape.
void Shape2DFree::subtractPolygon(const QPolygonF &polygon) {
  m_polygon = m_polygon.subtracted(polygon);
  resetBoundingRect();
}

/** Load shape 2D state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 * @return a new freefrom shape2D
 */
Shape2D *Shape2DFree::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);
  QPolygonF polygon;

  size_t paramCount = tsv.values("Parameters").size() - 1;

  tsv.selectLine("Parameters");
  for (size_t i = 0; i < paramCount; i += 2) {
    double x, y;
    tsv >> x >> y;
    polygon << QPointF(x, y);
  }

  return new Shape2DFree(polygon);
}

/** Save the state of the shape 2D to a Mantid project file
 * @return a string representing the state of the shape 2D
 */
std::string Shape2DFree::saveToProject() const {
  API::TSVSerialiser tsv;

  tsv.writeLine("Type") << "free";
  tsv.writeLine("Parameters");
  for (auto &point : m_polygon) {
    tsv << point.x() << point.y();
  }
  tsv.writeRaw(Shape2D::saveToProject());
  return tsv.outputLines();
}

Shape2DFree::Shape2DFree(const QPolygonF &polygon) : m_polygon(polygon) {
  resetBoundingRect();
}

} // namespace MantidWidgets
} // namespace MantidQt
