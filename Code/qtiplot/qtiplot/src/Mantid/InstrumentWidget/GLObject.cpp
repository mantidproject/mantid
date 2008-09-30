#ifdef WIN32
#include <windows.h>
#endif
#include "GLObject.h"

GLObject::GLObject():_changed(true)
{
     _displaylist= glGenLists(2);
}
GLObject::~GLObject()
{
    glDeleteLists(_displaylist,2);
}

/**
 * This method draws the opengl display list.
 */
void GLObject::draw()
{
    if (_changed) construct();
    glCallList(_displaylist);
}

/**
 * This method constructs the opengl display list
 */
void GLObject::construct()
{
    glNewList(_displaylist,GL_COMPILE); //Construct display list for object representation
         define();
    glEndList();
    
    glNewList(_displaylist+1,GL_COMPILE); // Construct display list for bounding box.
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
    glEndList();
    
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
        glCallList(_displaylist+1);
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