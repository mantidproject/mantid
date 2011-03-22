//#include "gts.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/CacheGeometryHandler.h"
#include "MantidGeometry/Rendering/CacheGeometryRenderer.h"
#include "MantidGeometry/Rendering/CacheGeometryGenerator.h"

namespace Mantid
{
	namespace Geometry
	{
		Kernel::Logger& CacheGeometryHandler::PLog(Kernel::Logger::get("CacheGeometryHandler"));
		CacheGeometryHandler::CacheGeometryHandler(IObjComponent *comp):GeometryHandler(comp)
		{
			Triangulator=NULL;
			Renderer    = new CacheGeometryRenderer();
		}

		CacheGeometryHandler::CacheGeometryHandler(boost::shared_ptr<Object> obj):GeometryHandler(obj)
		{
			Triangulator=new CacheGeometryGenerator(obj.get());
			Renderer    =new CacheGeometryRenderer();
		}

		CacheGeometryHandler::CacheGeometryHandler(Object* obj):GeometryHandler(obj)
		{
			Triangulator=new CacheGeometryGenerator(obj);
			Renderer    =new CacheGeometryRenderer();
		}

		CacheGeometryHandler::~CacheGeometryHandler()
		{
			if(Triangulator!=NULL) delete Triangulator;
			if(Renderer    !=NULL) delete Renderer;
		}

		GeometryHandler* CacheGeometryHandler::createInstance(IObjComponent *comp)
		{
			return new CacheGeometryHandler(comp);
		}

		GeometryHandler* CacheGeometryHandler::createInstance(boost::shared_ptr<Object> obj)
		{
			return new CacheGeometryHandler(obj);
		}

		void CacheGeometryHandler::Triangulate()
		{
			//Check whether Object is triangulated otherwise triangulate
			if(Obj!=NULL&&!boolTriangulated){
				Triangulator->Generate();
				boolTriangulated=true;
			}
		}

		void CacheGeometryHandler::Render()
		{
			if(Obj!=NULL){
				if(boolTriangulated==false)	Triangulate();
				Renderer->Render(Triangulator->getNumberOfPoints(),Triangulator->getNumberOfTriangles(),Triangulator->getTriangleVertices(),Triangulator->getTriangleFaces());
			}else if(ObjComp!=NULL){
				Renderer->Render(ObjComp);
			}
		}

		void CacheGeometryHandler::Initialize()
		{
			if(Obj!=NULL){
				Obj->updateGeometryHandler();
				if(boolTriangulated==false)	Triangulate();
				Renderer->Initialize(Triangulator->getNumberOfPoints(),Triangulator->getNumberOfTriangles(),Triangulator->getTriangleVertices(),Triangulator->getTriangleFaces());
			}else if(ObjComp!=NULL){
				Renderer->Initialize(ObjComp);
			}
		}

		int CacheGeometryHandler::NumberOfTriangles()
		{
			if(Obj!=NULL)
			{
				Obj->updateGeometryHandler();
				if(boolTriangulated==false)	Triangulate();
				return Triangulator->getNumberOfTriangles();
			}
			else
			{
				return 0;
			}
		}

		int CacheGeometryHandler::NumberOfPoints()
		{
			if(Obj!=NULL)
			{
				Obj->updateGeometryHandler();
				if(boolTriangulated==false)	Triangulate();
				return Triangulator->getNumberOfPoints();
			}
			else
			{
				return 0;
			}
		}

		double* CacheGeometryHandler::getTriangleVertices()
		{
			if(Obj!=NULL)
			{
				Obj->updateGeometryHandler();
				if(boolTriangulated==false)	Triangulate();
				return Triangulator->getTriangleVertices();
			}
			else
			{
				return NULL;
			}
		}

		int*    CacheGeometryHandler::getTriangleFaces()
		{
			if(Obj!=NULL)
			{
				Obj->updateGeometryHandler();
				if(boolTriangulated==false)	Triangulate();
				return Triangulator->getTriangleFaces();
			}
			else
			{
				return NULL;
			}
		}

		/**
		Sets the geometry cache using the triangulation information provided
		@param noPts :: the number of points
		@param noFaces :: the number of faces
		@param pts :: a double array of the points
		@param faces :: an int array of the faces
		*/
		void CacheGeometryHandler::setGeometryCache(int noPts,int noFaces,double* pts,int* faces)
		{
			Triangulator->setGeometryCache(noPts,noFaces,pts,faces);
			boolTriangulated=true;
		}
	}
}
