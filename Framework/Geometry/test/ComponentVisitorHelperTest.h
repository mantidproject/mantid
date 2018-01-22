#ifndef MANTID_GEOMETRY_COMPONENTVISITORHELPERTEST_H_
#define MANTID_GEOMETRY_COMPONENTVISITORHELPERTEST_H_

#include "MantidGeometry/Instrument/ComponentVisitorHelper.h"
#include <cxxtest/TestSuite.h>
using namespace Mantid::Geometry;

class ComponentVisitorHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentVisitorHelperTest *createSuite() {
    return new ComponentVisitorHelperTest();
  }
  static void destroySuite(ComponentVisitorHelperTest *suite) { delete suite; }

  void test_visit_as_tube() {
    // Test unsuccesful match. Match not close enough
    TS_ASSERT(!ComponentVisitorHelper::matchesPSDTube("bankoftube"));

    // Test unsuccessful match if not followed by digits
    TS_ASSERT(!ComponentVisitorHelper::matchesPSDTube("tube12x"));

    // Test successful match
    TS_ASSERT(ComponentVisitorHelper::matchesPSDTube("tube123"));

    // Test successful ignore case
    TS_ASSERT(ComponentVisitorHelper::matchesPSDTube("Tube1"));
  }

  void test_visit_as_bank_of_tubes() {
    // Match not close enough, should end with PACK.
    TS_ASSERT(!ComponentVisitorHelper::matchesPackOfTubes("somepackz"));

    // Match not close enough, alphabetical characters preceding only.
    TS_ASSERT(!ComponentVisitorHelper::matchesPackOfTubes("16pack"));

    // Test successful match
    TS_ASSERT(ComponentVisitorHelper::matchesPackOfTubes("sixteenpack"));

    // Test successful ignore case
    TS_ASSERT(ComponentVisitorHelper::matchesPackOfTubes("EightPack"));
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTVISITORHELPERTEST_H_ */
