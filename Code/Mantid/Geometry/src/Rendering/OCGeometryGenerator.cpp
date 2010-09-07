#include "MantidGeometry/Rendering/OCGeometryGenerator.h"
#include "MantidKernel/Logger.h"
#include <vector>
#include <cmath>
#include <climits> // Needed for g++4.4 on Mac with OpenCASCADE 6.3.0
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Torus.h"
#include "MantidGeometry/Objects/Rules.h"


#include "MantidGeometry/Rendering/OpenCascadeConfig.h"
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <StdFail_NotDone.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepMesh.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>

namespace Mantid
{

	namespace Geometry
	{
		Kernel::Logger& OCGeometryGenerator::PLog(Kernel::Logger::get("OCGeometryGenerator"));
		/**
		 * Constructor
		 * @param obj input object
		 */
		OCGeometryGenerator::OCGeometryGenerator(const Object *obj):Obj(obj)
		{
			ObjSurface=NULL;
		}

		/**
		 * Generate geometry, it uses OpenCascade to generate surface triangles.
		 */
		void OCGeometryGenerator::Generate()
		{
			if(ObjSurface==NULL){
				AnalyzeObject();
			}
		}

		/**
		 * Destroy the surface generated for the object
		 */
		OCGeometryGenerator::~OCGeometryGenerator(){
			if(ObjSurface!=NULL)
			{
				delete ObjSurface;
			}
		}

		/**
		 * Returns the shape generated.
		 */
		TopoDS_Shape* OCGeometryGenerator::getObjectSurface()
		{
			return ObjSurface;
		}

		/**
		 * Analyzes the rule tree in object and creates a Topology Shape
		 */
		void OCGeometryGenerator::AnalyzeObject()
		{
			if(Obj!=NULL) //If object exists
			{
				//Get the top rule tree in Obj
				const Rule* top=Obj->topRule();
				if(top==NULL)
				{
					ObjSurface=new TopoDS_Shape();
					return;
				}
				//Traverse through Rule
				TopoDS_Shape Result=AnalyzeRule((Rule*)top);
				try{					
					ObjSurface=new TopoDS_Shape(Result);
					BRepMesh::Mesh(Result,0.001);
				}
				catch(StdFail_NotDone)
				{
					PLog.error("Cannot build the geometry. Check the geometry definition");
				}

			}
		}
		/**
		 * Analyze intersection
     * \return the resulting TopoDS_Shape
		 */
		TopoDS_Shape OCGeometryGenerator::AnalyzeRule(Intersection* rule)
		{
			TopoDS_Shape left=AnalyzeRule(rule->leaf(0));
			TopoDS_Shape right=AnalyzeRule(rule->leaf(1));
			TopoDS_Shape Result=BRepAlgoAPI_Common(left,right);
			return Result;
		}
		TopoDS_Shape OCGeometryGenerator::AnalyzeRule(Union* rule)
		{
			TopoDS_Shape left=AnalyzeRule(rule->leaf(0));
			TopoDS_Shape right=AnalyzeRule(rule->leaf(1));
			TopoDS_Shape Result=BRepAlgoAPI_Fuse(left,right);
			return Result;
		}
		TopoDS_Shape OCGeometryGenerator::AnalyzeRule(SurfPoint* rule)
		{
			//Check for individual type of surfaces
			Surface* surf=rule->getKey();
			TopoDS_Shape Result=CreateShape(surf,rule->getSign());
			if(rule->getSign()>0&&surf->className()!="Plane") Result.Complement();
			return Result;
		}
		TopoDS_Shape OCGeometryGenerator::AnalyzeRule(CompGrp* rule)
		{
			TopoDS_Shape Result=AnalyzeRule(rule->leaf(0));
			Result.Complement();
			return 	Result;
		}
		TopoDS_Shape OCGeometryGenerator::AnalyzeRule(CompObj* rule)
		{
			Object* obj=rule->getObj();
			TopoDS_Shape Result=AnalyzeRule((Rule*)obj->topRule());
			Result.Complement();
			return 	Result;
		}
		TopoDS_Shape OCGeometryGenerator::AnalyzeRule(BoolValue* rule)
		{
			return TopoDS_Shape();
		}
		TopoDS_Shape OCGeometryGenerator::AnalyzeRule(Rule* rule)
		{
			if(rule==NULL)return TopoDS_Shape();
			if(rule->className()=="Intersection"){
				return AnalyzeRule((Intersection*)rule);
			}else if(rule->className()=="Union"){
				return AnalyzeRule((Union*)rule);
			}else if(rule->className()=="SurfPoint"){
				return AnalyzeRule((SurfPoint*)rule);
			}else if(rule->className()=="CompGrp"){
				return AnalyzeRule((CompGrp*)rule);
			}else if(rule->className()=="CompObj"){
				return AnalyzeRule((CompObj*)rule);
			}else if(rule->className()=="BoolValue"){
				return AnalyzeRule((BoolValue*)rule);
			}
			return TopoDS_Shape();
		}

