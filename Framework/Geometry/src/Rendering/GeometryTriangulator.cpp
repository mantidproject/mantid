// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rendering/GeometryTriangulator.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Rendering/RenderingMesh.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/WarningSuppressions.h"
#include <climits>

#ifdef ENABLE_OPENCASCADE
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

GNU_DIAG_OFF("conversion")
GNU_DIAG_OFF("cast-qual")

#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Version.hxx>
#include <StdFail_NotDone.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("cast-qual")

#ifdef __INTEL_COMPILER
#pragma warning enable 191
#endif

namespace {
auto getNode(const Handle(Poly_Triangulation) facing, Standard_Integer i) {
#if OCC_VERSION_MAJOR >= 7 && OCC_VERSION_MINOR >= 6
  return facing->Node(i);
#else
  // Compat shim to support OCCT 7.5 and below
  TColgp_Array1OfPnt tab(1, (facing->NbNodes()));
  tab = facing->Nodes();
  return tab.Value(i);
#endif
}

auto getTriangle(const Handle(Poly_Triangulation) facing, Standard_Integer i) {
#if OCC_VERSION_MAJOR >= 7 && OCC_VERSION_MINOR >= 6
  return facing->Triangle(i);
#else
  // Compat shim to support OCCT 7.5 and below
  Poly_Array1OfTriangle tri(1, facing->NbTriangles());
  tri = facing->Triangles();
  return tri.Value(i);
#endif
}
} // namespace
#endif // ENABLE_OPENCASCADE

