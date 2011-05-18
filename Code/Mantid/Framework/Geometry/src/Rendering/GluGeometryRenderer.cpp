#include "MantidGeometry/Rendering/GluGeometryRenderer.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Quat.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include <climits>

namespace Mantid
{
  namespace Geometry
  {

    Kernel::Logger& GluGeometryRenderer::PLog(Kernel::Logger::get("GluGeometryRenderer"));

/**
 * Constructor
 * Generated the display list
 */
    GluGeometryRenderer::GluGeometryRenderer()
    {
      boolDisplaylistCreated=false;
      iDisplaylistId=UINT_MAX;
    }

/**
 * Destructor
 * Deletes the display list
 */
    GluGeometryRenderer::~GluGeometryRenderer()
    {
      if(boolDisplaylistCreated && iDisplaylistId!=0 && glIsList(iDisplaylistId)==GL_TRUE)
	glDeleteLists(iDisplaylistId,1);
    }

    void GluGeometryRenderer::RenderSphere(const V3D& center,double radius)
    {
      if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
      {
	while(glGetError() != GL_NO_ERROR);
	iDisplaylistId=glGenLists(1);
	glNewList(iDisplaylistId,GL_COMPILE); //Construct display list for object representation
	CreateSphere(center,radius);
	glEndList();
	boolDisplaylistCreated=true;
	mErrorCode=glGetError();
      }else if(mErrorCode == GL_NO_ERROR){
	glCallList(iDisplaylistId);
      }else{
	CreateSphere(center,radius);
      }
    }


    void GluGeometryRenderer::RenderCube(const V3D& Point1,const V3D& Point2,const V3D& Point3,const V3D& Point4)
    {
      if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
      {
	while(glGetError() != GL_NO_ERROR );
	iDisplaylistId=glGenLists(1);
	glNewList(iDisplaylistId,GL_COMPILE); //Construct display list for object representation
	CreateCube(Point1,Point2,Point3,Point4);
	glEndList();
	mErrorCode=glGetError();
	boolDisplaylistCreated=true;
      }else if(mErrorCode == GL_NO_ERROR){
	glCallList(iDisplaylistId);
      }else{
	CreateCube(Point1,Point2,Point3,Point4);
      }
    }


    void GluGeometryRenderer::RenderCone(const V3D& center,const V3D& axis,double radius,double height)
    {
      if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
      {
	while(glGetError() != GL_NO_ERROR);
	iDisplaylistId=glGenLists(1);
	glNewList(iDisplaylistId,GL_COMPILE); //Construct display list for object representation
	CreateCone(center,axis,radius,height);
	glEndList();
	mErrorCode=glGetError();
	boolDisplaylistCreated=true;
      }else if(mErrorCode == GL_NO_ERROR){
	glCallList(iDisplaylistId);
      }else {
	CreateCone(center,axis,radius,height);
      }
    }


    void GluGeometryRenderer::RenderCylinder(const V3D& center,const V3D& axis,double radius,double height)
    {
      if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
      {
	GLenum error;
	do{
	  error=glGetError();
	  if(error==GL_OUT_OF_MEMORY)
	  {
	    mErrorCode=GL_OUT_OF_MEMORY;
	    return;
	  }
	}while(error != GL_NO_ERROR);
	iDisplaylistId=glGenLists(1);
	glNewList(iDisplaylistId,GL_COMPILE); //Construct display list for object representation
	CreateCylinder(center,axis,radius,height);
	glEndList();
	mErrorCode=glGetError();
	boolDisplaylistCreated=true;
      }else if(mErrorCode == GL_NO_ERROR){
	glCallList(iDisplaylistId);
      }else{
	CreateCylinder(center,axis,radius,height);
      }
    }

