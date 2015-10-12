#ifndef PROJECTION3D_H_
#define PROJECTION3D_H_

#include "ProjectionSurface.h"
#include "Viewport.h"

#include "MantidGeometry/IComponent.h"

#include <QGLWidget>
#include <QString>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

/**
  This is an implementation of ProjectionSurface for viewing the instrument in 3D.

*/
class Projection3D : public ProjectionSurface
{
    Q_OBJECT
  enum AxisDirection{ XPOSITIVE,YPOSITIVE,ZPOSITIVE,XNEGATIVE,YNEGATIVE,ZNEGATIVE};

public:
  Projection3D(const InstrumentActor* rootActor,int winWidth,int winHeight);
  ~Projection3D();
  virtual RectF getSurfaceBounds()const;

  void setViewDirection(const QString& vd);
  void set3DAxesState(bool on);
  void setWireframe(bool on);

  virtual void componentSelected(Mantid::Geometry::ComponentID = NULL);
  virtual void getSelectedDetectors(QList<int>& dets);
  virtual void getMaskedDetectors(QList<int>& dets)const;
  virtual void resize(int, int);
  virtual QString getInfoText()const;

signals:
  void finishedMove();

protected slots:
  void initTranslation(int x, int y);
  void translate(int x, int y);
  void initZoom(int x, int y);
  void zoom(int x, int y);
  void wheelZoom(int x, int y, int d);
  void initRotation(int x, int y);
  void rotate(int x, int y);
  void finishMove();

protected:
  virtual void init() {}
  virtual void drawSurface(MantidGLWidget* widget,bool picking = false)const;
  virtual void changeColorMap();

  void drawAxes(double axis_length = 100.0)const;
  void setLightingModel(bool picking)const;

  bool m_drawAxes;
  bool m_wireframe;

  Viewport m_viewport;

};

#endif /* PROJECTION3D_H_ */

