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
#include <cfloat>
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

//NOTES:
//1) if the sample buffers are not available then the paint of image on the mdi windows
//   seems to not work on intel chipset


GL3DWidget::GL3DWidget(QWidget* parent):
				QGLWidget(QGLFormat(QGL::DepthBuffer|QGL::NoAlphaChannel|QGL::SampleBuffers),parent)
{
	_viewport=new GLViewport;
	_trackball=new GLTrackball(_viewport);
	isKeyPressed=false;
	scene=boost::shared_ptr<GLActorCollection>(new GLActorCollection());
	mPickedActor=NULL;
	mPickingDraw=false;
	iInteractionMode=0;
	mPickBox=new GLGroupPickBox();
	setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);
	bgColor=QColor(0,0,0,1);
}
GL3DWidget::~GL3DWidget()
{
	delete _viewport;
	delete _trackball;
}

void GL3DWidget::setInteractionModePick()
{
	iInteractionMode=1;// Pick mode
	setMouseTracking(true);
	switchToPickingMode();
}

void GL3DWidget::setInteractionModeNormal()
{
	iInteractionMode=0;//Normal mode
	setMouseTracking(false);
	setCursor(Qt::PointingHandCursor);
	update();
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

}

/**
 * This method draws the scene onto the graphics context
 */
void GL3DWidget::drawDisplayScene()
{
	glEnable(GL_DEPTH_TEST);            // Enable Depth test
	glDepthFunc(GL_LEQUAL);             // Depth function for testing is Less than or equal
	glShadeModel(GL_SMOOTH);            // Shade model is smooth
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
	glClearColor(bgColor.red()/255.0,bgColor.green()/255.0,bgColor.blue()/255.0,1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Issue the rotation, translation and zooming of the trackball to the object
	_trackball->IssueRotation();

	glPushMatrix();
	if(isKeyPressed){
		glDisable(GL_LIGHTING);
		scene->draw();
		//scene->drawBoundingBox();
	}
	else
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		scene->draw();
		glPointSize(3.0);
		glBegin(GL_POINTS);
		glVertex3d(0.0,0.0,0.0);
		glEnd();
		QApplication::restoreOverrideCursor();
	}
	glPopMatrix();
}

/**
 * This method draws the scenc when in pick mode i.e with reference colors to actors
 */
void GL3DWidget::drawPickingScene()
{
	glEnable(GL_DEPTH_TEST);            // Enable Depth test
	glDepthFunc(GL_LEQUAL);             // Depth function for testing is Less than or equal
	glShadeModel(GL_FLAT);            // Shade model is flat
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
	drawSceneUsingColorID();
	glPopMatrix();
}

/**
 * Switches to picking mode does all the operations to create and set the images to GLGroupPickBox
 */
void GL3DWidget::switchToPickingMode()
{
	drawDisplayScene();
	glReadBuffer(GL_BACK);
	mPickBox->setDisplayImage(grabFrameBuffer(false));
    glDisable(GL_MULTISAMPLE);  //This will disable antialiasing which is build in by default for samplebuffers
	drawPickingScene();
	mPickBox->setPickImage(grabFrameBuffer(false));
    glEnable(GL_MULTISAMPLE);   //enable antialiasing
	mPickingDraw=false;
}

/**
 * This is overridden function which is called by Qt when the widget needs to be repainted.
 */
void GL3DWidget::paintEvent(QPaintEvent *event)
{
	makeCurrent();
	if(iInteractionMode){
		if(mPickingDraw==true)
			switchToPickingMode();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(format().sampleBuffers())
		{
			QPainter painter(this);
			painter.setRenderHint(QPainter::Antialiasing);
			mPickBox->draw(&painter);
			painter.end();
		}
		else
		{
			drawDisplayScene();
			QPainter painter(this);
			painter.setRenderHint(QPainter::Antialiasing);
			mPickBox->drawPickBox(&painter);
			painter.end();
		}
	}
	else
	{
		drawDisplayScene();
		QPainter painter(this);
		painter.end();
	}
}

/**
 * This function is also overridden from the parent. This method is invoked when the widget is resized
 * This method resizes the viewport according to the new widget width and height
 */
