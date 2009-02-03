#include "gts.h"
#include "MantidGeometry/Object.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/GeometryHandler.h"
#include "MantidGeometry/GtsGeometryHandler.h"
#include "MantidGeometry/GtsGeometryGenerator.h"
#include "MantidGeometry/GtsGeometryRenderer.h"

namespace Mantid
{
	namespace Geometry
	{
	   /*
		* This is private method used for traversing gts surface and retriving points
		* @param p Points of the surface
		* @param data user information, data points pointer
		*/
		static void mantid_gts_surface_vertex_point (GtsPoint * p, gpointer * data)
		{
			double* pts = (double*)*data;
			int index   = GPOINTER_TO_UINT (GTS_OBJECT (p)->reserved);
			pts[index*3]   = p->x;
			pts[index*3+1] = p->y;
			pts[index*3+2] = p->z;
		}

	   /*
		* This is private method used for traversing gts surface and retriving triangle indexes
		* @param t triangle faces of the surface
		* @param data user information, data[0] face index pointer and data[1] index
		*/
		static void mantid_gts_surface_vertex_face (GtsTriangle * t, gpointer * data)
		{
			int* pts  = (int*) data[0];
			int index = static_cast<int>(reinterpret_cast<long>(data[1]));
			GUINT_TO_POINTER ((*((guint *) data[1]))++);
			GtsVertex * v1, * v2, * v3;
			gts_triangle_vertices (t, &v1, &v2, &v3);
			pts[index*3]   = GPOINTER_TO_UINT (GTS_OBJECT (v1)->reserved);
			pts[index*3+1] = GPOINTER_TO_UINT (GTS_OBJECT (v2)->reserved);
			pts[index*3+2] = GPOINTER_TO_UINT (GTS_OBJECT (v3)->reserved);
		}

		GtsGeometryHandler::GtsGeometryHandler(IObjComponent *comp):GeometryHandler(comp)
		{
			Triangulator=NULL;
			Renderer    = new GtsGeometryRenderer();
		}

		GtsGeometryHandler::GtsGeometryHandler(boost::shared_ptr<Object> obj):GeometryHandler(obj)
		{
			Triangulator=new GtsGeometryGenerator(obj.get());
			Renderer    =new GtsGeometryRenderer();
		}

		GtsGeometryHandler::GtsGeometryHandler(Object* obj):GeometryHandler(obj)
		{
			Triangulator=new GtsGeometryGenerator(obj);
			Renderer    =new GtsGeometryRenderer();
		}

		GtsGeometryHandler::~GtsGeometryHandler()
		{
			if(Triangulator!=NULL) delete Triangulator;
			if(Renderer    !=NULL) delete Renderer;
		}

		GeometryHandler* GtsGeometryHandler::createInstance(IObjComponent *comp)
		{
			return new GtsGeometryHandler(comp);
		}

		GeometryHandler* GtsGeometryHandler::createInstance(boost::shared_ptr<Object> obj)
		{
			return new GtsGeometryHandler(obj);
		}

		void GtsGeometryHandler::Triangulate()
		{
			//Check whether Object is triangulated otherwise triangulate
			if(Obj!=NULL&&!boolTriangulated){
				Triangulator->Generate();
				boolTriangulated=true;
			}
		}

		void GtsGeometryHandler::Render()
		{
			if(Obj!=NULL){
				if(boolTriangulated==false)	Triangulate();
				Renderer->Render(Triangulator->getObjectSurface());
			}else if(ObjComp!=NULL){
				Renderer->Render(ObjComp);
			}
		}

		void GtsGeometryHandler::Initialize()
		{
			if(Obj!=NULL){
				if(boolTriangulated==false)	Triangulate();
				Renderer->Initialize(Triangulator->getObjectSurface());
			}else if(ObjComp!=NULL){
				Renderer->Initialize(ObjComp);
			}
		}

		int GtsGeometryHandler::NumberOfTriangles()
		{
			if(Obj!=NULL)
			{
				if(boolTriangulated==false) Triangulate();
				return gts_surface_face_number(Triangulator->getObjectSurface());
			}
			else
				return 0;
		}
		
		int GtsGeometryHandler::NumberOfPoints()
		{
			if(Obj!=NULL)
			{
				if(boolTriangulated==false) Triangulate();
				return gts_surface_vertex_number(Triangulator->getObjectSurface());
			}
			else
				return 0;
		}

		double* GtsGeometryHandler::getTriangleVertices()
		{
			double* points=NULL;
			int nPts=this->NumberOfPoints();
			if(nPts>0)
			{
				points=new double[nPts*3];
				gts_surface_foreach_vertex(Triangulator->getObjectSurface(),(GtsFunc)mantid_gts_surface_vertex_point,&points);
			}
			return points;
		}

		int*    GtsGeometryHandler::getTriangleFaces()
		{
			int* faces=NULL;
			int nFaces=this->NumberOfTriangles();
			if(nFaces>0)
			{
				faces=new int[nFaces*3];
				guint n = 0;
				gpointer data[2];
				data[0]=faces;
				data[1]=&n;
				gts_surface_foreach_vertex(Triangulator->getObjectSurface(),(GtsFunc)mantid_gts_surface_vertex_face,data);
			}
			return faces;
		}
	}
}
