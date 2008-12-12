#ifdef WIN32
#include <windows.h>
#endif
#include "GLTrackball.h"
#include "GL/glu.h"
#define _USE_MATH_DEFINES true
#include <math.h>
GLTrackball::GLTrackball(GLViewport* parent):_viewport(parent)
{
	reset();
    _rotationspeed=2.0;
	_modelCenter=Mantid::Geometry::V3D(0.0,0.0,0.0);
}
GLTrackball::~GLTrackball()
{
}
void GLTrackball::initRotationFrom(int a,int b)
{
	_lastpoint=projectOnSphere(a,b);
}
void GLTrackball::generateRotationTo(int a,int b)
{
	Mantid::Geometry::V3D _newpoint=projectOnSphere(a,b);
	Mantid::Geometry::V3D diff=_lastpoint-_newpoint;
    double angle;
	angle=0.5*M_PI*_rotationspeed*diff.norm();
	diff=_lastpoint.cross_prod(_newpoint); 
	diff.normalize();
	diff*=sin(0.5*angle);
	Mantid::Geometry::Quat temp(cos(0.5*angle),diff);
	_quaternion=temp*_quaternion;
    _quaternion.GLMatrix(_rotationmatrix);
}

void GLTrackball::initTranslateFrom(int a,int b)
{
	double x,y,z;
    int _viewport_w, _viewport_h;
	double xmin,xmax,ymin,ymax,zmin,zmax;
    _viewport->getViewport(&_viewport_w,&_viewport_h);
	_viewport->getProjection(xmin,xmax,ymin,ymax,zmin,zmax);
	x=static_cast<double>((xmin+((xmax-xmin)*((double)a/(double)_viewport_w))));
	y=static_cast<double>((ymin+((ymax-ymin)*(_viewport_h-b)/_viewport_h)));
	z=0.0;
	_lastpoint[0]=x;_lastpoint[1]=y;_lastpoint[2]=z;
}

void GLTrackball::generateTranslationTo(int a, int b)
{
	double x,y,z;
    int _viewport_w, _viewport_h;
	double xmin,xmax,ymin,ymax,zmin,zmax;
    _viewport->getViewport(&_viewport_w,&_viewport_h);
	_viewport->getProjection(xmin,xmax,ymin,ymax,zmin,zmax);
	x=static_cast<double>((xmin+((xmax-xmin)*((double)a/(double)_viewport_w))));
	y=static_cast<double>((ymin+((ymax-ymin)*(_viewport_h-b)/_viewport_h)));
	z=0.0;
	Mantid::Geometry::V3D _newpoint= Mantid::Geometry::V3D(x,y,z);
	Mantid::Geometry::V3D diff= _newpoint - _lastpoint;
	_translation += diff;
}

void GLTrackball::initZoomFrom(int a,int b)
{
	double x,y,z;
    int _viewport_w, _viewport_h;
    _viewport->getViewport(&_viewport_w,&_viewport_h);
	if(a>=_viewport_w || b>=_viewport_h || a <= 0||b<=0)return;
	x=static_cast<double>((_viewport_w-a));
	y=static_cast<double>((b-_viewport_h));
	z=0.0;
	_lastpoint[0] = x;
	_lastpoint[1] = y;
	_lastpoint[2] = z;
}

void GLTrackball::generateZoomTo(int a, int b)
{
	double x,y,z;
    int _viewport_w, _viewport_h;
    _viewport->getViewport(&_viewport_w,&_viewport_h);
	if(a>=_viewport_w || b>=_viewport_h||a <= 0||b<=0)return;
	x=static_cast<double>((_viewport_w-a));
	y=static_cast<double>((b-_viewport_h));
	z=0.0;
	if(y==0) y=_lastpoint[1];
	double diff= _lastpoint[1]/y ;
	_scaleFactor*=diff;
}


void GLTrackball::IssueRotation()
{	
	if (_viewport){
		glScaled(_scaleFactor,_scaleFactor,_scaleFactor);
		glTranslated(_translation[0],_translation[1],_translation[2]);
		glTranslated(_modelCenter[0],_modelCenter[1],_modelCenter[2]);
		glMultMatrixd(_rotationmatrix);
		glTranslated(-_modelCenter[0],-_modelCenter[1],-_modelCenter[2]);
	}
}

void GLTrackball::setModelCenter(Mantid::Geometry::V3D center)
{
	_modelCenter=center;
}

Mantid::Geometry::V3D GLTrackball::projectOnSphere(int a,int b)
{
    double x,y,z;
    int _viewport_w, _viewport_h;
    _viewport->getViewport(&_viewport_w,&_viewport_h);
	x=static_cast<double>((2.0*a-_viewport_w)/_viewport_w);
	y=static_cast<double>((_viewport_h-2.0*b)/_viewport_h);
	z=0.0;
	double norm=x*x+y*y;
	if (norm>1.0) 
	{
		x/=sqrt(norm);y/=sqrt(norm);
	}
	else
		z=sqrt(1.0-norm);
	return Mantid::Geometry::V3D(x,y,z);
}
void GLTrackball::setRotationSpeed(double r)
{
	if (_rotationspeed>0) _rotationspeed=r;
}
void GLTrackball::setViewport(GLViewport* v)
{
    if (v) _viewport=v;
}

void GLTrackball::reset()
{
	//Reset rotation,scale and translation
	_quaternion.init();
    _quaternion.GLMatrix(_rotationmatrix);
	_translation=Mantid::Geometry::V3D(0.0,0.0,0.0);
	_scaleFactor=1.0;
}

void GLTrackball::setViewToXPositive()
{
	reset();
	_translation=Mantid::Geometry::V3D(0.0,0.0,0.0);
	_translation[0]=_modelCenter[2]-_modelCenter[0];
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(1.0,0.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(_rotationmatrix);
}

void GLTrackball::setViewToYPositive()
{
	reset();
	_translation=Mantid::Geometry::V3D(0.0,0.0,0.0);
	_translation[1]=_modelCenter[2]-_modelCenter[1];
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(0.0,1.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(_rotationmatrix);
}

void GLTrackball::setViewToZPositive()
{
	reset();
	_quaternion.init();
	_quaternion.GLMatrix(_rotationmatrix);
}

void GLTrackball::setViewToXNegative()
{
	reset();
	_translation=Mantid::Geometry::V3D(0.0,0.0,0.0);
	_translation[0]=_modelCenter[2]-_modelCenter[0];
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(-1.0,0.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(_rotationmatrix);
}

void GLTrackball::setViewToYNegative()
{
	reset();
	_translation=Mantid::Geometry::V3D(0.0,0.0,0.0);
	_translation[1]=_modelCenter[2]-_modelCenter[1];
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(0.0,-1.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(_rotationmatrix);
}

void GLTrackball::setViewToZNegative()
{
	reset();
	Mantid::Geometry::Quat tempy(180.0,Mantid::Geometry::V3D(0.0,1.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(_rotationmatrix);
}