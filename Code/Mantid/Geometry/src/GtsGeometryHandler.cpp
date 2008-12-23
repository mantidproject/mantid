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
	}
}
