#include "Shape2DCollection.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QStringList>

#include <iostream>
#include <stdexcept>
#include <cmath>

Shape2DCollection::Shape2DCollection():
Shape2D(),
m_wx(0),
m_wy(0),
m_h(0),
m_currentShape(NULL),
m_currentCP(0),
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

/**
 * Draw the collection on screen.
 */
void Shape2DCollection::draw(QPainter& painter) const
{
  if (m_shapes.isEmpty()) return;

  // separate scalable and nonscalable shapes
  QList<Shape2D*> scalable;
  QList<Shape2D*> nonscalable;
  foreach(Shape2D* shape,m_shapes)
  {
    if ( !shape->isVisible() ) continue;
    if (shape->isScalable())
    {
      scalable << shape;
    }
    else
    {
      nonscalable << shape;
    }
  }

  // first draw the scalable ones
  painter.save();
  painter.setTransform(m_transform);
  foreach(const Shape2D* shape,scalable)
  {
    painter.save();
    shape->draw(painter);
    painter.restore();
  }
  painter.restore();

  // now the nonscalable
  foreach(const Shape2D* shape,nonscalable)
  {
    QPointF p0 = shape->origin();
    QPointF p1 = m_transform.map(p0);
    QPointF dp = p1 - p0;
    painter.save();
    painter.translate(dp);
    shape->draw(painter);
    painter.restore();
  }
  //std::cerr << m_transform.m11() << ' ' << m_transform.m22() << ' ' << m_transform.m33() << std::endl;
}

/**
 * Add a new shape to collection.
 * @param shape :: A pointer to the new shape.
 * @param slct :: A bool flag to select the shape after it's added.
 */
void Shape2DCollection::addShape(Shape2D* shape,bool slct)
{
  m_shapes.push_back(shape);
  m_boundingRect.unite( shape->getBoundingRect() );
  if (slct)
  {
    addToSelection(shape);
  }
  emit shapeCreated();
}

/**
 * Remove a shape from collection
 * @param shape :: Pointer to the shape to remove.
 * @param sendSignal :: Flag to send shapesRemoved() signal.
 */
void Shape2DCollection::removeShape(Shape2D* shape, bool sendSignal)
{
  if (shape && m_shapes.contains(shape))
  {
    m_shapes.removeOne(shape);
    m_selectedShapes.removeOne(shape);
    delete shape;
  }
  if ( sendSignal )
  {
      if (m_shapes.isEmpty())
      {
          emit cleared();
      }
      else
      {
          emit shapesRemoved();
      }
  }
}

/**
 * Remove a list of shapes.
 * @param shapeList :: A list of pointers to the shapes to be removed
 */
void Shape2DCollection::removeShapes(const QList<Shape2D*>& shapeList)
{
  foreach(Shape2D* shape, shapeList)
  {
    if ( shape == m_currentShape )
    {
      m_currentShape = NULL;
    }
    removeShape( shape, false );
  }
  if (m_shapes.isEmpty())
  {
      emit cleared();
  }
  else
  {
      emit shapesRemoved();
  }
}

/**
 */
void Shape2DCollection::setWindow(const RectF &surface,const QRect& viewport) const
{
  m_viewport = viewport;
  m_surfaceRect = surface;
  m_surfaceRect.findTransform( m_transform, viewport );
}

void Shape2DCollection::refit()
{
}

void Shape2DCollection::resetBoundingRect() 
{
  m_boundingRect = RectF();
  foreach(const Shape2D* shape,m_shapes)
  {
    m_boundingRect.unite( shape->getBoundingRect() );
  }
}

void Shape2DCollection::keyPressEvent(QKeyEvent* e)
{
  switch(e->key())
  {
  case Qt::Key_Delete:
  case Qt::Key_Backspace: removeSelectedShapes(); break;
  }
}

void Shape2DCollection::addShape(const QString& type,int x,int y, const QColor &borderColor, const QColor &fillColor)
{
  deselectAll();
  Shape2D *shape = createShape(type,x,y);
  if ( ! shape )
  {
    emit shapeSelected();
    return;
  }
  shape->setColor(borderColor);
  shape->setFillColor(fillColor);
  addShape( shape );
  addToSelection( shape );
  m_currentCP = 2;
  emit shapeSelected();
}

