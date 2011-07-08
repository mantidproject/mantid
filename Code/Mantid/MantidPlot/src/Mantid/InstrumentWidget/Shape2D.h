#ifndef MANTIDPLOT_SHAPE2D_H_
#define MANTIDPLOT_SHAPE2D_H_

#include <QColor>
#include <QPointF>
#include <QRectF>

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

  // --- Public pure virtual methods --- //

  virtual Shape2D* clone()const = 0;
  // modify path so painter.drawPath(path) could be used to draw the shape. needed for filling in complex shapes
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
  // will the shape be selected if clicked at a point
  virtual bool selectAt(const QPointF& )const{return false;}
  // is a point inside the shape (closed line)
  virtual bool contains(const QPointF& )const{return false;}
  // is a point "masked" by the shape. Only filled regians of a shape mask a point
  virtual bool isMasked(const QPointF& )const;

  // --- Properties. for gui interaction --- //

  // double properties
  virtual QStringList getDoubleNames()const{return QStringList();}
  virtual double getDouble(const QString& prop) const{(void)prop; return 0.0;}
  virtual void setDouble(const QString& prop, double value){(void)prop; (void)value;}

  // QPointF properties
  virtual QStringList getPointNames()const{return QStringList();}
  virtual QPointF getPoint(const QString& prop) const{(void)prop; return QPointF();}
  virtual void setPoint(const QString& prop, const QPointF& value){(void)prop; (void)value;}

protected:
  // --- Protected pure virtual methods --- //

  virtual void drawShape(QPainter& painter) const = 0;

  // return number of control points specific to this shape
  virtual size_t getShapeNControlPoints() const{return 0;}
  // returns position of a shape specific control point, 0 < i < getShapeNControlPoints()
  virtual QPointF getShapeControlPoint(size_t ) const{return QPointF();}
  // sets position of a shape specific control point, 0 < i < getShapeNControlPoints()
  virtual void setShapeControlPoint(size_t ,const QPointF& ){}
  // make sure the bounding box is correct
  virtual void adjustBoundingRect() {}

  // make sure that width and heigth are positive
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
  // double properties
  virtual QStringList getDoubleNames()const;
  virtual double getDouble(const QString& prop) const;
  virtual void setDouble(const QString& prop, double value);
  // QPointF properties
  virtual QStringList getPointNames()const{return QStringList("center");}
  virtual QPointF getPoint(const QString& prop) const;
  virtual void setPoint(const QString& prop, const QPointF& value);

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
  // double properties
  virtual QStringList getDoubleNames()const;
  virtual double getDouble(const QString& prop) const;
  virtual void setDouble(const QString& prop, double value);
  // QPointF properties
  virtual QStringList getPointNames()const{return QStringList("center");}
  virtual QPointF getPoint(const QString& prop) const;
  virtual void setPoint(const QString& prop, const QPointF& value);
protected:
  virtual void drawShape(QPainter& painter) const;
  virtual void addToPath(QPainterPath& ) const {}
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
