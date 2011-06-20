#ifndef MANTIDPLOT_SHAPE2DCOLLECTION_H_
#define MANTIDPLOT_SHAPE2DCOLLECTION_H_

#include "Shape2D.h"

#include <QList>
#include <QTransform>

class QPainter;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

class Shape2DCollection: public QObject, public Shape2D
{
  Q_OBJECT
public:
  Shape2DCollection();
  ~Shape2DCollection();
  Shape2D* clone()const{return NULL;}
  void setWindow(const QRectF& rect,const QRect& viewport) const;
  virtual void draw(QPainter& painter) const;
  virtual void addShape(Shape2D*);
  
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  void keyPressEvent(QKeyEvent*);

  void addShape(const QString& type,int x,int y);
  void startCreatingShape2D(const QString& type,const QColor& borderColor = Qt::red,const QColor& fillColor = QColor());
  void deselectAll();
  bool selectAt(int x,int y);
  void removeCurrentShape();

signals:

  void shapeCreated();

protected:
  virtual void drawShape(QPainter& painter) const{} // never called
  virtual void addToPath(QPainterPath& path) const{}
  virtual void refit();
  virtual void adjustBoundingRect();

  Shape2D* createShape(const QString& type,int x,int y)const;
  bool selectControlPointAt(int x,int y);
  bool isOverCurrentAt(int x,int y);

  QList<Shape2D*> m_shapes;
  mutable QRectF m_windowRect;
  mutable double m_wx,m_wy;
  mutable int m_h;
  mutable QTransform m_transform;

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
