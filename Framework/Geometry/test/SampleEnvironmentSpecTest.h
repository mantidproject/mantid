#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECTEST_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/SampleEnvironmentSpec.h"
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

  void test_Add_Can_Stores_Can_Linked_To_ID() {
    using Mantid::Geometry::Can;
    SampleEnvironmentSpec spec("CRYO-001");
    auto testCan = boost::make_shared<Can>("");
    testCan->setID("8mm");

    TS_ASSERT_EQUALS(0, spec.ncans());
    TS_ASSERT_THROWS_NOTHING(spec.addCan(testCan));
    TS_ASSERT_EQUALS(1, spec.ncans());
    auto retrieved = spec.findCan("8mm");
    TS_ASSERT(retrieved);
    TS_ASSERT_EQUALS(testCan, retrieved);
  }

  void test_AddObject_Stores_Reference_To_Object() {
    SampleEnvironmentSpec spec("CRYO-001");

    TS_ASSERT_EQUALS(0, spec.ncomponents());
    spec.addComponent(ComponentCreationHelper::createSphere(0.01));
    TS_ASSERT_EQUALS(1, spec.ncomponents());
  }
  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Add_Can_With_Empty_ID_Throws_Invalid_Argument() {
    using Mantid::Geometry::Can;
    SampleEnvironmentSpec spec("CRYO-001");
    auto testCan = boost::make_shared<const Can>("");

    TS_ASSERT_THROWS(spec.addCan(testCan), std::invalid_argument);
  }

  void test_Find_Throws_If_ID_Not_Found() {
    using Mantid::Geometry::Can;
    SampleEnvironmentSpec spec("CRYO-001");

    TS_ASSERT_THROWS(spec.findCan("8mm"), std::invalid_argument);
  }
};

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECTEST_H_ */
