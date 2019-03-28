// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rendering/RenderingHelpers.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/GeometryTriangulator.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/WarningSuppressions.h"
#include <climits>

#ifdef ENABLE_OPENCASCADE
// Squash a warning coming out of an OpenCascade header
#ifdef __INTEL_COMPILER
#pragma warning disable 191
#endif
// Opencascade defines _USE_MATH_DEFINES without checking whether it is already
// used.
// Undefine it here before we include the headers to avoid a warning
#ifdef _MSC_VER
#undef _USE_MATH_DEFINES
#ifdef M_SQRT1_2
#undef M_SQRT1_2
#endif
#endif

GNU_DIAG_OFF("conversion")
GNU_DIAG_OFF("cast-qual")

#include <BRep_Tool.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("cast-qual")

#ifdef __INTEL_COMPILER
#pragma warning enable 191
#endif
#endif

namespace Mantid {
namespace Geometry {
using Kernel::Quat;
using Kernel::V3D;

namespace {
// Render IObjectComponent
void render(const IObjComponent &ObjComp) {
  glPushMatrix();
  V3D pos = ObjComp.getPos();
  Quat rot = ObjComp.getRotation();
  double rotGL[16];
  rot.GLMatrix(&rotGL[0]);
  glTranslated(pos[0], pos[1], pos[2]);
  glMultMatrixd(rotGL);
  V3D scaleFactor = ObjComp.getScaleFactor();
  glScaled(scaleFactor[0], scaleFactor[1], scaleFactor[2]);
  ObjComp.drawObject();
  glPopMatrix();
}

// Render triangulated surface
void render(detail::GeometryTriangulator &triangulator) {
  const auto &faces = triangulator.getTriangleFaces();
  const auto &points = triangulator.getTriangleVertices();
  glBegin(GL_TRIANGLES);
  for (size_t i = 0; i < triangulator.numTriangleFaces(); i++) {
    auto index2 = static_cast<size_t>(faces[i * 3 + 1] * 3);
    auto index3 = static_cast<size_t>(faces[i * 3 + 2] * 3);
    auto index1 = static_cast<size_t>(faces[i * 3] * 3);
    // Calculate normal and normalize
    const V3D v1(points[index1], points[index1 + 1], points[index1 + 2]);
    const V3D v2(points[index2], points[index2 + 1], points[index2 + 2]);
    const V3D v3(points[index3], points[index3 + 1], points[index3 + 2]);
    const auto normal = normalize((v1 - v2).cross_prod(v2 - v3));
    glNormal3d(normal[0], normal[1], normal[2]);
    glVertex3dv(&points[index1]);
    glVertex3dv(&points[index2]);
    glVertex3dv(&points[index3]);
  }
  glEnd();
}

#ifdef ENABLE_OPENCASCADE
// Render OpenCascade Shape
void render(const TopoDS_Shape &ObjSurf) {
  glBegin(GL_TRIANGLES);
  if (!ObjSurf.IsNull()) {
    TopExp_Explorer Ex;
    for (Ex.Init(ObjSurf, TopAbs_FACE); Ex.More(); Ex.Next()) {
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
        gp_Pnt point1 = tab.Value(index1);
        gp_Pnt point2 = tab.Value(index2);
        gp_Pnt point3 = tab.Value(index3);
        gp_XYZ pt1 = tab.Value(index1).XYZ();
        gp_XYZ pt2 = tab.Value(index2).XYZ();
        gp_XYZ pt3 = tab.Value(index3).XYZ();

        gp_XYZ v1 = pt2 - pt1;
        gp_XYZ v2 = pt3 - pt2;

        gp_XYZ normal = v1 ^ v2;
        normal.Normalize();
        glNormal3d(normal.X(), normal.Y(), normal.Z());
        glVertex3d(point1.X(), point1.Y(), point1.Z());
        glVertex3d(point2.X(), point2.Y(), point2.Z());
        glVertex3d(point3.X(), point3.Y(), point3.Z());
      }
    }
  }
  glEnd();
}
#endif

void renderSphere(const detail::ShapeInfo &shapeInfo) {
  // create glu sphere
  GLUquadricObj *qobj = gluNewQuadric();
  gluQuadricDrawStyle(qobj, GLU_FILL);
  gluQuadricNormals(qobj, GL_SMOOTH);
  glPushMatrix();
  auto center = shapeInfo.points()[0];
  glTranslated(center[0], center[1], center[2]);
  gluSphere(qobj, shapeInfo.radius(), Sphere::g_nslices, Sphere::g_nstacks);
  glPopMatrix();
  gluDeleteQuadric(qobj);
}

void renderCuboid(const detail::ShapeInfo &shapeInfo) {
  const auto &points = shapeInfo.points();
  V3D vec0 = points[0];
  V3D vec1 = points[1] - points[0];
  V3D vec2 = points[2] - points[0];
  V3D vec3 = points[3] - points[0];
  V3D vertex[8];
  vertex[0] = vec0;
  vertex[1] = vec0 + vec3;
  vertex[2] = vec0 + vec3 + vec1;
  vertex[3] = vec0 + vec1;
  vertex[4] = vec0 + vec2;
  vertex[5] = vec0 + vec2 + vec3;
  vertex[6] = vec0 + vec2 + vec3 + vec1;
  vertex[7] = vec0 + vec1 + vec2;

  int faceindex[6][4] = {
      {0, 1, 2, 3}, // top
      {0, 3, 7, 4}, // left
      {3, 2, 6, 7}, // back
      {2, 1, 5, 6}, // right
      {0, 4, 5, 1}, // front
      {4, 7, 6, 5}, // bottom
  };
  // first face
  glBegin(GL_QUADS);
  for (auto &row : faceindex) {
    const auto normal = normalize((vertex[row[0]] - vertex[row[1]])
                 .cross_prod((vertex[row[0]] - vertex[row[2]])));
    glNormal3d(normal[0], normal[1], normal[2]);
    for (const int ij : row) {
      if (ij == 0)
        glTexCoord2i(0, 0);
      if (ij == 1)
        glTexCoord2i(1, 0);
      if (ij == 2)
        glTexCoord2i(1, 1);
      if (ij == 3)
        glTexCoord2i(0, 1);
      if (ij == 4)
        glTexCoord2i(0, 0);
      if (ij == 5)
        glTexCoord2i(1, 0);
      if (ij == 6)
        glTexCoord2i(1, 1);
      if (ij == 7)
        glTexCoord2i(0, 1);
      glVertex3d(vertex[ij][0], vertex[ij][1], vertex[ij][2]);
    }
  }
  glEnd();
}

void renderHexahedron(const detail::ShapeInfo &shapeInfo) {
  glBegin(GL_QUADS);
  const auto &points = shapeInfo.points();
  // bottom
  glVertex3d(points[0].X(), points[0].Y(), points[0].Z());
  glVertex3d(points[1].X(), points[1].Y(), points[1].Z());
  glVertex3d(points[2].X(), points[2].Y(), points[2].Z());
  glVertex3d(points[3].X(), points[3].Y(), points[3].Z());
  // front
  glVertex3d(points[1].X(), points[1].Y(), points[1].Z());
  glVertex3d(points[5].X(), points[5].Y(), points[5].Z());
  glVertex3d(points[6].X(), points[6].Y(), points[6].Z());
  glVertex3d(points[2].X(), points[2].Y(), points[2].Z());
  // right
  glVertex3d(points[2].X(), points[2].Y(), points[2].Z());
  glVertex3d(points[6].X(), points[6].Y(), points[6].Z());
  glVertex3d(points[7].X(), points[7].Y(), points[7].Z());
  glVertex3d(points[3].X(), points[3].Y(), points[3].Z());
  // back
  glVertex3d(points[3].X(), points[3].Y(), points[3].Z());
  glVertex3d(points[7].X(), points[7].Y(), points[7].Z());
  glVertex3d(points[4].X(), points[4].Y(), points[4].Z());
  glVertex3d(points[0].X(), points[0].Y(), points[0].Z());
  // left
  glVertex3d(points[0].X(), points[0].Y(), points[0].Z());
  glVertex3d(points[4].X(), points[4].Y(), points[4].Z());
  glVertex3d(points[5].X(), points[5].Y(), points[5].Z());
  glVertex3d(points[1].X(), points[1].Y(), points[1].Z());
  // top
  glVertex3d(points[4].X(), points[4].Y(), points[4].Z());
  glVertex3d(points[5].X(), points[5].Y(), points[5].Z());
  glVertex3d(points[6].X(), points[6].Y(), points[6].Z());
  glVertex3d(points[7].X(), points[7].Y(), points[7].Z());

  glEnd();
}

void renderCone(const detail::ShapeInfo &shapeInfo) {
  glPushMatrix();
  GLUquadricObj *qobj = gluNewQuadric();
  gluQuadricDrawStyle(qobj, GLU_FILL);
  gluQuadricNormals(qobj, GL_SMOOTH);
  auto center = shapeInfo.points()[0];
  glTranslated(center[0], center[1], center[2]);
  GLdouble mat[16];
  V3D unit(0, 0, 1);
  auto axis = shapeInfo.points()[1];
  Quat rot(unit, axis);
  rot.GLMatrix(&mat[0]);
  glMultMatrixd(mat);
  auto radius = shapeInfo.radius();
  auto height = shapeInfo.height();
  gluCylinder(qobj, 0, radius, height, Geometry::Cone::g_nslices,
              Geometry::Cone::g_nstacks);
  glTranslated(0.0, 0.0, height);
  gluDisk(qobj, 0, radius, Geometry::Cone::g_nslices, 1);
  glPopMatrix();
}

void renderCylinder(const detail::ShapeInfo &shapeInfo) {
  GLUquadricObj *qobj = gluNewQuadric();
  gluQuadricDrawStyle(qobj, GLU_FILL);
  gluQuadricNormals(qobj, GL_SMOOTH);
  gluQuadricTexture(qobj, true);
  glPushMatrix();
  auto center = shapeInfo.points()[0];
  glTranslated(center[0], center[1], center[2]);
  GLdouble mat[16];
  V3D unit(0, 0, 1);
  auto axis = shapeInfo.points()[1];
  Quat rot(unit, axis);
  rot.GLMatrix(&mat[0]);
  glMultMatrixd(mat);
  auto radius = shapeInfo.radius();
  auto height = shapeInfo.height();
  gluCylinder(qobj, radius, radius, height, Cylinder::g_nslices,
              Cylinder::g_nstacks);
  gluQuadricTexture(qobj, false);
  gluDisk(qobj, 0, radius, Cylinder::g_nslices, 1);
  glTranslated(0.0, 0.0, height);
  gluDisk(qobj, 0, radius, Cylinder::g_nslices, 1);
  glPopMatrix();
}
} // namespace

namespace RenderingHelpers {
void renderIObjComponent(const IObjComponent &objComp) { render(objComp); }

void renderTriangulated(detail::GeometryTriangulator &triangulator) {
#ifdef ENABLE_OPENCASCADE
  if (triangulator.hasOCSurface() && !triangulator.getOCSurface().IsNull())
    render(triangulator.getOCSurface());
  else
    render(triangulator);
#else
  render(triangulator);
#endif
}

void renderShape(const detail::ShapeInfo &shapeInfo) {
  switch (shapeInfo.shape()) {
  case detail::ShapeInfo::GeometryShape::CUBOID:
    renderCuboid(shapeInfo);
    break;
  case detail::ShapeInfo::GeometryShape::SPHERE:
    renderSphere(shapeInfo);
    break;
  case detail::ShapeInfo::GeometryShape::HEXAHEDRON:
    renderHexahedron(shapeInfo);
    break;
  case detail::ShapeInfo::GeometryShape::CONE:
    renderCone(shapeInfo);
    break;
  case detail::ShapeInfo::GeometryShape::CYLINDER:
    renderCylinder(shapeInfo);
    break;
  default:
    return;
  }
}
} // namespace RenderingHelpers
} // namespace Geometry
} // namespace Mantid
