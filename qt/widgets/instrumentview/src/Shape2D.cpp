// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/Shape2D.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <QApplication>
#include <QFont>
#include <QFontInfo>
#include <QLine>
#include <QMap>
#include <QRectF>
#include <QVector2D>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace MantidQt::MantidWidgets {

// number of control points common for all shapes
const size_t Shape2D::NCommonCP = 4;

/**
 * Set default border color to red and fill color to default Qt color
 * (==QColor()).
 */
Shape2D::Shape2D()
    : m_color(Qt::red), m_fill_color(QColor()), m_scalable(true), m_editing(false), m_selected(false), m_visible(true) {
}

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
    QRectF drawRect = m_boundingRect.translated(-m_boundingRect.center()).toQRectF();
    painter.save();
    painter.rotate(m_boundingRotation);
    painter.translate(QTransform().rotate(-m_boundingRotation).map(m_boundingRect.center()));
    painter.setPen(QPen(QColor(255, 255, 255, 100), 0));
    painter.drawRect(drawRect);
    painter.restore();
    size_t np = NCommonCP;
    double rsize = 2;
    int alpha = 100;
    if (m_editing) {
      // if editing show all CP, make them bigger and opaque
      np = getNControlPoints();
      rsize = static_cast<double>(controlPointSize());
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
size_t Shape2D::getNControlPoints() const { return NCommonCP + this->getShapeNControlPoints(); }

/**
 * Return the radius to use for the control points.
 */
int Shape2D::controlPointSize() const { return QFontInfo(QFont(QApplication::font().family(), 2)).pixelSize(); }

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
    return QTransform().rotate(m_boundingRotation).map(m_boundingRect.vertex(i) - m_boundingRect.center()) +
           m_boundingRect.center();

  return getShapeControlPoint(i - NCommonCP);
}

