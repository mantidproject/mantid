#ifdef WIN32
#include <windows.h>
#endif
#include "GLPicker.h"
#include <iostream>
#include <set> 

GLPicker::GLPicker(GLActorCollection* collection):_actors(collection)
{
    _pickingColor=GLColor(1.0,0.0,0.0,1.0); // Red color per default.
}
GLPicker::~GLPicker()
{ 
}

/**
 * The method sets the picker color
 * @param R input Red component of picker color
 * @param G input Green componenet of picker color
 * @param B input Blue componenet of picker color
 * @param A input Alpha component of picker color
 */
void GLPicker::setPickerColor(float R, float G, float B, float A)
{
    _pickingColor.set(R,G,B,A);
}

/**
 * Sets the picker color
 * @param color input color 
 */
void GLPicker::setPickerColor(const GLColor& color)
{
    _pickingColor=color;
}

/**
 * Sets the actor collection
 * @param collection input actor collection
 */
void GLPicker::setActorCollection(GLActorCollection* collection)
{
    _actors=collection;
}

/**
 * picking a actor at a input point
 * @param x input x-dim value of the point
 * @param y input y-dim value of the point
 */
GLActor* GLPicker::pickPoint(int x, int y) // Picking object at coordinate of the mouse (x,y)
{
      GLActor* picked=NULL;
      if (!_actors) return 0;
      unsigned char pixel[6];
      int vw, vh;
      _viewport->getViewport(&vw,&vh);
      //glDisable(GL_DEPTH_TEST);   
      //glDisable (GL_LIGHTING);
      //glDisable(GL_LIGHT0);
      //glDisable(GL_LINE_SMOOTH);
      //glDisable(GL_BLEND);
      //glDisable(GL_NORMALIZE);
      //_actors->drawColorID();
      glReadPixels(x, vh-y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
	  std::cout<<"Searching for pixel "<<int(pixel[0])<<" "<<int(pixel[1])<<" "<<int(pixel[2])<<std::endl;
      picked=_actors->findColorID(pixel);
      //_actors->construct();
      return picked;
}

/**
 * Sets the viewpoint
 * @param v input viewport
 */
void GLPicker::setViewport(GLViewport* v)
{
    _viewport=v;
}

/**
 * Set the starting point of the pick area
 * @param x input x-dim value of the pick point
 * @param y input y-dim value of the pick point
 */
void GLPicker::pickAreaStart(int x, int y)
{
    _rectx1=x;_recty1=y;
}

/**
 * Set the end point of the pick area
 * @param x input x-dim value of the pick point
 * @param y input y-dim value of the pick point
 */
void GLPicker::pickAreaFinish(int x, int y)
{
   _rectx2=x;
   _recty2=y;
}

/**
 * draws the scence with viewport set the rectangular area between _rect1,_rect2
 * and x,y. 
 * @param x input x-dim value of the pick point
 * @param y input y-dim value of the pick point
 */
void GLPicker::drawArea(int x, int y)
{
    _rectx2=x;_recty2=y;
    int vw, vh;
      _viewport->getViewport(&vw,&vh);
    double _point1[2]={-1.0+2.0*(double)_rectx1/vw,-1.0+2.0*(vh-_recty1)/vw};
    double _point2[2]={-1.0+2.0*(double)_rectx2/vw,-1.0+2.0*(vh-_recty2)/vw};
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix (); 
    glLoadIdentity (); 
    glMatrixMode (GL_PROJECTION); 
    glPushMatrix (); 
    glLoadIdentity ();
    glLineWidth(1.0);
    glPushAttrib(GL_COLOR_MATERIAL);
    _pickingColor.paint(GLColor::EMIT);
    glBegin (GL_LINE_LOOP);
    glVertex3d (_point1[0], _point1[1], -1); 
    glVertex3d (_point1[0], _point2[1], -1); 
    glVertex3d (_point2[0], _point2[1], -1); 
    glVertex3d (_point2[0], _point1[1], -1); 
    glEnd ();
    glPopAttrib();
    glPopMatrix (); 
    glMatrixMode (GL_MODELVIEW); 
    glPopMatrix ();
}
