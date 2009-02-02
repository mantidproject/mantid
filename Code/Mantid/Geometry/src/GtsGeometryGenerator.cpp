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
		 * This method assigns the point index
		 */ 
		void mantid_gts_assign_point_index(GtsPoint * p, gpointer * data)
		{
			GTS_OBJECT (p)->reserved = GUINT_TO_POINTER ((*((guint *) data))++);
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
				g.nx=g.ny=g.nz=11;
				g.x = xmin; g.dx = (xmax-xmin)/(gdouble) (g.nx - 1);
				g.y = ymin; g.dy = (ymax-ymin)/(gdouble) (g.ny - 1);
				g.z = zmin; g.dz = (zmax-zmin)/(gdouble) (g.nz - 1);
				//Temporary fix for the mesh, To consider the ends of the mesh.
				g.x-=g.dx; g.y-=g.dy; g.z-=g.dz;
				g.nx+=2  ; g.ny+=2  ; g.nz+=2;
				//------------------
				ObjectSurfaceEvaluator *qse=new ObjectSurfaceEvaluator(Obj);
				//gts_isosurface_tetra_bcl(gtsSurface,g,distance,qse,iso);
				//gts_isosurface_tetra(gtsSurface,g,distance,qse,iso);
				gts_isosurface_cartesian(gtsSurface,g,distance,qse,iso);
				//Try to coarse the surface
				double cost=100;
				GtsVolumeOptimizedParams params = { 0.5, 0.5, 0};//1e-10 };
				gts_surface_coarsen (gtsSurface, (GtsKeyFunc) gts_volume_optimized_cost, &params,
					(GtsCoarsenFunc) gts_volume_optimized_vertex, &params,
					(GtsStopFunc) gts_coarsen_stop_number, &cost,
					 90.0*M_PI/180.0);
				////gts_surface_write_vtk_file(gtsSurface,"C:\\ObjSurf.vtk");
				//gdouble x1=xmin;
				//gdouble y1=ymin;
				//gdouble z1=zmin;
				//gdouble x2=xmax;
				//gdouble y2=ymax;
				//gdouble z2=zmax;

				//GtsVertex * v0 = gts_vertex_new (gtsSurface->vertex_class, x1, y1, z1);
				//GtsVertex * v1 = gts_vertex_new (gtsSurface->vertex_class, x1, y1, z2);
				//GtsVertex * v2 = gts_vertex_new (gtsSurface->vertex_class, x1, y2, z2);
				//GtsVertex * v3 = gts_vertex_new (gtsSurface->vertex_class, x1, y2, z1);
				//GtsVertex * v4 = gts_vertex_new (gtsSurface->vertex_class, x2, y1, z1);
				//GtsVertex * v5 = gts_vertex_new (gtsSurface->vertex_class, x2, y1, z2);
				//GtsVertex * v6 = gts_vertex_new (gtsSurface->vertex_class, x2, y2, z2);
				//GtsVertex * v7 = gts_vertex_new (gtsSurface->vertex_class, x2, y2, z1);

				//GtsEdge * e1 = gts_edge_new (gtsSurface->edge_class, v0, v1);
				//GtsEdge * e2 = gts_edge_new (gtsSurface->edge_class, v1, v2);
				//GtsEdge * e3 = gts_edge_new (gtsSurface->edge_class, v2, v3);
				//GtsEdge * e4 = gts_edge_new (gtsSurface->edge_class, v3, v0);
				//GtsEdge * e5 = gts_edge_new (gtsSurface->edge_class, v0, v2);

				//GtsEdge * e6 = gts_edge_new (gtsSurface->edge_class, v4, v5);
				//GtsEdge * e7 = gts_edge_new (gtsSurface->edge_class, v5, v6);
				//GtsEdge * e8 = gts_edge_new (gtsSurface->edge_class, v6, v7);
				//GtsEdge * e9 = gts_edge_new (gtsSurface->edge_class, v7, v4);
				//GtsEdge * e10 = gts_edge_new (gtsSurface->edge_class, v4, v6);

				//GtsEdge * e11 = gts_edge_new (gtsSurface->edge_class, v3, v7);
				//GtsEdge * e12 = gts_edge_new (gtsSurface->edge_class, v2, v6);
				//GtsEdge * e13 = gts_edge_new (gtsSurface->edge_class, v1, v5);
				//GtsEdge * e14 = gts_edge_new (gtsSurface->edge_class, v0, v4);

				//GtsEdge * e15 = gts_edge_new (gtsSurface->edge_class, v1, v6);
				//GtsEdge * e16 = gts_edge_new (gtsSurface->edge_class, v2, v7);
				//GtsEdge * e17 = gts_edge_new (gtsSurface->edge_class, v3, v4);
				//GtsEdge * e18 = gts_edge_new (gtsSurface->edge_class, v0, v5);

				//GtsFaceClass * klass = gts_face_class ();

				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e1, e2, e5));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e5, e3, e4));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e6, e10, e7));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e10, e9, e8));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e2, e15, e12));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e15, e13, e7));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e3, e16, e11));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e16, e12, e8));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e17, e14, e4));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e17, e11, e9));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e18, e13, e1));
				//gts_surface_add_face (gtsSurface, gts_face_new (klass, e18, e14, e6));
				int val=0;
				gts_surface_foreach_vertex (gtsSurface, (GtsFunc) mantid_gts_assign_point_index,&val);
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
