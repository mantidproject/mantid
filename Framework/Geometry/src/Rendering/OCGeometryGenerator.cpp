#include "MantidGeometry/Rendering/OCGeometryGenerator.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Torus.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"

#include <climits> // Needed for g++4.4 on Mac with OpenCASCADE 6.3.0
#include <cmath>
#include <vector>

// Squash a warning coming out of an OpenCascade header
#ifdef __INTEL_COMPILER
#pragma warning disable 191
#endif
// Opencascade defines _USE_MATH_DEFINES without checking whether it is already
// used.
// Undefine it here before we include the headers to avoid a warning. Older
// versions
// also define M_SQRT1_2 so do the same if it is already defined
#ifdef _MSC_VER
#undef _USE_MATH_DEFINES
#ifdef M_SQRT1_2
#undef M_SQRT1_2
#endif
#endif

GCC_DIAG_OFF(conversion)
// clang-format off
GCC_DIAG_OFF(cast-qual)
// clang-format on
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
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
GCC_DIAG_ON(conversion)
// clang-format off
GCC_DIAG_ON(cast-qual)
// clang-format on

#ifdef __INTEL_COMPILER
#pragma warning enable 191
#endif

namespace Mantid {

namespace Geometry {
namespace {
/// static logger
Kernel::Logger g_log("OCGeometryGenerator");
}

/**
* Constructor
* @param obj :: input object
*/
OCGeometryGenerator::OCGeometryGenerator(const CSGObject *obj) : Obj(obj) {
  ObjSurface = nullptr;
}

/**
* Generate geometry, it uses OpenCascade to generate surface triangles.
*/
void OCGeometryGenerator::Generate() {
  if (ObjSurface == nullptr) {
    AnalyzeObject();
  }
}

/**
* Destroy the surface generated for the object
*/
OCGeometryGenerator::~OCGeometryGenerator() {
  if (ObjSurface != nullptr) {
    delete ObjSurface;
  }
}

/**
* Analyzes the rule tree in object and creates a Topology Shape
*/
void OCGeometryGenerator::AnalyzeObject() {
  if (Obj != nullptr) // If object exists
  {
    // Get the top rule tree in Obj
    const Rule *top = Obj->topRule();
    if (top == nullptr) {
      ObjSurface = new TopoDS_Shape();
      return;
    }
    // Traverse through Rule
    TopoDS_Shape Result = const_cast<Rule *>(top)->analyze();
    try {
      ObjSurface = new TopoDS_Shape(Result);
      BRepMesh_IncrementalMesh(Result, 0.001);
    } catch (StdFail_NotDone &) {
      g_log.error("Cannot build the geometry. Check the geometry definition");
    }
  }
}

/**
* Returns the shape generated.
*/
TopoDS_Shape *OCGeometryGenerator::getObjectSurface() { return ObjSurface; }

int OCGeometryGenerator::getNumberOfTriangles() {
  int countFace = 0;
  if (ObjSurface != nullptr) {
    TopExp_Explorer Ex;
    for (Ex.Init(*ObjSurface, TopAbs_FACE); Ex.More(); Ex.Next()) {
      TopoDS_Face F = TopoDS::Face(Ex.Current());
      TopLoc_Location L;
      Handle(Poly_Triangulation) facing = BRep_Tool::Triangulation(F, L);
      countFace += facing->NbTriangles();
    }
  }
  return countFace;
}

int OCGeometryGenerator::getNumberOfPoints() {
  int countVert = 0;
  if (ObjSurface != nullptr) {
    TopExp_Explorer Ex;
    for (Ex.Init(*ObjSurface, TopAbs_FACE); Ex.More(); Ex.Next()) {
      TopoDS_Face F = TopoDS::Face(Ex.Current());
      TopLoc_Location L;
      Handle(Poly_Triangulation) facing = BRep_Tool::Triangulation(F, L);
      countVert += facing->NbNodes();
    }
  }
  return countVert;
}

double *OCGeometryGenerator::getTriangleVertices() {
  double *points = nullptr;
  int nPts = this->getNumberOfPoints();
  if (nPts > 0) {
    points = new double[static_cast<std::size_t>(nPts) * 3];
    int index = 0;
    TopExp_Explorer Ex;
    for (Ex.Init(*ObjSurface, TopAbs_FACE); Ex.More(); Ex.Next()) {
      TopoDS_Face F = TopoDS::Face(Ex.Current());
      TopLoc_Location L;
      Handle(Poly_Triangulation) facing = BRep_Tool::Triangulation(F, L);
      TColgp_Array1OfPnt tab(1, (facing->NbNodes()));
      tab = facing->Nodes();
      for (Standard_Integer i = 1; i <= (facing->NbNodes()); i++) {
        gp_Pnt pnt = tab.Value(i);
        points[index * 3 + 0] = pnt.X();
        points[index * 3 + 1] = pnt.Y();
        points[index * 3 + 2] = pnt.Z();
        index++;
      }
    }
  }
  return points;
}

int *OCGeometryGenerator::getTriangleFaces() {
  int *faces = nullptr;
  int nFaces = this->getNumberOfTriangles(); // was Points
  if (nFaces > 0) {
    faces = new int[static_cast<std::size_t>(nFaces) * 3];
    TopExp_Explorer Ex;
    int maxindex = 0;
    int index = 0;
    for (Ex.Init(*ObjSurface, TopAbs_FACE); Ex.More(); Ex.Next()) {
      TopoDS_Face F = TopoDS::Face(Ex.Current());
      TopLoc_Location L;
      Handle(Poly_Triangulation) facing = BRep_Tool::Triangulation(F, L);
      TColgp_Array1OfPnt tab(1, (facing->NbNodes()));
      tab = facing->Nodes();
      Poly_Array1OfTriangle tri(1, facing->NbTriangles());
      tri = facing->Triangles();
      for (Standard_Integer i = 1; i <= (facing->NbTriangles()); i++) {
        Poly_Triangle trian = tri.Value(i);
        Standard_Integer index1, index2, index3;
        trian.Get(index1, index2, index3);
        faces[index * 3 + 0] = maxindex + index1 - 1;
        faces[index * 3 + 1] = maxindex + index2 - 1;
        faces[index * 3 + 2] = maxindex + index3 - 1;
        index++;
      }
      maxindex += facing->NbNodes();
    }
  }
  return faces;
}
} // NAMESPACE Geometry

} // NAMESPACE Mantid
