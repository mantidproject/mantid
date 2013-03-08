#ifndef PROJECTION3D_H_
#define PROJECTION3D_H_

#include "ProjectionSurface.h"
//#include "GLViewport.h"
//#include "GLTrackball.h"
#include "Viewport.h"

#include "MantidGeometry/IComponent.h"

#include <QGLWidget>
#include <QString>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

/**
  TODO: Add description

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
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

protected slots:
  void initTranslation(int x, int y);
  void translate(int x, int y);
  void initZoom(int x, int y);
  void zoom(int x, int y);
  void wheelZoom(int x, int y, int d);
  void initRotation(int x, int y);
  void rotate(int x, int y);

protected:
  virtual void init();
  virtual void drawSurface(MantidGLWidget* widget,bool picking = false)const;
  virtual void changeColorMap();

  void drawAxes(double axis_length = 100.0)const;
  void setLightingModel(bool picking)const;

  //const InstrumentActor& m_instrActor;
  //GLTrackball* m_trackball;       ///< Trackball for user interaction
  //GLViewport* m_viewport;         ///< Opengl View port [World -> Window]
  bool m_drawAxes;
  bool m_wireframe;

  Viewport m_vport;

};

#endif /* PROJECTION3D_H_ */

