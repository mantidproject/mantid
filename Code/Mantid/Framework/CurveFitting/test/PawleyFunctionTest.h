#ifndef MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PawleyFunction.h"
#include "MantidGeometry/Crystal/PointGroup.h"

using Mantid::CurveFitting::PawleyFunction;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class PawleyFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PawleyFunctionTest *createSuite() { return new PawleyFunctionTest(); }
  static void destroySuite(PawleyFunctionTest *suite) { delete suite; }

  void testCrystalSystem() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT(fn.hasAttribute("CrystalSystem"));

    // Cubic, check case insensitivity
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("CrystalSystem", "cubic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Cubic);
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("CrystalSystem", "Cubic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Cubic);
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("CrystalSystem", "CUBIC"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Cubic);

    // Tetragonal
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "tetragonal"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Tetragonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Tetragonal"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Tetragonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "TETRAGONAL"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Tetragonal);

    // Hexagonal
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "hexagonal"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Hexagonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Hexagonal"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Hexagonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "HEXAGONAL"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Hexagonal);

    // Orthorhombic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "orthorhombic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Orthorhombic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Orthorhombic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Orthorhombic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "ORTHORHOMBIC"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Orthorhombic);

    // Monoclinic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "monoclinic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Monoclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Monoclinic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Monoclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "MONOCLINIC"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Monoclinic);

    // Triclinic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "triclinic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Triclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Triclinic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Triclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "TRICLINIC"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Triclinic);

    // invalid string
    TS_ASSERT_THROWS(fn.setAttributeValue("CrystalSystem", "invalid"),
                     std::invalid_argument);
  }
};

#endif /* MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_ */
