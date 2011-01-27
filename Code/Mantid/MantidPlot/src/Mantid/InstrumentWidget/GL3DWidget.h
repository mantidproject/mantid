#ifndef GL3DWIDGET_H_
#define GL3DWIDGET_H_

#include "MantidGeometry/IComponent.h"

#include <QGLWidget>
#include <QString>
#include "GLViewport.h"
#include "GLTrackball.h"
#include "GLActorCollection.h"
#include "GLGroupPickBox.h"
#include "boost/shared_ptr.hpp"

class UnwrappedSurface;

/*!
  \class  GL3DWidget
  \brief  OpenGL Qt Widget which renders Mantid Geometry ObjComponents
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  This Class takes input as ObjComponents and renders them with in the Qt widget. also
  provides the user interaction with the rendered object.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/

class GL3DWidget : public QGLWidget
{
  Q_OBJECT
public:
  enum InteractionMode {MoveMode = 0, PickMode = 1};
  enum AxisDirection{ XPOSITIVE,YPOSITIVE,ZPOSITIVE,XNEGATIVE,YNEGATIVE,ZNEGATIVE};
  enum PolygonMode{ SOLID, WIREFRAME };
  enum RenderMode{ FULL3D = 0, CYLINDRICAL_Y, CYLINDRICAL_Z, CYLINDRICAL_X, SPHERICAL_Y, SPHERICAL_Z, SPHERICAL_X, RENDERMODE_SIZE };
  GL3DWidget(QWidget* parent=0); ///< Constructor
  virtual ~GL3DWidget();         ///< Destructor
  void setActorCollection(boost::shared_ptr<GLActorCollection>);
  void setInteractionModePick();
  void setInteractionModeNormal();
  InteractionMode getInteractionMode()const{return iInteractionMode;}
  GLActor* getPickedActor();
  void setViewDirection(AxisDirection);
  void setBackgroundColor(QColor);
  QColor currentBackgroundColor() const;
  void saveToFile(const QString & filename);
  void getViewport(int& w,int& h){_viewport->getViewport(&w,&h);}
  RenderMode getRenderMode()const{return m_renderMode;}

signals:
  void actorsPicked(const std::set<QRgb>& );
  void actorHighlighted( QRgb );
  void actorHighlighted( int );
  void increaseSelection(QRgb);                   ///< emitted while the user is draging with the left mouse button clicked over detectors

public slots:
  void enableLighting(bool);
  void setWireframe(bool);
  void resetUnwrappedViews();
  void componentSelected(Mantid::Geometry::ComponentID);
  void hidePickBox(){mPickBox->hide();}

protected:
  void initializeGL();
  void resetWidget();
  void MakeObject();
  void paintEvent(QPaintEvent *event);
  void resizeGL(int,int);
  void mousePressEvent(QMouseEvent*);
  void contextMenuEvent(QContextMenuEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent *);
  void keyPressEvent(QKeyEvent *);
  void keyReleaseEvent(QKeyEvent *);
  void defaultProjection();
  void redrawUnwrapped();
  void checkGLError(const QString& funName);
  
  virtual void drawSceneUsingColorID()=0;
  virtual void setSceneLowResolution()=0;
  virtual void setSceneHighResolution()=0;
  virtual void getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound)=0;
  virtual void showUnwrappedContextMenu() = 0;

  boost::shared_ptr<GLActorCollection> scene;      ///< Collection of actors
  GLTrackball* _trackball;       ///< Trackball for user interaction
  GLViewport* _viewport;         ///< Opengl View port [World -> Window]

protected slots:
  void set3DAxesState(int state);
  void setRenderMode(int);

private:
  void setRenderingOptions();
  void setLightingModel(int);
  void drawAxes(double axis_length = 100.0);
  void drawDisplayScene();
  void drawPickingScene();
  void switchToPickingMode();
  void draw3D();
  void drawUnwrapped();

  QColor bgColor; ///< Background color
  InteractionMode iInteractionMode;
  bool mPickingDraw;
  GLGroupPickBox* mPickBox;      ///< Picker used for user selecting a object in window
  GLActor* mPickedActor;
  bool isKeyPressed;
  int m3DAxesShown;              ///< true when the 3D axes are to be shown
  int m_lightingState;           ///< 0 = light off; 2 = light on
  PolygonMode m_polygonMode;     ///< SOLID or WIREFRAME
  bool m_firstFrame;

protected:
  // Unwrapping stuff
  RenderMode m_renderMode;       ///< 3D view or unwrapped
  UnwrappedSurface* m_unwrappedSurface;
  bool m_unwrappedSurfaceChanged;
  bool m_unwrappedViewChanged;   ///< set when the unwrapped image must be redrawn, but the surface is the same

};

#endif /*GL3DWIDGET_H_*/

