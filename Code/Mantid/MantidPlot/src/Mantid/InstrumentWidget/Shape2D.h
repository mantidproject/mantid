#ifndef MANTIDPLOT_SHAPE2D_H_
#define MANTIDPLOT_SHAPE2D_H_

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <QList>
#include <QTransform>

class QPainter;
class QPainterPath;
class QMouseEvent;
class QWheelEvent;

/**
  * Base class for an editable 2D shape, which can be drawn on ProjectionSurface.
  */
class Shape2D
{
public:
  Shape2D();
  virtual ~Shape2D(){}

  virtual Shape2D* clone()const = 0;
  virtual void addToPath(QPainterPath& path) const = 0;
  // make sure the shape is withing the bounding box
  virtual void refit() = 0;

  virtual void draw(QPainter& painter) const;
  virtual void moveBy(const QPointF& pos);
  virtual size_t getNControlPoints() const;
  virtual QPointF getControlPoint(size_t i) const;
  virtual void setControlPoint(size_t i,const QPointF& pos);
  virtual QRectF getBoundingRect() const {return m_boundingRect;}
  // move the left, top, right and bottom sides of the bounding rect
  // by dx1, dy1, dx2, and dy2 correspondingly
  virtual void adjustBoundingRect(qreal dx1,qreal dy1,qreal dx2,qreal dy2);
  virtual void setBoundingRect(const QRectF& rect);

  void setColor(const QColor& color){m_color = color;}
  void setFillColor(const QColor& color){m_fill_color = color;}
  void edit(bool on){m_editing = on;}
  bool isEditing()const{return m_editing;}
  virtual bool selectAt(const QPointF& p)const{return false;}
  virtual bool contains(const QPointF& p)const{return false;}

protected:
  virtual void drawShape(QPainter& painter) const = 0;

  // return number of control points specific to this shape
  virtual size_t getShapeNControlPoints() const{return 0;}
  // returns position of a shape specific control point, 0 < i < getShapeNControlPoints()
  virtual QPointF getShapeControlPoint(size_t i) const{return QPointF();}
  // sets position of a shape specific control point, 0 < i < getShapeNControlPoints()
  virtual void setShapeControlPoint(size_t i,const QPointF& pos){}
  // make sure the bounding box is correct
  virtual void adjustBoundingRect() {}

  void correctBoundingRect();

  static const size_t NCommonCP;
  static const qreal sizeCP;
  QRectF m_boundingRect;
  QColor m_color;
  QColor m_fill_color;
  bool m_editing;
};

class Shape2DEllipse: public Shape2D
{
public:
  Shape2DEllipse(const QPointF& center,double radius1,double radius2 = 0);
  virtual Shape2D* clone()const{return new Shape2DEllipse(*this);}
  virtual bool selectAt(const QPointF& p)const;
  virtual bool contains(const QPointF& p)const;
  virtual void addToPath(QPainterPath& path) const;
protected:
  virtual void drawShape(QPainter& painter) const;
  virtual void refit(){}
};

class Shape2DRectangle: public Shape2D
{
public:
  Shape2DRectangle(const QPointF& leftTop,const QPointF& bottomRight);
  Shape2DRectangle(const QPointF& leftTop,const QSizeF& size);
  virtual Shape2D* clone()const{return new Shape2DRectangle(*this);}
  virtual bool selectAt(const QPointF& p)const;
  virtual bool contains(const QPointF& p)const{return m_boundingRect.contains(p);}
  virtual void addToPath(QPainterPath& path) const;
protected:
  virtual void drawShape(QPainter& painter) const;
  virtual void refit(){}
};

class Shape2DRing: public Shape2D
{
public:
  Shape2DRing(Shape2D* shape);
  Shape2DRing(const Shape2DRing& ring);
  virtual Shape2D* clone()const{return new Shape2DRing(*this);}
  virtual bool selectAt(const QPointF& p)const;
  virtual bool contains(const QPointF& p)const;
protected:
  virtual void drawShape(QPainter& painter) const;
  virtual void addToPath(QPainterPath& path) const {}
  virtual void refit();
  virtual void adjustBoundingRect();
  virtual size_t getShapeNControlPoints() const{return 4;}
  virtual QPointF getShapeControlPoint(size_t i) const;
  virtual void setShapeControlPoint(size_t i,const QPointF& pos);
  Shape2D* m_outer_shape;
  Shape2D* m_inner_shape;
  qreal m_width;
  qreal m_stored_width;
};


#endif /*MANTIDPLOT_SHAPE2D_H_*/
