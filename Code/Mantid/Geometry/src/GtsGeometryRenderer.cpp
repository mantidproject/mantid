#include "gts.h"
#include "MantidGeometry/GtsGeometryRenderer.h"
#include "MantidGeometry/ObjComponent.h"
#ifdef _WIN32
#include "windows.h"
#endif
#include "GL/gl.h"

namespace Mantid
{
	namespace Geometry
	{

		/**
		* This is private method used for traversing gts surface and using opengl to render
		* @param t Triangle of the surface
		* @param data information related to the GTS (usually GTS surface pointer)
		*/
		static void gts_surface_opengl_render (GtsTriangle * t, gpointer * data)
		{	
			GtsVertex *v1,*v2,*v3;
			double    normalx,normaly,normalz;
			gts_triangle_vertices(t,&v1,&v2,&v3);
			gts_triangle_normal(t,&normalx,&normaly,&normalz);
			glNormal3d(normalx,normaly,normalz);
			glVertex3f(v1->p.x,v1->p.y,v1->p.z);
			glVertex3f(v2->p.x,v2->p.y,v2->p.z);
			glVertex3f(v3->p.x,v3->p.y,v3->p.z);
		}

		/**
		* Constructor
		* Generated the display list
		*/
		GtsGeometryRenderer::GtsGeometryRenderer()
		{
			//iDisplaylistId= glGenLists(1);
			boolDisplaylistCreated=false;
		}

		/**
		* Destructor
		* Deletes the display list
		*/
		GtsGeometryRenderer::~GtsGeometryRenderer()
		{
			//glDeleteLists(iDisplaylistId,1);
		}

		/**
		* Renders Object surface given as GtsSurface
		* @param ObjSurf object's surface stored in GtsSurface
		*/
		void GtsGeometryRenderer::Render(GtsSurface *ObjSurf)
		{
			//if(!boolDisplaylistCreated){
				GtsFace * first = NULL;
				glBegin(GL_TRIANGLES);
				gts_surface_foreach_face(ObjSurf,(GtsFunc)gts_surface_opengl_render,&first);
				glEnd();
				//gts_surface_write_vtk_file(ObjSurf,"C:\\ObjSurf.vtk");
			//}else{
			//	glCallList(iDisplaylistId);
			//}
		}

		/**
		* Render ObjComponent
		* @param ObjComp input to render
		*/
		void GtsGeometryRenderer::Render(ObjComponent *ObjComp)
		{
//			if(!boolDisplaylistCreated){
				glPushMatrix();
				V3D pos  =ObjComp->getPos();
				Quat rot =ObjComp->getRotation();
				double rotGL[16];
				rot.GLMatrix(rotGL);
				glMultMatrixd(rotGL);
				glTranslated(pos[0],pos[1],pos[2]);
				ObjComp->drawObject();
				glPopMatrix();
//			}else{
//				glCallList(iDisplaylistId);
//			}
		}

		/**
		* Initialze the object surface for rendering, this will create the display list helps in faster rendering
		* @param ObjSurf input to create display list
		*/
		void GtsGeometryRenderer::Initialize(GtsSurface *ObjSurf)
		{
//			if(!boolDisplaylistCreated){
				GtsFace * first = NULL;
//				glNewList(iDisplaylistId,GL_COMPILE); //Construct display list for object representation    
				glBegin(GL_TRIANGLES);
				gts_surface_foreach_face(ObjSurf,(GtsFunc)gts_surface_opengl_render,&first);
				glEnd();
//				glEndList();
				boolDisplaylistCreated=true;
				//gts_surface_write_vtk_file(ObjSurf,"C:\\ObjSurf.vtk");
//			}
		}

		/**
		* Initializes creates a display for the input ObjComponent
		* @param ObjComp input object component for creating display list
		*/
		void GtsGeometryRenderer::Initialize(ObjComponent *ObjComp)
		{
//			if(!boolDisplaylistCreated){
//				glNewList(iDisplaylistId,GL_COMPILE);
				glPushMatrix();
				V3D pos  =ObjComp->getPos();
				Quat rot =ObjComp->getRotation();
				double rotGL[16];
				rot.GLMatrix(rotGL);
				glMultMatrixd(rotGL);
				glTranslated(pos[0],pos[1],pos[2]);
				ObjComp->drawObject();
				glPopMatrix();
//				glEndList();
				boolDisplaylistCreated=true;
//			}
		}
	}
}