		TopoDS_Shape OCGeometryGenerator::CreateShape(Surface* surf,int orientation)
		{
		    //Check for the type of the surface object
			if(surf->className()=="Sphere")
			{
				return CreateSphere((Sphere*)surf);
			}
			else if(surf->className()=="Cone")
			{
				return CreateCone((Cone*)surf);
			}
			else if(surf->className()=="Cylinder")
			{
				return CreateCylinder((Cylinder*)surf);
			}
			else if(surf->className()=="Plane")
			{
				return CreatePlane((Plane*)surf,orientation);
			}
			else if(surf->className()=="Torus")
			{
				return CreateTorus((Torus*)surf);
			}
			return TopoDS_Shape();
		}
		TopoDS_Shape OCGeometryGenerator::CreateSphere(Sphere* sphere)
		{
			//Get the center to Sphere, Radius
			V3D center=sphere->getCentre();
			double radius=sphere->getRadius();
			TopoDS_Shape shape=BRepPrimAPI_MakeSphere(radius).Shape();
			gp_Trsf T;
			gp_Vec v(center[0],center[1],center[2]);
			T.SetTranslation(v);
			BRepBuilderAPI_Transform move(T);
			move.Perform(shape);
			return move.Shape();
		}
		TopoDS_Shape OCGeometryGenerator::CreateCylinder(Cylinder* cylinder)
		{
			//Get the Cylinder Centre,Normal,Radius
			V3D center=cylinder->getCentre();
			V3D axis=cylinder->getNormal();
			double radius=cylinder->getRadius();
			center[0]=center[0]-axis[0]*500;
			center[1]=center[1]-axis[1]*500;
			center[2]=center[2]-axis[2]*500;
			gp_Ax2 gpA(gp_Pnt(center[0],center[1],center[2]),gp_Dir(axis[0],axis[1],axis[2]));
			TopoDS_Shape shape=BRepPrimAPI_MakeCylinder(gpA,radius,1000,2*M_PI).Solid();
			return shape;
		}
		TopoDS_Shape OCGeometryGenerator::CreateCone(Cone* cone)
		{
			//Get the Cone Centre Normal Radius
			V3D center=cone->getCentre();
			V3D axis=cone->getNormal();
			double angle=cone->getCosAngle();
			gp_Ax2 gpA(gp_Pnt(center[0],center[1],center[2]),gp_Dir(axis[0],axis[1],axis[2]));
			TopoDS_Shape shape=BRepPrimAPI_MakeCone(gpA,0,1000/tan(acos(angle*M_PI/180.0)),1000,2*M_PI).Shape();
			return shape;
		}
		TopoDS_Shape OCGeometryGenerator::CreatePlane(Plane* plane,int orientation)
		{
			//Get Plane normal and distance.
			V3D normal= plane->getNormal();
			double distance=plane->getDistance();
			//Find point closest to origin
			double t=distance/normal.norm2();
			//Create Half Space
			TopoDS_Shape Result;
			if(orientation>0){
				TopoDS_Face P=BRepBuilderAPI_MakeFace(gp_Pln(normal[0],normal[1],normal[2],-distance)).Face();
				Result=BRepPrimAPI_MakeHalfSpace(P,gp_Pnt(normal[0]*(1+t),normal[1]*(1+t),normal[2]*(1+t))).Solid();
			}else{
				TopoDS_Face P=BRepBuilderAPI_MakeFace(gp_Pln(normal[0],normal[1],normal[2],-distance)).Face();
				P.Reverse();
				Result=BRepPrimAPI_MakeHalfSpace(P,gp_Pnt(normal[0]*(1+t),normal[1]*(1+t),normal[2]*(1+t))).Solid();
			}
			//create a box
			gp_Pnt p(-1000.0,-1000.0,-1000.0);
			TopoDS_Shape world=BRepPrimAPI_MakeBox(p,2000.0,2000.0,2000.0).Shape();
			Result=BRepAlgoAPI_Common(world,Result);
			return Result;
		}
		TopoDS_Shape OCGeometryGenerator::CreateTorus(Torus*)
		{
//NOTE:: Not yet implemented
			return TopoDS_Shape();
		}

