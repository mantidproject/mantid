#include "GLViewport.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "GL/gl.h"
#include "GL/glu.h"

GLViewport::GLViewport(int w, int h):_width(w),_height(h)
{
	projection=GLViewport::ORTHO;
}

GLViewport::~GLViewport()
{
}
void GLViewport::resize(int w,int h)
{
    _width=w;
    _height=h;
}
void GLViewport::getViewport(int* w, int* h) const
{
    *w=_width;
    *h=_height;
}
/**
 * This will set the projection to Ortho
 * @param l left side of the Ortho projection (xmin)
 * @param r right side of the Ortho projection (xmax)
 * @param b bottom side of the Ortho projection (ymin)
 * @param t top side of the Ortho projection (ymax)
 * @param near near side of the Ortho Projection (zmin)
 * @param far far side of the Ortho Projection (zmax)
 */
void GLViewport::setOrtho(double l,double r,double b,double t,double nearz,double farz)
{
	Left=l;
	Right=r;
	Bottom=b;
	Top=t;
	Near=nearz;
	Far=farz;
	projection=GLViewport::ORTHO;
}

/**
 * This will set the projection to perspective
 * @param l left side of the perspective projection (xmin)
 * @param r right side of the perspective projection (xmax)
 * @param b bottom side of the perspective projection (ymin)
 * @param t top side of the perspective projection (ymax)
 * @param near near side of the perspective Projection (zmin)
 * @param far far side of the perspective Projection (zmax)
 */
void GLViewport::setPrespective(double l,double r,double b,double t,double nearz,double farz)
{
	Left=l;
	Right=r;
	Bottom=b;
	Top=t;
	Near=nearz;
	Far=farz;
	projection=GLViewport::PERSPECTIVE;
}

GLViewport::ProjectionType GLViewport::getProjectionType()const
{
	return projection;
}

void GLViewport::getProjection(double& xmin,double& xmax,double& ymin,double& ymax,double& zmin,double& zmax)
{
	xmin=Left;
	xmax=Right;
	ymin=Bottom;
	ymax=Top;
	zmin=Near;
	zmax=Far;
}

void GLViewport::issueGL() const
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(projection==GLViewport::PERSPECTIVE){	
		glFrustum(Left, Right, Bottom, Top, Near, Far);
	}else{
		glOrtho(Left, Right, Bottom, Top, Near, Far);
	}
	glMatrixMode(GL_MODELVIEW);
}