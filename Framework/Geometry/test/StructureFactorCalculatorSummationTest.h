#ifndef MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORSUMMATIONTEST_H_
#define MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORSUMMATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/StructureFactorCalculatorSummation.h"

#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class StructureFactorCalculatorSummationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StructureFactorCalculatorSummationTest *createSuite() {
    return new StructureFactorCalculatorSummationTest();
  }
  static void destroySuite(StructureFactorCalculatorSummationTest *suite) {
    delete suite;
  }

  void testEquivalentPositionsAreUsed() {
    // Approximate crystal structure of Silicon
    CrystalStructure si = getCrystalStructure();

    StructureFactorCalculatorSummation calculator;
    calculator.setCrystalStructure(si);

    // {1 0 0} reflections are not allowed because of F centering
    TS_ASSERT_LESS_THAN(calculator.getFSquared(V3D(1, 0, 0)), 1e-9);

    // {2 2 2} is forbidden because of Si on special position
    TS_ASSERT_LESS_THAN(calculator.getFSquared(V3D(2, 2, 2)), 1e-9);

    // With space group P-1, those are allowed.
    si.setSpaceGroup(SpaceGroupFactory::Instance().createSpaceGroup("P -1"));

    calculator.setCrystalStructure(si);
    // {1 0 0} reflections are not allowed because of F centering
    TS_ASSERT_LESS_THAN(1e-9, calculator.getFSquared(V3D(1, 0, 0)));

    // {2 2 2} is forbidden because of Si on special position
    TS_ASSERT_LESS_THAN(1e-9, calculator.getFSquared(V3D(2, 2, 2)));
  }

  void testCreateWithFactory() {
    CrystalStructure si = getCrystalStructure();

    StructureFactorCalculator_sptr calculator =
        StructureFactorCalculatorFactory::create<
            StructureFactorCalculatorSummation>(si);

    // same reflections as in testEquivalentPositionsAreUsed
    TS_ASSERT_LESS_THAN(calculator->getFSquared(V3D(1, 0, 0)), 1e-9);
    TS_ASSERT_LESS_THAN(calculator->getFSquared(V3D(2, 2, 2)), 1e-9);
  }

private:
  CrystalStructure getCrystalStructure() {
    CompositeBraggScatterer_sptr scatterers = CompositeBraggScatterer::create();
    scatterers->addScatterer(BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer",
        R"({"Element":"Si","Position":"0,0,0","U":"0.05"})"));

    CrystalStructure si(
        UnitCell(5.43, 5.43, 5.43),
        SpaceGroupFactory::Instance().createSpaceGroup("F d -3 m"), scatterers);

    return si;
  }
};

#endif /* MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORSUMMATIONTEST_H_ */
