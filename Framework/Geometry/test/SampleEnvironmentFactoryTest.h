// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORYTEST_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORYTEST_H_

#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironmentFactory.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>

using Mantid::Geometry::SampleEnvironmentFactory;
using Mantid::Geometry::SampleEnvironmentSpecFileFinder;

//------------------------------------------------------------------------------
// SampleEnvironmentFactory tests
//------------------------------------------------------------------------------
class SampleEnvironmentFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleEnvironmentFactoryTest *createSuite() {
    return new SampleEnvironmentFactoryTest();
  }
  static void destroySuite(SampleEnvironmentFactoryTest *suite) {
    delete suite;
  }

  void tearDown() override {
    // Clear monostate cache
    SampleEnvironmentFactory().clearCache();
  }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Known_Specification_And_Container_Returns_Environment() {
    using Mantid::Geometry::SampleEnvironment_uptr;

    SampleEnvironmentFactory factory(
        std::make_unique<TestSampleEnvSpecFinder>());
    SampleEnvironment_uptr env;
    TS_ASSERT_THROWS_NOTHING(
        env = factory.create("facility", "inst", "CRYO001", "10mm"));
    TS_ASSERT_EQUALS("CRYO001", env->name());
    TS_ASSERT_EQUALS("10mm", env->containerID());
    TS_ASSERT_EQUALS(1, env->nelements());
    TS_ASSERT_EQUALS(1, factory.cacheSize());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Unknown_Specification_Throws_Error() {
    SampleEnvironmentFactory factory(
        std::make_unique<NullSampleEnvSpecFinder>());
    TS_ASSERT_THROWS(factory.create("unknown", "unknown", "unknown", "unknown"),
                     const std::runtime_error &);
  }

  void test_Known_Specification_Unknown_Container_Throws() {
    SampleEnvironmentFactory factory(
        std::make_unique<TestSampleEnvSpecFinder>());
    TS_ASSERT_THROWS(factory.create("unknown", "unknown", "CRYO001", "unknown"),
                     const std::invalid_argument &);
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  class NullSampleEnvSpecFinder final
      : public Mantid::Geometry::ISampleEnvironmentSpecFinder {
  public:
    // Never finds anything
    Mantid::Geometry::SampleEnvironmentSpec_uptr
    find(const std::string &, const std::string &,
         const std::string &) const override {
      throw std::runtime_error("Unable to find named specification");
    }
  };

  class TestSampleEnvSpecFinder final
      : public Mantid::Geometry::ISampleEnvironmentSpecFinder {
  public:
    Mantid::Geometry::SampleEnvironmentSpec_uptr
    find(const std::string &, const std::string &,
         const std::string &) const override {
      // Just make one
      using namespace Mantid::Geometry;
      using namespace Mantid::Kernel;

      ShapeFactory factory;
      auto small = boost::make_shared<Container>(factory.createShape(
          ComponentCreationHelper::sphereXML(0.004, V3D(), "sp-1")));
      small->setID("8mm");
      auto large = boost::make_shared<Container>(factory.createShape(
          ComponentCreationHelper::sphereXML(0.005, V3D(), "sp-2")));
      large->setID("10mm");

      // Prepare a sample environment spec
      auto spec = std::make_unique<SampleEnvironmentSpec>("CRYO001");
      spec->addContainer(small);
      spec->addContainer(large);
      return spec;
    }
  };
};

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORYTEST_H_ */