void Shape2D::setControlPoint(size_t i, const QPointF &pos) {
  if (i >= getNControlPoints()) {
    throw std::range_error("Control point index is out of range");
  }

  if (i < 4) {
    m_boundingRect.setVertex(i, QTransform().rotate(-m_boundingRotation).map(pos - m_boundingRect.center()) +
                                    m_boundingRect.center());

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
void Shape2D::adjustBoundingRect(double dx1, double dy1, double dx2, double dy2) {
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
  return m_fill_color != QColor() &&
         contains(QTransform().rotate(-m_boundingRotation).map(p - m_boundingRect.center()) + m_boundingRect.center());
}

/**
 * Check if the shape is intersecting a QRectF.
 *
 * @param rect :: The QRectF to check for intersecting.
 */
bool Shape2DRectangle::isIntersecting(const QRectF &rect) const { return rect.intersects(m_boundingRect.toQRectF()); }

/** Load shape 2D state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 * @return a new shape2D with old state applied
 */
Shape2D *Shape2D::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);

  if (!tsv.selectLine("Type"))
    return nullptr;

  std::string shapeType;
  tsv >> shapeType;

  Shape2D *shape = loadShape2DFromType(shapeType, lines);
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
Shape2D *Shape2D::loadShape2DFromType(const std::string &type, const std::string &lines) {
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
  const std::array<bool, 4> props = {m_scalable, m_editing, m_selected, m_visible};

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

Shape2DEllipse::Shape2DEllipse(const QPointF &center, double radius1, double radius2) : Shape2D() {
  if (radius2 == 0) {
    radius2 = radius1;
  }
  QPointF dr(radius1, radius2);
  m_boundingRect = RectF(center - dr, center + dr);
}

void Shape2DEllipse::drawShape(QPainter &painter) const {
  QRectF drawRect = m_boundingRect.translated(-m_boundingRect.center()).toQRectF();
  painter.save();
  painter.rotate(m_boundingRotation);
  painter.translate(QTransform().rotate(-m_boundingRotation).map(m_boundingRect.center()));
  painter.drawEllipse(drawRect);
  if (m_fill_color != QColor()) {
    QPainterPath path;
    path.addEllipse(drawRect);
    painter.fillPath(path, m_fill_color);
  }
  painter.restore();
}

void Shape2DEllipse::addToPath(QPainterPath &path) const { path.addEllipse(m_boundingRect.toQRectF()); }

bool Shape2DEllipse::selectAt(const QPointF &p) const {
  if (m_fill_color != QColor()) { // filled ellipse
    return contains(QTransform().rotate(-m_boundingRotation).map(p - m_boundingRect.center()) +
                    m_boundingRect.center());
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

Shape2DRectangle::Shape2DRectangle(const QPointF &p0, const QPointF &p1) { m_boundingRect = RectF(p0, p1); }

Shape2DRectangle::Shape2DRectangle(const QPointF &p0, const QSizeF &size) { m_boundingRect = RectF(p0, size); }

bool Shape2DRectangle::selectAt(const QPointF &p) const {
  if (m_fill_color != QColor()) { // filled rectangle
    return contains(QTransform().rotate(-m_boundingRotation).map(p - m_boundingRect.center()) +
                    m_boundingRect.center());
  }

  RectF outer(m_boundingRect);
  outer.adjust(QPointF(-2, -2), QPointF(2, 2));
  RectF inner(m_boundingRect);
  inner.adjust(QPointF(2, 2), QPointF(-2, -2));
  return outer.contains(p) && !inner.contains(p);
}

void Shape2DRectangle::drawShape(QPainter &painter) const {
  QRectF drawRect = m_boundingRect.translated(-m_boundingRect.center()).toQRectF();
  painter.save();
  painter.rotate(m_boundingRotation);
  painter.translate(QTransform().rotate(-m_boundingRotation).map(m_boundingRect.center()));
  painter.drawRect(drawRect);
  if (m_fill_color != QColor()) {
    QPainterPath path;
    path.addRect(drawRect);
    painter.fillPath(path, m_fill_color);
  }
  painter.restore();
}

void Shape2DRectangle::addToPath(QPainterPath &path) const { path.addRect(m_boundingRect.toQRectF()); }

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
    : Shape2D(), m_outer_shape(ring.m_outer_shape->clone()), m_inner_shape(ring.m_inner_shape->clone()),
      m_xWidth(ring.m_xWidth), m_yWidth(ring.m_yWidth) {
  resetBoundingRect();
}

bool Shape2DRing::selectAt(const QPointF &p) const { return contains(p); }

bool Shape2DRing::contains(const QPointF &p) const { return m_outer_shape->contains(p) && !m_inner_shape->contains(p); }

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

void Shape2DRing::resetBoundingRect() { m_boundingRect = m_outer_shape->getBoundingRect(); }

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

// --- Shape2DSector --- //

Shape2DSector::Shape2DSector(double innerRadius, double outerRadius, double startAngle, double endAngle,
                             const QPointF &center) {
  m_innerRadius = std::min(innerRadius, outerRadius);
  m_outerRadius = std::max(innerRadius, outerRadius);

  m_startAngle = std::fmod(startAngle, 2 * M_PI);
  m_endAngle = std::fmod(endAngle, 2 * M_PI);
  m_center = center;
  resetBoundingRect();
}

Shape2DSector::Shape2DSector(const Shape2DSector &sector)
    : Shape2D(), m_innerRadius(sector.m_innerRadius), m_outerRadius(sector.m_outerRadius),
      m_startAngle(sector.m_startAngle), m_endAngle(sector.m_endAngle), m_center(sector.m_center) {
  setColor(sector.getColor());
  resetBoundingRect();
}

/**
 * @brief Shape2DSector::selectAt
 * Checks if the sector can be selected at a given point
 * @param p :: the position to check
 * @return
 */
bool Shape2DSector::selectAt(const QPointF &p) const { return contains(p); }

/**
 * @brief Shape2DSector::contains
 * Checks if a given point is inside the sector
 * @param p :: the position to check
 * @return
 */
bool Shape2DSector::contains(const QPointF &p) const {
  QPointF relPos = p - m_center;

  double distance = distanceBetween(relPos, QPointF(0, 0));
  if (distance < m_innerRadius || distance > m_outerRadius) {
    return false;
  }

  double angle = std::atan2(relPos.y(), relPos.x());
  if (angle < 0) {
    angle += 2 * M_PI;
  }

  return ((m_startAngle <= angle && angle <= m_endAngle) ||
          (m_startAngle > m_endAngle && (angle <= m_endAngle || angle >= m_startAngle)));
}

/**
 * @brief Shape2DSector::drawShape
 * Uses Qt to actually draw the sector shape.
 * @param painter :: QPainter used for drawing.
 */
void Shape2DSector::drawShape(QPainter &painter) const {
  QPainterPath path;
  double to_degrees = 180 / M_PI;
  double x_origin = m_center.x() + std::cos(m_startAngle) * m_innerRadius;
  double y_origin = m_center.y() + std::sin(m_startAngle) * m_innerRadius;

  double x_arcEnd = m_center.x() + std::cos(m_endAngle) * m_outerRadius;
  double y_arcEnd = m_center.y() + std::sin(m_endAngle) * m_outerRadius;

  double sweepLength = (m_endAngle - m_startAngle) * to_degrees;
  if (sweepLength < 0) {
    sweepLength += 360;
  }

  path.moveTo(x_origin, y_origin);
  QRectF absoluteBBox(QPointF(-1, 1), QPointF(1, -1));

  path.arcTo(
      QRectF(absoluteBBox.topLeft() * m_innerRadius + m_center, absoluteBBox.bottomRight() * m_innerRadius + m_center),
      m_startAngle * to_degrees, sweepLength);
  path.lineTo(x_arcEnd, y_arcEnd);
  path.arcTo(
      QRectF(absoluteBBox.topLeft() * m_outerRadius + m_center, absoluteBBox.bottomRight() * m_outerRadius + m_center),
      m_endAngle * to_degrees, -sweepLength);
  path.closeSubpath();

  painter.drawPath(path);
  if (m_fill_color != QColor()) {
    painter.fillPath(path, m_fill_color);
  }
}

/**
 * Compute the bounding box of the sector defined by the attributes m_center,
 * m_startAngle, m_endAngle, m_innerRadius, m_outerRadius (and NOT using
 * m_boundingBox)
 **/
QRectF Shape2DSector::findSectorBoundingBox() {
  double xMin, xMax, yMin, yMax;

  // checking in turns the limits of the bounding box

  // the yMax value is the outerRaius if the sector reaches pi/2
  if ((m_startAngle <= M_PI / 2 && m_endAngle >= M_PI / 2) ||
      (m_startAngle > m_endAngle && !(m_startAngle >= M_PI / 2 && m_endAngle <= M_PI / 2))) {
    yMax = m_outerRadius;
    // else it has to be computed
  } else {
    yMax = std::max(std::sin(m_startAngle), std::sin(m_endAngle));
    yMax = std::max(yMax * m_innerRadius, yMax * m_outerRadius);
  }

  // xMin is -outerRadius if the sector reaches pi
  if ((m_startAngle <= M_PI && m_endAngle >= M_PI) ||
      (m_startAngle > m_endAngle && !(m_startAngle >= M_PI && m_endAngle <= M_PI))) {
    xMin = -m_outerRadius;
  } else {
    xMin = std::min(std::cos(m_startAngle), std::cos(m_endAngle));
    xMin = std::min(xMin * m_innerRadius, xMin * m_outerRadius);
  }

  // yMin is -outerRadius if the sector reaches 3pi/2
  if ((m_startAngle <= 3 * M_PI / 2 && m_endAngle >= 3 * M_PI / 2) ||
      (m_startAngle > m_endAngle && !(m_startAngle >= 3 * M_PI / 2 && m_endAngle <= 3 * M_PI / 2))) {
    yMin = -m_outerRadius;
  } else {
    yMin = std::min(std::sin(m_startAngle), std::sin(m_endAngle));
    yMin = std::min(yMin * m_innerRadius, yMin * m_outerRadius);
  }

  // and xMax is outerRadius if the sector reaches 0 (which is equivalent to
  // this condition, given the constraints on the angles)
  if (m_startAngle > m_endAngle) {
    xMax = m_outerRadius;
  } else {
    xMax = std::max(std::cos(m_startAngle), std::cos(m_endAngle));
    xMax = std::max(xMax * m_innerRadius, xMax * m_outerRadius);
  }

  QPointF topLeft(xMin, yMax);
  QPointF bottomRight(xMax, yMin);
  return QRectF(topLeft + m_center, bottomRight + m_center);
}

/**
 * @brief Shape2DSector::refit
 * Enforce coherence between all the parameters defining the sector. Used when
 * it is updated, mostly when moved or scaled.
 */
void Shape2DSector::refit() {
  constexpr double epsilon = 1e-6;

  // current real bounding box of the sector, based on the attributes, before
  // the user's modifications take place
  QRectF BBox = findSectorBoundingBox();

  // corners of the user-modified bounding box
  QPointF bRectTopLeft(std::min(m_boundingRect.p0().x(), m_boundingRect.p1().x()),
                       std::max(m_boundingRect.p0().y(), m_boundingRect.p1().y()));
  QPointF bRectBottomRight(std::max(m_boundingRect.p0().x(), m_boundingRect.p1().x()),
                           std::min(m_boundingRect.p0().y(), m_boundingRect.p1().y()));

  // check if the bounding box has been modified

  // since the calculus are made on relatively small numbers, some errors can
  // progressively appear and stack, so we have to take a range rather than a
  // strict equality
  if (BBox.topLeft().x() != bRectTopLeft.x() && BBox.topLeft().y() != bRectTopLeft.y() &&
      std::abs(BBox.bottomRight().x() - bRectBottomRight.x()) < epsilon &&
      std::abs(BBox.bottomRight().y() - bRectBottomRight.y()) < epsilon) {

    // top left corner is moving
    computeScaling(BBox.topLeft(), BBox.bottomRight(), bRectTopLeft, 0);

  } else if (BBox.topLeft().x() != bRectTopLeft.x() && BBox.bottomRight().y() != bRectBottomRight.y() &&
             std::abs(BBox.bottomRight().x() - bRectBottomRight.x()) < epsilon &&
             std::abs(BBox.topLeft().y() - bRectTopLeft.y()) < epsilon) {

    // bottom left corner is moving
    computeScaling(BBox.bottomLeft(), BBox.topRight(), QPointF(bRectTopLeft.x(), bRectBottomRight.y()), 1);

  } else if (BBox.bottomRight().x() != bRectBottomRight.x() && BBox.bottomRight().y() != bRectBottomRight.y() &&
             std::abs(BBox.topLeft().x() - bRectTopLeft.x()) < epsilon &&
             std::abs(BBox.topLeft().y() - bRectTopLeft.y()) < epsilon) {

    // bottom right corner is moving
    computeScaling(BBox.bottomRight(), BBox.topLeft(), bRectBottomRight, 2);

  } else if (BBox.bottomRight().x() != bRectBottomRight.x() && BBox.topLeft().y() != bRectTopLeft.y() &&
             std::abs(BBox.topLeft().x() - bRectTopLeft.x()) < epsilon &&
             std::abs(BBox.bottomRight().y() - bRectBottomRight.y()) < epsilon) {

    // top right corner is moving
    computeScaling(BBox.topRight(), BBox.bottomLeft(), QPointF(bRectBottomRight.x(), bRectTopLeft.y()), 3);
  }

  // check if the shape has moved
  if ((BBox.bottomRight().x() != bRectBottomRight.x() && BBox.topLeft().x() != bRectTopLeft.x() &&
       std::abs((BBox.bottomRight().x() - bRectBottomRight.x()) - (BBox.topLeft().x() - bRectTopLeft.x())) < epsilon) ||
      (BBox.bottomRight().y() != bRectBottomRight.y() && BBox.topLeft().y() != bRectTopLeft.y() &&
       std::abs((BBox.bottomRight().y() - bRectBottomRight.y()) - (BBox.topLeft().y() - bRectTopLeft.y())) < epsilon)) {
    // every corner has moved by the same distance -> the shape is being moved
    qreal xDiff = bRectBottomRight.x() - BBox.bottomRight().x();
    qreal yDiff = bRectBottomRight.y() - BBox.bottomRight().y();

    m_center.setX(m_center.x() + xDiff);
    m_center.setY(m_center.y() + yDiff);
    resetBoundingRect();
  }
}

/**
 * @brief Shape2DSector::computeScaling
 * Used when updating the bounding box after the user dragged a corner. Given
 * the constraints of a circular sector, the new bounding box cannot be exactly
 * the one drawed by the mouse of the user, and thus a number of corrections are
 * needed.
 * This method thus corrects this new value and then modifies the difining
 * parameters of the sector accordingly.
 *
 * @param BBoxCorner :: the corner modified by the user, before it has been
 * changed.
 * @param BBoxOpposedCorner :: the corner diagonally opposed to the one the user
 * modified, which has not changed
 * @param bRectCorner :: the new position of the corner moved by the user,
 * before any correction applies
 * @param vertexIndex :: the index of the vertex corresponding to the one
 * modified by the user in m_boundingRect
 */
void Shape2DSector::computeScaling(const QPointF &BBoxCorner, const QPointF &BBoxOpposedCorner,
                                   const QPointF &bRectCorner, int vertexIndex) {

  // first we need to find the best projection of the new corner on the
  // diagonal line of the rectangle, so its shape won't be modified, only
  // scaled.
  QPointF xProj, yProj, proj;
  qreal xPos, yPos;
  QVector2D slope;

  slope = QVector2D(BBoxCorner - BBoxOpposedCorner);
  xPos = (bRectCorner - BBoxCorner).x();
  yPos = slope.y() * xPos / slope.x(); // TODO : check if non zero
  xProj.setX(xPos);
  xProj.setY(yPos);

  yPos = (bRectCorner - BBoxCorner).y();
  xPos = slope.x() * yPos / slope.y();

  yProj.setX(xPos);
  yProj.setY(yPos);

  if (slope.x() != 0 && slope.y() != 0) {
    if (distanceBetween(xProj, QPointF(0, 0)) < distanceBetween(yProj, QPointF(0, 0))) {
      proj = xProj;
    } else {
      proj = yProj;
    }
  } else if (slope.x() != 0) {
    proj = xProj;
  } else if (slope.y() != 0) {
    proj = yProj;
  } else {
    // case that is not supposed to happen; it means the sector has been reduced
    // to a point, which is not possible
    return;
  }
  proj += BBoxCorner;

  // then we need to adapt the shape to the new size
  qreal ratio = distanceBetween(proj, BBoxOpposedCorner) / distanceBetween(slope.toPointF(), QPointF(0, 0));

  m_boundingRect.setVertex(vertexIndex, proj);

  m_innerRadius *= ratio;
  m_outerRadius = ratio != 0 ? m_outerRadius * ratio : 1e-4;
  m_center.setX((m_center.x() - BBoxOpposedCorner.x()) * ratio + BBoxOpposedCorner.x());
  m_center.setY((m_center.y() - BBoxOpposedCorner.y()) * ratio + BBoxOpposedCorner.y());
}

/**
 * @brief Shape2DSector::distanceBetween
 * Helper method to calculate the distance between 2 QPointF points.
 * @param p0 :: the first point
 * @param p1 :: the second point
 * @return  the distance
 */
double Shape2DSector::distanceBetween(const QPointF &p0, const QPointF &p1) const {
  return sqrt(pow(p0.x() - p1.x(), 2) + pow(p0.y() - p1.y(), 2));
}

/**
 * @brief Shape2DSector::resetBoundingRect
 * Compute m_boundingBox using the geometrical parameters of the sector (ie
 * center, angles and radii)
 **/
void Shape2DSector::resetBoundingRect() {

  QRectF BBox = findSectorBoundingBox();
  // because of how Mantid's rectangles are defined, it is necessary to pass the
  // arguments in this precise order in order to have a smooth scaling when
  // creating a shape from top left corner
  m_boundingRect = RectF(BBox.bottomLeft(), BBox.topRight());
}

/**
 * Return coordinates of i-th control point.
 * 0 controls the outer radius, 1 the inner, 2 the starting angle and 3 the
 * ending one.
 * @param i :: Index of a control point. 0 <= i < getNControlPoints().
 */
QPointF Shape2DSector::getShapeControlPoint(size_t i) const {
  double halfAngle = m_startAngle < m_endAngle ? std::fmod((m_startAngle + m_endAngle) / 2., 2 * M_PI)
                                               : std::fmod((m_startAngle + m_endAngle + 2 * M_PI) / 2, 2 * M_PI);
  double halfLength = (m_outerRadius + m_innerRadius) / 2;

  switch (i) {
  case 0:
    return QPointF(m_center.x() + std::cos(halfAngle) * m_outerRadius,
                   m_center.y() + std::sin(halfAngle) * m_outerRadius);
  case 1:
    return QPointF(m_center.x() + std::cos(halfAngle) * m_innerRadius,
                   m_center.y() + std::sin(halfAngle) * m_innerRadius);
  case 2:
    return QPointF(m_center.x() + std::cos(m_startAngle) * halfLength,
                   m_center.y() + std::sin(m_startAngle) * halfLength);
  case 3:
    return QPointF(m_center.x() + std::cos(m_endAngle) * halfLength, m_center.y() + std::sin(m_endAngle) * halfLength);
  default:
    return QPointF();
  }
}

/**
 * @brief Shape2DSector::setShapeControlPoint
 * Modify the sector when the i-th control point is moved.
 * 0 is outer radius, 1 is inner, 2 is starting angle, 3 is ending.
 * @param i :: index of the control point changed
 * @param pos :: the new position of the control point (might not be where it is
 * palced at the end of the update though, since there are constraints to take
 * into account)
 */
void Shape2DSector::setShapeControlPoint(size_t i, const QPointF &pos) {
  QPointF to_center = pos - m_center;
  constexpr double epsilon = 1e-6;
  double newAngle;

  switch (i) {
  case 0:
    m_outerRadius = distanceBetween(to_center, QPointF(0, 0));
    if (m_outerRadius < m_innerRadius) {
      m_outerRadius = m_innerRadius != 0 ? 1.01 * m_innerRadius : 1e-4;
    }
    break;
  case 1:
    m_innerRadius = distanceBetween(to_center, QPointF(0, 0));
    if (m_outerRadius < m_innerRadius) {
      m_innerRadius = 0.99 * m_outerRadius;
    }
    break;
  case 2:
    newAngle = std::atan2(to_center.y(), to_center.x());
    if (newAngle < 0)
      newAngle += 2 * M_PI;

    // conditions to prevent the startAngle from going over the endAngle
    // this one is in case of trigonometrical rotation
    if ((m_startAngle < m_endAngle && newAngle >= m_endAngle && std::abs(newAngle - m_startAngle) < M_PI) ||
        (newAngle < m_endAngle && m_startAngle < m_endAngle && std::abs(newAngle - m_startAngle) > M_PI &&
         newAngle < m_startAngle) ||
        (newAngle > m_endAngle && m_startAngle > m_endAngle && std::abs(newAngle - m_startAngle) > M_PI &&
         newAngle < m_startAngle)) {

      newAngle = m_endAngle - epsilon;

      if (newAngle < 0) {
        newAngle += 2 * M_PI;
      }

      // and this one is in case of clockwise rotation
    } else if ((m_startAngle > m_endAngle && newAngle <= m_endAngle && std::abs(newAngle - m_startAngle) < M_PI) ||
               (newAngle > m_endAngle && m_startAngle > m_endAngle && std::abs(newAngle - m_startAngle) > M_PI &&
                newAngle > m_startAngle) ||
               (newAngle < m_endAngle && m_startAngle < m_endAngle && std::abs(newAngle - m_startAngle) > M_PI &&
                newAngle > m_startAngle)) {

      newAngle = m_endAngle + epsilon;

      if (newAngle >= 2 * M_PI) {
        newAngle -= 2 * M_PI;
      }
    }

    m_startAngle = newAngle;
    break;

  case 3:
    newAngle = std::atan2(to_center.y(), to_center.x());
    if (newAngle < 0)
      newAngle += 2 * M_PI;

    // and conditions to prevent the endAngle from going over the startAngle
    // trigonometrical rotation
    if ((m_endAngle < m_startAngle && newAngle >= m_startAngle && std::abs(newAngle - m_endAngle) < M_PI) ||
        (newAngle < m_startAngle && m_endAngle < m_startAngle && std::abs(newAngle - m_endAngle) > M_PI &&
         newAngle < m_endAngle) ||
        (newAngle > m_startAngle && m_endAngle > m_startAngle && std::abs(newAngle - m_endAngle) > M_PI &&
         newAngle < m_endAngle)) {

      newAngle = m_startAngle - epsilon;

      if (newAngle < 0) {
        newAngle += 2 * M_PI;
      }
      // clockwise rotation
    } else if ((m_endAngle >= m_startAngle && newAngle <= m_startAngle && std::abs(newAngle - m_endAngle) < M_PI) ||
               (newAngle >= m_startAngle && m_endAngle >= m_startAngle && std::abs(newAngle - m_endAngle) > M_PI &&
                newAngle > m_endAngle) ||
               (newAngle < m_startAngle && m_endAngle < m_startAngle && std::abs(newAngle - m_endAngle) > M_PI &&
                newAngle > m_endAngle)) {

      newAngle = m_startAngle + epsilon;
      if (newAngle >= 2 * M_PI) {
        newAngle -= 2 * M_PI;
      }
    }

    m_endAngle = newAngle;
    break;
  default:
    return;
  }
  resetBoundingRect();
}

/**
 * @brief Shape2DSector::getDoubleNames
 * Getter method to access the names of the double attributes
 * @return
 */
QStringList Shape2DSector::getDoubleNames() const {
  QStringList res;
  res << "outerRadius"
      << "innerRadius"
      << "startAngle"
      << "endAngle";
  return res;
}

/**
 * @brief Shape2DSector::getDouble
 * Getter method to access the value of double attributes
 * @param prop :: the name of the sought attribute
 * @return
 */
double Shape2DSector::getDouble(const QString &prop) const {
  double to_degrees = 180 / M_PI;
  if (prop == "outerRadius")
    return m_outerRadius;
  if (prop == "innerRadius")
    return m_innerRadius;
  if (prop == "startAngle")
    return m_startAngle * to_degrees;
  if (prop == "endAngle")
    return m_endAngle * to_degrees;

  return 0.0;
}

/**
 * @brief Shape2DSector::setDouble
 * Set the value of a double attribute, after some checks, and update the
 * sector.
 * @param prop :: the name of the attribute to change
 * @param value :: the new value
 */
void Shape2DSector::setDouble(const QString &prop, double value) {
  double to_radians = M_PI / 180;
  if (prop == "outerRadius") {
    if (m_innerRadius < value)
      m_outerRadius = value;
    else
      m_outerRadius = m_innerRadius != 0 ? 1.01 * m_innerRadius : 1e-4;
  } else if (prop == "innerRadius") {
    value = std::max(0.0, value);
    m_innerRadius = m_outerRadius >= value ? value : 0.99 * m_outerRadius;
  } else if (prop == "startAngle") {
    m_startAngle = std::fmod(value, 360);
    m_startAngle = m_startAngle >= 0 ? m_startAngle : m_startAngle + 360;
    m_startAngle *= to_radians;
  } else if (prop == "endAngle") {
    m_endAngle = std::fmod(value, 360);
    m_endAngle = m_endAngle >= 0 ? m_endAngle : m_endAngle + 360;
    m_endAngle *= to_radians;
  } else
    return;
  resetBoundingRect();
}

/**
 * @brief Shape2DSector::getPoint
 * Getter method to access the value of point attributes (ie m_center).
 * @param prop :: the name of the sought attribute
 * @return
 */
QPointF Shape2DSector::getPoint(const QString &prop) const {
  if (prop == "center") {
    return m_center;
  }
  return QPointF();
}

/**
 * @brief Shape2DSector::setPoint
 * Set the value of a point attribute, after some checks, and update the
 * sector.
 * @param prop :: the name of the point to change
 * @param value :: the new value
 */
void Shape2DSector::setPoint(const QString &prop, const QPointF &value) {
  if (prop == "center") {
    m_center = value;
    resetBoundingRect();
  }
}

/** Load shape 2D state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 * @return a new shape2D with old state applied
 */
Shape2D *Shape2DSector::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);
  tsv.selectLine("Parameters");
  double innerRadius, outerRadius, startAngle, endAngle, xCenter, yCenter;
  tsv >> innerRadius >> outerRadius >> startAngle >> endAngle >> xCenter >> yCenter;

  return new Shape2DSector(innerRadius, outerRadius, startAngle, endAngle, QPointF(xCenter, yCenter));
}

/** Save the state of the sector to a Mantid project file
 * @return a string representing the state of the sector
 */
std::string Shape2DSector::saveToProject() const {
  // WARNING: Q1DWeighted heavily depends on the format of this function's
  // output (via "Save Shapes to table" in the instrument viewer draw tab).
  // Modify with great caution.
  API::TSVSerialiser tsv;

  tsv.writeLine("Type") << "sector";
  tsv.writeLine("Parameters") << m_innerRadius << m_outerRadius << m_startAngle << m_endAngle << m_center.x()
                              << m_center.y();
  tsv.writeRaw(Shape2D::saveToProject());
  return tsv.outputLines();
}
//------------------------------------------------------------------------------

/// Construct a zero-sized shape.
Shape2DFree::Shape2DFree(const QPointF &p) : m_polygon(QRectF(p, p)) { resetBoundingRect(); }

/// The shape can be selected if it contains the point.
bool Shape2DFree::selectAt(const QPointF &p) const { return contains(p); }

/// Check if a point is inside the shape.
bool Shape2DFree::contains(const QPointF &p) const { return m_polygon.containsPoint(p, Qt::OddEvenFill); }

/// Add to a larger shape.
void Shape2DFree::addToPath(QPainterPath &path) const { path.addPolygon(m_polygon); }

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
  using std::sort;
  sort(std::begin(breaks), std::end(breaks));

  m_outline.moveTo(m_polygon[0]);
  int j1 = 0;
  // Add contiguous portions of the polygon to the outline
  // and break at points from breaks list.
  for (int j : breaks) {
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

Shape2DFree::Shape2DFree(QPolygonF polygon) : m_polygon(std::move(polygon)) { resetBoundingRect(); }

} // namespace MantidQt::MantidWidgets
