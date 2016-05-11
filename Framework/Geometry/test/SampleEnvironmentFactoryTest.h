#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORYTEST_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Instrument/SampleEnvironmentFactory.h"
#include "MantidGeometry/Instrument/Can.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::Geometry::SampleEnvironmentFactory;

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

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Known_Specification_And_Can_Returns_Environment() {
    using Mantid::Geometry::SampleEnvironment_uptr;

    SampleEnvironmentFactory factory;
    SampleEnvironment_uptr env;
    TS_ASSERT_THROWS_NOTHING(
        env = std::move(factory.create<TestSampleEnvironmentSpecFinder>(
            "CRYO001", "10mm")));
    TS_ASSERT_EQUALS("CRYO001", env->name());
    TS_ASSERT_EQUALS("10mm", env->canID());
    TS_ASSERT_EQUALS(1, env->nelements());
  }

  void test_Finder_Called_Once_For_Same_Specification() {
    using Mantid::Geometry::SampleEnvironment_uptr;

    SampleEnvironmentFactory factory;
    TestSampleEnvironmentSpecFinder finder;
    TS_ASSERT_THROWS_NOTHING(factory.create(finder, "CRYO001", "10mm"));
    TS_ASSERT_EQUALS(1, finder.callCount());
    SampleEnvironment_uptr env8mm;
    TS_ASSERT_THROWS_NOTHING(
        env8mm = std::move(factory.create(finder, "CRYO001", "8mm")));
    TS_ASSERT_EQUALS(1, finder.callCount());
    TS_ASSERT_EQUALS("CRYO001", env8mm->name());
    TS_ASSERT_EQUALS("8mm", env8mm->canID());
    TS_ASSERT_EQUALS(1, env8mm->nelements());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Unknown_Specification_Throws_Error() {
    SampleEnvironmentFactory factory;
    TS_ASSERT_THROWS(
        factory.create<NullSampleEnvironmentSpecFinder>("unknown", "unknown"),
        std::runtime_error);
  }

  void test_Known_Specification_Unknown_Can_Throws() {
    SampleEnvironmentFactory factory;
    TS_ASSERT_THROWS(
        factory.create<TestSampleEnvironmentSpecFinder>("CRYO001", "unknown"),
        std::invalid_argument);
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  class NullSampleEnvironmentSpecFinder final
      : public Mantid::Geometry::ISampleEnvironmentSpecFinder {
  public:
    // Never finds anything
    Mantid::Geometry::SampleEnvironmentSpec_uptr
    find(const std::string &) const override {
      throw std::runtime_error("Unable to find named specification");
    }
  };

  class TestSampleEnvironmentSpecFinder final
      : public Mantid::Geometry::ISampleEnvironmentSpecFinder {
  public:
    TestSampleEnvironmentSpecFinder() : m_callCount(0) {}

    inline size_t callCount() const { return m_callCount; }

    Mantid::Geometry::SampleEnvironmentSpec_uptr
    find(const std::string &) const override {
      // Can't use gmock as it can't mock unique_ptr return as its not copyable
      ++m_callCount;
      // Just make one
      using namespace Mantid::Geometry;
      using namespace Mantid::Kernel;

      ShapeFactory factory;
      auto small = factory.createShape<Can>(
          ComponentCreationHelper::sphereXML(0.004, V3D(), "sp-1"));
      small->setID("8mm");
      auto large = factory.createShape<Can>(
          ComponentCreationHelper::sphereXML(0.005, V3D(), "sp-2"));
      large->setID("10mm");

      // Prepare a sample environment spec
      auto spec = Mantid::Kernel::make_unique<SampleEnvironmentSpec>("CRYO001");
      spec->addCan(small);
      spec->addCan(large);
      return std::move(spec);
    }

  private:
    mutable size_t m_callCount;
  };
};

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORYTEST_H_ */
