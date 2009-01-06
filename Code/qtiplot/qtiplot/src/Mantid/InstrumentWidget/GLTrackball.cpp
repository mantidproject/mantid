#ifdef WIN32
#include <windows.h>
#endif
#include "GLTrackball.h"
#include "GL/glu.h"
#define _USE_MATH_DEFINES true
#include <math.h>
#include <float.h>
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
	xmin*=_viewport->getZoomFactor();
	ymin*=_viewport->getZoomFactor();
	xmax*=_viewport->getZoomFactor();
	ymax*=_viewport->getZoomFactor();
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
	xmin*=_viewport->getZoomFactor();
	ymin*=_viewport->getZoomFactor();
	xmax*=_viewport->getZoomFactor();
	ymax*=_viewport->getZoomFactor();
	x=static_cast<double>((xmin+((xmax-xmin)*((double)a/(double)_viewport_w))));
	y=static_cast<double>((ymin+((ymax-ymin)*(_viewport_h-b)/_viewport_h)));
	z=0.0;
	Mantid::Geometry::V3D _newpoint= Mantid::Geometry::V3D(x,y,z);
	Mantid::Geometry::V3D diff= _newpoint - _lastpoint;
	_viewport->getTranslation(x,y);
	_viewport->setTranslation(x+diff[0],y+diff[1]);
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
	diff*=_viewport->getZoomFactor();
	_viewport->setZoomFactor(diff);
}


void GLTrackball::IssueRotation()
{	
	if (_viewport){
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
	_viewport->setTranslation(0.0,0.0);
	_viewport->setZoomFactor(1.0);
}

void GLTrackball::setViewToXPositive()
{
	reset();
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(1.0,0.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(_rotationmatrix);
}

void GLTrackball::setViewToYPositive()
{
	reset();
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
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(-1.0,0.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(_rotationmatrix);
}

void GLTrackball::setViewToYNegative()
{
	reset();
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

void GLTrackball::rotateBoundingBox(double& xmin,double& xmax,double& ymin,double& ymax,double& zmin,double& zmax)
{
	Mantid::Geometry::V3D maxT(xmax,ymax,zmax);
	Mantid::Geometry::V3D minT(xmin,ymin,zmin);
	Mantid::Geometry::V3D v0(minT[0],minT[1],minT[2]),v1(minT[0],minT[1],maxT[2]),v2(minT[0],maxT[1],minT[2]),v3(minT[0],maxT[1],maxT[2]),
		v4(maxT[0],minT[1],minT[2]),v5(maxT[0],minT[1],maxT[2]),v6(maxT[0],maxT[1],minT[2]),v7(maxT[0],maxT[1],maxT[2]);
	std::vector<Mantid::Geometry::V3D> points;
	points.clear();
	points.push_back(v0); points.push_back(v1); points.push_back(v2); points.push_back(v3);
	points.push_back(v4); points.push_back(v5); points.push_back(v6); points.push_back(v7);
	maxT[0]=-DBL_MAX;maxT[1]=-DBL_MAX;maxT[2]=-DBL_MAX;
	minT[0]=DBL_MAX; minT[1]=DBL_MAX; minT[2]=DBL_MAX;
	Mantid::Geometry::Quat Rotate = _quaternion;
	std::vector<Mantid::Geometry::V3D>::const_iterator vc;
	for(vc=points.begin();vc!=points.end();vc++)
	{
		Mantid::Geometry::V3D pt= (*vc);
		Rotate.rotate(pt);
		for(int i=0;i<3;i++)
		{
			if(maxT[i]<pt[i]) maxT[i]=pt[i];
			if(minT[i]>pt[i]) minT[i]=pt[i];
		}
	}
	xmax=maxT[0]; ymax=maxT[1]; zmax=maxT[2];
	xmin=minT[0]; ymin=minT[1]; zmin=minT[2];
}