#ifndef GL3DWIDGET_H_
#define GL3DWIDGET_H_

#include <QGLWidget> 
#include "GLViewport.h" 
#include "GLTrackball.h" 
#include "GLActorCollection.h" 
#include "GLPicker.h" 
#include "boost/shared_ptr.hpp"

/*!
  \class  GL3DWidget
  \brief  OpenGL Qt Widget which renders Mantid Geometry ObjComponents
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  This Class takes input as ObjComponents and renders them with in the Qt widget. also
  provides the user interaction with the rendered object.

  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
	GL3DWidget(QWidget* parent=0); ///< Constructor
	virtual ~GL3DWidget();         ///< Destructor
	void setActorCollection(boost::shared_ptr<GLActorCollection>);
	void setInteractionModePick();
	void setInteractionModeNormal();
	GLActor* getPickedActor();
signals:
        void actorPicked( GLActor* );
protected:
	void initializeGL();
    void MakeObject();
	void paintGL();
	void resizeGL(int,int);
	void mousePressEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
	void wheelEvent(QWheelEvent *);
	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);
	boost::shared_ptr<GLActorCollection> scene;      ///< Collection of actors
private:
	int iInteractionMode;
	bool mPickingDraw;
    GLTrackball* _trackball;       ///< Trackball for user interaction
    GLViewport* _viewport;         ///< Opengl View port [World -> Window]
    GLPicker* _picker;             ///< Picker used for user selecting a object in window
	GLActor* mPickedActor;
	bool isKeyPressed;
};

#endif /*GL3DWIDGET_H_*/

