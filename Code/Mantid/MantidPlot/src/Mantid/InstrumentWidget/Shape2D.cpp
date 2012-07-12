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
const qreal Shape2D::sizeCP = 2;

Shape2D::Shape2D():
m_color(Qt::red),
m_scalable(true),
m_editing(false)
{

}

void Shape2D::draw(QPainter& painter) const
{
  painter.setPen(m_color);
  this->drawShape(painter);
  if (m_editing)
  {
    QColor c(255,255,255,100);
    painter.setPen(c);
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.drawRect(m_boundingRect);
    for(size_t i = 0; i < getNControlPoints(); ++i)
    {
      QPointF p = painter.transform().map(getControlPoint(i));
      QRectF r(p - QPointF(sizeCP,sizeCP),p + QPointF(sizeCP,sizeCP));
      painter.save();
      painter.resetTransform();
      painter.fillRect(r,c);
      painter.restore();
    }
  }
}

size_t Shape2D::getNControlPoints() const
{
  return NCommonCP + this->getShapeNControlPoints();
}

QPointF Shape2D::getControlPoint(size_t i) const
{
  if ( i >= getNControlPoints())
  {
    throw std::range_error("Control point index is out of range");
  }

  switch(i)
  {
  case 0: return m_boundingRect.topLeft();
  case 1: return m_boundingRect.topRight();
  case 2: return m_boundingRect.bottomRight();
  case 3: return m_boundingRect.bottomLeft();
  }
  return getShapeControlPoint(i - NCommonCP);
}

void Shape2D::setControlPoint(size_t i,const QPointF& pos)
{
  if ( i >= getNControlPoints())
  {
    throw std::range_error("Control point index is out of range");
  }

  switch(i)
  {
  case 0: m_boundingRect.setTopLeft(pos); correctBoundingRect(); refit(); break;
  case 1: m_boundingRect.setTopRight(pos); correctBoundingRect(); refit(); break;
  case 2: m_boundingRect.setBottomRight(pos); correctBoundingRect(); refit(); break;
  case 3: m_boundingRect.setBottomLeft(pos); correctBoundingRect(); refit(); break;
  }
  setShapeControlPoint(i - NCommonCP, pos);
  resetBoundingRect();
}

void Shape2D::correctBoundingRect()
{
  qreal left = m_boundingRect.left();
  qreal top = m_boundingRect.top();
  qreal width = m_boundingRect.width();
  qreal height = m_boundingRect.height();
  if (m_boundingRect.width() < 0)
  {
    left = m_boundingRect.right();
    width *= -1;
  }

  if (m_boundingRect.height() < 0)
  {
    top = m_boundingRect.bottom();
    height *= -1;
  }

  m_boundingRect = QRectF(left,top,width,height);

}

void Shape2D::moveBy(const QPointF& dp)
{
  m_boundingRect.adjust(dp.x(),dp.y(),dp.x(),dp.y());
  refit();
}

void Shape2D::adjustBoundingRect(qreal dx1,qreal dy1,qreal dx2,qreal dy2)
{
  qreal dwidth = dx2 - dx1;
  if (dwidth <= - m_boundingRect.width())
  {
    qreal mu = m_boundingRect.width() / fabs(dwidth);
    dx1 *= mu;
    dx2 *= mu;
  }
  qreal dheight = dy2 - dy1;
  if (dheight <= - m_boundingRect.height())
  {
    qreal mu = m_boundingRect.height() / fabs(dheight);
    dy1 *= mu;
    dy2 *= mu;
  }
  m_boundingRect.adjust(dx1,dy1,dx2,dy2);
  refit();
}

void Shape2D::setBoundingRect(const QRectF& rect)
{
  m_boundingRect = rect;
  correctBoundingRect();
  refit();
}

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
  m_boundingRect = QRectF(center - dr, center + dr);
}

void Shape2DEllipse::drawShape(QPainter& painter) const
{
  painter.drawEllipse(m_boundingRect);
  if (m_fill_color != QColor())
  {
    QPainterPath path;
    path.addEllipse(m_boundingRect);
    painter.fillPath(path,m_fill_color);
  }
}

void Shape2DEllipse::addToPath(QPainterPath& path) const
{
  path.addEllipse(m_boundingRect);
}

bool Shape2DEllipse::selectAt(const QPointF& p)const
{
  if (m_fill_color != QColor())
  {// filled ellipse
    return contains(p);
  }

  double a = m_boundingRect.width() / 2;
  if (a == 0.0) a = 1.0;
  double b = m_boundingRect.height() / 2;
  if (b == 0.0) b = 1.0;
  double xx = m_boundingRect.left() + a - double(p.x());
  double yy = m_boundingRect.top() + b - double(p.y());

  double f = fabs(xx*xx/(a*a) + yy*yy/(b*b) - 1);

  return f < 0.1;
}

