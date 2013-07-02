#include "Shape2D.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cmath>

// number of control points common for all shapes
const size_t Shape2D::NCommonCP = 4;
// size (== width/2 == height/2) of each control point
const double Shape2D::sizeCP = 3;

/**
  * Set default border color to red and fill color to default Qt color (==QColor()).
  */
Shape2D::Shape2D():
    m_color(Qt::red),
    m_fill_color(QColor()),
    m_scalable(true),
    m_editing(false),
    m_selected(false),
    m_visible(true)
{
}

/**
  * Calls virtual drawShape() method to draw the actial shape.
  * Draws bounding rect and control points if the shape is selected.
  *
  * @param painter :: QPainter used for drawing.
  */
void Shape2D::draw(QPainter& painter) const
{
  if ( !m_visible ) return;
  painter.setPen(m_color);
  this->drawShape(painter);
  if ( m_editing || m_selected )
  {
    QColor c(255,255,255,100);
    painter.setPen(c);
    painter.drawRect(m_boundingRect.toQRectF());
    size_t np = NCommonCP;
    double rsize = 2;
    int alpha = 100;
    if ( m_editing )
    {
        // if editing show all CP, make them bigger and opaque
        np =  getNControlPoints();
        rsize = sizeCP;
        alpha = 255;
    }
    for(size_t i = 0; i < np; ++i)
    {
      QPointF p = painter.transform().map(getControlPoint(i));
      QRectF r(p - QPointF(rsize,rsize),p + QPointF(rsize,rsize));
      painter.save();
      painter.resetTransform();
      QColor c(255,255,255,alpha);
      painter.fillRect(r,c);
      r.adjust(-1,-1,0,0);
      painter.setPen( QColor( 0,0,0, alpha) );
      painter.drawRect(r);
      painter.restore();
    }
  }
}

/**
  * Return total number of control points for this shape.
  */
size_t Shape2D::getNControlPoints() const
{
  return NCommonCP + this->getShapeNControlPoints();
}

/**
  * Return coordinates of i-th control point.
  *
  * @param i :: Index of a control point. 0 <= i < getNControlPoints().
  */
QPointF Shape2D::getControlPoint(size_t i) const
{
  if ( i >= getNControlPoints())
  {
    throw std::range_error("Control point index is out of range");
  }

  if ( i < 4 ) return m_boundingRect.vertex( i );

  return getShapeControlPoint(i - NCommonCP);
}

