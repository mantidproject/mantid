#ifdef WIN32
#include <windows.h>
#endif
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidObject.h"
#include <GL/gl.h> 

MantidObject::MantidObject(boost::shared_ptr<Mantid::Geometry::IObjComponent> obj)
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

boost::shared_ptr<Mantid::Geometry::IObjComponent> MantidObject::getComponent()
{
	return Obj;
}