Shape2D* Shape2DCollection::createShape(const QString& type,int x,int y) const
{
  QPointF p = m_transform.inverted().map(QPointF(x,y));

  if (type.toLower() == "ellipse")
  {
     return new Shape2DEllipse(p,0.0);
  }
  else if (type.toLower() == "rectangle")
  {
     return new Shape2DRectangle(p,QSizeF(0,0));
  }

  QStringList complexType = type.split(' ',QString::SkipEmptyParts);

  if (complexType.size() < 2) return NULL;

  QString mainType = complexType[0];

  if (mainType.toLower() == "ring")
  {
    double xWidth = 10.0 / fabs(m_transform.m11());
    double yWidth = 10.0 / fabs(m_transform.m22());
    Shape2D* child = createShape(complexType[1],x,y);
    return new Shape2DRing(child, xWidth, yWidth);
  }

  throw std::invalid_argument("Shape " + type.toStdString() + " cannot be created");

}

/**
  * Deselect all selected shapes.
  */
void Shape2DCollection::deselectAll()
{
  foreach(Shape2D* shape,m_shapes)
  {
    shape->edit(false);
    shape->setSelected(false);
  }
  m_selectedShapes.clear();
  m_currentShape = NULL;
  if (m_overridingCursor)
  {
      m_overridingCursor = false;
      QApplication::restoreOverrideCursor();
  }
  emit shapesDeselected();
}

/**
  * Resize the current shape by moving the right-bottom control point to a loaction on the screen.
  */
void Shape2DCollection::moveRightBottomTo(int x, int y)
{
    if ( m_currentShape && m_currentShape->isEditing() )
    {
      QPointF p = m_transform.inverted().map(QPointF(x, y));
      m_currentShape->setControlPoint( 3, p );
      emit shapeChanged();
    }
}

/**
  * Select a shape or a control point at a location on the screen.
  * The control points of the currently selected shape are checked first.
  * If (x,y) doesn't point to anything deselect all currently selected shapes.
  */
void Shape2DCollection::selectShapeOrControlPointAt(int x, int y)
{
    if ( isOverSelectionAt( x, y ) ) return;
    bool ret = selectControlPointAt( x, y ) || selectAtXY( x, y );
    if ( !ret )
    {
        deselectAll();
    }
}

/**
 * Add a shape under the cursor to the selection.
 * @param x :: Mouse x coordinate.
 * @param y :: Mouse y coordinate.
 */
void Shape2DCollection::addToSelectionShapeAt(int x, int y)
{
    // if there is a selected shape under the cursor deselect it
    if ( isOverSelectionAt( x, y ) )
    {
        deselectAtXY(x,y);
        return;
    }
    // try selecting a shape without editing it
    if ( !selectAtXY( x, y, false ) )
    {
        deselectAll();
    }
}

/**
  * Move the current control point or entire shape by (dx,dy).
  * @param dx :: Shift in the x direction in screen pixels.
  * @param dy :: Shift in the y direction in screen pixels.
  */
void Shape2DCollection::moveShapeOrControlPointBy(int dx, int dy)
{
    if ( ! hasSelection() ) return;
    if ( m_currentShape && m_currentCP < m_currentShape->getNControlPoints() )
    {
        QPointF p = m_currentShape->getControlPoint( m_currentCP );
        QPointF screenP = m_transform.map( p ) + QPointF( dx, dy );
        p = m_transform.inverted().map( screenP );
        m_currentShape->setControlPoint( m_currentCP, p );
    }
    else
    {
        QPointF p0 = m_selectedShapes[0]->getControlPoint( 0 );
        QPointF screenP0 = m_transform.map( p0 );
        QPointF screenP1 = screenP0 + QPointF( dx, dy );
        QPointF p1 = m_transform.inverted().map( screenP1 );
        QPointF dp = p1 - p0;
        foreach(Shape2D* shape,m_selectedShapes)
        {
            shape->moveBy( dp );
        }
    }
    if ( !m_overridingCursor )
    {
      m_overridingCursor = true;
      QApplication::setOverrideCursor(Qt::SizeAllCursor);
    }
    emit shapeChanged();
}

/**
  * If mouse pointer at (x,y) touches the current shape or its control points
  * override the cursor image.
  */
void Shape2DCollection::touchShapeOrControlPointAt(int x, int y)
{
    if (selectControlPointAt( x, y ) || isOverSelectionAt( x, y ))
    {
        if ( !m_overridingCursor )
        {
          m_overridingCursor = true;
          QApplication::setOverrideCursor(Qt::SizeAllCursor);
        }
    }
    else if (m_overridingCursor)
    {
        deselectControlPoint();
        m_overridingCursor = false;
        QApplication::restoreOverrideCursor();
    }
}

/**
 * Select a shape which contains a point (x,y) of the screen.
 */
