//#include "gts.h"
#include "MantidGeometry/Rendering/CacheGeometryRenderer.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
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

		Kernel::Logger& CacheGeometryRenderer::PLog(Kernel::Logger::get("CacheGeometryRenderer"));
		/**
		* Constructor
		* Generated the display list
		*/
		CacheGeometryRenderer::CacheGeometryRenderer()
		{
			boolDisplaylistCreated=false;
			iDisplaylistId=UINT_MAX;
		}

		/**
		* Destructor
		* Deletes the display list
		*/
		CacheGeometryRenderer::~CacheGeometryRenderer()
		{
			if(boolDisplaylistCreated && glIsList(iDisplaylistId)==GL_TRUE)
				glDeleteLists(iDisplaylistId,1);
		}

		/**
		* Render ObjComponent
		* @param ObjComp input to render
		*/
		void CacheGeometryRenderer::Render(IObjComponent *ObjComp) const
		{
			glPushMatrix();
			V3D pos  =ObjComp->getPos();
			Quat rot =ObjComp->getRotation();
			double rotGL[16];
			rot.GLMatrix(&rotGL[0]);
			glTranslated(pos[0],pos[1],pos[2]);
			glMultMatrixd(rotGL);
			V3D scaleFactor=ObjComp->getScaleFactor();
			glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);
			ObjComp->drawObject();
			glPopMatrix();
		}

		void CacheGeometryRenderer::Render(int noPts,int noFaces,double* points,int* faces) const
		{
      (void) noPts; (void) noFaces; (void) points; (void) faces; //Avoid compiler warning
			glCallList(iDisplaylistId);
		}

		void CacheGeometryRenderer::Initialize(int noPts,int noFaces,double* points,int* faces)
		{
      (void) noPts; //Avoid compiler warning
			if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
			{
				iDisplaylistId=glGenLists(1);
				glNewList(iDisplaylistId,GL_COMPILE); //Construct display list for object representation
				glBegin(GL_TRIANGLES);
				int index1,index2,index3;
				V3D normal;
				for(int i=0;i<noFaces;i++)
				{
					index1=faces[i*3]*3;
					index2=faces[i*3+1]*3;
					index3=faces[i*3+2]*3;
					//Calculate normal and normalize
					V3D v1(points[index1],points[index1+1],points[index1+2]);
					V3D v2(points[index2],points[index2+1],points[index2+2]);
					V3D v3(points[index3],points[index3+1],points[index3+2]);
					normal=(v1-v2).cross_prod(v2-v3);
					normal.normalize();
					glNormal3d(normal[0],normal[1],normal[2]);
					glVertex3dv(points+index1);
					glVertex3dv(points+index2);
					glVertex3dv(points+index3);
				}
				glEnd();
				glEndList();
				boolDisplaylistCreated=true;
			}
		}

		void CacheGeometryRenderer::Initialize(IObjComponent *ObjComp)
		{
			glPushMatrix();
			V3D pos  =ObjComp->getPos();
			Quat rot =ObjComp->getRotation();
			double rotGL[16];
			rot.GLMatrix(&rotGL[0]);
			glTranslated(pos[0],pos[1],pos[2]);
			glMultMatrixd(rotGL);
			V3D scaleFactor=ObjComp->getScaleFactor();
			glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);
			ObjComp->drawObject();
			glPopMatrix();
		}


	}
}
