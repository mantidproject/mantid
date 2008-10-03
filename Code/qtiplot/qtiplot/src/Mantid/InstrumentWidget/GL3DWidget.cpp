#ifdef WIN32
#include <windows.h>
#endif
#include "GL3DWidget.h"
#include <QtOpenGL>
#include "GLActor.h" 
#include <QSpinBox> 
#include "GLColor.h" 
#include <map>
#include <string>
#include "boost/shared_ptr.hpp"
#include <QApplication>
#include <iostream>

GL3DWidget::GL3DWidget(QWidget* parent):QGLWidget(parent)
{
	_viewport=new GLViewport;
	_trackball=new GLTrackball(_viewport);
	_picker=new GLPicker;
	_picker->setViewport(_viewport);
	isKeyPressed=false;
	scene=boost::shared_ptr<GLActorCollection>(new GLActorCollection());
	_picker->setActorCollection(scene.get());
	mPickedActor=NULL;
	mPickingDraw=false;
}
GL3DWidget::~GL3DWidget()
{
	delete _picker;
	delete _viewport;
	delete _trackball;
}

void GL3DWidget::setInteractionModePick()
{
	iInteractionMode=1;// Pick mode
}

void GL3DWidget::setInteractionModeNormal()
{
	iInteractionMode=0;//Normal mode
}

GLActor* GL3DWidget::getPickedActor()
{
	return mPickedActor;
}

/**
 * This method initializes the opengl settings. its invoked defaultly by Qt when the widget
 * is initialized.
 */
void GL3DWidget::initializeGL()
{ 
	setCursor(Qt::PointingHandCursor); // This is to set the initial window mouse cursor to Hand icon
	glEnable(GL_DEPTH_TEST);           // Enable opengl depth test to render 3D object properly
	glShadeModel(GL_SMOOTH);           // Shade model is smooth (expensive but looks pleasing)  
	glEnable (GL_LIGHTING);            // Enable light
	glEnable(GL_LIGHT0);               // Enable opengl first light
	glEnable(GL_LINE_SMOOTH);          // Set line should be drawn smoothly
	glEnable(GL_BLEND);                // Enable blending
	glEnable(GL_NORMALIZE);            // Normalize the input normals (its expensive better to normalize normals manually)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //Set Blend function
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);  // This model lits both sides of the triangle
	// Set Light0 Attributes, Ambient, diffuse,specular and position
	// Its a directional light which follows camera position
	float lamp_ambient[4]={0.0,0.0,0.0,1.0}; 
	float lamp_diffuse[4]={1.0,1.0,1.0,1.0};
	float lamp_specular[4]={1.0,1.0,1.0,1.0};
	glLightfv(GL_LIGHT0, GL_AMBIENT,lamp_ambient );
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lamp_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lamp_specular);
	float lamp_pos[4]={0.0,0.0,1.0,0.0};
	glLightfv(GL_LIGHT0, GL_POSITION, lamp_pos);	
//	MakeObject();                      // This is default make object TODO: Remove this

}

/**
 * This is overridden function which is called by Qt when the widget needs to be repainted.
 */