bool Shape2DCollection::selectAtXY(int x, int y, bool edit)
{
    if ( edit )
    {
        // if shape has to be edited (resized) it must be the only selection
        deselectAll();
    }
  QPointF p = m_transform.inverted().map(QPointF(x,y));
  foreach(Shape2D* shape,m_shapes)
  {
    bool picked = shape->selectAt(p);
    if (picked) 
    {
      addToSelection(shape);
      return true;
    }
  }
  return false;
}

/**
 * Deselect a shape under the cursor.
 * @param x :: Mouse x coordinate.
 * @param y :: Mouse y coordinate.
 */
void Shape2DCollection::deselectAtXY(int x, int y)
{
    QPointF p = m_transform.inverted().map(QPointF(x,y));
    foreach(Shape2D* shape,m_shapes)
    {
      bool picked = shape->selectAt(p);
      if (picked)
      {
        removeFromSelection(shape);
        return;
      }
    }
}

/**
 * Select all shapes included in a rectangle.
 * @param rect :: Rectangle in current screen coordinates containing selected shapes.
 * @return :: True if any of the shapes is selected.
 */
bool Shape2DCollection::selectIn(const QRect& rect)
{
  RectF untransformedRect = QRectF(rect);
  RectF r( m_transform.inverted().mapRect( QRectF(rect) ) );
  bool selected = false;
  deselectAll();
  foreach(Shape2D* shape,m_shapes)
  {
    bool sel = false;
    if ( shape->isScalable() )
    {
        sel = r.contains( shape->getBoundingRect() );
    }
    else
    {
        QPointF dp = m_transform.map( shape->origin() ) - shape->origin();
        RectF br = shape->getBoundingRect();
        br.translate( dp );
        sel = untransformedRect.contains( br );
    }
    if ( sel )
    {
      addToSelection( shape );
      selected = true;
    }
  }
  return selected;
}

/**
 * Select a shape with index i.
 */
void Shape2DCollection::addToSelection(int i)
{
  if (i < static_cast<int>(size()))
  {
    addToSelection(m_shapes[i]);
  }
}

/**
 * Check if any of the shapes is selected.
 * @return :: True if there is a selection.
 */
bool Shape2DCollection::hasSelection() const
{
    foreach(Shape2D* shape,m_shapes)
    {
      if ( shape->isSelected() )
      {
        return true;
      }
    }
    return false;
}

/**
 * Add a shape to selection. If it's the only selection start editing it.
 * @param shape :: Pointer to a shape which is to become select.
 */
void Shape2DCollection::addToSelection(Shape2D* shape)
{
    if ( !m_selectedShapes.contains(shape) )
    {
        if ( m_selectedShapes.size() == 1 ) finishEdit();
        shape->setSelected( true );
        m_selectedShapes.append( shape );
        if ( m_selectedShapes.size() == 1 ) edit( shape );
    }
}

/**
 * Remove a shape from selection.
 * @param shape :: Pointer to a shape to deselect.
 */
void Shape2DCollection::removeFromSelection(Shape2D *shape)
{
    foreach(Shape2D* s, m_selectedShapes)
    {
        if ( s == shape )
        {
            shape->setSelected(false);
            shape->edit(false);
            m_selectedShapes.removeOne(shape);
            return;
        }
    }
}

/**
 * Start editing a shape.
 * @param shape :: A shape to edit.
 */
void Shape2DCollection::edit(Shape2D *shape)
{
    if (m_currentShape)
    {
      m_currentShape->edit(false);
    }
    m_currentShape = shape;
    m_currentShape->edit(true);
    m_currentCP = m_currentShape->getNControlPoints(); // no current cp until it is selected explicitly
    emit shapeSelected();
}

/**
 * Finish editing the current shape. The shape remains selected.
 */
void Shape2DCollection::finishEdit()
{
    if ( m_currentShape )
    {
        m_currentShape->edit( false );
        m_currentShape = NULL;
    }
}

/**
 * Checks if the screen point (x,y) is inside the current shape.
 */
bool Shape2DCollection::isOverCurrentAt(int x,int y)
{
  if (!m_currentShape) return false;
  QPointF p = m_transform.inverted().map(QPointF(x,y));
  return m_currentShape->selectAt(p);
}

/**
 * Checks if the screen point (x,y) is inside any of the selected shapes.
 */
bool Shape2DCollection::isOverSelectionAt(int x, int y)
{
    if ( m_selectedShapes.isEmpty() ) return false;
    QPointF p = m_transform.inverted().map(QPointF(x,y));
    foreach(Shape2D *shape, m_selectedShapes)
    {
        if ( shape->selectAt(p) ) return true;
    }
    return false;
}

