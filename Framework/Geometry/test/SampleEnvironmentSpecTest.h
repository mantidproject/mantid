// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECTEST_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/SampleEnvironmentSpec.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/make_shared.hpp>

using Mantid::Geometry::SampleEnvironmentSpec;

class SampleEnvironmentSpecTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleEnvironmentSpecTest *createSuite() {
    return new SampleEnvironmentSpecTest();
  }
  static void destroySuite(SampleEnvironmentSpecTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Constructor_Sets_Name_Property() {
    SampleEnvironmentSpec spec("CRYO-001");
    TS_ASSERT_EQUALS("CRYO-001", spec.name());
  }

  void test_Add_Container_Stores_Container_Linked_To_ID() {
    using Mantid::Geometry::Container;
    SampleEnvironmentSpec spec("CRYO-001");
    auto testContainer = boost::make_shared<Container>("");
    testContainer->setID("8mm");

    TS_ASSERT_EQUALS(0, spec.ncans());
    TS_ASSERT_THROWS_NOTHING(spec.addContainer(testContainer));
    TS_ASSERT_EQUALS(1, spec.ncans());
    auto retrieved = spec.findContainer("8mm");
    TS_ASSERT(retrieved);
    TS_ASSERT_EQUALS(testContainer, retrieved);
  }

  void test_AddObject_Stores_Reference_To_Object() {
    SampleEnvironmentSpec spec("CRYO-001");

    TS_ASSERT_EQUALS(0, spec.ncomponents());
    spec.addComponent(ComponentCreationHelper::createSphere(0.01));
    TS_ASSERT_EQUALS(1, spec.ncomponents());
  }

  void test_buildEnvironment_Creates_Expected_Environment() {
    using Mantid::Geometry::Container;
    using Mantid::Geometry::ShapeFactory;
    using Mantid::Kernel::V3D;

    ShapeFactory factory;
    auto small = boost::make_shared<Container>(factory.createShape(
        ComponentCreationHelper::sphereXML(0.004, V3D(), "sp-1")));
    small->setID("8mm");
    auto large = boost::make_shared<Container>(factory.createShape(
        ComponentCreationHelper::sphereXML(0.005, V3D(), "sp-2")));
    large->setID("10mm");

    SampleEnvironmentSpec spec("CRYO-001");
    spec.addContainer(small);
    spec.addContainer(large);
    spec.addComponent(
        ComponentCreationHelper::createSphere(0.05, V3D(0, 0, -0.1)));

    auto env = spec.buildEnvironment("10mm");
    TS_ASSERT(env);
    TS_ASSERT_EQUALS("CRYO-001", env->name());
    TS_ASSERT_EQUALS(2, env->nelements());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Add_Container_With_Empty_ID_Throws_Invalid_Argument() {
    using Mantid::Geometry::Container;
    SampleEnvironmentSpec spec("CRYO-001");
    auto testContainer = boost::make_shared<const Container>("");

    TS_ASSERT_THROWS(spec.addContainer(testContainer),
                     const std::invalid_argument &);
  }

  void test_Find_Throws_If_ID_Not_Found() {
    using Mantid::Geometry::Container;
    SampleEnvironmentSpec spec("CRYO-001");

    TS_ASSERT_THROWS(spec.findContainer("8mm"), const std::invalid_argument &);
  }

  void test_BuildEnvironment_Throws_If_ID_Not_Found() {
    using Mantid::Geometry::Container;
    SampleEnvironmentSpec spec("CRYO-001");

    TS_ASSERT_THROWS(spec.buildEnvironment("8mm"),
                     const std::invalid_argument &);
  }
};

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECTEST_H_ */
