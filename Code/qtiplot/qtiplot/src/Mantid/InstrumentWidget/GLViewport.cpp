#include "GLViewport.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "GL/gl.h"
#include "GL/glu.h"

GLViewport::GLViewport(int w, int h):mWidth(w),mHeight(h)
{
	mProjection=GLViewport::ORTHO;
	mZoomFactor=1.0;
	mXTrans=0.0;
	mYTrans=0.0;
}

GLViewport::~GLViewport()
{
}
void GLViewport::resize(int w,int h)
{
    mWidth=w;
    mHeight=h;
}
void GLViewport::getViewport(int* w, int* h) const
{
    *w=mWidth;
    *h=mHeight;
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
	mLeft=l;
	mRight=r;
	mBottom=b;
	mTop=t;
	mNear=nearz;
	mFar=farz;
	mProjection=GLViewport::ORTHO;
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
	mLeft=l;
	mRight=r;
	mBottom=b;
	mTop=t;
	mNear=nearz;
	mFar=farz;
	mProjection=GLViewport::PERSPECTIVE;
}

GLViewport::ProjectionType GLViewport::getProjectionType()const
{
	return mProjection;
}

void GLViewport::getProjection(double& xmin,double& xmax,double& ymin,double& ymax,double& zmin,double& zmax)
{
	xmin=mLeft;
	xmax=mRight;
	ymin=mBottom;
	ymax=mTop;
	zmin=mNear;
	zmax=mFar;
}

void GLViewport::setZoomFactor(double val)
{
	mZoomFactor=val;
	issueGL();
}

double GLViewport::getZoomFactor()
{
	return mZoomFactor;
}

void GLViewport::setTranslation(double xval,double yval)
{
	mXTrans=xval;
	mYTrans=yval;
	issueGL();
}
void GLViewport::getTranslation(double& xval,double& yval)
{
	xval=mXTrans;
	yval=mYTrans;
}

void GLViewport::issueGL() const
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0, mWidth, mHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(mProjection==GLViewport::PERSPECTIVE){	
		glFrustum(mLeft*mZoomFactor, mRight*mZoomFactor, mBottom*mZoomFactor, mTop*mZoomFactor, mNear*mZoomFactor, mFar);
	}else{
		glOrtho(mLeft*mZoomFactor-mXTrans, mRight*mZoomFactor-mXTrans, mBottom*mZoomFactor-mYTrans, mTop*mZoomFactor-mYTrans, mNear*mZoomFactor, mFar);
	}
	glMatrixMode(GL_MODELVIEW);
}