void GL3DWidget::resizeGL(int width, int height)
{

	_viewport->resize(width,height);
	_viewport->issueGL();

	if(iInteractionMode==1) //This is when in picking mode and the window is resized so update the image
	{
		mPickingDraw=true;
	}
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
		mPickBox->mousePressEvent(event);
		return;
	} //end of pick mode and start of normal mode
	if (event->buttons() & Qt::MidButton)
	{
		setCursor(Qt::SizeVerCursor);
		_trackball->initZoomFrom(event->x(),event->y());
		isKeyPressed=true;
		setSceneLowResolution();
	}
	else if (event->buttons() & Qt::LeftButton)
	{
		setCursor(Qt::OpenHandCursor);
		_trackball->initRotationFrom(event->x(),event->y());
		isKeyPressed=true;
		setSceneLowResolution();
	}
	else if(event->buttons() & Qt::RightButton)
	{
		setCursor(Qt::CrossCursor);
		_trackball->initTranslateFrom(event->x(),event->y());
		isKeyPressed=true;
		setSceneLowResolution();
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
	if(iInteractionMode==1){
		setCursor(Qt::CrossCursor);
		QRgb tmpColor=mPickBox->pickPoint(event->x(),event->y());
		emit actorHighlighted(tmpColor);
		mPickBox->mouseMoveEvent(event);
		update();
	}else{
		if (event->buttons() & Qt::LeftButton)
		{
			setCursor(Qt::ClosedHandCursor);
			_trackball->generateRotationTo(event->x(),event->y());
			update();
			_trackball->initRotationFrom(event->x(),event->y());
		}else if(event->buttons() & Qt::RightButton){ //Translate
			setCursor(Qt::CrossCursor);
			_trackball->generateTranslationTo(event->x(),event->y());
			update();
			_trackball->initTranslateFrom(event->x(),event->y());
		}else if(event->buttons() & Qt::MidButton){ //Zoom
			setCursor(Qt::SizeVerCursor);
			_trackball->generateZoomTo(event->x(),event->y());
			update();
			_trackball->initZoomFrom(event->x(),event->y());
		}
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
	setSceneHighResolution();
	if(iInteractionMode==1)
	{
		mPickBox->mouseReleaseEvent(event);
		std::set<QRgb> result=mPickBox->getListOfColorsPicked();
		if(result.size()!=0)
			emit actorsPicked(result);
	}
	update();
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
	update();
	setCursor(Qt::PointingHandCursor);
}

/**
 * This method is to handle keyboard events to mimic the mouse operations of click and move
 * @param event This is the event variable which has the status of the keyboard
 */
void GL3DWidget::keyPressEvent(QKeyEvent *event)
{
	grabKeyboard();
	if(iInteractionMode==1) return; ///Ignore keyboard event when in pick mode
	int width,height;
	_viewport->getViewport(&width,&height);
	int halfwidth=width/2;
	int halfheight=height/2;
	switch(event->key())
	{
		//-----------------------Translation-----------------
	case Qt::Key_Left:
		isKeyPressed=true;
		setCursor(Qt::CrossCursor);
		_trackball->initTranslateFrom(1,0);
		_trackball->generateTranslationTo(0,0);
		update();
		break;
	case Qt::Key_Right:
		isKeyPressed=true;
		setCursor(Qt::CrossCursor);
		_trackball->initTranslateFrom(0,0);
		_trackball->generateTranslationTo(1,0);
		update();
		break;
	case Qt::Key_Up:
		isKeyPressed=true;
		setCursor(Qt::CrossCursor);
		_trackball->initTranslateFrom(0,1);
		_trackball->generateTranslationTo(0,0);
		update();
		break;
	case Qt::Key_Down:
		isKeyPressed=true;
		setCursor(Qt::CrossCursor);
		_trackball->initTranslateFrom(0,0);
		_trackball->generateTranslationTo(0,1);
		update();
		break;
		//--------------------End of Translation---------------
		//--------------------Rotation-------------------------
	case Qt::Key_1:
		isKeyPressed=true;
		setCursor(Qt::ClosedHandCursor);
		_trackball->initRotationFrom(halfwidth,halfheight);
		_trackball->generateRotationTo(halfwidth-1,halfheight+1);
		update();
		break;
	case Qt::Key_2:
		isKeyPressed=true;
		setCursor(Qt::ClosedHandCursor);
		_trackball->initRotationFrom(halfwidth,halfheight);
		_trackball->generateRotationTo(halfwidth,halfheight+1);
		update();
		break;
	case Qt::Key_3:
		isKeyPressed=true;
		setCursor(Qt::ClosedHandCursor);
		_trackball->initRotationFrom(halfwidth,halfheight);
		_trackball->generateRotationTo(halfwidth+1,halfheight+1);
		update();
		break;
	case Qt::Key_4:
		isKeyPressed=true;
		setCursor(Qt::ClosedHandCursor);
		_trackball->initRotationFrom(halfwidth,halfheight);
		_trackball->generateRotationTo(halfwidth-1,halfheight);
		update();
		break;
	case Qt::Key_6:
		isKeyPressed=true;
		setCursor(Qt::ClosedHandCursor);
		_trackball->initRotationFrom(halfwidth,halfheight);
		_trackball->generateRotationTo(halfwidth+1,halfheight);
		update();
		break;
	case Qt::Key_7:
		isKeyPressed=true;
		setCursor(Qt::ClosedHandCursor);
		_trackball->initRotationFrom(halfwidth,halfheight);
		_trackball->generateRotationTo(halfwidth-1,halfheight-1);
		update();
		break;
	case Qt::Key_8:
		isKeyPressed=true;
		setCursor(Qt::ClosedHandCursor);
		_trackball->initRotationFrom(halfwidth,halfheight);
		_trackball->generateRotationTo(halfwidth,halfheight-1);
		update();
		break;
	case Qt::Key_9:
		isKeyPressed=true;
		setCursor(Qt::ClosedHandCursor);
		_trackball->initRotationFrom(halfwidth,halfheight);
		_trackball->generateRotationTo(halfwidth+1,halfheight-1);
		update();
		break;
		//---------------------------------End of Rotation--------------
		//---------------------------------Zoom-------------------------
	case Qt::Key_PageUp:
		isKeyPressed=true;
		setCursor(Qt::SizeVerCursor);
		_trackball->initZoomFrom(halfwidth,halfheight);
		_trackball->generateZoomTo(halfwidth,halfheight-1);
		update();
		break;
	case Qt::Key_PageDown:
		isKeyPressed=true;
		setCursor(Qt::SizeVerCursor);
		_trackball->initZoomFrom(halfwidth,halfheight);
		_trackball->generateZoomTo(halfwidth,halfheight+1);
		update();
		break;
	}
}

/**
 * This method is to handle keyboard events to mimic the mouse operations of mouse button up.
 * @param event This is the event variable which has the status of the keyboard
 */
void GL3DWidget::keyReleaseEvent(QKeyEvent *event)
{
	releaseKeyboard();
	setCursor(Qt::PointingHandCursor);
	isKeyPressed=false;
	if(!event->isAutoRepeat())
		update();
}
/**
 * This method sets the collection of actors that widget needs to display
 * @param col input collection of actors
 */
void GL3DWidget::setActorCollection(boost::shared_ptr<GLActorCollection> col)
{
	scene=col;
	int width,height;
	_viewport->getViewport(&width,&height);
	resizeGL(width,height);
	update();
}

/**
 * This default object initialization method. usually used for testing or added a default
 * object to all the widgets.
 */
void GL3DWidget::MakeObject()
{
}

/**
 * This saves the GL scene to PPM Image file, name of the file is given as input
 */
void GL3DWidget::saveToPPM(QString filename)
{
	paintGL();
	glReadBuffer(GL_BACK);
	QImage img=grabFrameBuffer(false);
	img.save(filename,"PPM");
}

/**
 * Sets the default view to input provided
 */
void GL3DWidget::setViewDirection(AxisDirection dir)
{
	Mantid::Geometry::V3D minPoint,maxPoint;
	double minValue,maxValue,_bbmin[3],_bbmax[3];
	getBoundingBox(minPoint,maxPoint);
	Mantid::Geometry::V3D centre=(maxPoint+minPoint)/2.0;
	defaultProjection();
	_viewport->getProjection(_bbmin[0],_bbmax[0],_bbmin[1],_bbmax[1],_bbmin[2],_bbmax[2]);
	switch(dir)
	{
	case XPOSITIVE:
		_trackball->setViewToXPositive();
		_trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
		_viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
		break;
	case YPOSITIVE:
		_trackball->setViewToYPositive();
		_trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
		_viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
		break;
	case ZPOSITIVE:
		_trackball->setViewToZPositive();
		_viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
		break;
	case XNEGATIVE:
		_trackball->setViewToXNegative();
		_trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
		_viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
		break;
	case YNEGATIVE:
		_trackball->setViewToYNegative();
		_trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
		_viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
		break;
	case ZNEGATIVE:
		_trackball->setViewToZNegative();
		_trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
		_viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
		break;
	}
	_viewport->issueGL();
	update();
}

/**
 * This method calculates the default projection
 */
void GL3DWidget::defaultProjection()
{
	// Getting the bounding box of the scene and setting the orthagonal projection
	// such that the orthogonal projection places the object completly in the screen
	// Its a simplified version of placing the object completly in screen with same
	// min and max values in all directions.
	Mantid::Geometry::V3D minPoint,maxPoint;
	getBoundingBox(minPoint,maxPoint);
	if(minPoint[0]==DBL_MAX||minPoint[1]==DBL_MAX||minPoint[2]==DBL_MAX||maxPoint[0]==-DBL_MAX||maxPoint[1]==-DBL_MAX||maxPoint[2]==-DBL_MAX)
	{
		minPoint=Mantid::Geometry::V3D(-1.0,-1.0,-1.0);
		maxPoint=Mantid::Geometry::V3D( 1.0, 1.0, 1.0);
	}
	double minValue,maxValue;
	minValue=minPoint[0];
	if(minValue>minPoint[1])minValue=minPoint[1];
	if(minValue>minPoint[2])minValue=minPoint[2];
	maxValue=maxPoint[0];
	if(maxValue<maxPoint[1])maxValue=maxPoint[1];
	if(maxValue<maxPoint[2])maxValue=maxPoint[2];
	if(minValue>maxValue)
	{
		double tmp;
		tmp=maxValue;
		maxValue=minValue;
		minValue=maxValue;
	}
	double temp=minValue-fabs(maxValue-minValue);
	maxValue=maxValue+fabs(maxValue-minValue);
	minValue=temp;

	_viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minValue*-1,maxValue*-1);
	Mantid::Geometry::V3D center;
	center=(minPoint+maxPoint)/2.0;
	_viewport->issueGL();
}

/**
 * This method set the background color.
 */
void GL3DWidget::setBackgroundColor(QColor input)
{
	bgColor=input;
}

/**
 * Resets the widget for new instrument definition
 */
void GL3DWidget::resetWidget()
{
	setActorCollection(boost::shared_ptr<GLActorCollection>(new GLActorCollection()));
}