void Shape2D::setControlPoint(size_t i,const QPointF& pos)
{
  if ( i >= getNControlPoints())
  {
    throw std::range_error("Control point index is out of range");
  }

  if ( i < 4 )
  {
      m_boundingRect.setVertex( i, pos );
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
void Shape2D::moveBy(const QPointF& dp)
{
  m_boundingRect.translate( dp );
  refit();
}

/**
  * Adjust the bound of the bounding rect. Calls virtual method refit()
  * to resize the shape in order to fit into the new bounds.
  */
void Shape2D::adjustBoundingRect(double dx1,double dy1,double dx2,double dy2)
{
  double dwidth = dx2 - dx1;
  if (dwidth <= - m_boundingRect.xSpan())
  {
    double mu = m_boundingRect.xSpan() / fabs(dwidth);
    dx1 *= mu;
    dx2 *= mu;
  }
  double dheight = dy2 - dy1;
  if (dheight <= - m_boundingRect.ySpan())
  {
    double mu = m_boundingRect.ySpan() / fabs(dheight);
    dy1 *= mu;
    dy2 *= mu;
  }
  m_boundingRect.adjust( QPointF(dx1,dy1), QPointF(dx2,dy2) );
  refit();
}

/**
  * Assign new bounding rect. Calls virtual method refit()
  * to resize the shape in order to fit into the new bounds.
  */
void Shape2D::setBoundingRect(const RectF &rect)
{
  m_boundingRect = rect;
  refit();
}

/**
  * Check if the shape masks a point.
  *
  * @param p :: Point to check.
  */
bool Shape2D::isMasked(const QPointF& p)const
{
    return m_fill_color != QColor() && contains(p);
}

// --- Shape2DEllipse --- //

Shape2DEllipse::Shape2DEllipse(const QPointF& center,double radius1,double radius2)
:Shape2D()
{
  if (radius2 == 0)
  {
    radius2 = radius1;
  }
  QPointF dr(radius1,radius2);
  m_boundingRect = RectF(center - dr, center + dr);
}

void Shape2DEllipse::drawShape(QPainter& painter) const
{
  QRectF drawRect = m_boundingRect.toQRectF();
  painter.drawEllipse(drawRect);
  if (m_fill_color != QColor())
  {
    QPainterPath path;
    path.addEllipse(drawRect);
    painter.fillPath(path,m_fill_color);
  }
}

void Shape2DEllipse::addToPath(QPainterPath& path) const
{
  path.addEllipse(m_boundingRect.toQRectF());
}

bool Shape2DEllipse::selectAt(const QPointF& p)const
{
  if (m_fill_color != QColor())
  {// filled ellipse
    return contains(p);
  }

  double a = m_boundingRect.xSpan() / 2;
  if (a == 0.0) a = 1.0;
  double b = m_boundingRect.ySpan() / 2;
  if (b == 0.0) b = 1.0;
  double xx = m_boundingRect.x0() + a - double(p.x());
  double yy = m_boundingRect.y0() + b - double(p.y());

  double f = fabs(xx*xx/(a*a) + yy*yy/(b*b) - 1);

  return f < 0.1;
}

bool Shape2DEllipse::contains(const QPointF& p)const
{
  QPointF pp = m_boundingRect.center() - p;
  double a = m_boundingRect.xSpan() / 2;
  if (a == 0.0) a = 1.0;
  double b = m_boundingRect.ySpan() / 2;
  if (b == 0.0) b = 1.0;
  double xx = pp.x();
  double yy = pp.y();

  double f = xx*xx/(a*a) + yy*yy/(b*b);

  return f <= 1.0;
}

QStringList Shape2DEllipse::getDoubleNames()const
{
  QStringList res;
  res << "radius1" << "radius2";
  return res;
}

double Shape2DEllipse::getDouble(const QString& prop) const
{
  if (prop == "radius1")
  {
    return m_boundingRect.width() / 2;
  }
  else if (prop == "radius2")
  {
    return m_boundingRect.height() / 2;
  }
  return 0.0;
}

void Shape2DEllipse::setDouble(const QString& prop, double value)
{
  if (prop == "radius1")
  {
    if (value <= 0.0) value = 1.0;
    double d = value - m_boundingRect.width() / 2;
    adjustBoundingRect(-d,0,d,0);
  }
  else if (prop == "radius2")
  {
    if (value <= 0.0) value = 1.0;
    double d = value - m_boundingRect.height() / 2;
    adjustBoundingRect(0,-d,0,d);
  }
}

QPointF Shape2DEllipse::getPoint(const QString& prop) const
{
  if (prop == "center" || prop == "centre")
  {
    return m_boundingRect.center();
  }
  return QPointF();
}

void Shape2DEllipse::setPoint(const QString& prop, const QPointF& value)
{
  if (prop == "center" || prop == "centre")
  {
    m_boundingRect.moveCenter(value);
  }
}


// --- Shape2DRectangle --- //

Shape2DRectangle::Shape2DRectangle()
{
  m_boundingRect = RectF();
}

Shape2DRectangle::Shape2DRectangle(const QPointF& p0,const QPointF& p1)
{
  m_boundingRect = RectF(p0,p1);
}

Shape2DRectangle::Shape2DRectangle(const QPointF& p0,const QSizeF& size)
{
  m_boundingRect = RectF(p0,size);
}

bool Shape2DRectangle::selectAt(const QPointF& p)const
{
  if (m_fill_color != QColor())
  {// filled rectangle
    return contains(p);
  }

  RectF outer(m_boundingRect);
  outer.adjust( QPointF(-2,-2), QPointF(2,2) );
  RectF inner(m_boundingRect);
  inner.adjust( QPointF(2,2), QPointF(-2,-2) );
  return outer.contains(p) && !inner.contains(p);
}

void Shape2DRectangle::drawShape(QPainter& painter) const
{
  QRectF drawRect = m_boundingRect.toQRectF();
  painter.drawRect(drawRect);
  if (m_fill_color != QColor())
  {
    QPainterPath path;
    path.addRect(drawRect);
    painter.fillPath(path,m_fill_color);
  }
}

void Shape2DRectangle::addToPath(QPainterPath& path) const
{
  path.addRect(m_boundingRect.toQRectF());
}

// --- Shape2DRing --- //

Shape2DRing::Shape2DRing(Shape2D* shape, double xWidth, double yWidth):
m_outer_shape(shape),
m_xWidth(xWidth),
m_yWidth(yWidth)
{
  m_inner_shape = m_outer_shape->clone();
  m_inner_shape->getBoundingRect();
  m_inner_shape->adjustBoundingRect(m_xWidth,m_yWidth,-m_xWidth,-m_yWidth);
  resetBoundingRect();
  m_outer_shape->setFillColor(QColor());
  m_inner_shape->setFillColor(QColor());
}

Shape2DRing::Shape2DRing(const Shape2DRing& ring):
Shape2D(),
m_outer_shape(ring.m_outer_shape->clone()),
m_inner_shape(ring.m_inner_shape->clone()),
m_xWidth(ring.m_xWidth),
m_yWidth(ring.m_yWidth)
{
  resetBoundingRect();
}

bool Shape2DRing::selectAt(const QPointF& p)const
{
  return contains(p);
}

bool Shape2DRing::contains(const QPointF& p)const
{
  return m_outer_shape->contains(p) && !m_inner_shape->contains(p);
}

void Shape2DRing::drawShape(QPainter& painter) const
{
  m_outer_shape->draw(painter);
  m_inner_shape->draw(painter);
  if (m_fill_color != QColor())
  {
    QPainterPath path;
    m_outer_shape->addToPath(path);
    m_inner_shape->addToPath(path);
    painter.fillPath(path,m_fill_color);
  }
}

void Shape2DRing::refit()
{
  if (m_xWidth <= 0) m_xWidth = 0.000001;
  if (m_yWidth <= 0) m_yWidth = 0.000001;
  double xWidth = m_xWidth;
  double yWidth = m_yWidth;
  double max_width = m_boundingRect.width() / 2;
  if (xWidth > max_width) xWidth = max_width;
  double max_height = m_boundingRect.height() / 2;
  if (yWidth > max_height) yWidth = max_height;
  m_outer_shape->setBoundingRect(m_boundingRect);
  m_inner_shape->setBoundingRect(m_boundingRect);
  m_inner_shape->adjustBoundingRect(xWidth,yWidth,-xWidth,-yWidth);
}

void Shape2DRing::resetBoundingRect()
{
  m_boundingRect = m_outer_shape->getBoundingRect();
}

QPointF Shape2DRing::getShapeControlPoint(size_t i) const
{
  RectF rect = m_inner_shape->getBoundingRect();
  switch(i)
  {
  case 0: return QPointF(rect.center().x(), rect.y1());
  case 1: return QPointF(rect.center().x(), rect.y0());
  case 2: return QPointF(rect.x0(),rect.center().y());
  case 3: return QPointF(rect.x1(),rect.center().y());
  }
  return QPointF();
}

void Shape2DRing::setShapeControlPoint(size_t i,const QPointF& pos)
{
  QPointF dp = pos - getShapeControlPoint(i);

  switch(i)
  {
  case 0: m_yWidth -= dp.y(); break;
  case 1: m_yWidth += dp.y(); break;
  case 2: m_xWidth += dp.x(); break;
  case 3: m_xWidth -= dp.x(); break;
  }
  refit();
}

QStringList Shape2DRing::getDoubleNames()const
{
  QStringList res;
  res << "xwidth" << "ywidth";
  return res;
}

double Shape2DRing::getDouble(const QString& prop) const
{
    if (prop == "xwidth")
    {
      return m_xWidth;
    }
    if (prop == "ywidth")
    {
      return m_yWidth;
    }
  return 0.0;
}

void Shape2DRing::setDouble(const QString& prop, double value)
{
    if (prop == "xwidth")
    {
      m_xWidth = value;
      refit();
    }
    if (prop == "ywidth")
    {
      m_yWidth = value;
      refit();
    }
}

QPointF Shape2DRing::getPoint(const QString& prop) const
{
  if (prop == "center")
  {
    return m_boundingRect.center();
  }
  return QPointF();
}

void Shape2DRing::setPoint(const QString& prop, const QPointF& value)
{
  if (prop == "center")
  {
    m_boundingRect.moveCenter(value);
  }
}

void Shape2DRing::setColor(const QColor &color)
{
    m_inner_shape->setColor(color);
    m_outer_shape->setColor(color);
}

