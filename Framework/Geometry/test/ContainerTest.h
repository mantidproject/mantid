// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_CANTEST_H_
#define MANTID_GEOMETRY_CANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Sphere.h"

#include <boost/make_shared.hpp>

using Mantid::Geometry::Container;

class ContainerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ContainerTest *createSuite() { return new ContainerTest(); }
  static void destroySuite(ContainerTest *suite) { delete suite; }

  // ---------------------------------------------------------------------------
  // Success tests
  // ---------------------------------------------------------------------------
  void test_Default_Constructor_Has_No_Sample_Shape() {
    Container can;
    TS_ASSERT_EQUALS(false, can.hasSampleShape());
    TS_ASSERT_THROWS(can.createSampleShape(Container::ShapeArgs()),
                     const std::runtime_error &);
  }

  void test_Construction_With_XML_Assumes_XML_For_Can_Itself() {
    std::string xml = "<cylinder>"
                      "<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" />"
                      "<axis x=\"0.0\" y=\"1.0\" z=\"0\" />"
                      "<radius val=\"0.0030\" />"
                      "<height val=\"0.05\" />"
                      "</cylinder>";
    Container can(xml);
    TS_ASSERT_EQUALS(false, can.hasSampleShape());
  }

  void test_SetSampleShape_Allows_Creating_Sample_Shape_Object() {
    using Mantid::Geometry::IObject_sptr;
    auto can = createTestCan();
    can->setSampleShape("<samplegeometry><sphere id=\"shape\"> "
                        "<radius val=\"1.0\" /> "
                        "</sphere></samplegeometry>");
    IObject_sptr sampleShape;
    TS_ASSERT_THROWS_NOTHING(
        sampleShape = can->createSampleShape(Container::ShapeArgs()));
    TS_ASSERT(sampleShape->hasValidShape());
    TS_ASSERT_DELTA(
        1.0,
        getSphereRadius(
            dynamic_cast<const Mantid::Geometry::CSGObject &>(*sampleShape)),
        1e-10);
  }

  void test_CreateSampleShape_Args_Override_Defaults() {
    using Mantid::Geometry::IObject_sptr;
    auto can = createTestCan();
    can->setSampleShape("<samplegeometry><sphere id=\"shape\"> "
                        "<radius val=\"1.0\" /> "
                        "</sphere></samplegeometry>");
    IObject_sptr sampleShape;
    Container::ShapeArgs args = {{"radius", 0.5}};
    TS_ASSERT_THROWS_NOTHING(sampleShape = can->createSampleShape(args));
    TS_ASSERT(sampleShape->hasValidShape());
    TS_ASSERT_DELTA(
        0.5,
        getSphereRadius(
            dynamic_cast<const Mantid::Geometry::CSGObject &>(*sampleShape)),
        1e-10);
  }

  void test_CreateSampleShape_Args_Not_Matching_Do_Nothing() {
    using Mantid::Geometry::IObject_sptr;
    auto can = createTestCan();
    can->setSampleShape("<samplegeometry><sphere id=\"shape\"> "
                        "<radius val=\"1.0\" /> "
                        "</sphere></samplegeometry>");
    IObject_sptr sampleShape;
    Container::ShapeArgs args = {{"height", 0.5}};
    TS_ASSERT_THROWS_NOTHING(sampleShape = can->createSampleShape(args));
    TS_ASSERT(sampleShape->hasValidShape());
    TS_ASSERT_DELTA(
        1.0,
        getSphereRadius(
            dynamic_cast<const Mantid::Geometry::CSGObject &>(*sampleShape)),
        1e-10);
  }

  // ---------------------------------------------------------------------------
  // Failure tests
  // ---------------------------------------------------------------------------
  void test_SetSampleShape_Throws_If_Top_Tag_Not_SampleGeometry() {
    auto can = createTestCan();
    TS_ASSERT_THROWS(can->setSampleShape("<sphere id=\"shape\"> "
                                         "<radius val=\"1.0\" /> "
                                         "</sphere>"),
                     const std::invalid_argument &);
  }

private:
  Mantid::Geometry::Container_sptr createTestCan() {
    return boost::make_shared<Container>(
        "<type name=\"usertype\"><cylinder>"
        "<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" />"
        "<axis x=\"0.0\" y=\"1.0\" z=\"0\" />"
        "<radius val =\"0.0030\" />"
        "<height val =\"0.05\" />"
        "</cylinder></type>");
  }
  double getSphereRadius(const Mantid::Geometry::CSGObject &shape) {
    using Mantid::Geometry::Sphere;
    using Mantid::Geometry::SurfPoint;
    auto topRule = shape.topRule();
    if (auto surfpoint = dynamic_cast<const SurfPoint *>(topRule)) {
      if (auto sphere = dynamic_cast<Sphere *>(surfpoint->getKey())) {
        return sphere->getRadius();
      } else {
        throw std::runtime_error("Expected Sphere as SurfPoint key.");
      }
    } else {
      throw std::runtime_error("Expected SurfPoint as top rule");
    }
  }
};

#endif /* MANTID_GEOMETRY_CANTEST_H_ */
