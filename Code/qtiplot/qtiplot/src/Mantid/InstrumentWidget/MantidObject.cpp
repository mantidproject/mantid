#ifdef WIN32
#include <windows.h>
#endif
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/ObjComponent.h"
#include "MantidObject.h"
#include <GL/gl.h> 

MantidObject::MantidObject(Mantid::Geometry::ObjComponent* obj)
{
	double xmax,ymax,zmax,xmin,ymin,zmin;
	xmax=ymax=zmax=10000;
	xmin=ymin=zmin=-10000;
	obj->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
    _bbmin(xmin,ymin,zmin);
    _bbmax(xmax,ymax,zmax);
	Obj=obj;
	Obj->initDraw();
}

MantidObject::~MantidObject()
{
}
void MantidObject::define()
{
    Obj->draw();
}

Mantid::Geometry::ObjComponent* MantidObject::getComponent()
{
	return Obj;
}
