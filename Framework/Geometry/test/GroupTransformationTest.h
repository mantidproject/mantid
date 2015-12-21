#ifndef MANTID_GEOMETRY_GROUPTRANSFORMATIONTEST_H_
#define MANTID_GEOMETRY_GROUPTRANSFORMATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/GroupTransformation.h"
#include "MantidGeometry/Crystal/SymmetryElementFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/CenteringGroup.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class GroupTransformationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTransformationTest *createSuite() {
    return new GroupTransformationTest();
  }
  static void destroySuite(GroupTransformationTest *suite) { delete suite; }

  void test_ConstructionSymmetryOperation() {
    TS_ASSERT_THROWS_NOTHING(GroupTransformation("x,y,z"));
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

  void test_TransformGroup_Rhombohedral() {
    SpaceGroup_const_sptr r3cHex =
        SpaceGroupFactory::Instance().createSpaceGroup("R -3 c");

    /* 2/3 -1/3 -1/3    0
     * 1/3  1/3 -2/3    0
     * 1/3  1/3  1/3    0
     */
    GroupTransformation hexToRhom(
        "2/3x-1/3y-1/3z,1/3x+1/3y-2/3z,1/3x+1/3y+1/3z");
    Group r3cRh = hexToRhom(*r3cHex);

    GroupTransformation rhomToHex("x+z,-x+y+z,-y+z");

    Group r3cHexBack = rhomToHex(r3cRh) * CenteringGroup("R");

    TS_ASSERT(r3cHexBack.isGroup());
    TS_ASSERT_EQUALS(r3cHex->order(), r3cHexBack.order());

    V3D point(0.882, 0.112, 0.126);

    std::vector<V3D> ptsOrig = r3cHex->getEquivalentPositions(point);
    std::vector<V3D> ptsTrfm = r3cHexBack * point;

    TS_ASSERT_EQUALS(ptsOrig.size(), ptsTrfm.size());

    std::sort(ptsOrig.begin(), ptsOrig.end());
    std::sort(ptsTrfm.begin(), ptsTrfm.end());

    for (size_t i = 0; i < ptsOrig.size(); ++i) {
      std::cout << i << " " << ptsOrig[i] << " " << ptsTrfm[i] << std::endl;
    }

    for (size_t i = 0; i < r3cHex->order(); ++i) {
      std::cout << r3cHex->getSymmetryOperations()[i] << " "
                << r3cHexBack.getSymmetryOperations()[i] << std::endl;
    }

    std::vector<SymmetryOperation> ops = r3cRh.getSymmetryOperations();

    std::cout << std::endl;
    for (auto op : ops) {
      std::cout << op << std::endl;
    }
  }
};

#endif /* MANTID_GEOMETRY_GROUPTRANSFORMATIONTEST_H_ */
