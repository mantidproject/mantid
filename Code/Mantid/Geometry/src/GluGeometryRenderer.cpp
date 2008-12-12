#include "gts.h"
#include "MantidGeometry/GluGeometryRenderer.h"
#include "MantidGeometry/ObjComponent.h"
#include "MantidGeometry/Quat.h"
#include <climits>
#ifdef _WIN32
#include "windows.h"
#endif
#include "GL/gl.h"
#include "GL/glu.h"

namespace Mantid
{
	namespace Geometry
	{

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
			if(glIsList(iDisplaylistId)==GL_TRUE)
				glDeleteLists(iDisplaylistId,1);
		}

	    void GluGeometryRenderer::RenderSphere(V3D center,double radius)
		{
			if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
			{
				iDisplaylistId=glGenLists(1);
				//create glu sphere
				GLUquadricObj *qobj=gluNewQuadric();
				gluQuadricDrawStyle(qobj,GLU_FILL);
				gluQuadricNormals(qobj,GL_SMOOTH);
				glNewList(iDisplaylistId,GL_COMPILE_AND_EXECUTE); //Construct display list for object representation
				glPushMatrix();
				glTranslated(center[0],center[1],center[2]);
				gluSphere(qobj,radius,5,5);
				glPopMatrix();
				glEndList();
				gluDeleteQuadric(qobj);
				boolDisplaylistCreated=true;
			}else{
				glCallList(iDisplaylistId);
			}
		}
		void GluGeometryRenderer::RenderCube(V3D Point1,V3D Point2,V3D Point3,V3D Point4)
		{
			if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
			{
				//Calculate the points for the cube using 4 points.
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
				//calculate normals not done yet
				iDisplaylistId=glGenLists(1);
				glNewList(iDisplaylistId,GL_COMPILE_AND_EXECUTE); //Construct display list for object representation
				//first face
				glBegin(GL_QUADS);
				for(int i=0;i<6;i++){
					normal=(vertex[faceindex[i][0]]-vertex[faceindex[i][1]]).cross_prod((vertex[faceindex[i][0]]-vertex[faceindex[i][2]]));
					normal.normalize();
					glNormal3d(normal[0],normal[1],normal[2]);
					for(int j=0;j<4;j++)
						glVertex3d(vertex[faceindex[i][j]][0],vertex[faceindex[i][j]][1],vertex[faceindex[i][j]][2]);
				}
				glEnd();
				glEndList();
				boolDisplaylistCreated=true;
			}else{
				glCallList(iDisplaylistId);
			}
		}
		void GluGeometryRenderer::RenderCone(V3D center,V3D axis,double radius,double height)
		{
			if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
			{
				GLUquadricObj *qobj=gluNewQuadric();
				gluQuadricDrawStyle(qobj,GLU_FILL);
				gluQuadricNormals(qobj,GL_SMOOTH);
				iDisplaylistId=glGenLists(1);
				glNewList(iDisplaylistId,GL_COMPILE_AND_EXECUTE); //Construct display list for object representation
				glTranslated(center[0],center[1],center[2]);
				GLdouble mat[16];
				Quat rot(V3D(0,0,1),axis);
				rot.GLMatrix(mat);
				glMultMatrixd(mat);
				gluCylinder(qobj,0,radius,height,10,5);
				glTranslated(0.0,0.0,height);
				gluDisk(qobj,0,radius,10,1);
				glPopMatrix();
				glEndList();
				boolDisplaylistCreated=true;
			}else{
				glCallList(iDisplaylistId);
			}
		}
		void GluGeometryRenderer::RenderCylinder(V3D center,V3D axis,double radius,double height)
		{
			if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
			{
				iDisplaylistId=glGenLists(1);
				//calculate angle 
				//create glu cylinder
				GLUquadricObj *qobj=gluNewQuadric();
				gluQuadricDrawStyle(qobj,GLU_FILL);
				gluQuadricNormals(qobj,GL_SMOOTH);
				glNewList(iDisplaylistId,GL_COMPILE_AND_EXECUTE); //Construct display list for object representation
				glPushMatrix();
				glTranslated(center[0],center[1],center[2]);
				GLdouble mat[16];
				Quat rot(V3D(0,0,1),axis);
				rot.GLMatrix(mat);
				glMultMatrixd(mat);
				gluCylinder(qobj,radius,radius,height,10,5);
				gluDisk(qobj,0,radius,10,1);
				glTranslated(0.0,0.0,height);
				gluDisk(qobj,0,radius,10,1);
				glPopMatrix();
				glEndList();
				boolDisplaylistCreated=true;
			}else{
				glCallList(iDisplaylistId);
			}
		}

		/**
		* Render ObjComponent
		* @param ObjComp input to render
		*/
		void GluGeometryRenderer::Render(ObjComponent *ObjComp)
		{
			glPushMatrix();
			V3D pos  =ObjComp->getPos();
			Quat rot =ObjComp->getRotation();
			double rotGL[16];
			rot.GLMatrix(rotGL);
			glTranslated(pos[0],pos[1],pos[2]);
			glMultMatrixd(rotGL);
			ObjComp->drawObject();
			glPopMatrix();
		}

	}
}