void GL3DWidget::paintGL()
{
	if(mPickingDraw){
		glEnable(GL_DEPTH_TEST);            // Enable Depth test
		glDepthFunc(GL_LEQUAL);             // Depth function for testing is Less than or equal
		glShadeModel(GL_FLAT);            // Shade model is smooth
		glDisable(GL_LIGHTING);              
		glDisable(GL_LIGHT0);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
		glDisable(GL_NORMALIZE);
		glClearColor(0.0,0.0,0.0,1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Issue the rotation, translation and zooming of the trackball to the object
		_trackball->IssueRotation();
		glPushMatrix();
		scene->drawColorID();
		glPopMatrix();
	} 
	else
	{
		glEnable(GL_DEPTH_TEST);            // Enable Depth test
		glDepthFunc(GL_LEQUAL);             // Depth function for testing is Less than or equal
		glShadeModel(GL_SMOOTH);            // Shade model is smooth
		glEnable(GL_LIGHTING);              
		glEnable(GL_LIGHT0);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
		glEnable(GL_NORMALIZE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glClearColor(0.0,0.0,0.0,1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Issue the rotation, translation and zooming of the trackball to the object
		_trackball->IssueRotation();

		glPushMatrix();
		if(isKeyPressed){
			glDisable(GL_LIGHTING);
			scene->drawBoundingBox();
		}
		else
		{
			QApplication::setOverrideCursor(Qt::WaitCursor);
			scene->draw();
			QApplication::restoreOverrideCursor();
		}
		glPopMatrix();
	}
}

/**
 * This function is also overridden from the parent. This method is invoked when the widget is resized
 * This method resizes the viewport according to the new widget width and height
 */
void GL3DWidget::resizeGL(int width, int height)
{
	// Getting the bounding box of the scene and setting the orthagonal projection
	// such that the orthogonal projection places the object completly in the screen
	// Its a simplified version of placing the object completly in screen with same
	// min and max values in all directions.
	Mantid::Geometry::V3D minPoint,maxPoint;
	scene->getBoundingBox(minPoint,maxPoint);
	double minValue,maxValue;
	minValue=minPoint[0];
	if(minValue>minPoint[1])minValue=minPoint[1];
	if(minValue>minPoint[2])minValue=minPoint[2];
	maxValue=maxPoint[0];
	if(maxValue<maxPoint[1])maxValue=maxPoint[1];
	if(maxValue<maxPoint[2])maxValue=maxPoint[2];
	double temp=minValue-(maxValue-minValue)/2;
	maxValue=maxValue+(maxValue-minValue)/2;
	minValue=temp;

	_viewport->resize(width,height);
	_viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
	_viewport->issueGL();
}

/**
 * Mouse press callback method, It implements mouse button press initialize methods.
 * Left Button: Zoom
 * Right Button: Rotate
 * Middle Button: Translate
 * Key + Left Button: Pick (TODO: Yet to implement)
 * @param event This is the event variable which has the position and button states
 */
void GL3DWidget::mousePressEvent(QMouseEvent* event)
{
	if(iInteractionMode==1 && (event->buttons() & Qt::LeftButton)) // Pick Mode
	{
		setCursor(Qt::CrossCursor);
		mPickingDraw=true;
		paintGL();
		mPickedActor=_picker->pickPoint(event->x(),event->y());
		mPickingDraw=false;
		updateGL();
		if(mPickedActor!=NULL)
			emit actorPicked(mPickedActor);
		return;
	} //end of pick mode and start of normal mode
	if (event->buttons() & Qt::MidButton)
	{
		//setCursor(Qt::CrossCursor);
		//_picker->pickPoint(event->x(),event->y());
		//updateGL();
		setCursor(Qt::SizeVerCursor);
		_trackball->initZoomFrom(event->x(),event->y());
		isKeyPressed=true;
	}
	else if (event->buttons() & Qt::LeftButton)
	{
		setCursor(Qt::OpenHandCursor);
		_trackball->initRotationFrom(event->x(),event->y());
		isKeyPressed=true;
	} 
	else if(event->buttons() & Qt::RightButton)
	{
		setCursor(Qt::CrossCursor);
		_trackball->initTranslateFrom(event->x(),event->y());
		isKeyPressed=true;
	}
}

/**
 * This is mouse move callback method. It implements the actions to be taken when the mouse is
 * moved with a particular button is pressed.
 * Left Button: Zoom
 * Right Button: Rotate
 * Middle Button: Translate
 * Key + Left Button: Pick (TODO: Yet to implement)
 * @param event This is the event variable which has the position and button states
 */
void GL3DWidget::mouseMoveEvent(QMouseEvent* event)
{
	if(iInteractionMode==1) return;
	if (event->buttons() & Qt::LeftButton)
	{
		setCursor(Qt::ClosedHandCursor);
		_trackball->generateRotationTo(event->x(),event->y());
		updateGL();
		_trackball->initRotationFrom(event->x(),event->y());
	}else if(event->buttons() & Qt::RightButton){ //Translate
		setCursor(Qt::CrossCursor);
		_trackball->generateTranslationTo(event->x(),event->y());
		updateGL();
		_trackball->initTranslateFrom(event->x(),event->y());
	}else if(event->buttons() & Qt::MidButton){ //Zoom
		setCursor(Qt::SizeVerCursor);
		_trackball->generateZoomTo(event->x(),event->y());
		updateGL();
		_trackball->initZoomFrom(event->x(),event->y());
	}
}

/**
 * This is mouse button release callback method. This resets the cursor to pointing hand cursor
 * @param event This is the event variable which has the position and button states
 */
void GL3DWidget::mouseReleaseEvent(QMouseEvent* event)
{
	setCursor(Qt::PointingHandCursor);
	isKeyPressed=false;
	updateGL();
}

/**
 * Mouse wheel event to set the zooming in and out
 * @param event This is the event variable which has the status of the wheel
 */
void GL3DWidget::wheelEvent(QWheelEvent* event)
{
	setCursor(Qt::SizeVerCursor);
	_trackball->initZoomFrom(event->x(),event->y());
	_trackball->generateZoomTo(event->x(),event->y()+event->delta());
	updateGL();	
	setCursor(Qt::PointingHandCursor);
}

/**
 * This method sets the collection of actors that widget needs to display
 * @param col input collection of actors
 */
void GL3DWidget::setActorCollection(boost::shared_ptr<GLActorCollection> col)
{
	scene=col;
	_picker->setActorCollection(scene.get());
	int width,height;
	_viewport->getViewport(&width,&height);
	resizeGL(width,height);
	updateGL();
}

/**
 * This default object initialization method. usually used for testing or added a default
 * object to all the widgets.
 */
void GL3DWidget::MakeObject()
{	
}
