#ifdef WIN32
#include <windows.h>
#endif
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Exception.h"
#include "MantidObject.h"
#include "TexObject.h"
#include <GL/gl.h>

/**
 * Constructor
 */
MantidObject::MantidObject(const boost::shared_ptr<const Mantid::Geometry::Object> obj,bool withDisplayList):GLObject(withDisplayList)
{
	Obj=obj;
	mHighResolution=true;
	Obj->initDraw();
}

/** 
 * Destructor
 */
MantidObject::~MantidObject()
{
}

/**
 * implementation to render Object
 */
void MantidObject::define()
{
  if(mHighResolution)
  {
    Obj->draw();
  }
  else
  {
    defineBoundingBox();
  }
}

/**
 * @return the Object
 */
const boost::shared_ptr<const Mantid::Geometry::Object> MantidObject::getObject()
{
	return Obj;
}


/**
 * Implementation of the rendering bounding box
 */
void MantidObject::defineBoundingBox()
{
	//Get Object bounding box
	double xmin,xmax,ymin,ymax,zmin,zmax;
	xmax=ymax=zmax=1000;
	xmin=ymin=zmin=-1000;
	Obj->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
	Mantid::Geometry::V3D _bbmin(xmin,ymin,zmin),_bbmax(xmax,ymax,zmax);
	//draw bounding box
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

/**
 * renders the object in high resolution
 */
void MantidObject::setResolutionToHigh()
{
	mHighResolution=true;
	mChanged=true;
	construct();
}

/**
 * renders the object in low resolution
 */
void MantidObject::setResolutionToLow()
{
	mHighResolution=false;
	mChanged=true;
	construct();
}
