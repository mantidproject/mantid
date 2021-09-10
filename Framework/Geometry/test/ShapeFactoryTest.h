// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include <vector>

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

using Poco::XML::Document;
using Poco::XML::DOMParser;
using Poco::XML::Element;

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ShapeFactoryTest : public CxxTest::TestSuite {
public:
  void testCuboid() {
    std::string xmlShape = "<cuboid id=\"shape\"> ";
    xmlShape += R"(<left-front-bottom-point x="0.005" y="-0.1" z="0.0" /> )";
    xmlShape += R"(<left-front-top-point x="0.005" y="-0.1" z="0.0001" />  )";
    xmlShape += R"(<left-back-bottom-point x="-0.005" y="-0.1" z="0.0" />  )";
    xmlShape += R"(<right-front-bottom-point x="0.005" y="0.1" z="0.0" />  )";
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

    std::string expectedXML = "<type name=\"userShape\"> " + xmlShape + " </type>";

    auto shape_sptr = getObject(xmlShape);
    TSM_ASSERT("Empty shape xml given.", !shape_sptr->getShapeXML().empty());
    TSM_ASSERT_EQUALS("Shape xml not relayed through to shape object.", expectedXML, shape_sptr->getShapeXML());
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
    xmlShape += R"(<left-front-bottom-point x="0.0" y="-0.0031" z="-0.037"  /> )";
    xmlShape += R"(<right-front-bottom-point x="0.0" y="0.0031" z="-0.037"  /> )";
    xmlShape += R"(<left-front-top-point x="0.0" y="-0.0104" z="0.037"  /> )";
    xmlShape += R"(<right-front-top-point x="0.0" y="0.0104" z="0.037"  /> )";
    xmlShape += R"(<left-back-bottom-point x="0.005" y="-0.0031" z="-0.037"  /> )";
    xmlShape += R"(<right-back-bottom-point x="0.005" y="0.0031" z="-0.037"  /> )";
    xmlShape += R"(<left-back-top-point x="0.005" y="-0.0104" z="0.037"  /> )";
    xmlShape += R"(<right-back-top-point x="0.005" y="0.0104" z="0.037"  /> )";
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
    const std::string xmlShape = "<hollow-cylinder id=\"an-1\">"
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
    const std::string xmlShape = "<hollow-cylinder id=\"an-1\">"
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
    const std::string xmlShape = "<hollow-cylinder id=\"an-1\">"
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
    const std::string xmlShape = "<hollow-cylinder id=\"an-1\">"
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
    const std::string xmlShape = "<hollow-cylinder id=\"an-1\">"
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

  std::shared_ptr<CSGObject> getObject(const std::string &xmlShape) {
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

  void testGenerateXRotation() {
    auto rotationMatrix = ShapeFactory::generateXRotation(90.0 * M_PI / 180);
    std::vector<double> vectorToMatch = {1, 0, 0, 0, 0, -1, 0, 1, 0};
    compareMatrix(vectorToMatch, rotationMatrix);
  }

  void testGenerateYRotation() {

    auto rotationMatrix = ShapeFactory::generateYRotation(90.0 * M_PI / 180);
    std::vector<double> vectorToMatch = {0, 0, 1, 0, 1, 0, -1, 0, 0};
    compareMatrix(vectorToMatch, rotationMatrix);
  }

  void testGenerateZRotation() {
    auto rotationMatrix = ShapeFactory::generateZRotation(90.0 * M_PI / 180);
    std::vector<double> vectorToMatch = {0, -1, 0, 1, 0, 0, 0, 0, 1};
    compareMatrix(vectorToMatch, rotationMatrix);
  }

  void testGenerateRotationMatrix() {
    auto rotationMatrix = ShapeFactory::generateMatrix(90.0 * M_PI / 180, 60.0 * M_PI / 180, 30.0 * M_PI / 180);
    std::vector<double> vectorToMatch = {0.4330127,  0.7500000,  0.5000000, 0.2500000, 0.4330127,
                                         -0.8660254, -0.8660254, 0.5000000, 0.0000000};
    compareMatrix(vectorToMatch, rotationMatrix);
  }

  void testRotateCuboid() {
    std::string xmlCuboidStart = "<cuboid id=\"cuboid\"> ";
    xmlCuboidStart += "<height val=\"2.0\" /> ";
    xmlCuboidStart += "<width val=\"2.0\" /> ";
    xmlCuboidStart += "<depth val=\"4.0\" /> ";
    xmlCuboidStart += "<centre x=\"10.0\" y=\"10.0\" z=\"10.0\"/> ";
    xmlCuboidStart += "<axis x=\"0\" y=\"0\" z=\"1\"/> ";
    std::string xmlCuboidEnd = "</cuboid> ";
    xmlCuboidEnd += "<algebra val=\"cuboid\"/> ";

    V3D top = V3D(10.0, 10.0, 12.0);
    V3D centre = V3D(10.0, 10.0, 10.0);
    V3D topRotate = V3D(10.0, 8.0, 10.0);
    V3D topRotateAll = V3D(10.0, -12.0, 10.0);
    V3D centreRotateAll = V3D(10.0, -10.0, 10.0);

    std::string xmlCuboid = xmlCuboidStart + xmlCuboidEnd;
    auto cuboid_sptr = getObject(xmlCuboid);
    TS_ASSERT(cuboid_sptr->isValid(top));
    TS_ASSERT(cuboid_sptr->isValid(centre));
    TS_ASSERT(!cuboid_sptr->isValid(topRotate));
    TS_ASSERT(!cuboid_sptr->isValid(centreRotateAll));

    std::string xmlCuboidRotate = xmlCuboidStart + "<rotate x=\"90\" y=\"0\" z=\"0\" /> " + xmlCuboidEnd;
    auto cuboidRotate_sptr = getObject(xmlCuboidRotate);
    TS_ASSERT(cuboidRotate_sptr->isValid(topRotate));
    TS_ASSERT(cuboidRotate_sptr->isValid(centre));
    TS_ASSERT(!cuboidRotate_sptr->isValid(top));
    TS_ASSERT(!cuboidRotate_sptr->isValid(centreRotateAll));

    std::string xmlCuboidRotateAll = xmlCuboid + "<rotate-all x=\"90\" y=\"0\" z=\"0\" /> ";
    auto cuboidRotateAll_sptr = getObject(xmlCuboidRotateAll);
    TS_ASSERT(cuboidRotateAll_sptr->isValid(topRotateAll));
    TS_ASSERT(cuboidRotateAll_sptr->isValid(centreRotateAll));
    TS_ASSERT(!cuboidRotateAll_sptr->isValid(topRotate));
    TS_ASSERT(!cuboidRotateAll_sptr->isValid(centre));
  }

  void testRotateTaperedGuide() {
    std::string xmlGuideStart = "<tapered-guide id=\"Guide\"> ";
    xmlGuideStart += "<aperture-start height=\"2.0\" width=\"2.0\" /> ";
    xmlGuideStart += "<length val=\"4.0\" /> ";
    xmlGuideStart += "<aperture-end height=\"4.0\" width=\"4.0\" /> ";
    xmlGuideStart += "<centre x=\"10.0\" y=\"10.0\" z=\"10.0\"/> ";
    xmlGuideStart += "<axis x=\"0\" y=\"0\" z=\"1\"/> ";
    std::string xmlGuideEnd = "</tapered-guide> ";
    xmlGuideEnd += "<algebra val=\"Guide\"/> ";

    V3D corner = V3D(8.0, 12.0, 14.0);
    V3D centre = V3D(10.0, 10.0, 10.0); // Actually centre of the base (aperture-start)
    V3D cornerRotate = V3D(8.0, 6.0, 12.0);
    V3D cornerRotateAll = V3D(12, -14.0, 8.0);
    V3D centreRotateAll = V3D(10.0, -10.0, 10.0);

    std::string xmlGuide = xmlGuideStart + xmlGuideEnd;
    auto guide_sptr = getObject(xmlGuide);
    TS_ASSERT(guide_sptr->isValid(corner));
    TS_ASSERT(guide_sptr->isValid(centre));
    TS_ASSERT(!guide_sptr->isValid(cornerRotate));
    TS_ASSERT(!guide_sptr->isValid(centreRotateAll));

    std::string xmlGuideRotate = xmlGuideStart + "<rotate x=\"90\" y=\"0\" z=\"0\" /> " + xmlGuideEnd;
    auto guideRotate_sptr = getObject(xmlGuideRotate);
    TS_ASSERT(guideRotate_sptr->isValid(cornerRotate));
    TS_ASSERT(guideRotate_sptr->isValid(centre));
    TS_ASSERT(!guideRotate_sptr->isValid(corner));
    TS_ASSERT(!guideRotate_sptr->isValid(centreRotateAll));

    std::string xmlGuideRotateAll = xmlGuide + "<rotate-all x=\"90\" y=\"0\" z=\"0\" /> ";
    auto guideRotateAll_sptr = getObject(xmlGuideRotateAll);
    TS_ASSERT(guideRotateAll_sptr->isValid(cornerRotateAll));
    TS_ASSERT(guideRotateAll_sptr->isValid(centreRotateAll));
    TS_ASSERT(!guideRotateAll_sptr->isValid(cornerRotate));
    TS_ASSERT(!guideRotateAll_sptr->isValid(centre));
  }

  void testRotateCylinder() {
    std::string xmlCylinderStart = "<cylinder id=\"Cylinder\"> ";
    xmlCylinderStart += "<height val=\"2.0\" /> ";
    xmlCylinderStart += "<radius val=\"0.2\" /> ";
    xmlCylinderStart += "<centre-of-bottom-base x=\"10.0\" y=\"10.0\" z=\"10.0\"/> ";
    xmlCylinderStart += "<axis x=\"0\" y=\"0\" z=\"1\"/> ";
    std::string xmlCylinderEnd = "</cylinder> ";
    xmlCylinderEnd += "<algebra val=\"Cylinder\"/> ";

    V3D top = V3D(10.0, 10.0, 12.0);
    V3D centre = V3D(10.0, 10.0, 11.0);
    V3D topRotate = V3D(10.0, 9.0, 11.0);
    V3D topRotateAll = V3D(10.0, -12.0, 10.0);
    V3D centreRotateAll = V3D(10.0, -11.0, 10.0);

    std::string xmlCylinder = xmlCylinderStart + xmlCylinderEnd;
    auto cylinder_sptr = getObject(xmlCylinder);
    TS_ASSERT(cylinder_sptr->isValid(top));
    TS_ASSERT(cylinder_sptr->isValid(centre));
    TS_ASSERT(!cylinder_sptr->isValid(topRotate));
    TS_ASSERT(!cylinder_sptr->isValid(centreRotateAll));

    std::string xmlCylinderRotate = xmlCylinderStart + "<rotate x=\"90\" y=\"0\" z=\"0\" /> " + xmlCylinderEnd;
    auto cylinderRotate_sptr = getObject(xmlCylinderRotate);
    TS_ASSERT(cylinderRotate_sptr->isValid(topRotate));
    TS_ASSERT(cylinderRotate_sptr->isValid(centre));
    TS_ASSERT(!cylinderRotate_sptr->isValid(top));
    TS_ASSERT(!cylinderRotate_sptr->isValid(centreRotateAll));

    std::string xmlCylinderRotateAll = xmlCylinder + "<rotate-all x=\"90\" y=\"0\" z=\"0\" /> ";
    auto cylinderRotateAll_sptr = getObject(xmlCylinderRotateAll);
    TS_ASSERT(cylinderRotateAll_sptr->isValid(topRotateAll));
    TS_ASSERT(cylinderRotateAll_sptr->isValid(centreRotateAll));
    TS_ASSERT(!cylinderRotateAll_sptr->isValid(topRotate));
    TS_ASSERT(!cylinderRotateAll_sptr->isValid(centre));
  }

  void testRotateHollowCylinder() {
    std::string xmlHollowCylinderStart = "<hollow-cylinder id=\"HollowCylinder\"> ";
    xmlHollowCylinderStart += "<height val=\"20.0\" /> ";
    xmlHollowCylinderStart += "<inner-radius val=\"1.0\" /> ";
    xmlHollowCylinderStart += "<outer-radius val=\"5.0\" /> ";
    xmlHollowCylinderStart += "<centre-of-bottom-base x=\"100.0\" y=\"100.0\" z=\"100.0\"/> ";
    xmlHollowCylinderStart += "<axis x=\"0\" y=\"0\" z=\"1\"/> ";
    std::string xmlHollowCylinderEnd = "</hollow-cylinder> ";
    xmlHollowCylinderEnd += "<algebra val=\"HollowCylinder\"/> ";

    // Centre @ (100,100,110)
    V3D point = V3D(103.0, 103.0, 120.0);
    V3D pointRotate = V3D(103.0, 90.0, 113.0);
    V3D pointRotateAll = V3D(103.0, -120.0, 103.0);

    std::string xmlHollowCylinder = xmlHollowCylinderStart + xmlHollowCylinderEnd;
    auto hollow_sptr = getObject(xmlHollowCylinder);
    TS_ASSERT(hollow_sptr->isValid(point));
    TS_ASSERT(!hollow_sptr->isValid(pointRotate));

    std::string xmlHollowCylinderRotate =
        xmlHollowCylinderStart + "<rotate x=\"90\" y=\"0\" z=\"0\" /> " + xmlHollowCylinderEnd;
    auto hollowRotate_sptr = getObject(xmlHollowCylinderRotate);
    TS_ASSERT(hollowRotate_sptr->isValid(pointRotate));
    TS_ASSERT(!hollowRotate_sptr->isValid(point));

    std::string xmlHollowCylinderRotateAll = xmlHollowCylinder + "<rotate-all x=\"90\" y=\"0\" z=\"0\" /> ";
    auto hollowRotateAll_sptr = getObject(xmlHollowCylinderRotateAll);
    TS_ASSERT(hollowRotateAll_sptr->isValid(pointRotateAll));
    TS_ASSERT(!hollowRotateAll_sptr->isValid(pointRotate));
  }

  void testRotateSphere() {
    std::string xmlSphereStart = "<sphere id=\"Sphere\"> ";
    xmlSphereStart += "<radius val=\"1.0\" /> ";
    xmlSphereStart += "<centre x=\"10.0\" y=\"10.0\" z=\"10.0\"/> ";
    std::string xmlSphereEnd = "</sphere> ";
    xmlSphereEnd += "<algebra val=\"Sphere\"/> ";

    V3D centre = V3D(10.0, 10.0, 10.0);
    V3D centreRotateAll = V3D(10.0, -10.0, 10.0);

    std::string xmlSphere = xmlSphereStart + xmlSphereEnd;
    auto sphere_sptr = getObject(xmlSphere);
    TS_ASSERT(sphere_sptr->isValid(centre));
    TS_ASSERT(!sphere_sptr->isValid(centreRotateAll));

    std::string xmlSphereRotate = xmlSphereStart + "<rotate x=\"90\" y=\"0\" z=\"0\" /> " + xmlSphereEnd;
    auto sphereRotate_sptr = getObject(xmlSphereRotate);
    TS_ASSERT(sphereRotate_sptr->isValid(centre));
    TS_ASSERT(!sphereRotate_sptr->isValid(centreRotateAll));

    std::string xmlSphereRotateAll = xmlSphere + "<rotate-all x=\"90\" y=\"0\" z=\"0\" /> ";
    auto sphereRotateAll_sptr = getObject(xmlSphereRotateAll);
    TS_ASSERT(sphereRotateAll_sptr->isValid(centreRotateAll));
    TS_ASSERT(!sphereRotateAll_sptr->isValid(centre));
  }

  void testRotateInfinitePlane() {
    std::string xmlInfinitePlaneStart = "<infinite-plane id=\"InfinitePlane\"> ";
    xmlInfinitePlaneStart += "<point-in-plane x=\"0.0\" y=\"0.0\" z=\"20.0\" /> ";
    xmlInfinitePlaneStart += "<normal-to-plane x=\"0.0\" y=\"0.0\" z=\"1.0\" /> ";
    std::string xmlInfinitePlaneEnd = "</infinite-plane> ";
    xmlInfinitePlaneEnd += "<algebra val=\"InfinitePlane\"/> ";

    V3D centre = V3D(0.0, 0.0, 20.0);
    V3D centreRotateAll = V3D(0.0, -20.0, 0.0);

    std::string xmlInfinitePlane = xmlInfinitePlaneStart + xmlInfinitePlaneEnd;
    auto plane_sptr = getObject(xmlInfinitePlane);
    TS_ASSERT(plane_sptr->isOnSide(centre));
    TS_ASSERT(!plane_sptr->isOnSide(centreRotateAll));

    std::string xmlInfinitePlaneRotate =
        xmlInfinitePlaneStart + "<rotate x=\"90\" y=\"0\" z=\"0\" /> " + xmlInfinitePlaneEnd;
    auto planeRotate_sptr = getObject(xmlInfinitePlaneRotate);
    TS_ASSERT(planeRotate_sptr->isOnSide(centre));
    TS_ASSERT(!planeRotate_sptr->isOnSide(centreRotateAll));

    std::string xmlInfinitePlaneRotateAll = xmlInfinitePlane + "<rotate-all x=\"90\" y=\"0\" z=\"0\" /> ";
    auto planeRotateAll_sptr = getObject(xmlInfinitePlaneRotateAll);
    TS_ASSERT(planeRotateAll_sptr->isOnSide(centreRotateAll));
    TS_ASSERT(!planeRotateAll_sptr->isOnSide(centre));
  }

  void testRotateInfiniteCylinder() {
    std::string xmlInfiniteCylinderStart = "<infinite-cylinder id=\"InfiniteCylinder\"> ";
    xmlInfiniteCylinderStart += "<centre x=\"10.0\" y=\"0.0\" z=\"20.0\" /> ";
    xmlInfiniteCylinderStart += "<axis x=\"0.0\" y=\"0.0\" z=\"1.0\" /> ";
    xmlInfiniteCylinderStart += "<radius val=\"1\" /> ";
    std::string xmlInfiniteCylinderEnd = "</infinite-cylinder> ";
    xmlInfiniteCylinderEnd += "<algebra val=\"InfiniteCylinder\"/> ";

    V3D centre = V3D(10.0, 0.0, 20.0);
    V3D centreRotateAll = V3D(10.0, -20.0, 0.0);

    std::string xmlInfiniteCylinder = xmlInfiniteCylinderStart + xmlInfiniteCylinderEnd;
    auto cylinder_sptr = getObject(xmlInfiniteCylinder);
    TS_ASSERT(cylinder_sptr->isValid(centre));
    TS_ASSERT(!cylinder_sptr->isValid(centreRotateAll));

    std::string xmlInfiniteCylinderRotate =
        xmlInfiniteCylinderStart + "<rotate x=\"90\" y=\"0\" z=\"0\" /> " + xmlInfiniteCylinderEnd;
    auto cylinderRotate_sptr = getObject(xmlInfiniteCylinderRotate);
    TS_ASSERT(cylinderRotate_sptr->isValid(centre));
    TS_ASSERT(!cylinderRotate_sptr->isValid(centreRotateAll));

    std::string xmlInfiniteCylinderRotateAll = xmlInfiniteCylinder + "<rotate-all x=\"90\" y=\"0\" z=\"0\" /> ";
    auto cylinderRotateAll_sptr = getObject(xmlInfiniteCylinderRotateAll);
    TS_ASSERT(cylinderRotateAll_sptr->isValid(centreRotateAll));
    TS_ASSERT(!cylinderRotateAll_sptr->isValid(centre));
  }

  void testRotateSlice() {
    std::string xmlSliceStart = "<slice-of-cylinder-ring id=\"Slice\"> ";
    xmlSliceStart += "<depth val=\"1.0\" /> ";
    xmlSliceStart += "<inner-radius val=\"2.0\" /> ";
    xmlSliceStart += "<outer-radius val=\"10.0\" /> ";
    xmlSliceStart += "<arc val = \"90.0\"/> ";
    std::string xmlSliceEnd = "</slice-of-cylinder-ring> ";
    xmlSliceEnd += "<algebra val=\"Slice\"/> ";

    // centre @ (-6,0,0)
    V3D point = V3D(0.0, 4.0, 0.0);
    V3D pointRotate = V3D(-6.0, 4.0, -6.0);
    V3D pointRotateAll = V3D(0.0, 4.0, 0.0);

    std::string xmlSlice = xmlSliceStart + xmlSliceEnd;
    auto slice_sptr = getObject(xmlSlice);
    TS_ASSERT(slice_sptr->isValid(point));
    TS_ASSERT(!slice_sptr->isValid(pointRotate));

    std::string xmlSliceRotate = xmlSliceStart + "<rotate x=\"0\" y=\"90\" z=\"0\" /> " + xmlSliceEnd;
    auto sliceRotate_sptr = getObject(xmlSliceRotate);
    TS_ASSERT(sliceRotate_sptr->isValid(pointRotate));
    TS_ASSERT(!sliceRotate_sptr->isValid(point));

    std::string xmlSliceRotateAll = xmlSlice + "<rotate-all x=\"0\" y=\"90\" z=\"0\" /> ";
    auto sliceRotateAll_sptr = getObject(xmlSliceRotateAll);
    TS_ASSERT(sliceRotateAll_sptr->isValid(pointRotateAll));
    TS_ASSERT(!sliceRotateAll_sptr->isValid(pointRotate));
  }

private:
  void compareMatrix(const std::vector<double> &vectorToMatch, const Matrix<double> &rotationMatrix) {
    auto checkVector = rotationMatrix.getVector();
    for (size_t i = 0; i < 9; ++i) {
      TS_ASSERT_DELTA(checkVector[i], vectorToMatch[i], 1e-7);
    }
  }
  std::string inputFile;
};
