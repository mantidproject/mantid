#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/OCGeometryHandler.h"
#include "MantidGeometry/Rendering/OCGeometryGenerator.h"
#include "MantidGeometry/Rendering/OCGeometryRenderer.h"

namespace Mantid
{
	namespace Geometry
	{
		OCGeometryHandler::OCGeometryHandler(IObjComponent *comp):GeometryHandler(comp)
		{
			Triangulator=NULL;
			Renderer    = new OCGeometryRenderer();
		}

		OCGeometryHandler::OCGeometryHandler(boost::shared_ptr<Object> obj):GeometryHandler(obj)
		{
			Triangulator=new OCGeometryGenerator(obj.get());
			Renderer    =new OCGeometryRenderer();
		}

		OCGeometryHandler::OCGeometryHandler(Object* obj):GeometryHandler(obj)
		{
			Triangulator=new OCGeometryGenerator(obj);
			Renderer    =new OCGeometryRenderer();
		}

		OCGeometryHandler::~OCGeometryHandler()
		{
			if(Triangulator!=NULL) delete Triangulator;
			if(Renderer    !=NULL) delete Renderer;
		}

		GeometryHandler* OCGeometryHandler::createInstance(IObjComponent *comp)
		{
			return new OCGeometryHandler(comp);
		}

		GeometryHandler* OCGeometryHandler::createInstance(boost::shared_ptr<Object> obj)
		{
			return new OCGeometryHandler(obj);
		}

		void OCGeometryHandler::Triangulate()
		{
			//Check whether Object is triangulated otherwise triangulate
			if(Obj!=NULL&&!boolTriangulated){
				Triangulator->Generate();
				boolTriangulated=true;
			}
		}

		void OCGeometryHandler::Render()
		{
			if(Obj!=NULL){
				if(boolTriangulated==false)	Triangulate();
				Renderer->Render(Triangulator->getObjectSurface());
			}else if(ObjComp!=NULL){
				Renderer->Render(ObjComp);
			}
		}

		void OCGeometryHandler::Initialize()
		{
			if(Obj!=NULL){
				if(boolTriangulated==false)	Triangulate();
				Renderer->Initialize(Triangulator->getObjectSurface());
			}else if(ObjComp!=NULL){
				Renderer->Initialize(ObjComp);
			}
		}

		int OCGeometryHandler::NumberOfTriangles()
		{
			if(Obj!=NULL)
			{
				if(boolTriangulated==false)	Triangulate();
				return Triangulator->getNumberOfTriangles();
			}
			else
			{
				return 0;
			}
		}

		int OCGeometryHandler::NumberOfPoints()
		{
			if(Obj!=NULL)
			{
				if(boolTriangulated==false)	Triangulate();
				return Triangulator->getNumberOfPoints();
			}
			else
			{
				return 0;
			}
		}

		double* OCGeometryHandler::getTriangleVertices()
		{
			if(Obj!=NULL)
			{
				if(boolTriangulated==false)	Triangulate();
				return Triangulator->getTriangleVertices();
			}
			else
			{
				return NULL;
			}
		}

		int*    OCGeometryHandler::getTriangleFaces()
		{
			if(Obj!=NULL)
			{
				if(boolTriangulated==false)	Triangulate();
				return Triangulator->getTriangleFaces();
			}
			else
			{
				return NULL;
			}
		}

	}
}
