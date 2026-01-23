// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/TranslateSampleShape.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/V3D.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>

#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <sstream>
#include <string>

using Mantid::API::AnalysisDataService;
using Mantid::API::ExperimentInfo;
using Mantid::API::ExperimentInfo_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::Workspace_sptr;
using Mantid::DataHandling::TranslateSampleShape;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Geometry::CSGObject;
using Mantid::Geometry::MeshObject;
using Mantid::Kernel::Material;
using Mantid::Kernel::V3D;

namespace {

// ---- General helpers --------------------------------------------------------

Workspace2D_sptr getWorkspaceWithCSGShape(const std::string &xmlContent) {
  auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
  AnalysisDataService::Instance().addOrReplace("test_ws", ws);
  Mantid::Geometry::ShapeFactory sf;
  ws->mutableSample().setShape(sf.createShape(xmlContent));
  return ws;
}

Workspace2D_sptr getWorkspaceWithMeshShape(std::unique_ptr<MeshObject> mesh) {
  Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(3, 3);
  AnalysisDataService::Instance().addOrReplace("test_ws", ws);
  ws->mutableSample().setShape(std::move(mesh));
  return ws;
}

std::unique_ptr<MeshObject> makeUnitCubeMesh(const V3D &centre) {
  const double size = 2.0;
  const double min = -0.5 * size;
  const double max = 0.5 * size;

  std::vector<V3D> vertices;
  vertices.emplace_back(centre + V3D(max, max, max));
  vertices.emplace_back(centre + V3D(min, max, max));
  vertices.emplace_back(centre + V3D(max, min, max));
  vertices.emplace_back(centre + V3D(min, min, max));
  vertices.emplace_back(centre + V3D(max, max, min));
  vertices.emplace_back(centre + V3D(min, max, min));
  vertices.emplace_back(centre + V3D(max, min, min));
  vertices.emplace_back(centre + V3D(min, min, min));

  std::vector<uint32_t> triangles;
  triangles.insert(triangles.end(), {0, 1, 2, 2, 1, 3});
  triangles.insert(triangles.end(), {0, 2, 4, 4, 2, 6});
  triangles.insert(triangles.end(), {0, 4, 1, 1, 4, 5});
  triangles.insert(triangles.end(), {7, 5, 6, 6, 5, 4});
  triangles.insert(triangles.end(), {7, 3, 5, 5, 3, 1});
  triangles.insert(triangles.end(), {7, 6, 3, 3, 6, 2});

  return std::make_unique<MeshObject>(std::move(triangles), std::move(vertices), Mantid::Kernel::Material());
}

void runTranslate(const Workspace2D_sptr &ws, const V3D &vec) {
  std::ostringstream vecString;
  vecString << std::setprecision(15) << vec.X() << "," << vec.Y() << "," << vec.Z();

  TranslateSampleShape alg;
  alg.initialize();
  alg.setPropertyValue("InputWorkspace", ws->getName());
  alg.setPropertyValue("TranslationVector", vecString.str());
  alg.execute();
  TS_ASSERT(alg.isExecuted());
}

std::string wsOutXml(const Workspace2D_sptr &ws) {
  auto ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);
  auto csg = std::dynamic_pointer_cast<CSGObject>(ei->sample().getShapePtr());
  return csg ? csg->getShapeXML() : std::string();
}

// ---- XML parsing helpers ------------------------------------------------------------

Poco::AutoPtr<Poco::XML::Document> parseXml(const std::string &xml) {
  Poco::XML::DOMParser p;
  return p.parseString(xml);
}

Poco::XML::Element *docRoot(Poco::AutoPtr<Poco::XML::Document> &doc) { return doc ? doc->documentElement() : nullptr; }

Poco::XML::Element *first(Poco::XML::Element *root, const std::string &tag) {
  if (!root)
    return nullptr;
  Poco::AutoPtr<Poco::XML::NodeList> nl = root->getElementsByTagName(tag);
  if (nl && nl->length() > 0)
    return dynamic_cast<Poco::XML::Element *>(nl->item(0));
  return nullptr;
}

