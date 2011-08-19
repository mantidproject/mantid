#ifndef MANTIDPLOT_SHAPE2DCOLLECTION_H_
#define MANTIDPLOT_SHAPE2DCOLLECTION_H_

#include "Shape2D.h"

#include <QList>
#include <QTransform>

class QPainter;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

/**
 * Class Shape2DCollection is a collection of 2D shapes. 
 */
class Shape2DCollection: public QObject, public Shape2D
{
  Q_OBJECT
public:
  Shape2DCollection();
  ~Shape2DCollection();
  Shape2D* clone()const{return NULL;}
  void setWindow(const QRectF& rect,const QRect& viewport) const;
  virtual void draw(QPainter& painter) const;
  virtual void addShape(Shape2D*,bool slct = false);
  
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  void keyPressEvent(QKeyEvent*);

  void addShape(const QString& type,int x,int y);
  void startCreatingShape2D(const QString& type,const QColor& borderColor = Qt::red,const QColor& fillColor = QColor());
  void deselectAll();
  bool selectAtXY(int x,int y);
  void removeCurrentShape();
  void clear();
  bool isEmpty()const{return m_shapes.isEmpty();}
  size_t size()const {return static_cast<size_t>(m_shapes.size());}
  void select(size_t i);

  QRectF getCurrentBoundingRect()const;
  void setCurrentBoundingRect(const QRectF& rect);
  // double properties
  QStringList getCurrentDoubleNames()const;
  double getCurrentDouble(const QString& prop) const;
  void setCurrentDouble(const QString& prop, double value);
  // QPointF properties
  QStringList getCurrentPointNames()const;
  QPointF getCurrentPoint(const QString& prop) const;
  void setCurrentPoint(const QString& prop, const QPointF& value);

  using Shape2D::isMasked; // Unhide base class method (avoids Intel compiler warning)
  // is a point in real space masked by any of the shapes
  bool isMasked(double x,double y)const;
  // collect all screen pixels that are masked by the shapes
  void getMaskedPixels(QList<QPoint>& pixels)const;

  void setCurrentBoundingRectReal(const QRectF& rect);

signals:

  void shapeCreated();
  void shapeSelected();
  void shapesDeselected();
  void shapeChanged();

protected:
  virtual void drawShape(QPainter& ) const{} // never called
  virtual void addToPath(QPainterPath& ) const{}
  virtual void refit();
  virtual void resetBoundingRect();

  Shape2D* createShape(const QString& type,int x,int y)const;
  bool selectControlPointAt(int x,int y);
  bool isOverCurrentAt(int x,int y);
  void select(Shape2D* shape);

  QList<Shape2D*> m_shapes;
  mutable QRectF m_windowRect; // original surface window in "real" cooerdinates
  mutable double m_wx,m_wy;
  mutable int m_h; // original screen viewport height
  mutable QRect m_viewport;  // current screen viewport
  mutable QTransform m_transform; // current transform

  bool m_creating;
  bool m_editing;
  bool m_moving;
  int m_x,m_y;
  QString m_shapeType;
  QColor m_borderColor, m_fillColor;
  Shape2D*  m_currentShape;
  size_t m_currentCP;
  bool m_leftButtonPressed;
  bool m_overridingCursor;
};

#endif /*MANTIDPLOT_SHAPE2DCOLLECTION_H_*/