    void GluGeometryRenderer::RenderSegmentedCylinder(const V3D& center,const V3D& axis,double radius,double height)
    {
      if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
      {
	while(glGetError() != GL_NO_ERROR);
	iDisplaylistId=glGenLists(1);
	glNewList(iDisplaylistId,GL_COMPILE); //Construct display list for object representation
	CreateSegmentedCylinder(center,axis,radius,height);
	glEndList();
	mErrorCode=glGetError();
	boolDisplaylistCreated=true;
      }else if(mErrorCode == GL_NO_ERROR){
	glCallList(iDisplaylistId);
      }else {
	CreateSegmentedCylinder(center,axis,radius,height);
      }
    }

/**
 * Render ObjComponent
 * @param ObjComp :: input to render
 */
    void GluGeometryRenderer::Render(IObjComponent *ObjComp) const
    {
      glPushMatrix();
      V3D pos  =ObjComp->getPos();
      Quat rot =ObjComp->getRotation();
      double rotGL[16];
      rot.GLMatrix(&rotGL[0]);
      glTranslated(pos[0],pos[1],pos[2]);
      glMultMatrixd(rotGL);
      ObjComp->drawObject();
      glPopMatrix();
    }

/**
 * Creates a GLU Sphere
 * @param center :: center of the sphere
 * @param radius :: radius of the sphere
 */
    void GluGeometryRenderer::CreateSphere(const V3D& center,double radius)
    {
      //create glu sphere
      GLUquadricObj *qobj=gluNewQuadric();
      gluQuadricDrawStyle(qobj,GLU_FILL);
      gluQuadricNormals(qobj,GL_SMOOTH);
      glPushMatrix();
      glTranslated(center[0],center[1],center[2]);
      gluSphere(qobj,radius, Geometry::Sphere::g_nslices, Geometry::Sphere::g_nstacks);
      glPopMatrix();
      gluDeleteQuadric(qobj);
    }

/**
 * Creates a Cube
 * @param Point1 :: first point of the cube
 * @param Point2 :: second point of the cube
 * @param Point3 :: thrid point of the cube
 * @param Point4 :: fourth point of the cube
 */
    void GluGeometryRenderer::CreateCube(const V3D& Point1,const V3D& Point2,const V3D& Point3,const V3D& Point4)
    {
      V3D vec0=Point1;
      V3D vec1=Point2-Point1;
      V3D vec2=Point3-Point1;
      V3D vec3=Point4-Point1;
      V3D vertex[8];
      vertex[0]=vec0;
      vertex[1]=vec0+vec3;
      vertex[2]=vec0+vec3+vec1;
      vertex[3]=vec0+vec1;
      vertex[4]=vec0+vec2;
      vertex[5]=vec0+vec2+vec3;
      vertex[6]=vec0+vec2+vec3+vec1;
      vertex[7]=vec0+vec1+vec2;
      //int faceindex[6][4]={{0,1,2,3},{0,3,7,4},{3,2,6,7},{2,1,5,6},{0,4,5,1},{4,7,6,5}};
      //int faceindex[6][4]={{0,3,2,1},{0,4,7,3},{3,7,6,2},{2,6,5,1},{0,1,5,4},{4,5,6,7}};
      int faceindex[6][4]={
	{0,1,2,3}, //top
	{0,3,7,4}, //left
	{3,2,6,7}, //back
	{2,1,5,6}, //right
	{0,4,5,1}, //front
	{4,7,6,5}, //bottom
      };
      V3D normal;
      //first face
      glBegin(GL_QUADS);
      for(int i=0;i<6;i++){
	normal=(vertex[faceindex[i][0]]-vertex[faceindex[i][1]]).cross_prod((vertex[faceindex[i][0]]-vertex[faceindex[i][2]]));
	normal.normalize();
	glNormal3d(normal[0],normal[1],normal[2]);
	for(int j=0;j<4;j++)
	{
	  int ij = faceindex[i][j];
	  if (ij == 0) glTexCoord2i(0, 0);
	  if (ij == 1) glTexCoord2i(1, 0);
	  if (ij == 2) glTexCoord2i(1, 1);
	  if (ij == 3) glTexCoord2i(0, 1);
	  if (ij == 4) glTexCoord2i(0, 0);
	  if (ij == 5) glTexCoord2i(1, 0);
	  if (ij == 6) glTexCoord2i(1, 1);
	  if (ij == 7) glTexCoord2i(0, 1);
	  glVertex3d(vertex[ij][0],vertex[ij][1],vertex[ij][2]);
	}
      }
      glEnd();
    }

/**
 * Creates a Cone
 * @param center :: center of the cone
 * @param axis ::   axis of the cone
 * @param radius :: radius of the cone
 * @param height :: height of the cone
 */
    void GluGeometryRenderer::CreateCone(const V3D& center,const V3D& axis,double radius,double height)
    {
      glPushMatrix();
      GLUquadricObj *qobj=gluNewQuadric();
      gluQuadricDrawStyle(qobj,GLU_FILL);
      gluQuadricNormals(qobj,GL_SMOOTH);
      glTranslated(center[0],center[1],center[2]);
      GLdouble mat[16];
      V3D unit(0,0,1);
      Quat rot(unit,axis);
      rot.GLMatrix(&mat[0]);
      glMultMatrixd(mat);
      gluCylinder(qobj,0,radius,height,Geometry::Cone::g_nslices,Geometry::Cone::g_nstacks);
      glTranslated(0.0,0.0,height);
      gluDisk(qobj,0,radius,Geometry::Cone::g_nslices,1);
      glPopMatrix();
    }

/**
 * Create a Cylinder
 * @param center :: center of the cylinder
 * @param axis ::  axis of the cylinder
 * @param radius :: radius of the cylinder
 * @param height :: height of the cylinder
 */
    void GluGeometryRenderer::CreateCylinder(const V3D& center,const V3D& axis,double radius,double height)
    {
      GLUquadricObj *qobj=gluNewQuadric();
      gluQuadricDrawStyle(qobj,GLU_FILL);
      gluQuadricNormals(qobj,GL_SMOOTH);
      gluQuadricTexture(qobj,true);
      glPushMatrix();
      glTranslated(center[0],center[1],center[2]);
      GLdouble mat[16];
      V3D unit(0,0,1);
      Quat rot(unit,axis);
      rot.GLMatrix(&mat[0]);
      glMultMatrixd(mat);
      gluCylinder(qobj,radius,radius,height, Cylinder::g_nslices, Cylinder::g_nstacks);
      gluQuadricTexture(qobj,false);
      gluDisk(qobj,0,radius,Cylinder::g_nslices, 1);
      glTranslated(0.0,0.0,height);
      gluDisk(qobj,0,radius,Cylinder::g_nslices, 1);
      glPopMatrix();
    }

    void GluGeometryRenderer::CreateSegmentedCylinder(const V3D& center,const V3D& axis,double radius,double height)
    {
      GLUquadricObj *qobj=gluNewQuadric();
      gluQuadricDrawStyle(qobj,GLU_FILL);
      gluQuadricNormals(qobj,GL_SMOOTH);
      gluQuadricTexture(qobj,true);
      glPushMatrix();
      glTranslated(center[0],center[1],center[2]);
      GLdouble mat[16];
      V3D unit(0,0,1);
      Quat rot(unit,axis);
      rot.GLMatrix(&mat[0]);
      glMultMatrixd(mat);
      gluCylinder(qobj,radius,radius,height, Cylinder::g_nslices, 1);
      gluQuadricTexture(qobj,false);
      gluDisk(qobj,0,radius,Cylinder::g_nslices, 1);
      glTranslated(0.0,0.0,height);
      gluDisk(qobj,0,radius,Cylinder::g_nslices, 1);
      glPopMatrix();
    }
  }
}
