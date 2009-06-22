#ifdef WIN32
#include <windows.h>
#endif
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidKernel/Exception.h"
#include "ObjComponentActor.h"
#include "MantidObject.h"
#include <GL/gl.h> 

using namespace Mantid;
using namespace Geometry;
ObjComponentActor::ObjComponentActor(MantidObject *obj,boost::shared_ptr<Mantid::Geometry::IObjComponent> objComp,bool withDisplayList):GLActor(withDisplayList)
{
	mObjComp=objComp;
	mObject=obj;
}

ObjComponentActor::~ObjComponentActor()
{
}

/**
 * Concrete implementation of rendering ObjComponent.
 */
void ObjComponentActor::define()
{
	glPushMatrix();
	V3D pos  =mObjComp->getPos();
	Quat rot =mObjComp->getRotation();
	double rotGL[16];
	rot.GLMatrix(rotGL);
	glTranslated(pos[0],pos[1],pos[2]);
	glMultMatrixd(rotGL);
	V3D scaleFactor=mObjComp->getScaleFactor();
	glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);
	mObject->draw();
	glPopMatrix();
}

/**
 * Return the bounding box 
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void ObjComponentActor::getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound)
{
	double xmin,ymin,zmin,xmax,ymax,zmax;
	xmin=ymin=zmin=-1000;
	xmax=ymax=zmax=1000;
	mObjComp->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
	minBound[0]=xmin;minBound[1]=ymin;minBound[2]=zmin;
	maxBound[0]=xmax;maxBound[1]=ymax;maxBound[2]=zmax;
}
