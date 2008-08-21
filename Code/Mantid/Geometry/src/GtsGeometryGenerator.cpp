#include "MantidKernel/Logger.h"
#include <vector>
#include <math.h>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/Object.h"
#include "gts.h"
#include "MantidGeometry/GtsGeometryGenerator.h"
#include "MantidGeometry/SurfaceEvaluator.h"
#include "MantidGeometry/ObjectSurfaceEvaluator.h"


namespace Mantid
{

	namespace Geometry
	{

		Kernel::Logger& GtsGeometryGenerator::PLog(Kernel::Logger::get("GtsGeometryGenerator"));
		/**
		 * This is a mesh point scalar fill method for surface generation
		 * @param[out] f slice of data
		 * @param g  cartesian grid
		 * @param k  k level in third dimension
		 * @param data any data passed by gts isosurface, here its used to pass surfaceevaluator
		 */
		void distance (gdouble ** f, GtsCartesianGrid g, guint k, gpointer data)
		{
			gdouble x, y, z = g.z;
			guint i, j;
			
			for (i = 0, x = g.x; i < g.nx; i++, x += g.dx)
				for (j = 0, y = g.y; j < g.ny; j++, y += g.dy)
					f[i][j] = ((SurfaceEvaluator*)data)->evaluate(V3D(x,y,z));
		}
		/**
		 * Constructor
		 * @param obj input object
		 */
		GtsGeometryGenerator::GtsGeometryGenerator(const Object *obj):Obj(obj)
		{			
			ObjSurface=NULL;
		}

		/**
		 * Generate geometry, it uses gts isosurface tetra to generate surface triangles.
		 */
		void GtsGeometryGenerator::Generate()
		{
			if(ObjSurface==NULL){
				GtsSurface* gtsSurface = gts_surface_new (gts_surface_class (),
					gts_face_class(),
					gts_edge_class(),
					gts_vertex_class());
				int c = 0;
				GtsCartesianGrid g;
				gdouble iso;
				iso=0.0;
				double xmax,ymax,zmax,xmin,ymin,zmin;
				xmax=ymax=zmax=100000;
				xmin=ymin=zmin=-100000;
				Obj->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
				double frac=modf((xmax-xmin),(double*)&g.nx);
				frac=modf((ymax-ymin),(double*)&g.ny);
				frac=modf((zmax-zmin),(double*)&g.nz);
				g.nx=g.ny=g.nz=51.0;
				g.x = xmin; g.dx = (xmax-xmin)/(gdouble) (g.nx - 1);
				g.y = ymin; g.dy = (ymax-ymin)/(gdouble) (g.ny - 1);
				g.z = zmin; g.dz = (zmax-zmin)/(gdouble) (g.nz - 1);
				//Temporary fix for the mesh, To consider the ends of the mesh.
				g.x-=g.dx; g.y-=g.dy; g.z-=g.dz;
				g.nx+=2  ; g.ny+=2  ; g.nz+=2;
				//------------------
				ObjectSurfaceEvaluator *qse=new ObjectSurfaceEvaluator(Obj);
				gts_isosurface_tetra(gtsSurface,g,distance,qse,iso);
				ObjSurface=gtsSurface;
				gtsSurface=NULL;
			}
		}
		
		/**
		 * Destroy the surface generated for the object
		 */
		GtsGeometryGenerator::~GtsGeometryGenerator(){
			if(ObjSurface!=NULL)
			{				
				  gts_object_destroy (GTS_OBJECT (ObjSurface));	
			}
		}

		/**
		 * Returns the object surface, this should not be deleted by the calling method.
		 * @return ObjSurface
		 */
		GtsSurface* GtsGeometryGenerator::getObjectSurface(){
			if(ObjSurface==NULL) Generate();
			return ObjSurface;
		}

	}  // NAMESPACE Geometry

}  // NAMESPACE Mantid
