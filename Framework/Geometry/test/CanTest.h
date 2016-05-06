#ifndef MANTID_GEOMETRY_CANTEST_H_
#define MANTID_GEOMETRY_CANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/Can.h"

using Mantid::Geometry::Can;

class CanTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CanTest *createSuite() { return new CanTest(); }
  static void destroySuite(CanTest *suite) { delete suite; }

  // ---------------------------------------------------------------------------
  // Success tests
  // ---------------------------------------------------------------------------
  void test_Default_Constructor_Has_No_Sample_Shape() {
    Can can;
    TS_ASSERT(can.sampleShapeTemplate().empty());
  }

  void test_Constructing_With_Default_Shape_Stores_Same_Default() {
    std::string xml = "<cylinder>"
                      "<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" />"
                      "<axis x =\"0.0\" y=\"1.0\" z=\"0\" />"
                      "<radius val =\"0.0030\" />"
                      "<height val =\"0.05\" />"
                      "</cylinder>";
    Can can(xml);
    TS_ASSERT_EQUALS(xml, can.sampleShapeTemplate());
  }
};

#endif /* MANTID_GEOMETRY_CANTEST_H_ */