bool Shape2DCollection::selectControlPointAt(int x,int y)
{
  //QPointF p = m_transform.inverted().map(QPointF(x,y));
  QPointF p = QPointF(x,y);
  if (!m_currentShape) return false;
  for(size_t i = 0; i < m_currentShape->getNControlPoints(); ++i)
  {
    //QPointF cp = m_currentShape->getControlPoint(i) - p;
    QPointF cp = m_transform.map(m_currentShape->getControlPoint(i)) - p;
    if (fabs(cp.x()) + fabs(cp.y()) <= sizeCP + 2)
    //if (cp.manhattanLength() <= sizeCP + 2)
    {
      m_currentCP = i;
      return true;
    }
  }
  // deselect control points
  m_currentCP = m_currentShape->getNControlPoints();
  return false;
}

void Shape2DCollection::deselectControlPoint()
{
    if ( m_currentShape )
    {
        m_currentCP = m_currentShape->getNControlPoints();
    }
}

void Shape2DCollection::removeCurrentShape()
{
  if (m_currentShape)
  {
    this->removeShape(m_currentShape);
    m_currentShape = NULL;
    emit shapesDeselected();
  }
}

/**
 * Removes the selected shapes from this collection.
 */
void Shape2DCollection::removeSelectedShapes()
{
  auto shapeList = getSelectedShapes();
  if ( !shapeList.isEmpty() )
  {
    removeShapes( shapeList );
    emit shapesDeselected();
  }
}

/**
  * Restore the cursor image to default.
  */
void Shape2DCollection::restoreOverrideCursor()
{
    if (m_overridingCursor)
    {
        m_overridingCursor = false;
        QApplication::restoreOverrideCursor();
    }
}

void Shape2DCollection::clear()
{
  foreach(Shape2D* shape,m_shapes)
  {
    delete shape;
  }
  m_shapes.clear();
  m_selectedShapes.clear();
  m_currentShape = NULL;
  emit shapesDeselected();
}

QStringList Shape2DCollection::getCurrentDoubleNames()const
{
  if (m_currentShape)
  {
    return m_currentShape->getDoubleNames();
  }
  return QStringList();
}

double Shape2DCollection::getCurrentDouble(const QString& prop) const
{
  if (m_currentShape)
  {
    return m_currentShape->getDouble(prop);
  }
  return 0.0;
}

void Shape2DCollection::setCurrentDouble(const QString& prop, double value)
{
  if (m_currentShape)
  {
    return m_currentShape->setDouble(prop,value);
  }
}

QStringList Shape2DCollection::getCurrentPointNames()const
{
  if (m_currentShape)
  {
    return m_currentShape->getPointNames();
  }
  return QStringList();
}

QPointF Shape2DCollection::getCurrentPoint(const QString& prop) const
{
  if (m_currentShape)
  {
    return m_currentShape->getPoint(prop);
  }
  return QPointF();
}

void Shape2DCollection::setCurrentPoint(const QString& prop, const QPointF& value)
{
  if (m_currentShape)
  {
    return m_currentShape->setPoint(prop,value);
  }
}

RectF Shape2DCollection::getCurrentBoundingRect()const
{
  if (m_currentShape)
  {
    return m_currentShape->getBoundingRect();
  }
  return RectF();
}

void Shape2DCollection::setCurrentBoundingRect(const RectF& rect)
{
  if (m_currentShape)
  {
    m_currentShape->setBoundingRect(rect);
  }
}

bool Shape2DCollection::isMasked(double x,double y)const
{
  QPointF p(x,y);
  foreach(Shape2D* shape,m_shapes)
  {
    if (shape->isMasked(p))
    {
      return true;
    }
  }
  return false;
}

void Shape2DCollection::getMaskedPixels(QList<QPoint>& pixels)const
{
  pixels.clear();
  QTransform inv = m_transform.inverted();
  for(int i = m_viewport.left(); i <= m_viewport.right(); ++i)
  {
    for(int j = m_viewport.top(); j <= m_viewport.bottom(); ++j)
    {
      QPoint p = QPoint(i,j);
      QPointF p0 = inv.map(QPointF(p));
      foreach(Shape2D* shape,m_shapes)
      {
        if (shape->isMasked(p0))
        {
          pixels.append(p);
        }
      }
    }
  }
}

/**
 * Set the bounding rect of the current shape in real coordinates.
 */
void Shape2DCollection::setCurrentBoundingRectReal(const QRectF& rect)
{
  if (!m_currentShape) return;
  m_currentShape->setBoundingRect(rect);
}

/**
  * Change border color.
  */
void Shape2DCollection::changeBorderColor(const QColor &color)
{
    foreach(Shape2D* shape,m_shapes)
    {
        shape->setColor( color );
    }
}

