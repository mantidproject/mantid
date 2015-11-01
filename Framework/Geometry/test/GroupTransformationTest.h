#ifndef MANTID_GEOMETRY_GROUPTRANSFORMATIONTEST_H_
#define MANTID_GEOMETRY_GROUPTRANSFORMATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/GroupTransformation.h"
#include "MantidGeometry/Crystal/SymmetryElementFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace Mantid::Geometry;

class GroupTransformationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTransformationTest *createSuite() {
    return new GroupTransformationTest();
  }
  static void destroySuite(GroupTransformationTest *suite) { delete suite; }

  void test_ConstructionSymmetryOperation() {
    TS_ASSERT_THROWS_NOTHING(GroupTransformation(SymmetryOperation("x,y,z")));
  }

  void test_ConstructionSymmetryOperationString() {
    TS_ASSERT_THROWS_NOTHING(GroupTransformation("x,y,z"));
    TS_ASSERT_THROWS_ANYTHING(GroupTransformation("invalid"));
  }

  void test_TransformGroup() {
    // Space group P 1 2/m 1
    Group group("-x,y,-z; -x,-y,-z; x,-y,z; x,y,z");

    // Transforming it to P 1 1 2/m
    Group transformed = GroupTransformation("y,z,x")(group);

    // The group order should not change
    TS_ASSERT_EQUALS(group.order(), transformed.order());

    /* Verify transformation by checking the symmetry elements.
     * The group should have the following symmetry elements:
     *
     *  1. Identity
     *  2. Inversion
     *  3. 2-fold rotation || z
     *  4. Mirror plane perpendicular to z.
     */
    std::set<std::string> elements;
    std::vector<SymmetryOperation> ops = transformed.getSymmetryOperations();
    for (auto op = ops.begin(); op != ops.end(); ++op) {
      SymmetryElement_sptr el =
          SymmetryElementFactory::Instance().createSymElement(*op);

      // Check for identity
      SymmetryElementIdentity_sptr identity =
          boost::dynamic_pointer_cast<SymmetryElementIdentity>(el);
      if (identity) {
        elements.insert(el->hmSymbol());
      }

      // Check for inversion
      SymmetryElementInversion_sptr inversion =
          boost::dynamic_pointer_cast<SymmetryElementInversion>(el);
      if (inversion) {
        elements.insert(el->hmSymbol());
      }

      // Check for 2 || z
      SymmetryElementRotation_sptr rotation =
          boost::dynamic_pointer_cast<SymmetryElementRotation>(el);
      if (rotation && rotation->getAxis() == V3R(0, 0, 1)) {
        elements.insert(el->hmSymbol());
      }

      // Check for m perpendicular to z
      SymmetryElementMirror_sptr mirror =
          boost::dynamic_pointer_cast<SymmetryElementMirror>(el);
      if (mirror && mirror->getAxis() == V3R(0, 0, 1)) {
        elements.insert(el->hmSymbol());
      }
    }

    TS_ASSERT_EQUALS(elements.size(), 4);
  }

  void test_TransformGroup_Reversible() {
    // Space group P 1 2/m 1
    Group group("-x,y,-z; -x,-y,-z; x,-y,z; x,y,z");

    // Transforming it to P 1 1 2/m
    GroupTransformation transform("y,z,x");
    Group transformed = transform(group);

    // It's not the same group anymore
    TS_ASSERT_DIFFERS(group, transformed);

    // Transform using the inverse
    Group reversed = transform.getInverse()(transformed);

    // Same group again
    TS_ASSERT_EQUALS(reversed, group);
  }
};

#endif /* MANTID_GEOMETRY_GROUPTRANSFORMATIONTEST_H_ */
