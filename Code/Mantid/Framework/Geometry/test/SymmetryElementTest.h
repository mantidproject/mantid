#ifndef MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_
#define MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class SymmetryElementTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SymmetryElementTest *createSuite() {
    return new SymmetryElementTest();
  }
  static void destroySuite(SymmetryElementTest *suite) { delete suite; }

  void testHMSymbolGetSet() {
    MockSymmetryElement element;

    TS_ASSERT_EQUALS(element.hmSymbol(), "");

    TS_ASSERT_THROWS_NOTHING(element.setHMSymbol("SomeSymbol"));
    TS_ASSERT_EQUALS(element.hmSymbol(), "SomeSymbol");
  }

  void testSymmetryElementIdentity() {
    TS_ASSERT_THROWS_NOTHING(SymmetryElementIdentity identity);

    SymmetryOperation identityOperation("x,y,z");

    /* SymmetryElementIdentity can only be initialized with the identity
     * operation x,y,z. All others operations throw std::invalid_argument
     */
    SymmetryElementIdentity identityElement;
    TS_ASSERT_THROWS_NOTHING(identityElement.init(identityOperation));
    TS_ASSERT_EQUALS(identityElement.hmSymbol(), "1");

    SymmetryOperation mirrorZ("x,y,-z");
    TS_ASSERT_THROWS(identityElement.init(mirrorZ), std::invalid_argument);
  }

  void testSymmetryElementInversion() {
    TS_ASSERT_THROWS_NOTHING(SymmetryElementInversion inversion);

    SymmetryOperation inversionOperation("-x,-y,-z");

    /* SymmetryElementInversion can only be initialized with the inversion
     * operation -x,-y,-z. All others operations throw std::invalid_argument
     */
    SymmetryElementInversion inversionElement;
    TS_ASSERT_THROWS_NOTHING(inversionElement.init(inversionOperation));
    TS_ASSERT_EQUALS(inversionElement.hmSymbol(), "-1");
    TS_ASSERT_EQUALS(inversionElement.getInversionPoint(), V3R(0, 0, 0));

    SymmetryOperation shiftedInversion("-x+1/4,-y+1/4,-z+1/4");
    TS_ASSERT_THROWS_NOTHING(inversionElement.init(shiftedInversion));

    // The operation shifts the inversion center to 1/8, 1/8, 1/8
    V3R inversionPoint = V3R(1, 1, 1) / 8;
    TS_ASSERT_EQUALS(inversionElement.getInversionPoint(), inversionPoint);

    SymmetryOperation mirrorZ("x,y,-z");
    TS_ASSERT_THROWS(inversionElement.init(mirrorZ), std::invalid_argument);
  }

  void testSymmetryElementWithAxisSetAxis() {
    MockSymmetryElementWithAxis element;

    V3R invalidAxis(0, 0, 0);
    TS_ASSERT_THROWS(element.setAxis(invalidAxis), std::invalid_argument);

    V3R validAxis(1, 0, 0);
    TS_ASSERT_THROWS_NOTHING(element.setAxis(validAxis));

    TS_ASSERT_EQUALS(element.getAxis(), validAxis);
  }

  void testSymmetryElementWithAxisSetTranslation() {
    MockSymmetryElementWithAxis element;

    V3R validAxis(1, 0, 0);
    TS_ASSERT_THROWS_NOTHING(element.setTranslation(validAxis));

    TS_ASSERT_EQUALS(element.getTranslation(), validAxis);
  }

  void testSymmetryElementWithAxisDetermineTranslation() {
    MockSymmetryElementWithAxis element;

    V3R screwVectorOneHalf = V3R(0, 0, 1) / 2;
    SymmetryOperation twoOneScrew("-x,-y,z+1/2");
    TS_ASSERT_EQUALS(element.determineTranslation(twoOneScrew),
                     screwVectorOneHalf);

    V3R screwVectorOneThird = V3R(0, 0, 1) / 3;
    SymmetryOperation threeOneScrew("-y,x-y,z+1/3");
    TS_ASSERT_EQUALS(element.determineTranslation(threeOneScrew),
                     screwVectorOneThird);

    V3R screwVectorTwoThirds = V3R(0, 0, 2) / 3;
    SymmetryOperation threeTwoScrew("-y,x-y,z+2/3");
    TS_ASSERT_EQUALS(element.determineTranslation(threeTwoScrew),
                     screwVectorTwoThirds);

    V3R glideVectorC = V3R(0, 0, 1) / 2;
    SymmetryOperation glidePlaneC("x,-y,z+1/2");
    TS_ASSERT_EQUALS(element.determineTranslation(glidePlaneC), glideVectorC);
  }

  void testGetGSLMatrix() {
    IntMatrix mantidMatrix(3, 3, true);
    gsl_matrix *matrix = getGSLMatrix(mantidMatrix);

    TS_ASSERT(matrix);
    TS_ASSERT_EQUALS(matrix->size1, mantidMatrix.numRows());
    TS_ASSERT_EQUALS(matrix->size2, mantidMatrix.numCols());

    for (size_t r = 0; r < mantidMatrix.numRows(); ++r) {
      for (size_t c = 0; c < mantidMatrix.numCols(); ++c) {
        TS_ASSERT_EQUALS(gsl_matrix_get(matrix, r, c), mantidMatrix[r][c]);
      }
    }

    gsl_matrix_free(matrix);
  }

  void testGetGSLIdentityMatrix() {
    gsl_matrix *matrix = getGSLIdentityMatrix(3, 3);

    TS_ASSERT_EQUALS(matrix->size1, 3);
    TS_ASSERT_EQUALS(matrix->size2, 3);

    gsl_matrix_free(matrix);
  }

  void testSymmetryElementWithAxisDetermineAxis() {
    MockSymmetryElementWithAxis element;

    V3R rotationAxisZ = V3R(0, 0, 1);
    SymmetryOperation fourFoldRotoInversionZ("y,-x,-z");
    TS_ASSERT_EQUALS(element.determineAxis(fourFoldRotoInversionZ.matrix()),
                     rotationAxisZ);

    SymmetryOperation sixFoldRotationZ("-y,x-y,z");
    TS_ASSERT_EQUALS(element.determineAxis(sixFoldRotationZ.matrix()),
                     rotationAxisZ);

    V3R rotationAxisY = V3R(0, 1, 0);
    SymmetryOperation glideMirrorCY("x,-y,z+1/2");
    TS_ASSERT_EQUALS(element.determineAxis(glideMirrorCY.matrix()),
                     rotationAxisY);

    V3R rotationAxisXYZ = V3R(1, 1, 1);
    SymmetryOperation threeFoldRation111("z,x,y");
    TS_ASSERT_EQUALS(element.determineAxis(threeFoldRation111.matrix()),
                     rotationAxisXYZ);

    V3R rotationAxisxyZ = V3R(1, -1, -1);
    SymmetryOperation threeFoldRationmm1("-z,-x,y");
    TS_ASSERT_EQUALS(element.determineAxis(threeFoldRationmm1.matrix()),
                     rotationAxisxyZ);

    V3R rotoInversionAxisxYz = V3R(-1, 1, -1);
    SymmetryOperation threeFoldRotoInversionm1mPlus("-z,x,y");
    TS_ASSERT_EQUALS(
        element.determineAxis(threeFoldRotoInversionm1mPlus.matrix()),
        rotoInversionAxisxYz);

    V3R rotationAxis2xx0 = V3R(2, 1, 0);
    SymmetryOperation twoFoldRotationHex210("x,x-y,-z");
    TS_ASSERT_EQUALS(element.determineAxis(twoFoldRotationHex210.matrix()),
                     rotationAxis2xx0);

    V3R rotationAxisx2x0 = V3R(1, 2, 0);
    SymmetryOperation twoFoldRotationHex120("y-x,y,-z");
    TS_ASSERT_EQUALS(element.determineAxis(twoFoldRotationHex120.matrix()),
                     rotationAxisx2x0);
  }

  void testSymmetryElementRotationDetermineRotationSense() {
    TestableSymmetryElementRotation element;

    // Test case 1: 3 [-1 1 -1] (Positive/Negative) in orthogonal system
    SymmetryOperation threeFoldRotoInversionm1mPlus("-z,x,y");
    V3R rotationAxism1m =
        element.determineAxis(threeFoldRotoInversionm1mPlus.matrix());
    TS_ASSERT_EQUALS(element.determineRotationSense(
                         threeFoldRotoInversionm1mPlus, rotationAxism1m),
                     SymmetryElementRotation::Positive);

    SymmetryOperation threeFoldRotoInversionm1mMinus("y,z,-x");
    V3R rotationAxism1m2 =
        element.determineAxis(threeFoldRotoInversionm1mPlus.matrix());

    TS_ASSERT_EQUALS(rotationAxism1m, rotationAxism1m2);

    TS_ASSERT_EQUALS(element.determineRotationSense(
                         threeFoldRotoInversionm1mMinus, rotationAxism1m2),
                     SymmetryElementRotation::Negative);

    // Test case 2: 6 [0 0 1] (Positive/Negative) in hexagonal system
    SymmetryOperation sixFoldRotationZPlus("x-y,x,z");
    V3R rotationAxisZ =
        element.determineAxis(sixFoldRotationZPlus.matrix());
    TS_ASSERT_EQUALS(element.determineRotationSense(
                         sixFoldRotationZPlus, rotationAxisZ),
                     SymmetryElementRotation::Positive);

    SymmetryOperation sixFoldRotationZMinus("y,y-x,z");
    V3R rotationAxisZ2 =
        element.determineAxis(sixFoldRotationZMinus.matrix());

    TS_ASSERT_EQUALS(rotationAxisZ, rotationAxisZ2);

    TS_ASSERT_EQUALS(element.determineRotationSense(
                         sixFoldRotationZMinus, rotationAxisZ2),
                     SymmetryElementRotation::Negative);
  }

  void xtestSymmetryElementWithAxisSpaceGroup() {
    MockSymmetryElementWithAxis element;

    SpaceGroup_const_sptr sg =
        SpaceGroupFactory::Instance().createSpaceGroup("P m -3");

    std::vector<SymmetryOperation> ops = sg->getSymmetryOperations();
    for (auto it = ops.begin(); it != ops.end(); ++it) {
      std::cout << (*it).identifier() << ": " << (*it).order() << " "
                << element.determineAxis((*it).matrix()) << std::endl;
    }
  }

private:
  class MockSymmetryElement : public SymmetryElement {
    friend class SymmetryElementTest;

  public:
    MOCK_METHOD1(init, void(const SymmetryOperation &operation));
  };

  class MockSymmetryElementWithAxis : public SymmetryElementWithAxis {
    friend class SymmetryElementTest;

  public:
    MOCK_METHOD1(init, void(const SymmetryOperation &operation));
  };

  class TestableSymmetryElementRotation : public SymmetryElementRotation {
    friend class SymmetryElementTest;
  };
};

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_ */