double attrD(Poco::XML::Element *el, const char *name) {
  TS_ASSERT(el != nullptr);
  TS_ASSERT(el->hasAttribute(name));
  return std::stod(el->getAttribute(name));
}

} // namespace

class TranslateSampleShapeTest : public CxxTest::TestSuite {
public:
  // define the setup and utility functions

  void tearDown() override {
    // Clean ADS between tests to avoid name clashes
    auto &ads = AnalysisDataService::Instance();
    for (auto const &name : ads.getObjectNames()) {
      ads.remove(name);
    }
  }

  void assertElementXYZTranslated(const Workspace2D_sptr &ws, std::string tag, V3D point, V3D translation) {
    auto doc = parseXml(wsOutXml(ws));
    auto root = docRoot(doc);
    auto element = first(root, tag);
    TS_ASSERT_DELTA(attrD(element, "x"), point.X() + translation.X(), 1e-12);
    TS_ASSERT_DELTA(attrD(element, "y"), point.Y() + translation.Y(), 1e-12);
    TS_ASSERT_DELTA(attrD(element, "z"), point.Z() + translation.Z(), 1e-12);
  }

  void assertElementValTranslated(const Workspace2D_sptr &ws, std::string tag, double point, double translation) {
    auto doc = parseXml(wsOutXml(ws));
    auto root = docRoot(doc);
    auto element = first(root, tag);
    TS_ASSERT_DELTA(attrD(element, "val"), point + translation, 1e-12);
  }

  // ##########   CSG Tests  ###############
  // Here we will try and test the translation of all the shapes defined in
  // https://docs.mantidproject.org/nightly/concepts/HowToDefineGeometricShape.html

