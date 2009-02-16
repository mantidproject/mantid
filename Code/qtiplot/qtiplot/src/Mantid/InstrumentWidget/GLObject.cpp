#ifdef WIN32
#include <windows.h>
#endif
#include "GLObject.h"
#include "MantidKernel/Exception.h"

GLObject::GLObject(bool withDisplayList):_changed(true)
{
	if(withDisplayList)
	{
		_displaylist= glGenLists(2);
	}
	else
	{
		_displaylist=0;
	}
	_bbmax(0.0,0.0,0.0);
	_bbmin(0.0,0.0,0.0);
}
GLObject::~GLObject()
{
	if(_displaylist!=0)
		glDeleteLists(_displaylist,2);
}

/**
 * This method draws the opengl display list.
 */
void GLObject::draw()
{
    if (_changed) construct();
	if (_displaylist!=0)
		glCallList(_displaylist);
	else
		define();
}

/**
 * This method constructs the opengl display list
 */
void GLObject::construct()
{
	if(_displaylist==0) 
	{
		_changed=false;
		return;
	}
    glNewList(_displaylist,GL_COMPILE); //Construct display list for object representation
         define();
    glEndList();
    
    glNewList(_displaylist+1,GL_COMPILE); // Construct display list for bounding box.
		defineBoundingBox();
    glEndList();

    if(glGetError()==GL_OUT_OF_MEMORY) //Throw an exception
		throw Mantid::Kernel::Exception::OpenGLError("OpenGL: Out of video memory");
    _changed=false;  //Object Marked as changed.
}

/**
 * Virtual method which constructs the opengl rendering commands.
 */
void GLObject::define()
{
}

/**
 * This method draws the bounding box
 */
void GLObject::drawBoundingBox()
{   
        if (_changed) construct();
		if(_displaylist!=0)
			glCallList(_displaylist+1);
		else
			defineBoundingBox();
}

/**
 * This method returns the object bounding box
 * @param minPoint output min point of the bounding box
 * @param maxPoint output max point of the bounding box
 */
void GLObject::getBoundingBox(Mantid::Geometry::V3D& minPoint,Mantid::Geometry::V3D& maxPoint)
{
	minPoint=_bbmin;
	maxPoint=_bbmax;
}

/**
 * Renders the bounding box
 */
void GLObject::defineBoundingBox()
{
        glBegin(GL_LINE_LOOP);
            glVertex3d(_bbmin[0],_bbmin[1],_bbmin[2]);
            glVertex3d(_bbmin[0],_bbmax[1],_bbmin[2]);
            glVertex3d(_bbmax[0],_bbmax[1],_bbmin[2]);
            glVertex3d(_bbmax[0],_bbmin[1],_bbmin[2]);
        glEnd();
        glBegin(GL_LINE_LOOP);
            glVertex3d(_bbmin[0],_bbmin[1],_bbmax[2]);
            glVertex3d(_bbmin[0],_bbmax[1],_bbmax[2]);
            glVertex3d(_bbmax[0],_bbmax[1],_bbmax[2]);
            glVertex3d(_bbmax[0],_bbmin[1],_bbmax[2]);
        glEnd();
         glBegin(GL_LINES);
            glVertex3d(_bbmin[0],_bbmin[1],_bbmin[2]);
            glVertex3d(_bbmin[0],_bbmin[1],_bbmax[2]);
        glEnd();
        glBegin(GL_LINES);
            glVertex3d(_bbmin[0],_bbmax[1],_bbmin[2]);
            glVertex3d(_bbmin[0],_bbmax[1],_bbmax[2]);
        glEnd();
        glBegin(GL_LINES);
            glVertex3d(_bbmax[0],_bbmin[1],_bbmin[2]);
            glVertex3d(_bbmax[0],_bbmin[1],_bbmax[2]);
        glEnd();
        glBegin(GL_LINES);
            glVertex3d(_bbmax[0],_bbmax[1],_bbmin[2]);
            glVertex3d(_bbmax[0],_bbmax[1],_bbmax[2]);
        glEnd();
}