#include "Shape2DCollection.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QStringList>

#include <iostream>
#include <stdexcept>

Shape2DCollection::Shape2DCollection():
Shape2D(),
m_creating(false),
m_editing(false),
m_moving(false),
m_x(0),
m_y(0),
m_currentShape(NULL),
m_leftButtonPressed(false),
m_overridingCursor(false)
{
}

Shape2DCollection::~Shape2DCollection()
{
  foreach(Shape2D* shape,m_shapes)
  {
    delete shape;
  }
}

void Shape2DCollection::draw(QPainter& painter) const
{
  painter.save();
  painter.setTransform(m_transform);
  foreach(const Shape2D* shape,m_shapes)
  {
    shape->draw(painter);
  }
  painter.restore();
}

void Shape2DCollection::addShape(Shape2D* shape)
{
  m_shapes.push_back(shape);
  m_boundingRect |= shape->getBoundingRect();
  emit shapeCreated();
}

void Shape2DCollection::setWindow(const QRectF& rect,const QRect& viewport) const
{
  m_transform.reset();
  if ( m_windowRect.isNull() )
  {
    m_windowRect = rect;
    m_h = viewport.height();
    m_wx = viewport.width() / rect.width();
    m_wy = m_h / rect.height();
  }
  else
  {
    double wx = viewport.width() / rect.width();
    double wy = viewport.height() / rect.height();
    double rx = m_windowRect.left() - rect.left();
    double ry = m_windowRect.top() - rect.top();
    qreal sx = wx / m_wx;
    qreal sy = wy / m_wy;
    qreal dx = rx * wx;
    qreal dy = viewport.height() - sy * m_h - ry * wy;

    m_transform.translate(dx,dy);
    m_transform.scale(sx,sy);
  }
}

void Shape2DCollection::refit()
{
}

void Shape2DCollection::adjustBoundingRect() 
{
  m_boundingRect = QRectF();
  foreach(const Shape2D* shape,m_shapes)
  {
    m_boundingRect |= shape->getBoundingRect();
  }
}

void Shape2DCollection::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton)
  {
    m_leftButtonPressed = true;
    if (m_creating && !m_shapeType.isEmpty())
    {
      deselectAll();
      addShape(m_shapeType,e->x(),e->y());
      if (!m_currentShape) return;
      m_currentShape->edit(true);
      m_currentCP = 2;
      m_editing = true;
    }
    else if (selectControlPointAt(e->x(),e->y()))
    {
      m_editing = true;
    }
    else if (selectAt(e->x(),e->y()))
    {
      m_x = e->x();
      m_y = e->y();
      m_moving = true;
    }
    else
    {
      deselectAll();
    }
  }
}

void Shape2DCollection::mouseMoveEvent(QMouseEvent* e)
{
  if (m_editing)
  {
    if (m_leftButtonPressed && m_currentShape && m_currentShape->isEditing() && m_currentCP < m_currentShape->getNControlPoints())
    {
      QPointF p = m_transform.inverted().map(QPointF(e->x(),e->y()));
      m_currentShape->setControlPoint(m_currentCP,p);
    }
  }
  else if (m_moving && m_leftButtonPressed && m_currentShape)
  {
    QPointF p1 = m_transform.inverted().map(QPointF( e->x(),e->y() ));
    QPointF p2 = m_transform.inverted().map(QPointF( m_x, m_y ));
    m_currentShape->moveBy(p1 - p2);
    m_x = e->x();
    m_y = e->y();
  }
  else if (selectControlPointAt(e->x(),e->y()) || isOverCurrentAt(e->x(),e->y()))
  {
    m_overridingCursor = true;
    QApplication::setOverrideCursor(Qt::SizeAllCursor);
  }
  else if (m_overridingCursor)
  {
    QApplication::restoreOverrideCursor();
  }
}

void Shape2DCollection::mouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton)
  {
    m_leftButtonPressed = false;
  }
  m_creating = false;
  m_editing = false;
  m_moving = false;
}

void Shape2DCollection::wheelEvent(QWheelEvent* e)
{
}

void Shape2DCollection::keyPressEvent(QKeyEvent* e)
{
  switch(e->key())
  {
  case Qt::Key_Delete:
  case Qt::Key_Backspace: removeCurrentShape(); break;
  }
}

void Shape2DCollection::addShape(const QString& type,int x,int y)
{
  m_currentShape = createShape(type,x,y);
  if ( ! m_currentShape ) return;
  m_currentShape->setColor(m_borderColor);
  m_currentShape->setFillColor(m_fillColor);
  m_creating = true;
  addShape( m_currentShape);
}

Shape2D* Shape2DCollection::createShape(const QString& type,int x,int y) const
{
  QPointF p = m_transform.inverted().map(QPointF(x,y));

  if (type.toLower() == "ellipse")
  {
     return new Shape2DEllipse(p,1.0);
  }
  else if (type.toLower() == "rectangle")
  {
     return new Shape2DRectangle(p,QSizeF(1,1));
  }

  QStringList complexType = type.split(' ',QString::SkipEmptyParts);

  if (complexType.size() < 2) return NULL;

  QString mainType = complexType[0];

  if (mainType.toLower() == "ring")
  {
    Shape2D* child = createShape(complexType[1],x,y);
    return new Shape2DRing(child);
  }

  throw std::invalid_argument("Shape " + type + " cannot be created");

}

void Shape2DCollection::startCreatingShape2D(const QString& type,const QColor& borderColor,const QColor& fillColor)
{
  m_creating = true;
  m_shapeType = type;
  m_borderColor = borderColor;
  m_fillColor = fillColor;
}

void Shape2DCollection::deselectAll()
{
  foreach(Shape2D* shape,m_shapes)
  {
    shape->edit(false);
  }
  m_currentShape = NULL;
}

bool Shape2DCollection::selectAt(int x,int y)
{
  QPointF p = m_transform.inverted().map(QPointF(x,y));
  foreach(Shape2D* shape,m_shapes)
  {
    bool picked = shape->selectAt(p);
    if (picked) 
    {
      if (m_currentShape)
      {
        m_currentShape->edit(false);
      }
      m_currentShape = shape;
      m_currentShape->edit(true);
      return true;
    }
  }
  return false;
}

bool Shape2DCollection::isOverCurrentAt(int x,int y)
{
  if (!m_currentShape) return false;
  QPointF p = m_transform.inverted().map(QPointF(x,y));
  return m_currentShape->selectAt(p);
}

bool Shape2DCollection::selectControlPointAt(int x,int y)
{
  QPointF p = m_transform.inverted().map(QPointF(x,y));
  if (!m_currentShape) return false;
  for(size_t i = 0; i < m_currentShape->getNControlPoints(); ++i)
  {
    QPointF cp = m_currentShape->getControlPoint(i) - p;
    if (cp.manhattanLength() <= sizeCP + 2)
    {
      m_currentCP = i;
      return true;
    }
  }
  return false;
}

void Shape2DCollection::removeCurrentShape()
{
  if (m_currentShape)
  {
    m_shapes.removeOne(m_currentShape);
  }
}