  // ---------- Sphere ----------
  void test_sphere_centre_is_translated() {
    const V3D centre(1, 2, 3);
    const V3D d(0.1, -0.2, 0.3);

    auto xml = ComponentCreationHelper::sphereXML(0.5, centre, "S");
    auto ws = getWorkspaceWithCSGShape(xml);

    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "centre", centre, d);
  }

  // ---------- Cylinder (finite) ----------
  void test_cylinder_bottom_base_translated_axis_unchanged() {
    const V3D base(-0.5, 0.0, 0.0), axis(1.0, 0.0, 0.0), d(0.1, -0.2, 0.3);
    const double r = 0.05, h = 1.0;
    std::ostringstream xml;
    xml << "<cylinder id=\"C\">"
        << "<centre-of-bottom-base x=\"" << base.X() << "\" y=\"" << base.Y() << "\" z=\"" << base.Z() << "\"/>"
        << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
        << "<radius val=\"" << r << "\"/><height val=\"" << h << "\"/></cylinder>"
        << "<algebra val=\"C\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "centre-of-bottom-base", base, d);
    // check axis hasn't moved (i.e. translated by 0,0,0)
    assertElementXYZTranslated(ws, "axis", axis, V3D(0, 0, 0));
  }

  // ---------- Hollow Cylinder (finite) ----------
  void test_hollow_cylinder_bottom_base_translated_axis_unchanged() {
    const V3D base(0.0, 0.0, 0.0), axis(0.0, 1.0, 0.0), d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<hollow-cylinder id=\"HC\">"
        << "<centre-of-bottom-base x=\"" << base.X() << "\" y=\"" << base.Y() << "\" z=\"" << base.Z() << "\"/>"
        << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
        << "<inner-radius val=\"0.007\"/><outer-radius val=\"0.01\"/><height val=\"0.05\"/></hollow-cylinder>"
        << "<algebra val=\"HC\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "centre-of-bottom-base", base, d);
    // check axis hasn't moved (i.e. translated by 0,0,0)
    assertElementXYZTranslated(ws, "axis", axis, V3D(0, 0, 0));
  }

  // ---------- Infinite Cylinder ----------
  void test_infinite_cylinder_centre_translated_axis_unchanged() {
    const V3D centre(0.0, 0.2, 0.0), axis(0.0, 0.2, 0.0), d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<infinite-cylinder id=\"IC\">"
        << "<centre x=\"" << centre.X() << "\" y=\"" << centre.Y() << "\" z=\"" << centre.Z() << "\"/>"
        << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
        << "<radius val=\"1\"/></infinite-cylinder>"
        << "<algebra val=\"IC\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "centre", centre, d);
    // check axis hasn't moved (i.e. translated by 0,0,0)
    assertElementXYZTranslated(ws, "axis", axis, V3D(0, 0, 0));
  }

  // ---------- Slice of Cylinder Ring ----------
  void test_slice_of_cylinder_ring_no_translatable_points() {

    const double iR = 0.05, oR = 0.1, depth = 1.0, arc = 45.0;
    const V3D d(0.1, -0.2, 0.3);

    std::ostringstream xml;
    xml << "<slice-of-cylinder-ring id=\"R\">"
        << "<inner-radius val=\"" << iR << "\"/>"
        << "<outer-radius val=\"" << oR << "\"/>"
        << "<depth val=\"" << depth << "\"/>"
        << "<arc val=\"" << arc << "\"/>"
        << "</slice-of-cylinder-ring>"
        << "<algebra val=\"R\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    // check none of these have changed
    assertElementValTranslated(ws, "inner-radius", iR, 0.0);
    assertElementValTranslated(ws, "outer-radius", oR, 0.0);
    assertElementValTranslated(ws, "depth", depth, 0.0);
    assertElementValTranslated(ws, "arc", arc, 0.0);
  }

  // ---------- Cone ----------
  void test_cone_tip_translated_axis_unchanged() {
    const V3D tip(0.0, 0.2, 0.0), axis(0.0, 0.2, 0.0), d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<cone id=\"CN\">"
        << "<tip-point x=\"" << tip.X() << "\" y=\"" << tip.Y() << "\" z=\"" << tip.Z() << "\"/>"
        << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
        << "<angle val=\"30.1\"/><height val=\"10.2\"/></cone>"
        << "<algebra val=\"CN\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "tip-point", tip, d);
    // check axis hasn't moved (i.e. translated by 0,0,0)
    assertElementXYZTranslated(ws, "axis", axis, V3D(0, 0, 0));
  }

  // ---------- Infinite Cone ----------
  void test_infinite_cone_tip_translated_axis_unchanged() {
    const V3D tip(0.0, 0.2, 0.0), axis(0.0, 0.2, 0.0), d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<infinite-cone id=\"ICN\">"
        << "<tip-point x=\"" << tip.X() << "\" y=\"" << tip.Y() << "\" z=\"" << tip.Z() << "\"/>"
        << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
        << "<angle val=\"30.1\"/></infinite-cone>"
        << "<algebra val=\"ICN\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "tip-point", tip, d);
    // check axis hasn't moved (i.e. translated by 0,0,0)
    assertElementXYZTranslated(ws, "axis", axis, V3D(0, 0, 0));
  }

  // ---------- Infinite Plane ----------
  void test_infinite_plane_point_translated_normal_unchanged() {
    const V3D pip(0.0, 0.2, 0.0), normal(0.0, 0.2, 0.0), d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<infinite-plane id=\"P\">"
        << "<point-in-plane x=\"" << pip.X() << "\" y=\"" << pip.Y() << "\" z=\"" << pip.Z() << "\"/>"
        << "<normal-to-plane x=\"" << normal.X() << "\" y=\"" << normal.Y() << "\" z=\"" << normal.Z() << "\"/>"
        << "</infinite-plane><algebra val=\"P\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "point-in-plane", pip, d);
    // check normal hasn't moved (i.e. translated by 0,0,0)
    assertElementXYZTranslated(ws, "normal-to-plane", normal, V3D(0, 0, 0));
  }

  // ---------- Cuboid (centre form) ----------
  void test_cuboid_centre_form_translated() {
    const V3D centre(10.0, 10.0, 10.0), d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<cuboid id=\"CB\">"
        << "<width val=\"2.0\"/><height val=\"4.0\"/><depth val=\"0.2\"/>"
        << "<centre x=\"" << centre.X() << "\" y=\"" << centre.Y() << "\" z=\"" << centre.Z() << "\"/>"
        << "</cuboid><algebra val=\"CB\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "centre", centre, d);
  }

  // ---------- Cuboid (four-point form) ----------
  void test_cuboid_four_point_form_translated_all_points() {
    const V3D p1(1, -0.4, -0.3), p2(1, -0.4, 0.3), p3(-1, -0.4, -0.3), p4(1, 0.4, -0.3);
    const V3D d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<cuboid id=\"shape\">"
        << "<left-front-bottom-point x=\"" << p1.X() << "\" y=\"" << p1.Y() << "\" z=\"" << p1.Z() << "\"/>"
        << "<left-front-top-point x=\"" << p2.X() << "\" y=\"" << p2.Y() << "\" z=\"" << p2.Z() << "\"/>"
        << "<left-back-bottom-point x=\"" << p3.X() << "\" y=\"" << p3.Y() << "\" z=\"" << p3.Z() << "\"/>"
        << "<right-front-bottom-point x=\"" << p4.X() << "\" y=\"" << p4.Y() << "\" z=\"" << p4.Z() << "\"/>"
        << "</cuboid><algebra val=\"shape\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "left-front-bottom-point", p1, d);
    assertElementXYZTranslated(ws, "left-front-top-point", p2, d);
    assertElementXYZTranslated(ws, "left-back-bottom-point", p3, d);
    assertElementXYZTranslated(ws, "right-front-bottom-point", p4, d);
  }

  // ---------- Hexahedron ----------
  void test_hexahedron_all_corners_translated() {

    const V3D p1(0.0, 0.0, 0.0), p2(1.0, 0.0, 0.0), p3(1.0, 1.0, 0.0), p4(0.0, 1.0, 0.0), p5(0.0, 0.0, 2.0),
        p6(0.5, 0.0, 2.0), p7(0.5, 0.5, 2.0), p8(0.0, 0.5, 2.0);
    const V3D d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<hexahedron id=\"shape\">"
        << "<left-back-bottom-point x=\"" << p1.X() << "\" y=\"" << p1.Y() << "\" z=\"" << p1.Z() << "\"/>"
        << "<left-front-bottom-point x=\"" << p2.X() << "\" y=\"" << p2.Y() << "\" z=\"" << p2.Z() << "\"/>"
        << "<right-front-bottom-point x=\"" << p3.X() << "\" y=\"" << p3.Y() << "\" z=\"" << p3.Z() << "\"/>"
        << "<right-back-bottom-point x=\"" << p4.X() << "\" y=\"" << p4.Y() << "\" z=\"" << p4.Z() << "\"/>"
        << "<left-back-top-point x=\"" << p5.X() << "\" y=\"" << p5.Y() << "\" z=\"" << p5.Z() << "\"/>"
        << "<left-front-top-point x=\"" << p6.X() << "\" y=\"" << p6.Y() << "\" z=\"" << p6.Z() << "\"/>"
        << "<right-front-top-point x=\"" << p7.X() << "\" y=\"" << p7.Y() << "\" z=\"" << p7.Z() << "\"/>"
        << "<right-back-top-point x=\"" << p8.X() << "\" y=\"" << p8.Y() << "\" z=\"" << p8.Z() << "\"/>"
        << "</hexahedron><algebra val=\"shape\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "left-back-bottom-point", p1, d);
    assertElementXYZTranslated(ws, "left-front-bottom-point", p2, d);
    assertElementXYZTranslated(ws, "right-front-bottom-point", p3, d);
    assertElementXYZTranslated(ws, "right-back-bottom-point", p4, d);
    assertElementXYZTranslated(ws, "left-back-top-point", p5, d);
    assertElementXYZTranslated(ws, "left-front-top-point", p6, d);
    assertElementXYZTranslated(ws, "right-front-top-point", p7, d);
    assertElementXYZTranslated(ws, "right-back-top-point", p8, d);
  }

  // ---------- Tapered Guide ----------
  void test_tapered_guide_centre_translated_axis_unchanged() {
    const V3D centre(0.0, 5.0, 10.0), axis(0.5, 1.0, 0.0), d(0.1, -0.2, 0.3);
    std::ostringstream xml;
    xml << "<tapered-guide id=\"G\">"
        << "<aperture-start height=\"2.0\" width=\"2.0\"/>"
        << "<length val=\"3.0\"/>"
        << "<aperture-end height=\"4.0\" width=\"4.0\"/>"
        << "<centre x=\"" << centre.X() << "\" y=\"" << centre.Y() << "\" z=\"" << centre.Z() << "\"/>"
        << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
        << "</tapered-guide><algebra val=\"G\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementXYZTranslated(ws, "centre", centre, d);
    // check normal hasn't moved (i.e. translated by 0,0,0)
    assertElementXYZTranslated(ws, "axis", axis, V3D(0, 0, 0));
  }

  // ####### General CSG tests ##############
  // here we will test CSG shape functionality which is independent of the shape itself

  // ---------- Bounding-Box ----------
  void test_bounding_box_limits_are_translated() {
    const V3D centre(0.0, 1.0, 2.0), d(0.1, -0.2, 0.3);
    const double xMin = -1, xMax = 1, yMin = 0, yMax = 2, zMin = 1, zMax = 3;
    std::ostringstream xml;
    xml << "<sphere id=\"s\">"
        << "<centre x=\"" << centre.X() << "\" y=\"" << centre.Y() << "\" z=\"" << centre.Z() << "\"/>"
        << "<radius val=\"1.0\"/></sphere>"
        << "<bounding-box><x-min val=\"" << xMin << "\"/><x-max val=\"" << xMax << "\"/>"
        << "<y-min val=\"" << yMin << "\"/><y-max val=\"" << yMax << "\"/>"
        << "<z-min val=\"" << zMin << "\"/><z-max val=\"" << zMax << "\"/></bounding-box>"
        << "<algebra val=\"s\"/>";

    auto ws = getWorkspaceWithCSGShape(xml.str());
    runTranslate(ws, d);

    assertElementValTranslated(ws, "x-min", xMin, d.X());
    assertElementValTranslated(ws, "x-max", xMax, d.X());
    assertElementValTranslated(ws, "y-min", yMin, d.Y());
    assertElementValTranslated(ws, "y-max", yMax, d.Y());
    assertElementValTranslated(ws, "z-min", zMin, d.Z());
    assertElementValTranslated(ws, "z-max", zMax, d.Z());
  }

  // ---------- Algebra preservation ----------
  void test_algebra_string_is_preserved() {
    const V3D d(0.1, -0.2, 0.3);
    std::string xml;
    xml += "<sphere id=\"a\"><centre x=\"0\" y=\"0\" z=\"0\"/><radius val=\"1\"/></sphere>";
    xml += "<sphere id=\"b\"><centre x=\"2\" y=\"0\" z=\"0\"/><radius val=\"1\"/></sphere>";
    xml += "<algebra val=\"a : b\"/>";

    auto ws = getWorkspaceWithCSGShape(xml);
    runTranslate(ws, d);

    auto doc = parseXml(wsOutXml(ws));
    auto root = docRoot(doc);
    auto alg = first(root, "algebra");
    TS_ASSERT(alg != nullptr);
    TS_ASSERT_EQUALS(alg->getAttribute("val"), std::string("a : b"));
  }

  // ########### Mesh Test ############################
  // Here we will test that the alg also works for a generic mesh
  // (this is just calling the inbuilt translate method, which is already tested, so we will just check it doesn't
  // crash)

  // ---------- Mesh passthrough ----------
  void test_mesh_shape_executes_and_remains_mesh() {
    auto mesh = makeUnitCubeMesh(V3D(0, 0, 0));
    const V3D d(0.1, -0.2, 0.3);

    auto ws = getWorkspaceWithMeshShape(std::move(mesh));
    runTranslate(ws, d);
    auto ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);
    auto meshOut = std::dynamic_pointer_cast<MeshObject>(ei->sample().getShapePtr());
    TS_ASSERT(meshOut != nullptr);
  }

  // ---------- Error paths ----------
  void test_throws_if_no_shape() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    AnalysisDataService::Instance().addOrReplace("test_ws", ws);

    TranslateSampleShape alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", ws->getName());
    alg.setPropertyValue("TranslationVector", "0.1,0.2,0.3");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }
};