		int OCGeometryGenerator::getNumberOfTriangles()
		{
			int countFace=0;
			if(ObjSurface!=NULL)
			{
				TopExp_Explorer Ex;
				for(Ex.Init(*ObjSurface,TopAbs_FACE);Ex.More();Ex.Next())
				{
					TopoDS_Face F=TopoDS::Face(Ex.Current());
					TopLoc_Location L;
					Handle (Poly_Triangulation) facing=BRep_Tool::Triangulation(F,L);
					countFace+=facing->NbTriangles();
				}
			}
			return countFace;
		}

		int OCGeometryGenerator::getNumberOfPoints()
		{
			int countVert=0;
			if(ObjSurface!=NULL)
			{
				TopExp_Explorer Ex;
				for(Ex.Init(*ObjSurface,TopAbs_FACE);Ex.More();Ex.Next())
				{
					TopoDS_Face F=TopoDS::Face(Ex.Current());
					TopLoc_Location L;
					Handle (Poly_Triangulation) facing=BRep_Tool::Triangulation(F,L);
					countVert+=facing->NbNodes();
				}
			}
			return countVert;
		}

		double* OCGeometryGenerator::getTriangleVertices()
		{
			double* points=NULL;
			int nPts=this->getNumberOfPoints();
			if(nPts>0)
			{
				points=new double[nPts*3];
				int index=0;
				TopExp_Explorer Ex;
				for(Ex.Init(*ObjSurface,TopAbs_FACE);Ex.More();Ex.Next())
				{
					TopoDS_Face F=TopoDS::Face(Ex.Current());
					TopLoc_Location L;
					Handle (Poly_Triangulation) facing=BRep_Tool::Triangulation(F,L);
					TColgp_Array1OfPnt tab(1,(facing->NbNodes()));
					tab = facing->Nodes();
					for (Standard_Integer i=1;i<=(facing->NbNodes());i++) {
						gp_Pnt pnt=tab.Value(i);
						points[index*3+0]=pnt.X();
						points[index*3+1]=pnt.Y();
						points[index*3+2]=pnt.Z();
						index++;
					}
				}				
			}
			return points;
		}

		int* OCGeometryGenerator::getTriangleFaces()
		{
            int* faces=NULL;
            int nFaces=this->getNumberOfTriangles(); // was Points
			if(nFaces>0)
			{
				faces=new int[nFaces*3];
				TopExp_Explorer Ex;
				int maxindex=0;
				int index=0;
				for(Ex.Init(*ObjSurface,TopAbs_FACE);Ex.More();Ex.Next())
				{
					TopoDS_Face F=TopoDS::Face(Ex.Current());
					TopLoc_Location L;
					Handle (Poly_Triangulation) facing=BRep_Tool::Triangulation(F,L);
					TColgp_Array1OfPnt tab(1,(facing->NbNodes()));
					tab = facing->Nodes();
					Poly_Array1OfTriangle tri(1,facing->NbTriangles());
					tri = facing->Triangles();
					for (Standard_Integer i=1;i<=(facing->NbTriangles());i++) {
						Poly_Triangle trian = tri.Value(i);
						Standard_Integer index1,index2,index3;
						trian.Get(index1,index2,index3);
						faces[index*3+0]=maxindex+index1-1;
						faces[index*3+1]=maxindex+index2-1;
						faces[index*3+2]=maxindex+index3-1;
						index++;
					}		
					maxindex+=facing->NbNodes();
				}
			}
			return faces;
		}
	}  // NAMESPACE Geometry

}  // NAMESPACE Mantid