namespace Mantid::Geometry::detail {
namespace {
/// static logger
Kernel::Logger g_log("GeometryTriangulator");
} // namespace

GeometryTriangulator::GeometryTriangulator(const CSGObject *obj)
    : m_isTriangulated(false), m_nFaces(0), m_nPoints(0), m_csgObj(obj)
#ifdef ENABLE_OPENCASCADE
      ,
      m_objSurface(nullptr)
#endif
{
}

GeometryTriangulator::GeometryTriangulator(std::unique_ptr<RenderingMesh> obj)
    : m_isTriangulated(false), m_meshObj(std::move(obj)) {}

GeometryTriangulator::~GeometryTriangulator() = default;

void GeometryTriangulator::triangulate() {
#ifdef ENABLE_OPENCASCADE
  if (m_objSurface == nullptr)
    OCAnalyzeObject();
#endif
  if (m_meshObj) {
    generateMesh();
    m_nPoints = m_meshObj->numberOfVertices();
    m_nFaces = m_meshObj->numberOfTriangles();
    m_points = m_meshObj->getVertices();
    m_faces = m_meshObj->getTriangles();
  }
  m_isTriangulated = true;
}

void GeometryTriangulator::generateMesh() { /* Placeholder function to replace MeshGeometryGenerator::Generate()*/ }

#ifdef ENABLE_OPENCASCADE
bool GeometryTriangulator::hasOCSurface() const { return m_objSurface != nullptr; }
/// Return OpenCascade surface.
const TopoDS_Shape &GeometryTriangulator::getOCSurface() {
  checkTriangulated();
  return *m_objSurface;
}
#endif

void GeometryTriangulator::checkTriangulated() {
  if (m_csgObj != nullptr || m_meshObj != nullptr) {
    if (!m_isTriangulated) {
      triangulate();
    }
  }
}
size_t GeometryTriangulator::numTriangleFaces() {
  checkTriangulated();
  return m_nFaces;
}

size_t GeometryTriangulator::numTriangleVertices() {
  checkTriangulated();
  return m_nPoints;
}

/// get a pointer to the 3x(NumberOfPoints) coordinates (x1,y1,z1,x2..) of
/// mesh
const std::vector<double> &GeometryTriangulator::getTriangleVertices() {
  checkTriangulated();
  return m_points;
}
/// get a pointer to the 3x(NumberOFaces) integers describing points forming
/// faces (p1,p2,p3)(p4,p5,p6).
const std::vector<uint32_t> &GeometryTriangulator::getTriangleFaces() {
  checkTriangulated();
  return m_faces;
}

#ifdef ENABLE_OPENCASCADE
void GeometryTriangulator::OCAnalyzeObject() {
  if (m_csgObj != nullptr) // If object exists
  {
    // Get the top rule tree in Obj
    const auto *top = m_csgObj->topRule();
    if (top == nullptr) {
      m_objSurface.reset(new TopoDS_Shape());
      return;
    } else {
      // Traverse through Rule
      TopoDS_Shape Result = const_cast<Rule *>(top)->analyze();
      try {
        m_objSurface.reset(new TopoDS_Shape(Result));
        BRepMesh_IncrementalMesh(Result, 0.001);
      } catch (StdFail_NotDone &) {
        g_log.error("Cannot build the geometry. Check the geometry definition");
      }
    }
  }

  setupPoints();
  setupFaces();
}

size_t GeometryTriangulator::numPoints() const {
  size_t countVert = 0;
  if (m_objSurface != nullptr) {
    TopExp_Explorer Ex;
    for (Ex.Init(*m_objSurface, TopAbs_FACE); Ex.More(); Ex.Next()) {
      TopoDS_Face F = TopoDS::Face(Ex.Current());
      TopLoc_Location L;
      Handle(Poly_Triangulation) facing = BRep_Tool::Triangulation(F, L);
      countVert += static_cast<size_t>(facing->NbNodes());
    }
  }
  return countVert;
}

size_t GeometryTriangulator::numFaces() const {
  size_t countFace = 0;
  if (m_objSurface != nullptr) {
    TopExp_Explorer Ex;
    for (Ex.Init(*m_objSurface, TopAbs_FACE); Ex.More(); Ex.Next()) {
      TopoDS_Face F = TopoDS::Face(Ex.Current());
      TopLoc_Location L;
      Handle(Poly_Triangulation) facing = BRep_Tool::Triangulation(F, L);
      countFace += static_cast<size_t>(facing->NbTriangles());
    }
  }
  return countFace;
}

void GeometryTriangulator::setupPoints() {
  m_nPoints = numPoints();
  if (m_nPoints > 0) {
    size_t index = 0;
    m_points.resize(m_nPoints * 3);
    TopExp_Explorer Ex;
    for (Ex.Init(*m_objSurface, TopAbs_FACE); Ex.More(); Ex.Next()) {
      TopoDS_Face F = TopoDS::Face(Ex.Current());
      TopLoc_Location L;
      Handle(Poly_Triangulation) facing = BRep_Tool::Triangulation(F, L);
      for (Standard_Integer i = 1; i <= (facing->NbNodes()); i++) {
        const auto pnt = getNode(facing, i);
        m_points[index * 3 + 0] = pnt.X();
        m_points[index * 3 + 1] = pnt.Y();
        m_points[index * 3 + 2] = pnt.Z();
        index++;
      }
    }
  }
}

void GeometryTriangulator::setupFaces() {
  m_nFaces = numFaces();
  if (m_nFaces > 0) {
    m_faces.resize(m_nFaces * 3);
    TopExp_Explorer Ex;
    int maxindex = 0;
    size_t index = 0;
    for (Ex.Init(*m_objSurface, TopAbs_FACE); Ex.More(); Ex.Next()) {
      TopoDS_Face F = TopoDS::Face(Ex.Current());
      TopLoc_Location L;
      Handle(Poly_Triangulation) facing = BRep_Tool::Triangulation(F, L);
      for (Standard_Integer i = 1; i <= (facing->NbTriangles()); i++) {
        Poly_Triangle trian = getTriangle(facing, i);
        Standard_Integer index1, index2, index3;
        trian.Get(index1, index2, index3);
        m_faces[index * 3 + 0] = static_cast<uint32_t>(maxindex + index1 - 1);
        m_faces[index * 3 + 1] = static_cast<uint32_t>(maxindex + index2 - 1);
        m_faces[index * 3 + 2] = static_cast<uint32_t>(maxindex + index3 - 1);
        index++;
      }
      maxindex += facing->NbNodes();
    }
  }
}
#endif

void GeometryTriangulator::setGeometryCache(size_t nPoints, size_t nFaces, std::vector<double> &&points,
                                            std::vector<uint32_t> &&faces) {
  m_nPoints = nPoints;
  m_nFaces = nFaces;
  m_points = std::move(points);
  m_faces = std::move(faces);
  m_isTriangulated = true;
}
} // namespace Mantid::Geometry::detail
