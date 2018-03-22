#ifndef SHAPEFACTORYTEST_H_
#define SHAPEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include <vector>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/AutoPtr.h>

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ShapeFactoryTest : public CxxTest::TestSuite {
public:
  void testCuboid() {
    std::string xmlShape = "<cuboid id=\"shape\"> ";
    xmlShape += R"(<left-front-bottom-point x="0.005" y="-0.1" z="0.0" /> )";
    xmlShape +=
        R"(<left-front-top-point x="0.005" y="-0.1" z="0.0001" />  )";
    xmlShape +=
        R"(<left-back-bottom-point x="-0.005" y="-0.1" z="0.0" />  )";
    xmlShape +=
        R"(<right-front-bottom-point x="0.005" y="0.1" z="0.0" />  )";
    xmlShape += "</cuboid> ";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, 0.00001)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 0.001)));
    TS_ASSERT(shape_sptr->isValid(V3D(-0.004, 0.0, 0.00001)));
    TS_ASSERT(!shape_sptr->isValid(V3D(-0.006, 0.0, 0.00001)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.09, 0.00001)));
  }

  void testAlternateCuboid() {
    std::string xmlShape;
    xmlShape += "<cuboid id=\"some-shape\">";
    xmlShape += "<height val=\"0.2\" />";
    xmlShape += "<width val=\"0.1\" />";
    xmlShape += "<depth val=\"0.4\" />";
    xmlShape += R"(<centre x="1.0" y="1.0" z="1.0" />)";
    xmlShape += R"(<axis x="1" y="0" z="0" />)"; // Note non-default axis.
    xmlShape += "</cuboid>";
    xmlShape += "<algebra val=\"some-shape\" />";

    auto cuboid = getObject(xmlShape);

    TS_ASSERT(cuboid->isValid(V3D(1.20, 1.10, 0.95)));
    TS_ASSERT(cuboid->isValid(V3D(0.80, 1.10, 0.95)));
    TS_ASSERT(cuboid->isValid(V3D(1.20, 0.90, 0.95)));
    TS_ASSERT(cuboid->isValid(V3D(0.80, 0.90, 0.95)));
    TS_ASSERT(cuboid->isValid(V3D(1.20, 1.10, 1.05)));
    TS_ASSERT(cuboid->isValid(V3D(0.80, 1.10, 1.05)));
    TS_ASSERT(cuboid->isValid(V3D(1.20, 0.90, 1.05)));
    TS_ASSERT(cuboid->isValid(V3D(0.80, 0.90, 1.05)));

    TS_ASSERT(!cuboid->isValid(V3D(1.21, 1.11, 0.94)));
    TS_ASSERT(!cuboid->isValid(V3D(0.79, 1.11, 0.94)));
    TS_ASSERT(!cuboid->isValid(V3D(1.21, 0.89, 0.94)));
    TS_ASSERT(!cuboid->isValid(V3D(0.79, 0.89, 0.94)));
    TS_ASSERT(!cuboid->isValid(V3D(1.21, 1.11, 1.06)));
    TS_ASSERT(!cuboid->isValid(V3D(0.79, 1.11, 1.06)));
    TS_ASSERT(!cuboid->isValid(V3D(1.21, 0.89, 1.06)));
    TS_ASSERT(!cuboid->isValid(V3D(0.79, 0.89, 1.06)));
  }

  void testAlternateCuboidDefaultAxis() {
    std::string xmlShape;
    xmlShape += "<cuboid id=\"some-shape\">";
    xmlShape += "<height val=\"0.2\" />";
    xmlShape += "<width val=\"0.1\" />";
    xmlShape += "<depth val=\"0.4\" />";
    xmlShape += R"(<centre x="1.0" y="1.0" z="1.0" />)";
    xmlShape += "</cuboid>";
    xmlShape += "<algebra val=\"some-shape\" />";

    auto cuboid = getObject(xmlShape);

    TS_ASSERT(cuboid->isValid(V3D(1.05, 1.10, 1.20)));
    TS_ASSERT(cuboid->isValid(V3D(1.05, 1.10, 0.80)));
    TS_ASSERT(cuboid->isValid(V3D(1.05, 0.90, 1.20)));
    TS_ASSERT(cuboid->isValid(V3D(1.05, 0.90, 0.80)));
    TS_ASSERT(cuboid->isValid(V3D(0.95, 1.10, 1.20)));
    TS_ASSERT(cuboid->isValid(V3D(0.95, 1.10, 0.80)));
    TS_ASSERT(cuboid->isValid(V3D(0.95, 0.90, 1.20)));
    TS_ASSERT(cuboid->isValid(V3D(0.95, 0.90, 0.80)));

    TS_ASSERT(!cuboid->isValid(V3D(1.06, 1.11, 1.21)));
    TS_ASSERT(!cuboid->isValid(V3D(1.06, 1.11, 0.79)));
    TS_ASSERT(!cuboid->isValid(V3D(1.06, 0.89, 1.21)));
    TS_ASSERT(!cuboid->isValid(V3D(1.06, 0.89, 0.79)));
    TS_ASSERT(!cuboid->isValid(V3D(0.94, 1.11, 1.21)));
    TS_ASSERT(!cuboid->isValid(V3D(0.94, 1.11, 0.79)));
    TS_ASSERT(!cuboid->isValid(V3D(0.94, 0.89, 1.21)));
    TS_ASSERT(!cuboid->isValid(V3D(0.94, 0.89, 0.79)));
  }

  void testAlternateCuboidDefaultCentre() {
    std::string xmlShape;
    xmlShape += "<cuboid id=\"some-shape\">";
    xmlShape += "<height val=\"0.2\" />";
    xmlShape += "<width val=\"0.1\" />";
    xmlShape += "<depth val=\"0.4\" />";
    xmlShape += R"(<axis x="0" y="0" z="1" />)";
    xmlShape += "</cuboid>";
    xmlShape += "<algebra val=\"some-shape\" />";

    auto cuboid = getObject(xmlShape);

    TS_ASSERT(cuboid->isValid(V3D(0.05, 0.10, 0.20)));
    TS_ASSERT(cuboid->isValid(V3D(0.05, 0.10, -0.20)));
    TS_ASSERT(cuboid->isValid(V3D(0.05, -0.10, 0.20)));
    TS_ASSERT(cuboid->isValid(V3D(0.05, -0.10, -0.20)));
    TS_ASSERT(cuboid->isValid(V3D(-0.05, 0.10, 0.20)));
    TS_ASSERT(cuboid->isValid(V3D(-0.05, 0.10, -0.20)));
    TS_ASSERT(cuboid->isValid(V3D(-0.05, -0.10, 0.20)));
    TS_ASSERT(cuboid->isValid(V3D(-0.05, -0.10, -0.20)));

    TS_ASSERT(!cuboid->isValid(V3D(0.06, 0.11, 0.21)));
    TS_ASSERT(!cuboid->isValid(V3D(0.06, 0.11, -0.21)));
    TS_ASSERT(!cuboid->isValid(V3D(0.06, -0.11, 0.21)));
    TS_ASSERT(!cuboid->isValid(V3D(0.06, -0.11, -0.21)));
    TS_ASSERT(!cuboid->isValid(V3D(-0.06, 0.11, 0.21)));
    TS_ASSERT(!cuboid->isValid(V3D(-0.06, 0.11, -0.21)));
    TS_ASSERT(!cuboid->isValid(V3D(-0.06, -0.11, 0.21)));
    TS_ASSERT(!cuboid->isValid(V3D(-0.06, -0.11, -0.21)));
  }

  void testRelayShapeXML() {
    // Create a cuboid.
    std::string xmlShape = "<cuboid id=\"shape\"> ";
    xmlShape += R"(<left-front-bottom-point x="0.005" y="-0.1" z="0.0"/> )";
    xmlShape += R"(<left-front-top-point x="0.005" y="-0.1" z="0.0001"/>  )";
    xmlShape += R"(<left-back-bottom-point x="-0.005" y="-0.1" z="0.0"/>  )";
    xmlShape += R"(<right-front-bottom-point x="0.005" y="0.1" z="0.0"/>  )";
    xmlShape += "</cuboid> ";
    xmlShape += "<algebra val=\"shape\"/> ";

    std::string expectedXML =
        "<type name=\"userShape\"> " + xmlShape + " </type>";

    auto shape_sptr = getObject(xmlShape);
    TSM_ASSERT("Empty shape xml given.", !shape_sptr->getShapeXML().empty());
    TSM_ASSERT_EQUALS("Shape xml not relayed through to shape object.",
                      expectedXML, shape_sptr->getShapeXML());
  }

  void testHexahedron() {
    std::string xmlShape = "<hexahedron id=\"shape\"> ";
    xmlShape += R"(<left-back-bottom-point  x="0.0" y="0.0" z="0.0"  /> )";
    xmlShape += R"(<left-front-bottom-point x="1.0" y="0.0" z="0.0"  /> )";
    xmlShape += R"(<right-front-bottom-point x="1.0" y="1.0" z="0.0"  /> )";
    xmlShape += R"(<right-back-bottom-point  x="0.0" y="1.0" z="0.0"  /> )";
    xmlShape += R"(<left-back-top-point  x="0.0" y="0.0" z="2.0"  /> )";
    xmlShape += R"(<left-front-top-point  x="0.5" y="0.0" z="2.0"  /> )";
    xmlShape += R"(<right-front-top-point  x="0.5" y="0.5" z="2.0"  /> )";
    xmlShape += R"(<right-back-top-point  x="0.0" y="0.5" z="2.0"  /> )";
    xmlShape += "</hexahedron> ";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(1.1, 0.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.9, 0.9, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.49, 0.49, 1.99)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.49, 0.81, 1.99)));
  }

  void testHexahedron2() {
    std::string xmlShape = "<hexahedron id=\"shape\"> ";
    xmlShape +=
        R"(<left-front-bottom-point x="0.0" y="-0.0031" z="-0.037"  /> )";
    xmlShape +=
        R"(<right-front-bottom-point x="0.0" y="0.0031" z="-0.037"  /> )";
    xmlShape +=
        R"(<left-front-top-point x="0.0" y="-0.0104" z="0.037"  /> )";
    xmlShape +=
        R"(<right-front-top-point x="0.0" y="0.0104" z="0.037"  /> )";
    xmlShape +=
        R"(<left-back-bottom-point x="0.005" y="-0.0031" z="-0.037"  /> )";
    xmlShape +=
        R"(<right-back-bottom-point x="0.005" y="0.0031" z="-0.037"  /> )";
    xmlShape +=
        R"(<left-back-top-point x="0.005" y="-0.0104" z="0.037"  /> )";
    xmlShape +=
        R"(<right-back-top-point x="0.005" y="0.0104" z="0.037"  /> )";
    xmlShape += "</hexahedron> ";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(0.0001, 0.0, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0055, 0.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.004, 0.003, 0.003)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0. - 0.003, -0.036)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, -0.003, -0.038)));
  }

  void testTaperedGuideDefaults() {
    std::string xmlShape = "<tapered-guide id=\"shape\">";
    xmlShape += R"(<aperture-start height="2.0" width="2.0" />)";
    xmlShape += "<length val=\"2.0\" />";
    xmlShape += R"(<aperture-end height="4.0" width="4.0" />)";
    xmlShape += "</tapered-guide>";
    xmlShape += "<algebra val=\"shape\"/>";

    auto shape_sptr = getObject(xmlShape);

    // Vertices.
    TS_ASSERT(shape_sptr->isValid(V3D(2.0, -2.0, 2.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(2.0, 2.0, 2.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(-2.0, 2.0, 2.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(-2.0, -2.0, 2.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(1.0, -1.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(1.0, 1.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(-1.0, 1.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(-1.0, -1.0, 0.0)));

    // Middle of edges that connect front and back faces.
    TS_ASSERT(shape_sptr->isValid(V3D(1.5, -1.5, 1.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(1.5, 1.5, 1.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(-1.5, 1.5, 1.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(-1.5, -1.5, 1.0)));

    // Close to, but outside of shape.
    TS_ASSERT(!shape_sptr->isValid(V3D(1.6, -1.6, 1.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(1.6, 1.6, 1.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(-1.6, 1.6, 1.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(-1.6, -1.6, 1.0)));
  }

  void testTaperedGuideDifferentAxisAndCentre() {
    std::string xmlShape = "<tapered-guide id=\"shape\">";
    xmlShape += R"(<aperture-start height="2.0" width="2.0" />)";
    xmlShape += "<length val=\"2.0\" />";
    xmlShape += R"(<aperture-end height="4.0" width="4.0" />)";
    xmlShape += R"(<centre x="0.0" y="0.0" z="1.0" />)";
    xmlShape += R"(<axis x="1.0" y="0.0" z="0" />)";
    xmlShape += "</tapered-guide>";
    xmlShape += "<algebra val=\"shape\"/>";

    auto shape_sptr = getObject(xmlShape);

    // Vertices.
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, -1.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 1.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, -1.0, 2.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 1.0, 2.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(2.0, -2.0, -1.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(2.0, 2.0, -1.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(2.0, -2.0, 3.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(2.0, 2.0, 3.0)));

    // Middle of edges that connect front and back faces.
    TS_ASSERT(shape_sptr->isValid(V3D(1.0, -1.5, -0.5)));
    TS_ASSERT(shape_sptr->isValid(V3D(1.0, 1.5, -0.5)));
    TS_ASSERT(shape_sptr->isValid(V3D(1.0, -1.5, 2.5)));
    TS_ASSERT(shape_sptr->isValid(V3D(1.0, 1.5, 2.5)));

    // Close to, but outside of shape.
    TS_ASSERT(!shape_sptr->isValid(V3D(1.0, -1.6, -0.6)));
    TS_ASSERT(!shape_sptr->isValid(V3D(1.0, 1.6, -0.6)));
    TS_ASSERT(!shape_sptr->isValid(V3D(1.0, -1.6, 2.6)));
    TS_ASSERT(!shape_sptr->isValid(V3D(1.0, 1.6, 2.6)));
  }

  void testSphere() {
    // algebra line is essential
    std::string xmlShape = "<sphere id=\"shape\"> ";
    xmlShape += R"(<centre x="4.1"  y="2.1" z="8.1" /> )";
    xmlShape += "<radius val=\"3.2\" /> ";
    xmlShape += "</sphere>";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(4.1, 2.1, 8.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(47.1, 2.1, 8.1)));
    TS_ASSERT(shape_sptr->isValid(V3D(5.1, 2.1, 8.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(-0.006, 0.0, 0.00001)));
    TS_ASSERT(shape_sptr->isValid(V3D(4.1, 2.1, 9.1)));
  }

  void testTwoSpheres() {
    // algebra line is essential
    std::string xmlShape = "<sphere id=\"shape1\"> ";
    xmlShape += R"(<centre x="4.1"  y="2.1" z="8.1" /> )";
    xmlShape += "<radius val=\"3.2\" /> ";
    xmlShape += "</sphere>";
    xmlShape += "<sphere id=\"shape2\"> ";
    xmlShape += R"(<centre x="2.1"  y="2.1" z="8.1" /> )";
    xmlShape += "<radius val=\"3.2\" /> ";
    xmlShape += "</sphere>";
    xmlShape += "<algebra val=\"shape1 : shape2\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(4.1, 2.1, 8.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(47.1, 2.1, 8.1)));
    TS_ASSERT(shape_sptr->isValid(V3D(5.1, 2.1, 8.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(-0.006, 0.0, 0.00001)));
    TS_ASSERT(shape_sptr->isValid(V3D(4.1, 2.1, 9.1)));
    TS_ASSERT(shape_sptr->isValid(V3D(-0.8, 2.1, 9.1)));
    TS_ASSERT(shape_sptr->isValid(V3D(7.1, 2.1, 9.1)));
  }

  void testTwoSpheresNoAlgebraString() {
    // algebra line is essential
    std::string xmlShape = "<sphere id=\"shape1\"> ";
    xmlShape += R"(<centre x="4.1"  y="2.1" z="8.1" /> )";
    xmlShape += "<radius val=\"3.2\" /> ";
    xmlShape += "</sphere>";
    xmlShape += "<sphere id=\"shape2\"> ";
    xmlShape += R"(<centre x="2.1"  y="2.1" z="8.1" /> )";
    xmlShape += "<radius val=\"3.2\" /> ";
    xmlShape += "</sphere>";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(4.1, 2.1, 8.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(47.1, 2.1, 8.1)));
    TS_ASSERT(shape_sptr->isValid(V3D(5.1, 2.1, 8.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(-0.006, 0.0, 0.00001)));
    TS_ASSERT(shape_sptr->isValid(V3D(4.1, 2.1, 9.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(-0.8, 2.1, 9.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(7.1, 2.1, 9.1)));
  }

  void testSphereWithDefaultCentre() {
    std::string xmlShape = "<sphere id=\"shape\"> ";
    xmlShape += "<radius val=\"1.0\" /> ";
    xmlShape += "</sphere>";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(1.0, 0.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 1.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, 1.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(-1.0, 0.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, -1.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, -1.0)));

    TS_ASSERT(!shape_sptr->isValid(V3D(1.1, 0.0, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 1.1, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 1.1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(-1.1, 0.0, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, -1.1, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, -1.1)));
  }

  void testCylinder() {
    // algebra line is essential
    std::string xmlShape = "<cylinder id=\"shape\"> ";
    xmlShape += R"(<centre-of-bottom-base x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<radius val=\"0.1\" /> ";
    xmlShape += "<height val=\"3\" /> ";
    xmlShape += "</cylinder>";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, 1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 10)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.05, 1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.15, 1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.01, 0.01, 1)));
  }

  void testCylinderNoAlgebraString() {
    // algebra line is essential
    std::string xmlShape = "<cylinder id=\"shape\"> ";
    xmlShape += R"(<centre-of-bottom-base x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<radius val=\"0.1\" /> ";
    xmlShape += "<height val=\"3\" /> ";
    xmlShape += "</cylinder>";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, 1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 10)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.05, 1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.15, 1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.01, 0.01, 1)));
  }

  void testCylinderTwoAlgebraStrings() {
    // algebra line is essential
    std::string xmlShape = "<cylinder id=\"shape\"> ";
    xmlShape += R"(<centre-of-bottom-base x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<radius val=\"0.1\" /> ";
    xmlShape += "<height val=\"3\" /> ";
    xmlShape += "</cylinder>";
    xmlShape += "<algebra val=\"shape\" /> ";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 10)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.05, 1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.15, 1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.01, 0.01, 1)));
  }

  void testHollowCylinder() {
    const std::string xmlShape =
        "<hollow-cylinder id=\"an-1\">"
        " <centre-of-bottom-base x=\"0.0\" y=\"-0.025\" z=\"0.0\" />"
        " <axis x =\"0.0\" y=\"1.0\" z=\"0.0\" />"
        " <inner-radius val=\"0.006\" />"
        " <outer-radius val=\"0.0065\" />"
        " <height val=\"0.05\" />"
        "</hollow-cylinder> ";

    auto shape_sptr = getObject(xmlShape);
    TS_ASSERT(shape_sptr);
    TS_ASSERT(shape_sptr->hasValidShape());
    // centre is not valid
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 0.0)));
    // inside a wall
    TS_ASSERT(shape_sptr->isValid(V3D(-0.0061, 0.0, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0061, 0.0, 0.0)));
    // inside in Y
    TS_ASSERT(shape_sptr->isValid(V3D(0.0061, -0.024, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0061, 0.024, 0.0)));
    // outside in Y
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0061, -0.026, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0061, 0.026, 0.0)));
  }

  void testHollowCylinderWithOuterSmallerThanInner() {
    const std::string xmlShape =
        "<hollow-cylinder id=\"an-1\">"
        " <centre-of-bottom-base x=\"0.0\" y=\"-0.025\" z=\"0.0\" />"
        " <axis x =\"0.0\" y=\"1.0\" z=\"0.0\" />"
        " <inner-radius val=\"0.0065\" />"
        " <outer-radius val=\"0.006\" />"
        " <height val=\"0.05\" />"
        "</hollow-cylinder> ";

    auto shape_sptr = getObject(xmlShape);
    TS_ASSERT(shape_sptr);
    TS_ASSERT(!shape_sptr->hasValidShape());
  }

  void testHollowCylinderWithNegativeHeight() {
    const std::string xmlShape =
        "<hollow-cylinder id=\"an-1\">"
        " <centre-of-bottom-base x=\"0.0\" y=\"-0.025\" z=\"0.0\" />"
        " <axis x =\"0.0\" y=\"1.0\" z=\"0.0\" />"
        " <inner-radius val=\"0.006\" />"
        " <outer-radius val=\"0.0065\" />"
        " <height val=\"-0.05\" />"
        "</hollow-cylinder> ";

    auto shape_sptr = getObject(xmlShape);
    TS_ASSERT(shape_sptr);
    TS_ASSERT(!shape_sptr->hasValidShape());
  }

  void testHollowCylinderWithNegativeOrZeroInnerRadius() {
    const std::string xmlShape =
        "<hollow-cylinder id=\"an-1\">"
        " <centre-of-bottom-base x=\"0.0\" y=\"-0.025\" z=\"0.0\" />"
        " <axis x =\"0.0\" y=\"1.0\" z=\"0.0\" />"
        " <inner-radius val=\"-0.0\" />"
        " <outer-radius val=\"0.0065\" />"
        " <height val=\"-0.05\" />"
        "</hollow-cylinder> ";

    auto shape_sptr = getObject(xmlShape);
    TS_ASSERT(shape_sptr);
    TS_ASSERT(!shape_sptr->hasValidShape());
  }

  void testHollowCylinderWithNegativeOuterRadius() {
    const std::string xmlShape =
        "<hollow-cylinder id=\"an-1\">"
        " <centre-of-bottom-base x=\"0.0\" y=\"-0.025\" z=\"0.0\" />"
        " <axis x =\"0.0\" y=\"1.0\" z=\"0.0\" />"
        " <inner-radius val=\"0.006\" />"
        " <outer-radius val=\"-0.0\" />"
        " <height val=\"-0.05\" />"
        "</hollow-cylinder> ";

    auto shape_sptr = getObject(xmlShape);
    TS_ASSERT(shape_sptr);
    TS_ASSERT(!shape_sptr->hasValidShape());
  }

  void testInfiniteCylinder() {
    // algebra line is essential
    std::string xmlShape = "<infinite-cylinder id=\"shape\"> ";
    xmlShape += R"(<centre x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<radius val=\"0.1\" /> ";
    xmlShape += "</infinite-cylinder>";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, 1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, 10)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.05, 1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.15, 1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.01, 0.01, 1)));
  }

  void testCone() {
    // algebra line is essential
    std::string xmlShape = "<cone id=\"shape\"> ";
    xmlShape += R"(<tip-point x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<angle val=\"8.1\" /> ";
    xmlShape += "<height val=\"4\" /> ";
    xmlShape += "</cone>";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, -1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.001, 1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.001, -1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.01, 0.01, -1)));
  }

  void testConeUseDirectStringArgument() {
    // algebra line is essential
    std::string xmlShape = "<cone id=\"shape\"> ";
    xmlShape += R"(<tip-point x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<angle val=\"8.1\" /> ";
    xmlShape += "<height val=\"4\" /> ";
    xmlShape += "</cone>";
    xmlShape += "<algebra val=\"shape\" /> ";

    ShapeFactory sFactory;
    auto shape_sptr = sFactory.createShape(xmlShape);

    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, -1)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.001, 1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.001, -1)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.01, 0.01, -1)));
  }

  void testComplement() {
    std::string xmlShape = "<cylinder id=\"stick\"> ";
    xmlShape += R"(<centre-of-bottom-base x="-0.5" y="0.0" z="0.0" />)";
    xmlShape += R"(<axis x="1.0" y="0.0" z="0.0" />)";
    xmlShape += "<radius val=\"0.05\" />";
    xmlShape += "<height val=\"1.0\" />";
    xmlShape += "</cylinder>";
    xmlShape += "<sphere id=\"some-sphere\">";
    xmlShape += R"(<centre x="0.0"  y="0.0" z="0.0" />)";
    xmlShape += "<radius val=\"0.5\" />";
    xmlShape += "</sphere>";
    xmlShape += "<algebra val=\"some-sphere # stick\" />";

    auto shape_sptr = getObject(xmlShape);

    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, -0.04)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.0, -0.06)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.04, 0.0)));
    TS_ASSERT(shape_sptr->isValid(V3D(0.0, 0.06, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.06, 0.0, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.51, 0.0, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.51, 0.0)));
    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 0.51)));
  }

  void testNoneExistingShape() {
    // algebra line is essential
    std::string xmlShape = "<c5one id=\"shape\"> ";
    xmlShape += R"(<tip-point x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<angle val=\"8.1\" /> ";
    xmlShape += "<height val=\"4\" /> ";
    xmlShape += "</c5one>";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape); // should return empty object

    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 1)));
  }

  void testTypingErrorInSubElement() {
    // algebra line is essential
    std::string xmlShape = "<cone id=\"shape\"> ";
    xmlShape += R"(<tip-point x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<angle val=\"8.1\" /> ";
    xmlShape += "<heeight val=\"4\" /> ";
    xmlShape += "</cone>";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape); // should return empty object

    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 1)));
  }

  void testTypingErrorInAttribute() {
    // algebra line is essential
    std::string xmlShape = "<cone id=\"shape\"> ";
    xmlShape += R"(<tip-point x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="1" /> )";
    xmlShape += "<angle val=\"8.1\" /> ";
    xmlShape += "<height vaal=\"4\" /> ";
    xmlShape += "</cone>";
    xmlShape += "<algebra val=\"shape\" /> ";

    auto shape_sptr = getObject(xmlShape); // should return empty object

    TS_ASSERT(!shape_sptr->isValid(V3D(0.0, 0.0, 1)));
  }

  boost::shared_ptr<CSGObject> getObject(std::string xmlShape) {
    std::string shapeXML = "<type name=\"userShape\"> " + xmlShape + " </type>";

    // Set up the DOM parser and parse xml string
    DOMParser pParser;
    Poco::AutoPtr<Document> pDoc = pParser.parseString(shapeXML);

    // Get pointer to root element
    Element *pRootElem = pDoc->documentElement();

    // convert into a Geometry object
    ShapeFactory sFactory;
    auto shape_sptr = sFactory.createShape(pRootElem);
    return shape_sptr;
  }

private:
  std::string inputFile;
};

#endif /*SHAPEFACTORYTEST_H_*/