bool Shape2DEllipse::contains(const QPointF& p)const
{
  QPointF pp = m_boundingRect.center() - p;
  double a = m_boundingRect.width() / 2;
  if (a == 0.0) a = 1.0;
  double b = m_boundingRect.height() / 2;
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
    qreal d = value - m_boundingRect.width() / 2;
    adjustBoundingRect(-d,0,d,0);
  }
  else if (prop == "radius2")
  {
    if (value <= 0.0) value = 1.0;
    qreal d = value - m_boundingRect.height() / 2;
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
  m_boundingRect = QRectF();
}

Shape2DRectangle::Shape2DRectangle(const QPointF& leftTop,const QPointF& bottomRight)
{
  m_boundingRect = QRectF(leftTop,bottomRight);
}

Shape2DRectangle::Shape2DRectangle(const QPointF& leftTop,const QSizeF& size)
{
  m_boundingRect = QRectF(leftTop,size);
}

bool Shape2DRectangle::selectAt(const QPointF& p)const
{
  if (m_fill_color != QColor())
  {// filled rectangle
    return contains(p);
  }

  QRectF outer(m_boundingRect);
  outer.adjust(-2,-2,2,2);
  QRectF inner(m_boundingRect);
  inner.adjust(2,2,-2,-2);
  return outer.contains(p) && !inner.contains(p);
}

void Shape2DRectangle::drawShape(QPainter& painter) const
{
  painter.drawRect(m_boundingRect);
  if (m_fill_color != QColor())
  {
    QPainterPath path;
    path.addRect(m_boundingRect);
    painter.fillPath(path,m_fill_color);
  }
}

void Shape2DRectangle::addToPath(QPainterPath& path) const
{
  path.addRect(m_boundingRect);
}

// --- Shape2DRing --- //

Shape2DRing::Shape2DRing(Shape2D* shape):
m_outer_shape(shape),
m_width(10.0),
m_stored_width(10.0)
{
  m_inner_shape = m_outer_shape->clone();
  m_inner_shape->getBoundingRect();
  m_inner_shape->adjustBoundingRect(m_width,m_width,-m_width,-m_width);
  resetBoundingRect();
  m_outer_shape->setFillColor(QColor());
  m_inner_shape->setFillColor(QColor());
}

Shape2DRing::Shape2DRing(const Shape2DRing& ring):
Shape2D(),
m_outer_shape(ring.m_outer_shape->clone()),
m_inner_shape(ring.m_inner_shape->clone()),
m_width(ring.m_width),
m_stored_width(ring.m_stored_width)
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
  if (m_stored_width <= 0) m_stored_width = 1.0;
  m_width = m_stored_width;
  qreal max_width = std::max(m_boundingRect.width() / 2, m_boundingRect.height() / 2) - 1.0;
  if (m_width > max_width) m_width = max_width;
  m_outer_shape->setBoundingRect(m_boundingRect);
  m_inner_shape->setBoundingRect(m_boundingRect);
  m_inner_shape->adjustBoundingRect(m_width,m_width,-m_width,-m_width);
}

void Shape2DRing::resetBoundingRect()
{
  m_boundingRect = m_outer_shape->getBoundingRect();
}

QPointF Shape2DRing::getShapeControlPoint(size_t i) const
{
  QRectF rect = m_inner_shape->getBoundingRect();
  switch(i)
  {
  case 0: return QPointF(m_boundingRect.center().x(), m_boundingRect.top());
  case 1: return QPointF(m_boundingRect.center().x(), m_boundingRect.bottom());
  case 2: return QPointF(rect.left(),rect.center().y());
  case 3: return QPointF(rect.right(),rect.center().y());
  }
  return QPointF();
}

void Shape2DRing::setShapeControlPoint(size_t i,const QPointF& pos)
{
  QPointF dp = pos - getShapeControlPoint(i);

  switch(i)
  {
  case 0: Shape2D::adjustBoundingRect(dp.y(),dp.y(),-dp.y(),-dp.y()); break;
  case 1: Shape2D::adjustBoundingRect(-dp.y(),-dp.y(),dp.y(),dp.y()); break;
  case 2: m_stored_width += dp.x(); 
          refit(); break;
  case 3: m_stored_width -= dp.x(); 
          refit(); break;
  }

}

QStringList Shape2DRing::getDoubleNames()const
{
  QStringList res;
  res << "width";
  return res;
}

double Shape2DRing::getDouble(const QString& prop) const
{
  if (prop == "width")
  {
    return m_stored_width;
  }
  return 0.0;
}

void Shape2DRing::setDouble(const QString& prop, double value)
{
  if (prop == "width")
  {
    m_stored_width = value;
    refit();
  }
}

QPointF Shape2DRing::getPoint(const QString& prop) const
{
  if (prop == "center" || prop == "centre")
  {
    return m_boundingRect.center();
  }
  return QPointF();
}

void Shape2DRing::setPoint(const QString& prop, const QPointF& value)
{
  if (prop == "center" || prop == "centre")
  {
    m_boundingRect.moveCenter(value);
  }
}

