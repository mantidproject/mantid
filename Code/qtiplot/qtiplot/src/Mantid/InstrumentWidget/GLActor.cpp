#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include "GLActor.h"


boost::shared_ptr<GLColor> redColor(new GLColor(1.0,0.5,0.0,1.0));

GLActor::GLActor(char* name):_name(name), _picked(false)
{
	_colorID[0]=0;_colorID[1]=0;_colorID[2]=0;
	_color=redColor;
	_pos=new Mantid::Geometry::V3D;
}
GLActor::~GLActor()
{
	delete _pos;
}

/**
 * This method draws the GL object
 */
void GLActor::draw()
{
    glPushMatrix();
        glTranslated((*_pos)[0],(*_pos)[1],(*_pos)[2]);
		_color->paint(GLColor::MATERIAL);
		_color->paint(GLColor::PLAIN);
        _representation->draw();
    glPopMatrix();
}

/**
 * This method draws the bounding box of the GLObject
 */
void GLActor::drawBoundingBox()
{
       glPushMatrix();
        glTranslated((*_pos)[0],(*_pos)[1],(*_pos)[2]);
		_color->paint(GLColor::PLAIN);
        _representation->drawBoundingBox();
       glPopMatrix();
}

void GLActor::setColorID(unsigned char r, unsigned char g, unsigned char b)
{
	_colorID[0]=r;_colorID[1]=g;_colorID[2]=b;
}
/**
 * This method get the bounding box values of the GLObject
 * @param minPoint Its the return value of the min point of actor bounding box
 * @param maxPoint Its the return value of teh max point of actor bounding box
 */
void GLActor::getBoundingBox(Mantid::Geometry::V3D& minPoint,Mantid::Geometry::V3D& maxPoint)
{
	_representation->getBoundingBox(minPoint,maxPoint);
	minPoint = minPoint+(*_pos);
	maxPoint = maxPoint+(*_pos);
}

/**
 * This method sets the position of the actor
 * @param x input x-dim value of the position of actor
 * @param y input y-dim value of the position of actor
 * @param z input z-dim value of the position of actor
 */
void GLActor::setPos(double x, double y, double z)
{
	(*_pos)=Mantid::Geometry::V3D(x,y,z);
}

/**
 * This method sets the actor's GLObject
 * @param o input GLObject
 */
void GLActor::setRepresentation(boost::shared_ptr<GLObject> o)
{
    _representation=o;
    _representation->draw();
}

/**
 * This method returns the GLobject held by the actor
 * @return internal GLobject
 */
boost::shared_ptr<GLObject> GLActor::getRepresentation()
{
	return _representation;
}

/**
 * This method marks the Actor as picked
 */
void GLActor::markPicked()
{
    _picked=true;
}

/**
 * This method unmarks/clears the Actor pick. set the status as unpicked
 */
void GLActor::markUnPicked()
{
    _picked=false;
}

/**
 * This method sets the color for the actor
 * @param color input color for the actor
 */
void GLActor::setColor(boost::shared_ptr<GLColor> color)
{
    _color=color;
}

/**
 * This method draws the object with color id.
 */
void GLActor::drawIDColor()
{
    glPushMatrix();
        glTranslated((*_pos)[0],(*_pos)[1],(*_pos)[2]);
         glColor3f(_colorID[0]/255.0f,_colorID[1]/255.0f,_colorID[2]/255.0f);
        _representation->draw();
    glPopMatrix();
}
